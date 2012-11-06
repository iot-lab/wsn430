/*
 * Copyright  2008-2009 INRIA/SensLab
 * 
 * <dev-team@senslab.info>
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

/**
 * \file
 * \brief Main task implementation
 * \author Clément Burin des Roziers
 * \date 2009
 */

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Drivers */
#include "leds.h"
#include "uart1.h"
#include "tsl2550.h"
#include "ds1722.h"
#include "ina209.h"

/* Objects */
#include "constants.h"
#include "global.h"
#include "mainTask.h"
#include "pollingTask.h"
#include "radioTask.h"
#include "supply-control.h"
#include "dac.h"

// events
#define EVENT_FRAME_RX 0x1
#define EVENT_FRAME_TX 0x2

/* Function Prototypes */
static void vMainTask(void* pvParameters);
static void vDriversInit(void);
static void vParseFrame(void);
static void vSendFrame(uint8_t* data, int16_t len);
static void vCharHandler_irq(uint8_t c);

/* Local variables */
static xQueueHandle xMainEventQ, xFrameQ;

static xRXFrame_t xRxFrame;
static xTXFrame_t xTxFrame;
static xPollingFrame_t xPollingFrame;

void mainTask_create(uint16_t priority) {
	/* Create the event queue */
	xMainEventQ = xQueueCreate(5, sizeof(uint16_t));

	/* Create a TX frame queue */
	xFrameQ = xQueueCreate(2, sizeof(xPollingFrame_t));

	/* Create the task */
	xTaskCreate(vMainTask, "main", 1024, NULL, priority, NULL);
}

void vSendPollingFrame(void *frame) {
	uint16_t event;
	// place the frame in the queue, don't wait if not possible
	xQueueSendToBack(xFrameQ, frame, 0);

	event = EVENT_FRAME_TX;
	xQueueSendToBack( xMainEventQ, &event, 0);
}

static void vMainTask(void* pvParameters) {
	uint16_t event;

	/* setup the different device drivers */
	vDriversInit();

	while (1) {
		/* Wait until event received */
		if (xQueueReceive( xMainEventQ, &event, portMAX_DELAY ) == pdTRUE) {
			switch (event) {
			case EVENT_FRAME_RX:
				vParseFrame();
				vSendFrame(xTxFrame.data, xTxFrame.len + 2);
				break;
			case EVENT_FRAME_TX:
				if (xQueueReceive(xFrameQ, &xPollingFrame, 0) == pdTRUE) {
					LED_GREEN_TOGGLE();

					vSendFrame(xPollingFrame.data, xPollingFrame.len + 2);
				}
				break;
			}
		}
	}
}

static void vDriversInit(void) {
	LEDS_OFF();

	// setup IOs for relays control
	setup_senslabgw_ios();
	set_opennode_main_supply_on();
	set_opennode_battery_supply_on();

	// setup uart
	vGetUART();
	uart1_rx_cb = vCharHandler_irq;
	uart1_init(UART_CONFIG);
	uart1_register_callback((uart1_cb_t) uart1_rx_cb);
	vReleaseUART();

	// setup luminosity sensor
	vGetTSL2550();
	tsl2550_powerup();
	vReleaseTSL2550();

	// setup current sensor
	vGetINA209();
	ina209_init();
	vReleaseINA209();

	// setup temperature sensor
	vGetDS1722();
	ds1722_set_res(12);
	ds1722_sample_cont();
	vReleaseDS1722();

	// Setup DAC
	dac_init();

	// by default, control node leds do not show up
	leds_policy = MANUAL_LED_USE;
	LEDS_ON();
}

static void vParseFrame() {
	uint16_t value;
	uint32_t longValue;

	if (leds_policy == DEFAULT_LED_USE)
		LED_GREEN_TOGGLE();

	// Fill known fields of response frame
	xTxFrame.sync = SYNC_BYTE;
	// set to NACK, change it later if command successful
	xTxFrame.type = xRxFrame.type;
	xTxFrame.ack = NACK;
	// default payload length is 2
	xTxFrame.len = 2;

	/* Analyse the Command, and take corresponding action. */
	switch (xRxFrame.type) {
	case OPENNODE_START:
		set_opennode_battery_supply_on();
		set_opennode_main_supply_on();
		xTxFrame.ack = ACK;
		break;

	case OPENNODE_STARTBATTERY:
		set_opennode_battery_supply_on();
		set_opennode_main_supply_off();
		xTxFrame.ack = ACK;
		break;

	case OPENNODE_STOP:
		set_opennode_battery_supply_off();
		set_opennode_main_supply_off();
		xTxFrame.ack = ACK;
		break;

	case GET_RSSI:
		if (radio_getrssi(xTxFrame.payload+1) == 0) {
			// Failed to fetch
			break;
		}
		xTxFrame.payload[0] = 0;
		xTxFrame.len += 2;
		xTxFrame.ack = ACK;
		break;

	case GET_TEMPERATURE:
		vGetDS1722();
		xTxFrame.payload[0] = ds1722_read_MSB();
		xTxFrame.payload[1] = ds1722_read_LSB();
		vReleaseDS1722();
		xTxFrame.len += 2;
		xTxFrame.ack = ACK;
		break;

	case GET_LUMINOSITY:
		vGetTSL2550();
		xTxFrame.payload[0] = tsl2550_read_adc0();
		xTxFrame.payload[1] = tsl2550_read_adc1();
		vReleaseTSL2550();
		xTxFrame.len += 2;
		xTxFrame.ack = ACK;
		break;

	case GET_BATTERY_VOLTAGE:
		vGetINA209();
		value = ina209_read_batt_voltage();
		xTxFrame.payload[0] = (uint8_t) ((value & 0xFF00) >> 8);
		xTxFrame.payload[1] = (uint8_t) (value & 0x00FF);
		vReleaseINA209();
		xTxFrame.len += 2;
		xTxFrame.ack = ACK;
		break;

	case GET_BATTERY_CURRENT:
		vGetINA209();
		value = ina209_read_batt_current();
		xTxFrame.payload[0] = (uint8_t) ((value & 0xFF00) >> 8);
		xTxFrame.payload[1] = (uint8_t) (value & 0x00FF);
		vReleaseINA209();
		xTxFrame.len += 2;
		xTxFrame.ack = ACK;
		break;

	case GET_BATTERY_POWER:
		vGetINA209();
		value = ina209_read_batt_power();
		xTxFrame.payload[0] = (uint8_t) ((value & 0xFF00) >> 8);
		xTxFrame.payload[1] = (uint8_t) (value & 0x00FF);
		vReleaseINA209();
		xTxFrame.len += 2;
		xTxFrame.ack = ACK;
		break;

	case GET_DC_VOLTAGE:
		vGetINA209();
		value = ina209_read_dc_voltage();
		xTxFrame.payload[0] = (uint8_t) ((value & 0xFF00) >> 8);
		xTxFrame.payload[1] = (uint8_t) (value & 0x00FF);
		xTxFrame.ack = ACK;
		xTxFrame.len += 2;
		vReleaseINA209();
		break;

	case GET_DC_CURRENT:
		vGetINA209();
		value = ina209_read_dc_current();
		xTxFrame.payload[0] = (uint8_t) ((value & 0xFF00) >> 8);
		xTxFrame.payload[1] = (uint8_t) (value & 0x00FF);
		vReleaseINA209();
		xTxFrame.len += 2;
		xTxFrame.ack = ACK;
		break;

	case GET_DC_POWER:
		value = ina209_read_dc_power();
		vGetINA209();
		xTxFrame.payload[0] = (uint8_t) ((value & 0xFF00) >> 8);
		xTxFrame.payload[1] = (uint8_t) (value & 0x00FF);
		vReleaseINA209();
		xTxFrame.len += 2;
		xTxFrame.ack = ACK;
		break;

	case GET_DIFF_CURRENT:
		vGetINA209();
		value = ina209_read_dc_current() + ina209_read_batt_current();
		xTxFrame.payload[0] = (uint8_t) ((value & 0xFF00) >> 8);
		xTxFrame.payload[1] = (uint8_t) (value & 0x00FF);
		vReleaseINA209();
		xTxFrame.len += 2;
		xTxFrame.ack = ACK;
		break;

	case GET_DIFF_POWER:
		vGetINA209();
		value = ina209_read_dc_power() + ina209_read_batt_power();
		xTxFrame.payload[0] = (uint8_t) ((value & 0xFF00) >> 8);
		xTxFrame.payload[1] = (uint8_t) (value & 0x00FF);
		vReleaseINA209();
		xTxFrame.len += 2;
		xTxFrame.ack = ACK;
		break;

	case CONFIG_CC1100:
		// Freq
		longValue = xRxFrame.payload[0];
		longValue <<= 8;
		longValue += xRxFrame.payload[1];
		longValue <<= 8;
		longValue += xRxFrame.payload[2];

		if (!radio_cc1100_setfreq(longValue)) {
			break;
		}
		if (!radio_cc1100_setchanbw(xRxFrame.payload[3], xRxFrame.payload[4])) {
			break;
		}
		if (!radio_cc1100_setdatarate(xRxFrame.payload[5], xRxFrame.payload[6])) {
			break;
		}
		if (!radio_cc1100_setmod(xRxFrame.payload[7])) {
			break;
		}
		if (!radio_cc1100_settxpower(xRxFrame.payload[8])) {
			break;
		}

		xTxFrame.ack = ACK;
		break;

	case CONFIG_CC2420:
		value = xRxFrame.payload[0];
		value <<= 8;
		value += xRxFrame.payload[1];

		if (!radio_cc2420_setfreq(value)) {
			break;
		}
		if (!radio_cc2420_settxpower(xRxFrame.payload[2])) {
			break;
		}
		xTxFrame.ack = ACK;
		break;

	case CONFIG_POWERPOLL:
		value = xRxFrame.payload[3];
		value <<= 8;
		value += xRxFrame.payload[4];

		if (!set_power_polling_sensors(xRxFrame.payload[0],
				xRxFrame.payload[1], xRxFrame.payload[2])) {
			break;
		}

		if (!set_power_polling_period(value)) {
			break;
		}

		xTxFrame.ack = ACK;
		break;

	case CONFIG_SENSORPOLL:
		value = xRxFrame.payload[1];
		value <<= 8;
		value += xRxFrame.payload[2];

		if (!set_sensor_polling_sensors(xRxFrame.payload[0] & 0xF0,
				xRxFrame.payload[0] & 0x0F)) {
			break;
		}

		if (!set_sensor_polling_period(value)) {
			break;
		}

		xTxFrame.ack = ACK;

		break;

	case CONFIG_RADIOPOLL:
		value = xRxFrame.payload[0];
		value <<= 8;
		value += xRxFrame.payload[1];

		if (set_radio_polling(value)) {
			xTxFrame.ack = ACK;
		}
		break;

	case CONFIG_RADIONOISE:
		if (radio_setnoise(xRxFrame.payload[0]) == 0) {
			// Failed to fetch
			break;
		}
		xTxFrame.ack = ACK;
		break;

	case SET_DAC0:
		value = xRxFrame.payload[0];
		value <<= 8;
		value += xRxFrame.payload[1];

		dac0_set(value);
		xTxFrame.ack = ACK;
		break;

	case SET_DAC1:
		value = xRxFrame.payload[0];
		value <<= 8;
		value += xRxFrame.payload[1];

		dac1_set(value);
		xTxFrame.ack = ACK;
		break;

	case SET_TIME:
		value = xRxFrame.payload[0];
		value <<= 8;
		value += xRxFrame.payload[1];

		myTime = value;

		value += xRxFrame.payload[2];
		value <<= 8;
		value += xRxFrame.payload[3];

		TBR = value;

		xTxFrame.ack = ACK;
		break;

	default:
		break;

	}
}

static void vSendFrame(uint8_t* data, int16_t len) {
	int16_t i;

	vGetUART();
	if (leds_policy == DEFAULT_LED_USE)
		LED_RED_TOGGLE();

	for (i = 0; i < len; i++) {
		uart1_putchar(data[i]);
	}

	vReleaseUART();
	LED_BLUE_TOGGLE();

}

static void vCharHandler_irq(uint8_t c) {
	static uint16_t ix = 0;
	uint16_t higherPriority = pdFALSE;

	if (leds_policy == DEFAULT_LED_USE)
		LED_BLUE_TOGGLE();

	if (ix == 0) {
		if (c == SYNC_BYTE) {
			// First byte, SYNC
			xRxFrame.data[ix] = c;
			ix++;
		}
	} else if (ix == 1) {
		if (0 < c <= FRAME_LENGTH_MAX) {
			// Valid length byte
			xRxFrame.data[ix] = c;
			ix++;
		} else {
			// Reset frame
			ix = 0;
		}
	} else if (ix == (uint16_t) (xRxFrame.len + 1)) {
		uint16_t event = EVENT_FRAME_RX;
		xRxFrame.data[ix] = c;

		xQueueSendToBackFromISR( xMainEventQ, &event, &higherPriority);
		ix = 0;

	} else {
		xRxFrame.data[ix] = c;
		ix++;
	}

	if (higherPriority == pdTRUE) {
		taskYIELD();
	}
}
