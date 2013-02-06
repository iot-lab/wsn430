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
 * \brief Polling task implementation
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
#include "timerB.h"
#include "tsl2550.h"
#include "ds1722.h"
#include "ina209.h"

/* Objects */
#include "constants.h"
#include "global.h"
#include "pollingTask.h"
#include "mainTask.h"
#include "radioTask.h"

/* Function Prototypes */
static void vPowerPollingTask(void* pvParameters);
static void vSensorPollingTask(void* pvParameters);
static void vRadioPollingTask(void* pvParameters);
static uint16_t uPowerPollTime(void);

static void vAppendData(xPollingFrame_t *frame, uint16_t data);
static void vSendFrame(xPollingFrame_t *frame);

/* Local Variables */
static enum powerMode xVoltage, xCurrent, xPower;
static uint16_t xPowerPolling, xPowerCountMax;
static xSemaphoreHandle xPowerSem, xPowerDelaySem;

static portTickType xSensorPreviousWakeTime, xSensorWakePeriod;
static uint8_t xTemperature, xLuminosity;
static uint16_t xSensorPolling, xSensorCountMax;
static xSemaphoreHandle xSensorSem;

static portTickType xRadioPreviousWakeTime, xRadioWakePeriod;
static uint16_t xRadioPolling, xRadioCountMax;
static xSemaphoreHandle xRadioSem;

static xPollingFrame_t xPowerFrame;
static xPollingFrame_t xSensorFrame;
static xPollingFrame_t xRadioFrame;

static void vAppendData(xPollingFrame_t* frame, uint16_t data) {
	if (frame->index == 0) {
		frame->starttime[0] = (myTime >> 8) & 0xFF;
		frame->starttime[1] = myTime & 0xFF;
		frame->starttime[2] = (timerB_time() >> 8) & 0xFF;
		frame->starttime[3] = timerB_time() & 0xFF;
	}

	frame->measures[frame->index][0] = data >> 8;
	frame->measures[frame->index][1] = data & 0xFF;
	frame->index++;
}
static void vSendFrame(xPollingFrame_t *frame) {
	if (frame->index == 0)
		// If no data in frame, skip
		return;

	LED_RED_TOGGLE();
	frame->sync = SYNC_BYTE;
	frame->len = 9 + frame->index * 2;
	frame->type = POLL_DATA;

	// give the frame to the mainTask, in order to send it
	vSendPollingFrame(frame);

	// clear the count and index fields
	frame->count = 0;
	frame->index = 0;
}

void pollingTask_create(uint16_t priority) {
	/* Create the semaphores */
	vSemaphoreCreateBinary( xPowerSem );
	vSemaphoreCreateBinary( xRadioSem );
	vSemaphoreCreateBinary( xSensorSem );
	vSemaphoreCreateBinary( xPowerDelaySem );

	/* Create the tasks */
	xTaskCreate(vPowerPollingTask, "Powerpoll", configMINIMAL_STACK_SIZE, NULL,
			priority, NULL);
	xTaskCreate(vSensorPollingTask, "Sensorpoll", configMINIMAL_STACK_SIZE,
			NULL, priority, NULL);
	xTaskCreate(vRadioPollingTask, "Radiopoll", configMINIMAL_STACK_SIZE, NULL,
			priority, NULL);

	return;
}
static enum powerMode getMode(int value) {
	switch (value) {
	case BATTERY:
		return BATTERY;
	case DC:
		return DC;
	case DIFF:
		return DIFF;
	default:
		return OFF;
	}

}
uint16_t set_power_polling_sensors(uint8_t voltage, uint8_t current,
		uint8_t power) {
	// send data if there was any
	vSendFrame(&xPowerFrame);

	xVoltage = getMode(voltage);
	if (xVoltage == DIFF)
		xVoltage = OFF;
	xCurrent = getMode(current);
	xPower = getMode(power);

	// update sensors field in frame
	xPowerFrame.sensors = xVoltage ? (1 << VOLTAGE_BIT) : 0;
	xPowerFrame.sensors |= xCurrent ? (1 << CURRENT_BIT) : 0;
	xPowerFrame.sensors |= xPower ? (1 << POWER_BIT) : 0;

	xPowerCountMax = POLLING_MEASURE_MAX / (!!xVoltage + !!xCurrent
			+ !!xPower);
	uint16_t period;
	period = xPowerFrame.period[0];
	period <<= 8;
	period += xPowerFrame.period[1];

	if (xPowerCountMax > (POLLING_TIME_MAX * TIMERB_SECOND) / period) {
		xPowerCountMax = (POLLING_TIME_MAX * TIMERB_SECOND) / period;
	}

	return 1;
}

uint16_t set_power_polling_period(uint16_t period) {
	// send data if there was any
	vSendFrame(&xPowerFrame);

	if (period == 0) {
		timerB_unset_alarm(POWERPOLL_ALARM);
		xSemaphoreTake(xPowerSem, 0);
		xPowerPolling = 0;
		return 1;
	}

	if (period < POWER_MIN_PERIOD) {
		period = POWER_MIN_PERIOD;
	}

	if (period <= TIMERB_SECOND / 1000) {
		ina209_set_sample_period(PERIOD_1MS);
	} else if (period <= TIMERB_SECOND / 500) {
		ina209_set_sample_period(PERIOD_2MS);
	} else if (period <= TIMERB_SECOND / 250) {
		ina209_set_sample_period(PERIOD_4MS);
	} else if (period <= TIMERB_SECOND / 125) {
		ina209_set_sample_period(PERIOD_8MS);
	} else if (period <= (((uint32_t) 17 * TIMERB_SECOND) / 1000)) {
		ina209_set_sample_period(PERIOD_17MS);
	} else if (period <= (((uint32_t) 17 * TIMERB_SECOND) / 500)) {
		ina209_set_sample_period(PERIOD_34MS);
	} else {
		ina209_set_sample_period(PERIOD_68MS);
	}

	xPowerCountMax = POLLING_MEASURE_MAX / (!!xVoltage + !!xCurrent
			+ !!xPower);
	if (xPowerCountMax > (POLLING_TIME_MAX * TIMERB_SECOND) / period) {
		xPowerCountMax = (POLLING_TIME_MAX * TIMERB_SECOND) / period;
	}

	// update polling period
	xPowerPolling = 1;
	timerB_set_alarm_from_now(POWERPOLL_ALARM, period, period);
	timerB_register_cb(POWERPOLL_ALARM, uPowerPollTime);

	// update period field in frame
	xPowerFrame.period[0] = period >> 8;
	xPowerFrame.period[1] = period & 0xFF;

	xSemaphoreGive(xPowerSem);

	return 1;
}

static uint16_t uPowerPollTime(void) {
	portBASE_TYPE higherPriority = pdFALSE;
	xSemaphoreGiveFromISR(xPowerDelaySem, &higherPriority);

	if (higherPriority == pdTRUE) {
		taskYIELD();
	}

	return 1;
}

static void vPowerPollingTask(void* pvParameters) {

	// no initial active sensors
	xVoltage = OFF;
	xCurrent = OFF;
	xPower = OFF;
	xPowerPolling = 0;

	xSemaphoreTake(xPowerSem, 0);
	xSemaphoreTake(xPowerDelaySem, 0);

	// initialize the frame
	xPowerFrame.count = 0;
	xPowerFrame.index = 0;
	xPowerFrame.sensors = 0;
	xPowerFrame.period[0] = 0;
	xPowerFrame.period[1] = 0;

	while (1) {
		if (xSemaphoreTake(xPowerSem, portMAX_DELAY) != pdTRUE) {
			continue;
		}

		// Wait for the Sampling semaphore
		if (xSemaphoreTake(xPowerDelaySem, portMAX_DELAY) != pdTRUE) {
			continue;
		}

		LED_BLUE_TOGGLE();

		vGetINA209();

		switch (xVoltage) {
		case BATTERY:
			vAppendData(&xPowerFrame, ina209_read_batt_voltage());
			break;
		case DC:
			vAppendData(&xPowerFrame, ina209_read_dc_voltage());
			break;
		default:
			break;
		}
		switch (xCurrent) {
		case BATTERY:
			vAppendData(&xPowerFrame, ina209_read_batt_current());
			break;
		case DC:
			vAppendData(&xPowerFrame, ina209_read_dc_current());
			break;
		case DIFF:
			vAppendData(&xPowerFrame, ina209_read_dc_current()
					+ ina209_read_batt_current());
			break;
		default:
			break;
		}
		switch (xPower) {
		case BATTERY:
			vAppendData(&xPowerFrame, ina209_read_batt_power());
			break;
		case DC:
			vAppendData(&xPowerFrame, ina209_read_dc_power());
			break;
		case DIFF:
			vAppendData(&xPowerFrame, ina209_read_dc_power()
					+ ina209_read_batt_power());
			break;
		default:
			break;
		}

		vReleaseINA209();
		xPowerFrame.count++;

		if (xPowerFrame.count >= xPowerCountMax) {
			vSendFrame(&xPowerFrame);
		}

		if (xPowerPolling) {
			xSemaphoreGive(xPowerSem);
		}
	}
}

uint16_t set_sensor_polling_sensors(uint8_t temperature, uint8_t luminosity) {

	// send data if there was any
	vSendFrame(&xSensorFrame);

	xTemperature = !!temperature;
	xLuminosity = !!luminosity;

	// update sensors field in frame
	xSensorFrame.sensors = xTemperature ? (1 << TEMPERATURE_BIT) : 0;
	xSensorFrame.sensors |= xLuminosity ? (1 << LUMINOSITY_BIT) : 0;


	xSensorCountMax = POLLING_MEASURE_MAX / ((xTemperature != 0)
			+ (xLuminosity != 0));

	if (xSensorCountMax > (POLLING_TIME_MAX * SYSTEM_SECOND) / xSensorWakePeriod) {
		xSensorCountMax = (POLLING_TIME_MAX * SYSTEM_SECOND) / xSensorWakePeriod;
	}

	return 1;
}

uint16_t set_sensor_polling_period(uint16_t period) {

	// send data if there was any
	vSendFrame(&xSensorFrame);

	if (period == 0) {
		xSemaphoreTake(xSensorSem, 0);
		xSensorPolling = 0;
		return 1;
	}

	if (period < SENSOR_MIN_PERIOD)
		period = SENSOR_MIN_PERIOD;

	xSensorWakePeriod = period;
	xSensorPreviousWakeTime = xTaskGetTickCount();
	xSensorPolling = 1;

	xSensorCountMax = POLLING_MEASURE_MAX / ((xTemperature != 0)
			+ (xLuminosity != 0));

	if (xSensorCountMax * xSensorWakePeriod > POLLING_TIME_MAX * SYSTEM_SECOND) {
		xSensorCountMax = (POLLING_TIME_MAX * SYSTEM_SECOND) / xSensorWakePeriod;
	}

	// update period field in frame
	xSensorFrame.period[0] = period >> 8;
	xSensorFrame.period[1] = period & 0xFF;

	xSemaphoreGive(xSensorSem);
	return 1;
}

static void vSensorPollingTask(void* pvParameters) {
	uint16_t value;

	// get actual time
	xSensorPreviousWakeTime = xTaskGetTickCount();

	// initial period to 1s
	xSensorWakePeriod = 0;

	// no initial active sensors
	xTemperature = 0;
	xLuminosity = 0;
	xSensorPolling = 0;
	xSemaphoreTake(xSensorSem, 0);

	// initialize the frame
	xSensorFrame.count = 0;
	xSensorFrame.index = 0;
	xSensorFrame.sensors = 0;
	xSensorFrame.period[0] = 0;
	xSensorFrame.period[1] = 0;

	while (1) {
		if (xSemaphoreTake(xSensorSem, portMAX_DELAY) != pdTRUE) {
			continue;
		}

		// Wait until 'xWakePeriod' ticks have elapsed since last call
		vTaskDelayUntil(&xSensorPreviousWakeTime, xSensorWakePeriod);
		LED_GREEN_TOGGLE();

		if (xTemperature) {
			vGetDS1722();
			value = ds1722_read_MSB();
			value <<= 8;
			value += ds1722_read_LSB();
			vReleaseDS1722();
			vAppendData(&xSensorFrame, value);
		}

		if (xLuminosity) {
			vGetTSL2550();
			value = tsl2550_read_adc0();
			value <<= 8;
			value += tsl2550_read_adc1();
			vReleaseTSL2550();
			vAppendData(&xSensorFrame, value);
		}

		xSensorFrame.count++;
		if (xSensorFrame.count >= xSensorCountMax) {
			vSendFrame(&xSensorFrame);
		}

		if (xSensorPolling) {
			xSemaphoreGive(xSensorSem);
		}
	}
}

uint16_t set_radio_polling(uint16_t period) {

	// send data if there was any
	vSendFrame(&xRadioFrame);

	if (period == 0) {
		xRadioPolling = 0;
		xSemaphoreTake(xRadioSem, 0);
		return 1;
	}

	if (period < RADIO_MIN_PERIOD)
		period = RADIO_MIN_PERIOD;

	xRadioWakePeriod = period;
	xRadioPolling = 1;

	xRadioCountMax = POLLING_MEASURE_MAX;

	if (xRadioCountMax * period > POLLING_TIME_MAX * SYSTEM_SECOND) {
		xRadioCountMax = POLLING_TIME_MAX * SYSTEM_SECOND / period;
	}

	// update sensors field in frame
	xRadioFrame.sensors = (1 << RSSI_BIT);
	// update period field in frame
	xRadioFrame.period[0] = period >> 8;
	xRadioFrame.period[1] = period & 0xFF;

	xSemaphoreGive(xRadioSem);
	return 1;
}

static void vRadioPollingTask(void* pvParameters) {
	uint8_t value;

	// get actual time
	xRadioPreviousWakeTime = xTaskGetTickCount();

	// initial period to 1s
	xRadioWakePeriod = 1000;

	// no initial active sensors
	xRadioPolling = 0;
	xSemaphoreTake(xRadioSem, 0);

	// initialize the frame
	xRadioFrame.count = 0;
	xRadioFrame.index = 0;
	xRadioFrame.sensors = 0;
	xRadioFrame.period[0] = 0;
	xRadioFrame.period[1] = 0;

	while (1) {
		if (xRadioPolling) {
			// Wait until 'xWakePeriod' ticks have elapsed since last call
			vTaskDelayUntil(&xRadioPreviousWakeTime, xRadioWakePeriod);

			LED_RED_TOGGLE();

			// poll sensor
			radio_getrssi(&value);
			vAppendData(&xRadioFrame, (int16_t) value);
			xRadioFrame.count++;

			if (xRadioFrame.count >= xRadioCountMax) {
				vSendFrame(&xRadioFrame);
			}

		} else {
			xSemaphoreTake(xRadioSem, portMAX_DELAY);
		}
	}
}
