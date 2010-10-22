/*
 * Copyright  2008-2009 INRIA/SensTools
 *
 * <dev-team@sentools.info>
 *
 * This software is a set of libraries designed to develop applications
 * for the WSN430 embedded hardware platform.
 *
 * This software is governed by the CeCILL license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 */

#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Project Includes */
#include "mac.h"
#include "phy.h"
#include "leds.h"

/* Drivers Include */
#include "ds2411.h"
#include "timerB.h"

enum {
	EVT_FRAME_TO_SEND = 1,
	EVT_ACK_RECEIVED = 2,
	EVT_BACKOFF = 4,
	EVT_ACK_TIMEOUT = 8
};

enum {
	HEADER_LENGTH = 5, MAX_PAYLOAD_LENGTH = 120
};

enum {
	CTRL_TYPE_DATA = 0x1,
	CTRL_TYPE_ACK = 0x2,
	CTRL_TYPE_MASK = 0x3,
	CTRL_ACK_REQ = 0x80
};

enum {
	ACK_WAIT_TIME = 500
};

typedef union frame {
	uint8_t data[MAX_PAYLOAD_LENGTH + HEADER_LENGTH + 1];
	struct {
		uint8_t dst_addr[2];
		uint8_t src_addr[2];
		uint8_t ctrl;
		uint8_t payload[MAX_PAYLOAD_LENGTH];
		uint8_t length;
	};
} frame_t;

typedef union ack {
	uint8_t data[HEADER_LENGTH + 1];
	struct {
		uint8_t dst_addr[2];
		uint8_t src_addr[2];
		uint8_t ctrl;
		uint8_t length;
	};
} ack_t;

/* Function Prototypes */
static void mac_task(void* param);
static uint16_t block_until_event(uint16_t event);
static void init(void);
static void frame_received(uint8_t * data, uint16_t length, int8_t rssi,
		uint16_t time);
static void set_random_wait(uint16_t range);
static void set_ack_timeout(void);
static inline uint16_t ntoh_s(uint8_t*);
static inline void hton_s(uint8_t*, uint16_t);
static inline void addr_copy(uint8_t*, const uint8_t*);

// Timer callback
static uint16_t wait_done(void);
static uint16_t timeout(void);

/* Local Variables */
static xQueueHandle tx_queue, event_queue;
static mac_rx_callback_t received_cb;
static frame_t frame_to_send, frame_to_queue;
static ack_t ack_frame;
uint16_t mac_addr;

void mac_init(xSemaphoreHandle spi_mutex, mac_rx_callback_t rx_cb,
		uint8_t channel) {

	// Initialize the PHY layer
	phy_init(spi_mutex, frame_received, channel, MAC_TX_POWER);

	// Create a frame Queue for the frames to send
	tx_queue = xQueueCreate(MAC_TX_QUEUE_LENGTH, sizeof(frame_t));

	// Create the Event Queue
	event_queue = xQueueCreate(8, sizeof(uint16_t));

	// Store the received frame callback
	received_cb = rx_cb;

	// Create the task
	xTaskCreate( mac_task, (const signed char*)"CSMA", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES-2, NULL );
}

uint16_t mac_send(uint16_t dest_addr, uint8_t* data, uint16_t length,
		int16_t ack) {
	// check the packet length
	if (length > MAX_PAYLOAD_LENGTH) {
		return 0;
	}

	// compute the frame
	frame_to_queue.length = length + HEADER_LENGTH;
	frame_to_queue.dst_addr[0] = dest_addr >> 8;
	frame_to_queue.dst_addr[1] = dest_addr & 0xFF;
	frame_to_queue.src_addr[0] = mac_addr >> 8;
	frame_to_queue.src_addr[1] = mac_addr & 0xFF;
	frame_to_queue.ctrl = CTRL_TYPE_DATA;

	if (ack)
		frame_to_queue.ctrl |= CTRL_ACK_REQ;

	// copy the packet
	memcpy(frame_to_queue.payload, data, length);

	// try to send the frame to the queue
	if (!xQueueSendToBack(tx_queue, &frame_to_queue, 0)) {
		return 0;
	}

	return 1;
}

static void mac_task(void* param) {
	int16_t loop;
	uint16_t event;

	// Set the node MAC address
	init();

	LEDS_OFF();

	// Enable RX
	phy_rx();

	// Infinite Loop
	for (;;) {
		LED_RED_ON();

		// Clear frame_to_send destination, to prevent ack sending
		hton_s(frame_to_send.dst_addr, 0x0);

		// Get a frame to send
		if (xQueueReceive(tx_queue, &frame_to_send, portMAX_DELAY) != pdTRUE) {
			continue;
		}


		// We have a frame to send, loop for max tries
		for (loop = 0; loop < MAC_MAX_RETRY; loop++) {
			// Wait a random back-off
			set_random_wait(loop);
			block_until_event(EVT_BACKOFF);

			// Try to send
			if (!phy_send_cca(frame_to_send.data, frame_to_send.length, 0)) {
				// Frame send failed (channel busy?)
				continue;
			}

			if (frame_to_send.ctrl & CTRL_ACK_REQ) {
				// Set timeout
				set_ack_timeout();

				// Wait until ACK or timeout
				event = block_until_event(EVT_ACK_RECEIVED | EVT_ACK_TIMEOUT);
				if (event == EVT_ACK_TIMEOUT) {
					// loop and retry
					continue;
				} else {
					// Remove timeout
					timerB_unset_alarm(TIMERB_ALARM_CCR0);
				}
			}
			// All good
			break;
		}

	}
}

static uint16_t block_until_event(uint16_t mask) {
	uint16_t evt;
	do {
		xQueueReceive(event_queue, &evt, portMAX_DELAY);
		if ((evt & mask) != evt) {
			printf("Discarded event %x (mask %x)\n", evt, mask);
		}
	} while ((evt & mask) != evt);
	return evt;
}

static void init(void) {
	// Initialize the unique electronic signature and read it
	ds2411_init();
	// Use it to set the node address
	mac_addr = (((uint16_t) ds2411_id.serial1) << 8) + (ds2411_id.serial0);

	// Seed the random number generator
	srand(mac_addr);

	// start the timerB
	timerB_start_ACLK_div(1);
}

static void frame_received(uint8_t * data, uint16_t length, int8_t rssi,
		uint16_t time) {
	frame_t* rx_frame;
	uint16_t src, dst;
	rx_frame = (frame_t*) (void*) data;

	/* Check if addresses are correct */
	dst = ntoh_s(rx_frame->dst_addr);
	src = ntoh_s(rx_frame->src_addr);

	if (dst == mac_addr) {
		// For me, check its type:
		if ((rx_frame->ctrl & CTRL_TYPE_MASK) == CTRL_TYPE_ACK) {
			// It's an ACK, check if it matches our sending frame
			if (src == ntoh_s(frame_to_send.dst_addr)) {
				uint16_t event = EVT_ACK_RECEIVED;
				// push an ACK event
				xQueueSendToBack(event_queue, &event, 0);
			}
			// otherwise drop it
			return;
		} else
		// Check if it's a data frame
		if ((rx_frame->ctrl & CTRL_TYPE_MASK) == CTRL_TYPE_DATA) {
			// If an ACK is required, send it now
			if (rx_frame->ctrl & CTRL_ACK_REQ) {
				hton_s(ack_frame.src_addr, mac_addr);
				addr_copy(ack_frame.dst_addr, rx_frame->src_addr);
				ack_frame.ctrl = CTRL_TYPE_ACK;

				phy_send(ack_frame.data, HEADER_LENGTH, 0x0);
			}
		}
	} else if (dst != 0xFFFF) {
		// If not for me and not broadcast, drop
		return;
	}

	// if callback, call it
	if (received_cb != 0x0) {
		received_cb(src, rx_frame->payload, length - HEADER_LENGTH, rssi);
	}
}

static void set_random_wait(uint16_t range) {
	uint16_t wait_time;

	// pick up a random time to wait
	wait_time = rand();

	// limit it by the given range
	wait_time &= (1 << (range + 5)) - 1;

	// Add a minimal delay
	wait_time += 16;

	// Set the timerB to generate an interrupt
	timerB_register_cb(TIMERB_ALARM_CCR0, wait_done);
	timerB_set_alarm_from_now(TIMERB_ALARM_CCR0, wait_time, 0);
}

static void set_ack_timeout() {
	// Set the timerB to generate an interrupt
	timerB_register_cb(TIMERB_ALARM_CCR0, timeout);
	timerB_set_alarm_from_now(TIMERB_ALARM_CCR0, ACK_WAIT_TIME, 0);
}

static inline uint16_t ntoh_s(uint8_t* addr) {
	return (((uint16_t) *addr) << 8) + *(addr + 1);
}

static inline void hton_s(uint8_t* dst, uint16_t addr) {
	*dst++ = addr >> 8;
	*dst = addr;
}
static inline void addr_copy(uint8_t* dst, const uint8_t* src) {
	*dst++ = *src++;
	*dst = *src;
}

static uint16_t wait_done(void) {
	portBASE_TYPE yield;
	uint16_t event = EVT_BACKOFF;

	if (xQueueSendToBackFromISR(event_queue, &event, &yield) == pdTRUE) {
#if configUSE_PREEMPTION
		if (yield) {
			portYIELD();
		}
#endif
	}

	return 1;
}
static uint16_t timeout(void) {
	portBASE_TYPE yield;
	uint16_t event = EVT_ACK_TIMEOUT;

	if (xQueueSendToBackFromISR(event_queue, &event, &yield) == pdTRUE) {
#if configUSE_PREEMPTION
		if (yield) {
			portYIELD();
		}
#endif
	}

	return 1;
}
