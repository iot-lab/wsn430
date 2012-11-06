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
 * \brief Radio task implementation for the CC1100
 * \author Clément Burin des Roziers
 * \date 2009
 */

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Drivers */
#include "cc1100.h"
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

// CC1100 specific configuration
int radio_cc1100_setfreq(uint32_t freq) {
	if (state != STATE_IDLE) {
		// Actual state can't accept this request
		return 0;
	}

	/* get the required mutex */
	vGetCC1100();

	/* set the frequency */
	cc1100_cfg_freq(freq);

	cc1100_cmd_calibrate();

	/* Release the mutex */
	vReleaseCC1100();

	return 1;
}

int radio_cc1100_setmod(uint8_t mod_format) {
	if (state != STATE_IDLE) {
		// Actual state can't accept this request
		return 0;
	}

	/* get the required mutex */
	vGetCC1100();

	/* update the modulation format */
	cc1100_cfg_mod_format(mod_format);

	/* Release the mutex */
	vReleaseCC1100();

	return 1;
}

int radio_cc1100_settxpower(uint8_t patable) {
	if (state != STATE_IDLE) {
		// Actual state can't accept this request
		return 0;
	}

	/* get the required mutex */
	vGetCC1100();

	uint8_t table[1];
	table[0] = patable; // -10dBm
	cc1100_cfg_patable(table, 1);
	cc1100_cfg_pa_power(0);

	/* Release the mutex */
	vReleaseCC1100();

	return 1;
}

int radio_cc1100_setchanbw(uint8_t chanbw_e, uint8_t chanbw_m) {
	if (state != STATE_IDLE) {
		// Actual state can't accept this request
		return 0;
	}

	/* get the required mutex */
	vGetCC1100();

	// set channel bandwidth (560 kHz)
	cc1100_cfg_chanbw_e(chanbw_e);
	cc1100_cfg_chanbw_m(chanbw_m);

	cc1100_cmd_calibrate();

	/* Release the mutex */
	vReleaseCC1100();

	return 1;
}

int radio_cc1100_setdatarate(uint8_t drate_e, uint8_t drate_m) {
	if (state != STATE_IDLE) {
		// Actual state can't accept this request
		return 0;
	}

	/* get the required mutex */
	vGetCC1100();

	// set data rate (0xD/0x2F is 250kbps)
	cc1100_cfg_drate_e(drate_e);
	cc1100_cfg_drate_m(drate_m);

	cc1100_cmd_calibrate();

	/* Release the mutex */
	vReleaseCC1100();

	return 1;
}

// CC2420 specific configuration
int radio_cc2420_setfreq(uint16_t freq) {
	// wrong radio, return error
	return 0;
}

int radio_cc2420_settxpower(uint8_t palevel) {
	// wrong radio return error
	return 0;
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
	vGetCC1100();

	/* Initialize the radio to standard parameters */
	cc1100_init();

	cc1100_cfg_append_status(CC1100_APPEND_STATUS_DISABLE);
	cc1100_cfg_crc_autoflush(CC1100_CRC_AUTOFLUSH_DISABLE);
	cc1100_cfg_white_data(CC1100_DATA_WHITENING_ENABLE);
	cc1100_cfg_crc_en(CC1100_CRC_CALCULATION_ENABLE);
	cc1100_cfg_freq_if(0x0C);
	cc1100_cfg_fs_autocal(CC1100_AUTOCAL_NEVER);
	cc1100_cfg_mod_format(CC1100_MODULATION_MSK);
	cc1100_cfg_sync_mode(CC1100_SYNCMODE_30_32);
	cc1100_cfg_manchester_en(CC1100_MANCHESTER_DISABLE);

	// set channel bandwidth (560 kHz)
	cc1100_cfg_chanbw_e(0);
	cc1100_cfg_chanbw_m(2);

	// set data rate (0xD/0x2F is 250kbps)
	cc1100_cfg_drate_e(0x0D);
	cc1100_cfg_drate_m(0x2F);

	uint8_t table[1];
	table[0] = 0x27; // -10dBm
	cc1100_cfg_patable(table, 1);
	cc1100_cfg_pa_power(0);

	/* Release the mutex */
	vReleaseCC1100();
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
	vGetCC1100();

	/* flush RX FIFO */
	cc1100_cmd_idle();
	cc1100_cmd_flush_rx();

	/* calibrate */
	cc1100_cmd_calibrate();

	/* start RX */
	cc1100_cmd_rx();

	/* wait for ~1ms */
	timerB_register_cb(RADIO_ALARM, radio_alarm_elapsed);
	timerB_set_alarm_from_now(RADIO_ALARM, TIMERB_SECOND / 1000, 0);
	xSemaphoreTake(xWaitDoneSem, 1);

	/* read rssi */
	lastRssi= cc1100_status_rssi();

	/* go back to idle */
	cc1100_cmd_idle();
	cc1100_cmd_flush_rx();

	/* Give back semaphore */
	vReleaseCC1100();

	state = STATE_IDLE;
}

static void start_noise(void) {
	/* get the required mutex */
	vGetCC1100();

	state = STATE_NOISE;

	/* flush TX FIFO */
	cc1100_cmd_idle();
	cc1100_cmd_flush_tx();

	/* calibrate */
	cc1100_cmd_calibrate();

	/* start TX (noise) */
	cc1100_cmd_tx();

	/* Give back semaphore */
	vReleaseCC1100();
}

static void stop_noise(void) {
	/* get the required mutex */
	vGetCC1100();

	/* stop TX */
	cc1100_cmd_idle();

	state = STATE_IDLE;

	/* Give back semaphore */
	vReleaseCC1100();

}
