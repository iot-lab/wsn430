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


/* Global includes */
#include <io.h>
#include <stdlib.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Project Includes */
#include "leds.h"
#include "phy.h"

/* Drivers Include */
#include "cc2420.h"
#include "spi1.h"
#include "timerB.h"

/* Type def */
enum phy_state {
	IDLE = 1, RX = 2
};

#define PHY_MAX_LENGTH 125
#define PHY_FOOTER_LENGTH 2

/* Function Prototypes */
static void cc2420_task(void* param);
static void cc2420_driver_init(void);
static void handle_received_frame(void);
static void restore_state(void);

/* CC2420 callback functions */
static uint16_t sync_irq(void);
static uint16_t rx_irq(void);

/* Static variables */
static xSemaphoreHandle rx_sem, spi_mutex;
static phy_rx_callback_t rx_cb;
static uint8_t radio_channel, radio_power;
static enum phy_state state;
static volatile uint16_t sync_word_time;
static uint8_t rx_data[PHY_MAX_LENGTH + PHY_FOOTER_LENGTH];
static uint16_t rx_data_length;

void phy_init(xSemaphoreHandle spi_m, phy_rx_callback_t callback,
		uint8_t channel, enum phy_tx_power power) {
	// Store the SPI mutex
	spi_mutex = spi_m;

	// Store the callback
	rx_cb = callback;

	// Store the channel
	radio_channel = channel;

	// Store the TX power
	switch (power) {
	case PHY_TX_0dBm:
		radio_power = 31;
		break;
	case PHY_TX_5dBm:
		radio_power = 19;
		break;
	case PHY_TX_10dBm:
		radio_power = 11;
		break;
	case PHY_TX_20dBm:
		radio_power = 4;
		break;
	default:
		radio_power = 31;
		break;
	}

	// Initialize the semaphore
	vSemaphoreCreateBinary(rx_sem);

	// Set initial state
	state = IDLE;

	// Create the task
	xTaskCreate(cc2420_task, (const signed char*) "phy_cc2420", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES-1, NULL);
}

void phy_rx(void) {
	// Take the SPI mutex
	xSemaphoreTake(spi_mutex, portMAX_DELAY);

	// set IDLE state, flush
	cc2420_cmd_idle();
	cc2420_cmd_flushrx();
	cc2420_io_sfd_int_clear();
	cc2420_io_fifop_int_clear();

	// Start RX
	cc2420_cmd_rx();

	// Release Semaphore
	xSemaphoreGive(spi_mutex);

	// Update state
	state = RX;
}

void phy_idle(void) {
	// Take the Semaphore
	xSemaphoreTake(spi_mutex, portMAX_DELAY);

	// Set Idle
	cc2420_cmd_idle();

	// Release semaphore
	xSemaphoreGive(spi_mutex);

	// Update state
	state = IDLE;
}

uint16_t phy_send(uint8_t* data, uint16_t length, uint16_t *timestamp) {
	uint8_t tx_length;

	if (length > PHY_MAX_LENGTH) {
		return 0;
	}

	// Take semaphore
	xSemaphoreTake(spi_mutex, portMAX_DELAY);

	// Stop radio and flush TX FIFO
	cc2420_cmd_idle();
	cc2420_cmd_flushtx();

	// Start TX
	cc2420_cmd_tx();

	// Send length byte and first set
	tx_length = length + 2;
	cc2420_fifo_put(&tx_length, 1);
	cc2420_fifo_put(data, length);

	// Wait until the last byte is sent
	while (!cc2420_io_sfd_read()) {
		nop();
	}

	while (cc2420_io_sfd_read()) {
		nop();
	}

	// Release semaphore
	xSemaphoreGive(spi_mutex);

	// Set timestamp if required
	if (timestamp != 0x0) {
		*timestamp = sync_word_time;
	}

	// Restore the state
	restore_state();

	return 1;
}

uint16_t phy_send_cca(uint8_t* data, uint16_t length, uint16_t *timestamp) {

	// Take semaphore
	xSemaphoreTake(spi_mutex, portMAX_DELAY);

	// If not in RX, set RX
	if (state != RX) {
		cc2420_cmd_rx();
	}

	// Wait until a valid RSSI is measured
	while ((cc2420_get_status() & CC2420_STATUS_RSSI_VALID) == 0) {
		nop();
	}

	// Release semaphore
	xSemaphoreGive(spi_mutex);

	// Check if channel is clear
	if (cc2420_io_cca_read()) {
		// If not, restore
		restore_state();
		return 0;
	}

	// If clear, force Send
	return phy_send(data, length, timestamp);
}

static void cc2420_task(void* param) {
	// Initialize the radio
	cc2420_driver_init();

	// Make sure the semaphore is taken
	xSemaphoreTake(rx_sem, 0);

	// Infinite loop for handling received frames
	while (1) {
		if (xSemaphoreTake(rx_sem, portMAX_DELAY) == pdTRUE) {
			handle_received_frame();
		}
	}
}

static void cc2420_driver_init(void) {
	// Take the SPI mutex before any radio access
	xSemaphoreTake(spi_mutex, portMAX_DELAY);

	// Initialize the radio driver
	cc2420_init();
	cc2420_write_reg(CC2420_REG_MDMCTRL0, 0x02E2);
	cc2420_write_reg(CC2420_REG_RXCTRL1, 0x2A56);
	cc2420_write_reg(CC2420_REG_SECCTRL0, 0x0);
	cc2420_write_reg(CC2420_REG_IOCFG0, 0x007F);

	// Set channel
	cc2420_set_frequency(2405 + 5 * radio_channel);

	// Set TX power
	cc2420_set_txpower(radio_power);

	// Set interrupts
	cc2420_io_sfd_register_cb(sync_irq);
	cc2420_io_sfd_int_set_rising();
	cc2420_io_sfd_int_clear();
	cc2420_io_sfd_int_enable();

	cc2420_io_fifop_register_cb(rx_irq);
	cc2420_io_fifop_int_set_rising();
	cc2420_io_fifop_int_clear();
	cc2420_io_fifop_int_enable();

	// Release mutex
	xSemaphoreGive(spi_mutex);
}

static void restore_state(void) {
	// Reset old state
	switch (state) {
	case IDLE:
		phy_idle();
		break;
	case RX:
		phy_rx();
		break;
	}
}

static void handle_received_frame(void) {
	uint8_t length;
	// Take semaphore
	xSemaphoreTake(spi_mutex, portMAX_DELAY);

	// Check there is at least one byte in FIFO
	if (!cc2420_io_fifo_read()) {
		xSemaphoreGive(spi_mutex);
		restore_state();
		return;
	}

	// Get length byte
	cc2420_fifo_get(&length, 1);

	// Check length
	if (length > (PHY_MAX_LENGTH + PHY_FOOTER_LENGTH)) {
		xSemaphoreGive(spi_mutex);
		restore_state();
		return;
	}

	rx_data_length = length;

	// Get data
	cc2420_fifo_get(rx_data, rx_data_length);

	// Release semaphore
	xSemaphoreGive(spi_mutex);

	// Check CRC
	if (!(rx_data[rx_data_length - 1] & 0x80)) {
		// Bad CRC
		restore_state();
		return;
	}

	// Get RSSI
	int8_t rssi = (int8_t) rx_data[rx_data_length - 2] - 45;

	// Call callback function if any
	if (rx_cb) {
		rx_cb(rx_data, rx_data_length - 2, rssi, sync_word_time);
	}

	// Restore state
	restore_state();
}

static uint16_t sync_irq(void) {
	sync_word_time = timerB_time();
	return 0;
}

static uint16_t rx_irq(void) {
	portBASE_TYPE yield;

	if (xSemaphoreGiveFromISR(rx_sem, &yield) == pdTRUE) {
#if configUSE_PREEMPTION
		if (yield) {
			portYIELD();
		}
#endif
	}

	return 1;
}
