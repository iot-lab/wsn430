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
 * \brief Radio task implementation for the CC2420
 * \author Clément Burin des Roziers
 * \date 2009
 */

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Drivers */
#include "cc2420.h"
#include "timerB.h"

#include "global.h"
#include "mainTask.h"
#include "radioTask.h"

enum radioState {
	STATE_IDLE = 0x1, STATE_RSSI = 0x2, STATE_NOISE = 0x3
};

enum radioEvent {
	EVENT_GETRSSI = 0x1, EVENT_STARTNOISE = 0x2, EVENT_STOPNOISE = 0x3
};

/* Local function prototypes */
static void vRadioTask(void* pvParameters);
static void vRadioInit(void);
static void get_rssi(void);
static void start_noise(void);
static void stop_noise(void);

/* Local variables */
static int state = STATE_IDLE;
static xQueueHandle xRadioEventQ;
static xSemaphoreHandle xDataReadySem;
static xSemaphoreHandle xWaitDoneSem;

static uint8_t lastRssi;

void radioTask_create(int priority) {
	/* Create the event queue */
	xRadioEventQ = xQueueCreate(1, sizeof(int));

	// Create a binary semaphore for data ready synchronization
	vSemaphoreCreateBinary( xDataReadySem );
	// Make sure it's taken
	xSemaphoreTake(xDataReadySem, 0);

	// Create a binary semaphore for data ready synchronization
	vSemaphoreCreateBinary( xWaitDoneSem );
	// Make sure it's taken
	xSemaphoreTake(xWaitDoneSem, 0);

	/* Create the task */
	xTaskCreate( vRadioTask, "radio", configMINIMAL_STACK_SIZE, NULL, priority,
			NULL );

	return;
}

int radio_getrssi(uint8_t * data) {
	int event = EVENT_GETRSSI;

	if (state != STATE_IDLE) {
		// Actual state can't accept this request
		return 0;
	}

	// Send command, wait for 100ms max
	if (xQueueSendToBack(xRadioEventQ, &event, 1) == pdFALSE) {
		// Error putting event in queue
		return 0;
	}

	// Get the data ready semaphore, wait for 100ms at most
	if (xSemaphoreTake(xDataReadySem, 1) == pdFALSE) {
		// Error getting result
		return 0;
	}

	// Data has been placed!
	*data = lastRssi;

	return 1;
}

int radio_setnoise(uint16_t on) {
	int event;

	if (on) {
		event = EVENT_STARTNOISE;
	} else {
		event = EVENT_STOPNOISE;
	}

	if (state != STATE_IDLE) {
		// Actual state can't accept this request
		return 0;
	}

	// Send command, wait for 10ms max
	if (xQueueSendToBack(xRadioEventQ, &event, 1) == pdFALSE) {
		// Error putting event in queue
		return 0;
	}

	// Get the data ready semaphore, wait for 10ms at most
	if (xSemaphoreTake(xDataReadySem, 1) == pdFALSE) {
		// Error getting result
		return 0;
	}

	return 1;
}

// CC1101 specific configuration
int radio_cc1101_setfreq(uint32_t freq) {
	// wrong radio, return error
	return 0;
}

int radio_cc1101_setmod(uint8_t mod_format) {
	// wrong radio, return error
	return 0;
}

int radio_cc1101_settxpower(uint8_t patable) {
	// wrong radio, return error
	return 0;
}

int radio_cc1101_setchanbw(uint8_t chanbw_e, uint8_t chanbw_m) {
	// wrong radio, return error
	return 0;
}

int radio_cc1101_setdatarate(uint8_t drate_e, uint8_t drate_m) {
	// wrong radio, return error
	return 0;
}

// CC2420 specific configuration
int radio_cc2420_setfreq(uint16_t freq) {
	int ret;
	if (state != STATE_IDLE) {
		// Actual state can't accept this request
		return 0;
	}

	/* get the required mutex */
	vGetCC2420();

	/* update the modulation format */
	ret = cc2420_set_frequency(freq);

	/* Release the mutex */
	vReleaseCC2420();

	return ret;
}

int radio_cc2420_settxpower(uint8_t palevel) {
	int ret;
	if (state != STATE_IDLE) {
		// Actual state can't accept this request
		return 0;
	}

	/* get the required mutex */
	vGetCC2420();

	/* update the modulation format */
	ret = cc2420_set_txpower(palevel);

	/* Release the mutex */
	vReleaseCC2420();

	return ret;
}

static void vRadioTask(void* pvParameters) {
	static int event;

	vRadioInit();

	state = STATE_IDLE;

	while (1) {
		if (xQueueReceive( xRadioEventQ, &event, portMAX_DELAY) == pdTRUE) {
			switch (event) {
			case EVENT_GETRSSI:
				if (state != STATE_IDLE) {
					break;
				}
				get_rssi();
				break;
			case EVENT_STARTNOISE:
				if (state != STATE_IDLE) {
					break;
				}
				start_noise();
				break;
			case EVENT_STOPNOISE:
				if (state != STATE_NOISE) {
					break;
				}
				stop_noise();
				break;
			}

			// Give the semaphore to tell command is successful
			xSemaphoreGive(xDataReadySem);
		}
	}
}

static void vRadioInit(void) {
	/* get the required mutex */
	vGetCC2420();

	/* Initialize the radio driver */
	cc2420_init();
	cc2420_cmd_idle();

	/* Release the mutex */
	vReleaseCC2420();
}

static uint16_t radio_alarm_elapsed() {
	uint16_t higherPriority = pdFALSE;
	xSemaphoreGiveFromISR(xWaitDoneSem, &higherPriority);

	if (higherPriority == pdTRUE) {
		taskYIELD();
	}

	return 1;
}

static void get_rssi(void) {
	state = STATE_RSSI;

	/* get the required mutex */
	vGetCC2420();

	/* flush RX FIFO */
	cc2420_cmd_idle();
	cc2420_cmd_flushrx();

	/* start RX */
	cc2420_cmd_rx();

	/* wait for ~1ms */
	timerB_register_cb(RADIO_ALARM, radio_alarm_elapsed);
	timerB_set_alarm_from_now(RADIO_ALARM, TIMERB_SECOND / 1000, 0);
	xSemaphoreTake(xWaitDoneSem, 1);

	/* read rssi */
	lastRssi = cc2420_get_rssi();

	/* go back to idle */
	cc2420_cmd_idle();

	/* Give back semaphore */
	vReleaseCC2420();

	state = STATE_IDLE;
}

static void start_noise(void) {
	/* get the required mutex */
	vGetCC2420();

	state = STATE_NOISE;

	/* flush TX FIFO */
	cc2420_cmd_idle();
	cc2420_cmd_flushtx();

	/* start TX (noise) */
	cc2420_cmd_tx();

	/* Give back semaphore */
	vReleaseCC2420();
}

static void stop_noise(void) {
	/* get the required mutex */
	vGetCC2420();

	/* stop TX */
	cc2420_cmd_idle();

	state = STATE_IDLE;

	/* Give back semaphore */
	vReleaseCC2420();

}
