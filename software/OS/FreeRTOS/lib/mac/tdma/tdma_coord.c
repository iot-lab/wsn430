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
#include "phy.h"
#include "tdma_coord.h"
#include "tdma_common.h"
#include "tdma_table.h"
#include "leds.h"

/* Drivers Include */
#include "timerB.h"
#include "uart0.h"
#ifndef __MAC__
#include "ds2411.h"
#endif

#define DEBUG 1
#if DEBUG == 1
#define PRINTF printf
#else
#define PRINTF(...)
#endif

#define LEDS 0

/* Function Prototypes */
static void vMacTask(void* pvParameters);

static void frame_received(uint8_t *data, uint16_t length, int8_t rssi,
		uint16_t time);

static void mac_init(void);
static void beacon_send(void);
static uint16_t beacon_append(uint16_t dest_addr, uint8_t type, uint8_t length,
		uint8_t* data);
static uint16_t slot_time_evt(void);
static uint16_t block_until_event(uint16_t event);

/* Local Variables */
static xQueueHandle xEventQueue;
uint16_t mac_addr;
static beacon_t beacon_frame;
static uint8_t* beacon_data_ptr;
static uint16_t beacon_time;
uint16_t slot_running;

static void (*node_associated_handler)(uint16_t node);
static void (*data_received_handler)(uint16_t node, uint8_t* data,
		uint16_t length);
static void (*beacon_handler)(uint8_t id, uint16_t timestamp);

void mac_create_task(xSemaphoreHandle xSPIMutex) {
	// Create the Event Queue
	xEventQueue = xQueueCreate(8, sizeof(uint16_t));

	// Create the PHY task
	phy_init(xSPIMutex, frame_received, RADIO_CHANNEL, RADIO_POWER);

	// Create the task
	xTaskCreate(vMacTask, (const signed char * const ) "MAC",
			configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, NULL );

	node_associated_handler = 0x0;
	data_received_handler = 0x0;
	beacon_handler = 0x0;
}

uint16_t mac_send(uint16_t node, uint8_t* data, uint16_t length) {
	if (length > MAX_COORD_SEND_LENGTH) {
		return 0;
	}
	return beacon_append(node, MGT_DATA, length, data);
}

void mac_set_node_associated_handler(void(*handler)(uint16_t node)) {
	node_associated_handler = handler;
}
void mac_set_data_received_handler(void(*handler)(uint16_t node, uint8_t* data,
		uint16_t length)) {
	data_received_handler = handler;
}
void mac_set_beacon_handler(void(*handler)(uint8_t id, uint16_t timestamp)) {
	beacon_handler = handler;
}

static void vMacTask(void* pvParameters) {
	mac_init();

	PRINTF(
			"TDMA parameters: %u slots, %u ticks [%u ms] each, channel %u, addr %.4x\n",
			SLOT_COUNT, TIME_SLOT, SLOT_TIME_MS, RADIO_CHANNEL, mac_addr);
#if LEDS == 1
	LEDS_OFF();
	LED_BLUE_ON();
#endif
	/* Packet Sending/Receiving */
	for (;;) {
#if LEDS == 1
		LED_RED_TOGGLE();
#endif

		// send beacon
		beacon_send();
		if (beacon_handler) {
			beacon_handler(beacon_frame.beacon_id - 1, beacon_time);
		}

		// Block until slot time
		block_until_event(EVENT_SLOT_TIME);

		// loop on slots
		for (slot_running = 1; slot_running <= SLOT_COUNT; slot_running++) {
#if LEDS == 1
			LED_GREEN_TOGGLE();
#endif

			// Block until next slot, frames are handled directly
			block_until_event(EVENT_SLOT_TIME);
		}

		slot_running = 0;
	}
}

static void mac_init(void) {
#ifndef __MAC__
	/* Initialize the unique electronic signature and read it */
	ds2411_init();
	mac_addr = (((uint16_t) ds2411_id.serial1) << 8) + (ds2411_id.serial0);
#else
	mac_addr = __MAC__;
#endif

	/* Seed the random number generator */
	srand(mac_addr);

	// Clear the association table
	tdma_table_clear();

	// Set RX
	phy_rx();

	// Setup TimerB
	timerB_init();
	timerB_start_ACLK_div(TIMERB_DIV_1);
	timerB_register_cb(ALARM_SLOT, slot_time_evt);
	timerB_set_alarm_from_now(ALARM_SLOT, TIME_SLOT, TIME_SLOT);

	// Fill beacon data
	beacon_frame.length = FRAME_HEADER_LENGTH + 1;
	hton_s(mac_addr, beacon_frame.srcAddr);
	hton_s(0xFFFF, beacon_frame.dstAddr);
	beacon_frame.type = FRAME_TYPE_BEACON;
	beacon_frame.beacon_id = 0;
	beacon_data_ptr = beacon_frame.beacon_data;
}

static void beacon_send(void) {
	// Compute length
	beacon_frame.length = beacon_data_ptr - beacon_frame.raw;

	// Send frame and store sync time in beacon_time
	phy_send(beacon_frame.raw, beacon_frame.length, &beacon_time);

	// Update fields
	beacon_data_ptr = beacon_frame.beacon_data;
	beacon_frame.beacon_id++;
}

static uint16_t beacon_append(uint16_t dest_addr, uint8_t type, uint8_t length,
		uint8_t* data) {
	if (beacon_data_ptr + 3 + length > beacon_frame.beacon_data
			+ MAX_BEACON_DATA_LENGTH) {
		// Beacon too big
		return 0;
	}

	hton_s(dest_addr, beacon_data_ptr);
	beacon_data_ptr += 2;

	*beacon_data_ptr = (type & MGT_TYPE_MASK) | (length << 4);
	beacon_data_ptr++;

	memcpy(beacon_data_ptr, data, length);
	beacon_data_ptr += length;

	return 1;
}

static uint16_t slot_time_evt(void) {
	const uint16_t evt = EVENT_SLOT_TIME;
	portBASE_TYPE yield = pdFALSE;

	if (xQueueSendFromISR(xEventQueue, &evt, &yield) == pdTRUE) {
#if configUSE_PREEMPTION
		if (yield) {
			portYIELD();
		}
#endif
	}
	return 1;
}

static uint16_t block_until_event(uint16_t mask) {
	uint16_t evt;
	do {
		xQueueReceive(xEventQueue, &evt, portMAX_DELAY);
		if ((evt & mask) != evt) {
			PRINTF("Discarded event %x (mask %x)\n", evt, mask);
		}
	} while ((evt & mask) != evt);
	return evt;
}

static void frame_received(uint8_t * data, uint16_t length, int8_t rssi,
		uint16_t time) {
	frame_t *frame;
	uint8_t slot_result;
	uint16_t srcAddr;

	// Check length
	if ((length < FRAME_HEADER_LENGTH) || (length > FRAME_HEADER_LENGTH
			+ MAX_PACKET_LENGTH)) {
		PRINTF("RX: bad length\n");
		return;
	}

	// Cast the frame
	frame = (frame_t*) (void*) data;

	if (ntoh_s(frame->dstAddr) != mac_addr) {
		// Bad destination
		PRINTF("RX: bad dst %x\n", ntoh_s(frame->dstAddr));
		return;
	}

	srcAddr = ntoh_s(frame->srcAddr);

	switch (frame->type) {
	case FRAME_TYPE_DATA:
		slot_result = tdma_table_pos(srcAddr);
		if ((slot_running == slot_result) && data_received_handler) {
			data_received_handler(srcAddr, frame->data, length
					- FRAME_HEADER_LENGTH);
		} else {
			PRINTF("RX: out of slot\n");
		}
		break;
	case FRAME_TYPE_MGT:
		switch (frame->data[0]) {
		case MGT_ASSOCIATE:
			slot_result = tdma_table_add(srcAddr);
			if (slot_result != 0) {
				beacon_append(srcAddr, MGT_ASSOCIATE, 1, &slot_result);
				if (node_associated_handler) {
					node_associated_handler(srcAddr);
				}
			}
			break;
		case MGT_DISSOCIATE:
			slot_result = tdma_table_del(srcAddr);
			beacon_append(srcAddr, MGT_DISSOCIATE, 1, &slot_result);
			if (node_associated_handler) {
				node_associated_handler(srcAddr);
			}
			break;
		default:
			PRINTF("RX: bad data[0] %u\n", frame->data[0]);
			break;
		}
		break;
	default:
		PRINTF("RX: bad type\n");
		// Bad type
		return;
	}
}

