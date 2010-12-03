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
#include <stdio.h>
#include <string.h>

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
	HEADER_LENGTH = 5, MAX_PAYLOAD_LENGTH = 120
};

enum {
	FRAME_TYPE_PREAMB = 0x1, FRAME_TYPE_ACK = 0x2, FRAME_TYPE_DATA = 0x3

};

enum {
	EVENT_RX_TIME = 1,
	EVENT_TX_TIME = 2,
	EVENT_TIMEOUT = 4,
	EVENT_RX = 8,
	EVENT_TX = 16
};

// XMAC time constants
enum {
	RECEIVER_WAKEUP_PERIOD = 32768 * 128 / 1000, // 128ms
	RECEIVER_WAKEUP_DURATION = 32768 * 6 / 1000, // 6ms
	SENDER_PREAMBLE_TX_DURATION = 32768 * 2 / 1000, // 2ms
	SENDER_ACK_RX_DURATION = 32768 * 4 / 1000, // 4ms
	RECEIVER_DATA_RX_DURATION = 32768 * 15 / 1000, // 15ms
	SENDER_INTERPACKET_DURATION = 32768 * 1 / 1000, // 1ms
};
typedef union frame {
	uint8_t data[HEADER_LENGTH + MAX_PAYLOAD_LENGTH + 1];
	struct {
		uint8_t type;
		uint8_t dst_addr[2];
		uint8_t src_addr[2];
		uint8_t payload[MAX_PAYLOAD_LENGTH];
		uint8_t length;
	};
} frame_t;

typedef union frame_small {
	uint8_t data[HEADER_LENGTH + 1];
	struct {
		uint8_t type;
		uint8_t dst_addr[2];
		uint8_t src_addr[2];
		uint8_t length;
	};
} frame_small_t;

/* Function Prototypes */
static void mac_task(void* param);
static uint16_t wait_until(uint16_t mask);
static void init(void);
static void receiver_idle(void);
static void receiver_rx_preamble(void);
static void sender_rx_ack(void);
static void receiver_rx_data(void);
static uint16_t sender_tx_first_preamble(void);
static void sender_wait_tx(void);
static void sender_tx_preamble(void);
static void receiver_send_ack(void);
static void sender_tx_data(void);
static uint16_t sender_handle_ack(void);
static uint16_t receiver_handle_frame(void);

static void frame_received_cb(uint8_t * data, uint16_t length, int8_t rssi,
		uint16_t time);

/* callback function prototypes */
static uint16_t tx_time(void);
static uint16_t rx_time(void);
static uint16_t timeout(void);

/* Local Variables */
static xQueueHandle tx_queue, event_queue;
static mac_rx_callback_t received_cb;
static frame_t frame_to_send, frame_to_queue, frame_received;
static frame_small_t frame_small;
uint16_t mac_addr;
static uint16_t frame_received_dst, frame_received_src;
static int8_t frame_received_rssi;
static uint8_t keep_rx;

void mac_init(xSemaphoreHandle spi_mutex, mac_rx_callback_t rx_cb,
		uint8_t channel) {
	// Initialize the PHY layer
	phy_init(spi_mutex, frame_received_cb, channel, MAC_TX_POWER);

	// Create a frame queue for the frames to send
	tx_queue = xQueueCreate(MAC_TX_QUEUE_LENGTH, sizeof(frame_t));

	// Create a frame queue for the events
	event_queue = xQueueCreate(8, sizeof(uint16_t));

	// Store callback
	received_cb = rx_cb;

	// Create the task
	xTaskCreate( mac_task, (const signed char*)"X-MAC", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES-2, NULL );
}

uint16_t mac_send(uint16_t dest_addr, uint8_t* data, uint16_t length) {
	// check the packet length
	if (length > MAX_PAYLOAD_LENGTH) {
		return 0;
	}

	// compute the frame
	frame_to_queue.length = length + HEADER_LENGTH;
	frame_to_queue.type = FRAME_TYPE_DATA;
	frame_to_queue.dst_addr[0] = dest_addr >> 8;
	frame_to_queue.dst_addr[1] = dest_addr & 0xFF;
	frame_to_queue.src_addr[0] = mac_addr >> 8;
	frame_to_queue.src_addr[1] = mac_addr & 0xFF;

	// copy the packet
	memcpy(frame_to_queue.payload, data, length);

	// try to send the frame to the queue
	if (!xQueueSendToBack(tx_queue, &frame_to_queue, 0)) {
		return 0;
	}

	// Send an event
	uint16_t event = EVENT_TX;
	xQueueSendToBack(event_queue, &event, 0);

	return 1;
}

static void mac_task(void* param) {
	uint16_t event;

	// Set the node MAC address
	init();

	printf("MAC address %.4x\n", mac_addr);

	LEDS_OFF();

	/* Packet Sending/Receiving */
	for (;;) {

		if (keep_rx > 0) {
			//			keep_rx--;
			// Stay in RX, set timeout
			timerB_set_alarm_from_now(TIMERB_ALARM_CCR0,
					RECEIVER_WAKEUP_PERIOD, 0);
			timerB_register_cb(TIMERB_ALARM_CCR0, timeout);

		} else {
			// Set Idle and RX timeout for one sleep period
			receiver_idle();

			// Wait for RX time or TX
			event = wait_until(EVENT_RX_TIME | EVENT_TX);

			if (event == EVENT_TX) {
				// Check CCA, prepare the preamble frame
				if (!sender_tx_first_preamble()) {
					continue;
				}
				sender_rx_ack();

				uint16_t start = timerB_time();

				// CCA is clear, let's send!
				while (timerB_time() - start < RECEIVER_WAKEUP_PERIOD) {
					sender_wait_tx();
					event = wait_until(EVENT_TX_TIME | EVENT_RX);
					if (event == EVENT_TX_TIME) {
						sender_tx_preamble();
					} else {
						// Handle the received frame
						if (sender_handle_ack()) {
							break;
						}
					}
				}
				// We're if the ACK has been received from the destination node,
				// or if we have sent all our preamble frames
				if (frame_to_send.dst_addr[0] == 0xFF
						&& frame_to_send.dst_addr[1] == 0xFF) {
					start = timerB_time();
					while (timerB_time()-start < SENDER_INTERPACKET_DURATION) {
						nop();
					}

				}
				sender_tx_data();
				// OK, tx done
				continue;
			}

			// Let's do normal periodic RX
			receiver_rx_preamble();

		}

		// Wait for RX  or RX timeout
		event = wait_until(EVENT_RX | EVENT_TIMEOUT);

		if (event == EVENT_TIMEOUT) {
			// Found nothing, loop
			continue;
		} else {
			// Disable timeout
			timerB_unset_alarm(TIMERB_ALARM_CCR0);
		}

		// Got a frame, handle it
		frame_received_dst = ((uint16_t) frame_received.dst_addr[0]) << 8;
		frame_received_dst += frame_received.dst_addr[1];
		frame_received_src = ((uint16_t) frame_received.src_addr[0]) << 8;
		frame_received_src += frame_received.src_addr[1];

		switch (receiver_handle_frame()) {
		case FRAME_TYPE_PREAMB:

			if (frame_received_dst == mac_addr) {
				frame_received.length = 0;

				// For me! Send an ACK
				receiver_send_ack();

				// Set RX for data RX
				receiver_rx_data();

				// Wait for RX  or RX timeout
				event = wait_until(EVENT_RX | EVENT_TIMEOUT);
				if (event == EVENT_RX) {
					// Compute src/dst
					frame_received_dst
							= ((uint16_t) frame_received.dst_addr[0]) << 8;
					frame_received_dst += frame_received.dst_addr[1];
					frame_received_src
							= ((uint16_t) frame_received.src_addr[0]) << 8;
					frame_received_src += frame_received.src_addr[1];

					// Check frame
					if (receiver_handle_frame() == FRAME_TYPE_DATA) {
						if (received_cb) {
							received_cb(frame_received_src,
									frame_received.payload,
									frame_received.length - HEADER_LENGTH, frame_received_rssi);
						}
					}
					frame_received.length = 0;
				}

			} else if (frame_received_dst == MAC_BROADCAST_ADDR) {
				frame_received.length = 0;
				// broadcast
				keep_rx = 1;
			}
			break;
		case FRAME_TYPE_DATA:
			if (received_cb) {
				received_cb(frame_received_src, frame_received.payload,
						frame_received.length - HEADER_LENGTH, frame_received_rssi);
			}
			frame_received.length = 0;
			break;
		default:
			puts("err");
			break;
		}
	}
}

static uint16_t wait_until(uint16_t mask) {
	uint16_t event;

	do {
		if (xQueueReceive(event_queue, &event, portMAX_DELAY) != pdTRUE) {
			continue;
		}

		// if TX received, but not wanted, put event back in queue for later
		if ((event == EVENT_TX) && ((mask & EVENT_TX) == 0)) {
			xQueueSendToBack(event_queue, &event, 0);
		}

		// if RX received, but not wanted, discard frame
		if ((event == EVENT_RX) && ((mask & EVENT_RX) == 0)) {
			frame_received.length = 0;
		}

	} while ((event & mask) != event);

	return event;
}

static void init() {
	// Initialize the unique electronic signature and read it
	ds2411_init();
	mac_addr = (((uint16_t) ds2411_id.serial1) << 8) + (ds2411_id.serial0);

	// Seed the random number generator
	srand(mac_addr);

	// start the timerB
	timerB_start_ACLK_div(1);

	keep_rx = 0;

	frame_received.length = 0;
}

static void receiver_idle() {
	// Stop PHY
	phy_idle();

	// Set RX time
	timerB_set_alarm_from_now(TIMERB_ALARM_CCR0, RECEIVER_WAKEUP_PERIOD
			- RECEIVER_WAKEUP_DURATION, 0);
	timerB_register_cb(TIMERB_ALARM_CCR0, rx_time);
}

static void receiver_rx_preamble() {
	// Start PHY RX
	phy_rx();

	// Set timeout
	timerB_set_alarm_from_now(TIMERB_ALARM_CCR0, RECEIVER_WAKEUP_DURATION, 0);
	timerB_register_cb(TIMERB_ALARM_CCR0, timeout);
}

static void sender_rx_ack() {
	// Start PHY RX
	phy_rx();
}

static void receiver_rx_data() {
	// Set timeout
	timerB_set_alarm_from_now(TIMERB_ALARM_CCR0, RECEIVER_DATA_RX_DURATION, 0);
	timerB_register_cb(TIMERB_ALARM_CCR0, timeout);
}

static void frame_received_cb(uint8_t * data, uint16_t length, int8_t rssi,
		uint16_t time) {
	if (frame_received.length != 0) {
		// Buffer busy, drop frame
		return;
	}

	memcpy(frame_received.data, data, length);
	frame_received.length = length;
	frame_received_rssi = rssi;

	uint16_t event;
	event = EVENT_RX;

	if (xQueueSendToBack(event_queue, &event, 0) != pdTRUE) {
		// If queue failed, drop frame
		frame_received.length = 0;
	}
}

static uint16_t sender_tx_first_preamble() {
	// Try to get the frame to send from the FIFO
	if (xQueueReceive(tx_queue, &frame_to_send, 0) != pdTRUE) {
		return 0;
	}

	frame_small.length = HEADER_LENGTH;
	frame_small.type = FRAME_TYPE_PREAMB;
	frame_small.src_addr[0] = mac_addr >> 8;
	frame_small.src_addr[1] = mac_addr;

	frame_small.dst_addr[0] = frame_to_send.dst_addr[0];
	frame_small.dst_addr[1] = frame_to_send.dst_addr[1];

	if (phy_send_cca(frame_small.data, frame_small.length, 0x0)) {
		return 1;
	}

	return 0;
}

static void sender_wait_tx() {
	timerB_set_alarm_from_now(TIMERB_ALARM_CCR0, SENDER_ACK_RX_DURATION, 0);
	timerB_register_cb(TIMERB_ALARM_CCR0, tx_time);
}

static void sender_tx_preamble() {
	phy_send(frame_small.data, frame_small.length, 0x0);
}

static void receiver_send_ack() {
	// Start PHY RX for Data
	phy_rx();

	frame_small.length = HEADER_LENGTH;
	frame_small.dst_addr[0] = frame_received.src_addr[0];
	frame_small.dst_addr[1] = frame_received.src_addr[1];
	frame_small.src_addr[0] = mac_addr >> 8;
	frame_small.src_addr[1] = mac_addr & 0xFF;
	frame_small.type = FRAME_TYPE_ACK;

	phy_send(frame_small.data, frame_small.length, 0x0);
}

static void sender_tx_data() {
	phy_send(frame_to_send.data, frame_to_send.length, 0x0);
}

static uint16_t receiver_handle_frame() {
	if ((frame_received_dst != mac_addr) && (frame_received_dst
			!= MAC_BROADCAST_ADDR)) {
		// Discard the frame
		frame_received.length = 0;
		return 0;
	}

	if (frame_received.type == FRAME_TYPE_DATA) {
		if (frame_received.length < HEADER_LENGTH) {
			// Discard the frame
			frame_received.length = 0;
			return 0;
		}

		return FRAME_TYPE_DATA;

	} else if (frame_received.type == FRAME_TYPE_PREAMB) {
		if (frame_received.length != HEADER_LENGTH) {
			// Discard the frame
			frame_received.length = 0;
			return 0;
		}

		return FRAME_TYPE_PREAMB;
	}

	// Discard the frame
	frame_received.length = 0;
	return 0;
}

static uint16_t sender_handle_ack(void) {
	uint16_t dst;

	if ((frame_received.length != HEADER_LENGTH) || (frame_received.type
			!= FRAME_TYPE_ACK)) {
		frame_received.length = 0;
		return 0;
	}

	memcpy(frame_small.data, frame_received.data, frame_received.length);
	frame_received.length = 0;

	dst = ((uint16_t) frame_small.dst_addr[0]) << 8;
	dst += frame_small.dst_addr[1];

	if (dst != mac_addr) {
		return 0;
	}

	return 1;
}

/*-------------TIMER ISR--------------*/

static uint16_t tx_time() {
	portBASE_TYPE yield;
	uint16_t event;

	event = EVENT_TX_TIME;

	if (xQueueSendToBackFromISR(event_queue, &event, &yield) == pdTRUE) {
#if configUSE_PREEMPTION
		if (yield) {
			portYIELD();
		}
#endif
	}

	return 1;
}
static uint16_t rx_time() {
	portBASE_TYPE yield;
	uint16_t event;

	event = EVENT_RX_TIME;

	if (xQueueSendToBackFromISR(event_queue, &event, &yield) == pdTRUE) {
#if configUSE_PREEMPTION
		if (yield) {
			portYIELD();
		}
#endif
	}

	return 1;
}
static uint16_t timeout() {
	portBASE_TYPE yield;
	uint16_t event;

	event = EVENT_TIMEOUT;

	if (xQueueSendToBackFromISR(event_queue, &event, &yield) == pdTRUE) {
#if configUSE_PREEMPTION
		if (yield) {
			portYIELD();
		}
#endif
	}

	return 1;
}
