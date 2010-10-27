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
#include "tdma_node.h"
#include "tdma_common.h"
#include "leds.h"

/* Drivers Include */
#include "timerB.h"
#include "ds2411.h"
#include "uart0.h"

#define DEBUG 0
#if DEBUG == 1
#define PRINTF PRINTF
#else
#define PRINTF(...)
#endif

/* Function Prototypes */
static void vMacTask(void* pvParameters);

static void frame_received(uint8_t *data, uint16_t length, int8_t rssi,
		uint16_t time);

static void mac_init(void);
static uint16_t block_until_event(uint16_t event);

static void idle(void);
static void beacon_search(uint16_t timeout);

static void slot_wait(uint16_t slot);
static void attach_send(void);

static uint16_t data_send(void);
static void interpacket_wait(void);

static uint16_t beacon_time_evt(void);
static uint16_t slot_time_evt(void);
static uint16_t timeout_evt(void);

/* Local Variables */
static xQueueHandle xEventQueue;
static xQueueHandle tx_queue;
uint16_t mac_addr;
static uint16_t coordAddr;
static uint16_t beacon_loss, associate_wait;

static enum mac_state state;

static frame_t data_frame;
static uint16_t beacon_time;

static void (*handler_beacon)(uint8_t id, uint16_t beacon) = 0x0;
static void (*handler_asso)(void) = 0x0;
static void (*handler_disasso)(void) = 0x0;
static void (*handler_rx)(uint8_t* data, uint16_t length) = 0x0;
static void (*handler_lost)(void) = 0x0;

static uint8_t slot_dedicated;

void mac_create_task(xSemaphoreHandle xSPIMutex) {
	// Start the PHY layer
	phy_init(xSPIMutex, frame_received, RADIO_CHANNEL, RADIO_POWER);

	// Create a frame Queue for the frames to send
	tx_queue = xQueueCreate(MAC_TX_QUEUE_LENGTH, sizeof(frame_t));

	// Create the Event Queue
	xEventQueue = xQueueCreate(8, sizeof(uint16_t));

	// Create the task
	xTaskCreate(vMacTask, (const signed char * const ) "MAC",
			configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES-2, NULL );
}

void mac_send_command(enum mac_command cmd) {
	uint16_t evt;
	switch (cmd) {
	case MAC_ASSOCIATE:
		evt = EVENT_ASSOCIATE_REQ;
		xQueueSendToBack(xEventQueue, &evt, 0);
		break;
	case MAC_DISASSOCIATE:
		evt = EVENT_DISSOCIATE_REQ;
		xQueueSendToBack(xEventQueue, &evt, 0);
		break;
	}
}

void mac_set_beacon_handler(void(*handler)(uint8_t id, uint16_t beacon_time)) {
	handler_beacon = handler;
}
void mac_set_event_handler(enum mac_event evt, void(*handler)(void)) {
	switch (evt) {
	case MAC_ASSOCIATED:
		handler_asso = handler;
		break;
	case MAC_DISASSOCIATED:
		handler_disasso = handler;
		break;
	case MAC_LOST:
		handler_lost = handler;
		break;
	}
}

void mac_set_data_received_handler(void(*handler)(uint8_t* data,
		uint16_t length)) {
	handler_rx = handler;
}

uint16_t mac_send(uint8_t* data, uint16_t length) {
	// Routine checks
	if (state != STATE_ASSOCIATED) {
		return 0;
	}

	if (length > MAX_PACKET_LENGTH) {
		return 0;
	}

	// Prepare data frame
	hton_s(mac_addr, data_frame.srcAddr);
	hton_s(coordAddr, data_frame.dstAddr);
	data_frame.type = FRAME_TYPE_DATA;
	memcpy(data_frame.data, data, length);
	data_frame.length = FRAME_HEADER_LENGTH + length;

	// Put frame in queue
	if (xQueueSendToBack(tx_queue, &data_frame, 0) == pdTRUE) {
		return 1;
	}

	return 0;
}

static void vMacTask(void* pvParameters) {
	mac_init();

	PRINTF(
			"TDMA parameters: %u slots, %u ticks [%u ms] each, channel %u, addr %.4x\n",
			SLOT_COUNT, TIME_SLOT, SLOT_TIME_MS, RADIO_CHANNEL, mac_addr);

	LEDS_OFF();
	LED_BLUE_ON();

	state = STATE_IDLE;

	for (;;) {
		switch (state) {
		case STATE_IDLE:
			idle();
			block_until_event(EVENT_ASSOCIATE_REQ);
			state = STATE_BEACON_SEARCH;
			break;

		case STATE_BEACON_SEARCH:
			// Search for the beacon
			beacon_search(0);
			beacon_loss = 0;
			block_until_event(EVENT_RX);
			phy_idle();
			state = STATE_ASSOCIATING;
			associate_wait = 0;
			break;

		case STATE_ASSOCIATING:
			block_until_event(EVENT_BEACON_TIME);
			beacon_search(TIME_SLOT / 2);
			if (block_until_event(EVENT_RX | EVENT_TIMEOUT) == EVENT_RX) {
				timerB_unset_alarm(ALARM_TIMEOUT);
				phy_idle();
				if (associate_wait == 1) {
					slot_wait(SLOT_COUNT);
					block_until_event(EVENT_SLOT_TIME);
					attach_send();
					putchar('a');
				} else if ((associate_wait == 0)
						&& (state == STATE_ASSOCIATING)) {
					do {
						associate_wait = rand() & 0xF;
					} while (associate_wait > 16);
					associate_wait += 2;
				}
				associate_wait--;
			} else {
				phy_idle();
				beacon_loss++;
				if (beacon_loss == BEACON_LOSS_MAX) {
					state = STATE_BEACON_SEARCH;
					timerB_unset_alarm(ALARM_BEACON);
					if (handler_lost) {
						handler_lost();
					}
				}
			}
			break;

		case STATE_ASSOCIATED:
			// Loop on the synchronized beacon
			block_until_event(EVENT_BEACON_TIME);
			beacon_search(TIME_SLOT / 2);
			if (block_until_event(EVENT_RX | EVENT_TIMEOUT) == EVENT_RX) {
				timerB_unset_alarm(ALARM_TIMEOUT);
				phy_idle();
				// Set slot alarm
				slot_wait(slot_dedicated);
				// Wait until beginning of slot
				block_until_event(EVENT_SLOT_TIME);
				// Send all data we have while we have time
				while (data_send() && ((beacon_time + (slot_dedicated
						* TIME_SLOT) - timerB_time()) < TIME_GUARD)) {
					interpacket_wait();
					block_until_event(EVENT_TIMEOUT);
				}
			} else {
				phy_idle();
				beacon_loss++;
				if (beacon_loss == BEACON_LOSS_MAX) {
					state = STATE_BEACON_SEARCH;
					timerB_unset_alarm(ALARM_BEACON);
					if (handler_lost) {
						handler_lost();
					}
				}
			}
			break;
		case STATE_WAITING:
			block_until_event(EVENT_TIMEOUT);
			break;
		}

		LED_RED_TOGGLE();
	}
}

static void frame_received(uint8_t * data, uint16_t length, int8_t rssi,
		uint16_t timestamp) {
	beacon_t *frame;
	uint16_t srcAddr;
	uint8_t beacon_type, beacon_length;

	// Check length
	if ((length < FRAME_HEADER_LENGTH) || (length > FRAME_HEADER_LENGTH
			+ MAX_PACKET_LENGTH)) {
		PRINTF("bad length\n");
		return;
	}

	// Cast the frame
	frame = (beacon_t*) (void*) data;

	// Check destination address
	if ((ntoh_s(frame->dstAddr) != mac_addr) && (ntoh_s(frame->dstAddr)
			!= 0xFFFF)) {
		// Bad destination
		PRINTF("bad dst %x\n", ntoh_s(frame->dstAddr));
		return;
	}

	// Compute source address
	srcAddr = ntoh_s(frame->srcAddr);

	// Check type
	if (frame->type != FRAME_TYPE_BEACON) {
		// Bad type
		PRINTF("bad type %u\n", frame->type);
		return;
	}

	if (coordAddr == 0x0) {
		coordAddr = ntoh_s(frame->srcAddr);
		PRINTF("coord = %.4x\n", coordAddr);
	} else if (ntoh_s(frame->srcAddr) != coordAddr) {
		PRINTF("bad src addr\n");
		return;
	}

	// Everything is okay!, set timer for beacon time
	beacon_time = timestamp;

	timerB_set_alarm_from_time(ALARM_BEACON, (SLOT_COUNT + 1) * TIME_SLOT,
			(SLOT_COUNT + 1) * TIME_SLOT, beacon_time - TIME_GUARD);
	timerB_register_cb(ALARM_BEACON, beacon_time_evt);

	// Loop on beacon payload for data
	uint8_t* beacon_data_ptr = frame->beacon_data;
	uint16_t dst;

	while (beacon_data_ptr < frame->raw + length + 1) {
		// Fetch destination, type and length
		dst = ntoh_s(beacon_data_ptr);
		beacon_data_ptr += 2;

		beacon_type = *beacon_data_ptr & MGT_TYPE_MASK; // low 4 bits
		beacon_length = *beacon_data_ptr >> 4; // high 4 bits
		beacon_data_ptr++;

		if (dst == mac_addr || dst == 0xFFFF) {
			switch (beacon_type) {
			case MGT_ASSOCIATE:
				if (beacon_length == 1) {
					slot_dedicated = *beacon_data_ptr;
					state = STATE_ASSOCIATED;
					if (handler_asso) {
						handler_asso();
					}
				}
				break;
			case MGT_DISSOCIATE:
				if (beacon_length == 0) {
					slot_dedicated = 0;
					state = STATE_IDLE;
					if (handler_disasso) {
						handler_disasso();
					}
				}
				break;
			case MGT_DATA:
				if (handler_rx) {
					handler_rx(beacon_data_ptr, length);
				}
				break;
			}
		}

		// increment pointer of payload
		beacon_data_ptr += beacon_length;
	}

	uint16_t event = EVENT_RX;
	xQueueSendToBack(xEventQueue, &event, 0);

	if (handler_beacon) {
		handler_beacon(frame->beacon_id, beacon_time);
	}
}

static void mac_init(void) {
	/* Initialize the unique electronic signature and read it */
	ds2411_init();
	mac_addr = (((uint16_t) ds2411_id.serial1) << 8) + (ds2411_id.serial0);

	/* Seed the random number generator */
	srand(mac_addr);

	// Setup TimerB ACLK 32kHz
	timerB_init();
	timerB_start_ACLK_div(TIMERB_DIV_1);
	timerB_register_cb(ALARM_BEACON, beacon_time_evt);
	timerB_register_cb(ALARM_SLOT, slot_time_evt);
	timerB_register_cb(ALARM_TIMEOUT, timeout_evt);
}

static void idle(void) {
	coordAddr = 0x0;
	beacon_loss = 0;

	phy_idle();
}

static void beacon_search(uint16_t timeout) {
	phy_rx();

	if (timeout) {
		timerB_set_alarm_from_now(ALARM_TIMEOUT, timeout, 0);
		timerB_register_cb(ALARM_TIMEOUT, timeout_evt);
	}
}

static void slot_wait(uint16_t slot) {
	timerB_set_alarm_from_time(ALARM_SLOT, slot * TIME_SLOT, 0, beacon_time);
}

static void attach_send(void) {
	// Prepare management frame
	hton_s(mac_addr, data_frame.srcAddr);
	hton_s(coordAddr, data_frame.dstAddr);
	data_frame.type = FRAME_TYPE_MGT;
	data_frame.data[0] = MGT_ASSOCIATE;

	phy_send(data_frame.raw, FRAME_HEADER_LENGTH + 1, 0);

	data_frame.length = 0;
}

static uint16_t data_send(void) {
	// Try to get a frame to send
	if (xQueueReceive(tx_queue, &data_frame, 0) != pdTRUE) {
		// No frame to send
		return 0;
	}

	phy_send(data_frame.raw, data_frame.length, 0);
	return 1;
}

static void interpacket_wait(void) {
	timerB_set_alarm_from_now(ALARM_TIMEOUT, TIME_INTERPACKET, 0);
	timerB_register_cb(ALARM_TIMEOUT, timeout_evt);
}

static uint16_t beacon_time_evt(void) {
	const uint16_t evt = EVENT_BEACON_TIME;
	portBASE_TYPE wakeup = pdFALSE;

	xQueueSendFromISR(xEventQueue, &evt, &wakeup);

	// wake-up if needed
	if (wakeup == pdTRUE) {
		taskYIELD();
	}
	return 1;
}

static uint16_t slot_time_evt(void) {
	const uint16_t evt = EVENT_SLOT_TIME;
	portBASE_TYPE wakeup = pdFALSE;

	xQueueSendFromISR(xEventQueue, &evt, &wakeup);

	// wake-up if needed
	if (wakeup == pdTRUE) {
		taskYIELD();
	}
	return 1;
}

static uint16_t timeout_evt(void) {
	const uint16_t evt = EVENT_TIMEOUT;
	portBASE_TYPE wakeup = pdFALSE;

	xQueueSendFromISR(xEventQueue, &evt, &wakeup);

	// wake-up if needed
	if (wakeup == pdTRUE) {
		taskYIELD();
	}
	return 1;
}

static uint16_t block_until_event(uint16_t mask) {
	uint16_t evt;
	do {
		xQueueReceive(xEventQueue, &evt, portMAX_DELAY);
		if ((evt & mask) != evt) {
			PRINTF("D%xM%xS%u\n", evt, mask, state);
		}
	} while ((evt & mask) != evt);
	return evt;
}
