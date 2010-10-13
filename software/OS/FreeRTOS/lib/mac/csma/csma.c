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
	HEADER_LENGTH = 4, MAX_PAYLOAD_LENGTH = 121
};

typedef union frame {
	uint8_t data[MAX_PAYLOAD_LENGTH + HEADER_LENGTH + 1];
	struct {
		uint8_t dst_addr[2];
		uint8_t src_addr[2];
		uint8_t payload[MAX_PAYLOAD_LENGTH];
		uint8_t length;
	};
} frame_t;

/* Function Prototypes */
static void mac_task(void* param);
static void init(void);
static void frame_received(uint8_t * data, uint16_t length, int8_t rssi,
		uint16_t time);
static void random_wait(uint16_t range);

// Timer callback
static uint16_t wait_done(void);

/* Local Variables */
static xQueueHandle tx_queue;
static xSemaphoreHandle wait_sem;
static mac_rx_callback_t received_cb;
static frame_t frame_to_send, frame_to_queue;
uint16_t mac_addr;

void mac_init(xSemaphoreHandle spi_mutex, mac_rx_callback_t rx_cb,
		uint8_t channel) {
	// Initialize the PHY layer
	phy_init(spi_mutex, frame_received, channel, MAC_TX_POWER);

	// Create a frame Queue for the frames to send
	tx_queue = xQueueCreate(MAC_TX_QUEUE_LENGTH, sizeof(frame_t));

	// Create a binary semaphore for backoff waiting
	vSemaphoreCreateBinary(wait_sem);

	// Store the received frame callback
	received_cb = rx_cb;

	// Create the task
	xTaskCreate( mac_task, (const signed char*)"CSMA", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES-2, NULL );
}

uint16_t mac_send(uint16_t dest_addr, uint8_t* data, uint16_t length) {
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

	// Set the node MAC address
	init();

	LEDS_OFF();

	// Enable RX
	phy_rx();

	// Infinite Loope
	for (;;) {
		LED_RED_ON();

		// Get a frame to send
		if (xQueueReceive(tx_queue, &frame_to_send, portMAX_DELAY) != pdTRUE) {
			continue;
		}

		// We have a frame to send, loop for max tries
		for (loop = 0; loop < MAC_MAX_RETRY; loop++) {
			// Wait a random back-off
			random_wait(loop);

			// Try to send
			if (phy_send_cca(frame_to_send.data, frame_to_send.length, 0)) {
				// Frame sent OK
				break;
			}

		}
	}
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
	dst = (((uint16_t) rx_frame->dst_addr[0]) << 8) + rx_frame->dst_addr[1];
	src = (((uint16_t) rx_frame->src_addr[0]) << 8) + rx_frame->src_addr[1];
	if (dst != mac_addr && dst != 0xFFFF) {
		return;
	}

	// if callback, call it
	if (received_cb != 0x0) {
		received_cb(src, rx_frame->payload, length - HEADER_LENGTH, rssi);
	}
}

static void random_wait(uint16_t range) {
	uint16_t wait_time;

	// take the wait semaphore
	xSemaphoreTake(wait_sem, 0);

	// pick up a random time to wait
	wait_time = rand();

	// limit it by the given range
	wait_time &= (1 << (range + 5)) - 1;

	// Add a minimal delay
	wait_time += 16;

	// Set the timerB to generate an interrupt
	timerB_register_cb(TIMERB_ALARM_CCR0, wait_done);
	timerB_set_alarm_from_now(TIMERB_ALARM_CCR0, wait_time, 0);

	// block taking the semaphore, it should be given by the interrupt routine
	xSemaphoreTake(wait_sem, portMAX_DELAY);
}

static uint16_t wait_done(void) {
	portBASE_TYPE yield;

	if (xSemaphoreGiveFromISR(wait_sem, &yield) == pdTRUE) {
#if configUSE_PREEMPTION
		if (yield) {
			portYIELD();
		}
#endif
	}

	return 1;
}
