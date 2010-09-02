/*
 * Copyright (c) 2007, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * @(#)$Id: cc1100.c,v 1.30 2009/04/29 11:38:50 adamdunkels Exp $
 */
/*
 * This code is almost device independent and should be easy to port.
 */
/**
 * \file
 *         CC1100 driver
 * \author
 *         Clément Burin des Roziers <clement.burin-des-roziers@inrialpes.fr>
 * 
 * This driver has been mostly copied from the cc2420... files from Adam Dunkels
 */
#include <io.h>
#include <stdio.h>
#include <string.h>

#include "contiki.h"

#include "dev/leds.h"
#include "dev/cc1100-radio.h"
#include "cc1100.h"

#include "net/rime/packetbuf.h"
#include "net/rime/rimestats.h"
#include "rtimer.h"

#define WITH_SEND_CCA 0

#define CCA_WAIT_TIME (5 * RTIMER_ARCH_SECOND / 10000)

#define FOOTER_LEN 2
#define CRC_OK      0x80

#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif

/*---------------------------------------------------------------------------*/
PROCESS(cc1100_radio_process, "CC1100 driver")
;
/*---------------------------------------------------------------------------*/
// Interface Methods
static int cc1100_on(void);
static int cc1100_off(void);
static int cc1100_read(void *buf, unsigned short bufsize);
static int cc1100_send(const void *data, unsigned short len);
static void cc1100_set_receiver(void(* recv)(const struct radio_driver *d));
const struct radio_driver cc1100_radio_driver = { cc1100_send, cc1100_read,
		cc1100_set_receiver, cc1100_on, cc1100_off, };

// Helpful Functions
/**
 * Callback function for the CC1100 driver, called with FIFO threshold or EOP
 * \return 1 to wake the CPU up
 */
static uint16_t irq_rx(void);
/**
 * Try to receive a packet from FIFO.
 * If packet is started, but still receiving, wait until EOP.
 *
 * \return 1 if packet received and available in rx_buffer;
 *     0 otherwise
 */
static int16_t rx(void);

/**
 * Actually start RX
 */
static void on(void);
/**
 * Force the radio to IDLE, flush FIFOs.
 */
static void off(void);

// Local Buffer
#define BUFFER_SIZE 127
static uint8_t rx_buffer[BUFFER_SIZE + FOOTER_LEN];
static uint8_t rx_buffer_len = 0;
static uint8_t rx_buffer_ptr = 0;

static uint8_t receive_on = 0;
static volatile int16_t rx_flag = 0;

static void (* receiver_callback)(const struct radio_driver *);

/*---------------------------------------------------------------------------*/
static void cc1100_set_receiver(void(* recv)(const struct radio_driver *)) {
	receiver_callback = recv;
}
/*---------------------------------------------------------------------------*/
void cc1100_radio_init(void) {
	cc1100_init();
	cc1100_cmd_idle();

	cc1100_cfg_append_status(CC1100_APPEND_STATUS_ENABLE);
	cc1100_cfg_crc_autoflush(CC1100_CRC_AUTOFLUSH_DISABLE);
	cc1100_cfg_white_data(CC1100_DATA_WHITENING_ENABLE);
	cc1100_cfg_crc_en(CC1100_CRC_CALCULATION_ENABLE);
	cc1100_cfg_freq_if(0x0C);

	cc1100_cfg_fs_autocal(CC1100_AUTOCAL_4TH_TX_RX_TO_IDLE);
	cc1100_cfg_mod_format(CC1100_MODULATION_MSK);
	cc1100_cfg_sync_mode(CC1100_SYNCMODE_30_32);
	cc1100_cfg_manchester_en(CC1100_MANCHESTER_DISABLE);

	cc1100_cfg_txoff_mode(CC1100_TXOFF_MODE_IDLE);
	cc1100_cfg_rxoff_mode(CC1100_RXOFF_MODE_IDLE);

	// set channel bandwidth (560 kHz)
	cc1100_cfg_chanbw_e(0);
	cc1100_cfg_chanbw_m(2);

	// set data rate (0xD/0x2F is 250kbps)
	cc1100_cfg_drate_e(0x0D);
	cc1100_cfg_drate_m(0x2F);

	cc1100_cfg_chan(6);

	// Set the TX Power
	uint8_t table[1];
	table[0] = 0xC2; // +10dBm
	cc1100_cfg_patable(table, 1);
	cc1100_cfg_pa_power(0);

	// Calibrate once at start
	cc1100_cmd_calibrate();

	// Clear flags
	receive_on = 0;
	rx_flag = 0;

	// Start the process
	process_start(&cc1100_radio_process, NULL);
}
/*---------------------------------------------------------------------------*/
static int cc1100_off(void) {
	PRINTF("cc1100_off\n");
	/* Don't do anything if we are already off. */
	if (receive_on == 0) {
		return 1;
	}

	off();
	return 1;
}
/*---------------------------------------------------------------------------*/
static int cc1100_on(void) {
	PRINTF("cc1100_on\n");
	/* Don't do anything if we are already on. */
	if (receive_on) {
		return 1;
	}

	on();
	return 1;
}
/*---------------------------------------------------------------------------*/
static int cc1100_send(const void *payload, unsigned short payload_len) {
	uint16_t len, ptr;

	// check the length
	if (payload_len > BUFFER_SIZE) {
		PRINTF("cc1100_send: packet too big [%u]\n", payload_len);
		return RADIO_TX_ERR;
	}

	// Check the radio is not receiving a packet
	if (((cc1100_status() & CC1100_STATUS_MASK) == CC1100_STATUS_RX)
			&& (cc1100_gdo0_read() != 0)) {
		// We're in RX with sync word set, hence receiving a packet, abort
		PRINTF("cc1100_send: radio busy, RX\n");
		return RADIO_TX_ERR;
	}

	PRINTF("cc1100_send: sending %u bytes\n", payload_len);

	// Disable Interrupts
	cc1100_gdo0_int_disable();
	cc1100_gdo2_int_disable();

	if (cc1100_status_txbytes() != 0) {
		// Should not happen
		cc1100_cmd_idle();
		cc1100_cmd_flush_tx();
		cc1100_cmd_flush_rx();
	}

	/* If CCA required, do it */
#if WITH_SEND_CCA
	uint16_t now;
	cc1100_cmd_rx();
	now = RTIMER_NOW();
	while (RTIMER_CLOCK_LT(RTIMER_NOW(), now + CCA_WAIT_TIME));
	cc1100_cmd_tx();

	// Check status
	if ( (cc1100_status() & CC1100_STATUS_MASK) == CC1100_STATUS_RX) {
		// Status is still RX, therefore we could not start TX
		if (receive_on) {
			// Configure GDO IRQ
			cc1100_cfg_fifo_thr(0); // 4 bytes in RX FIFO
			cc1100_cfg_gdo0(CC1100_GDOx_SYNC_WORD);
			cc1100_cfg_gdo2(CC1100_GDOx_RX_FIFO_EOP);
			cc1100_gdo2_int_set_rising_edge();
			cc1100_gdo2_register_callback(irq_rx);
			cc1100_gdo2_int_clear();
			cc1100_gdo2_int_enable();
		} else {
			// If we were off, stop the radio
			off();
		}
		/* If we are using WITH_SEND_CCA, we get here if the packet wasn't
		 transmitted because of other channel activity. */
		RIMESTATS_ADD(contentiondrop);
		PRINTF("cc1100: do_send() transmission never started\n");

		return RADIO_TX_ERR; /* Transmission never started! */
	}

#else /* WITH_SEND_CCA */
	cc1100_cmd_idle();
	cc1100_cmd_tx();
#endif /* WITH_SEND_CCA */

	/* Configure GDOx */
	cc1100_cfg_gdo0(CC1100_GDOx_SYNC_WORD);
	cc1100_cfg_gdo2(CC1100_GDOx_TX_FIFO);
	cc1100_cfg_fifo_thr(12); // 13 bytes in TX FIFO

	/* Write packet to TX FIFO. */
	ptr = 0;
	len = (payload_len > 63) ? 63 : payload_len;
	ptr += len;

	uint8_t totlen = (uint8_t) payload_len;
	PRINTF("cc1100: total_len = %u bytes\n", totlen);

	/* Put the frame length byte */
	cc1100_fifo_put(&totlen, 1);

	/* Put the maximum number of bytes */
	cc1100_fifo_put((uint8_t*) payload, len);

	if (receive_on) {
		ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
	}
	ENERGEST_ON(ENERGEST_TYPE_TRANSMIT);

	// wait until transfer started
	while (cc1100_gdo0_read() == 0)
		;

	// Now is time to transmit everything
	while (ptr != payload_len) {

		// wait until fifo threshold
		while (cc1100_gdo2_read())
			;

		// refill fifo
		len = ((payload_len - ptr) > 50) ? 50 : (payload_len - ptr);
		cc1100_fifo_put(&((uint8_t*) payload)[ptr], len);
		ptr += len;
	}

	/* wait until frame is sent */
	while (cc1100_gdo0_read())
		;

	PRINTF("cc1100: done sending\n");

	ENERGEST_OFF(ENERGEST_TYPE_TRANSMIT);

	if (receive_on) {
		ENERGEST_ON(ENERGEST_TYPE_LISTEN);
		on();
	} else {
		off();
	}

	return RADIO_TX_OK;
}
/*---------------------------------------------------------------------------*/
static int cc1100_read(void *buf, unsigned short bufsize) {
	int retlen;

	// check if there is a packet in the local buffer
	if (rx_buffer_len == 0) {
		if (rx_flag) {
			rx();
		}

		// If n opacket received, return
		if (rx_buffer_len == 0)
			return 0;
	}

	// Copy received packet to buf
	memcpy(buf, rx_buffer, rx_buffer_len);

	packetbuf_set_attr(PACKETBUF_ATTR_RSSI, rx_buffer[rx_buffer_len]);
	packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY,
			rx_buffer[rx_buffer_len + 1] & (~CRC_OK));

	RIMESTATS_ADD(llrx);

	PRINTF("cc1100_read len=%u\n", rx_buffer_len);
	retlen = rx_buffer_len;

	// Reset the buffer length
	rx_buffer_len = 0;
	return retlen;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(cc1100_radio_process, ev, data) {
	PROCESS_BEGIN();
		PRINTF("cc1100_radio_process: started\n");

		while(1) {
			PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
			// The only event we can receive is when packet RX has started

			if (rx() && receiver_callback) {
				receiver_callback(&cc1100_radio_driver);
			}
		}
		PROCESS_END();
	}

	/*---------------------------------------------------------------------------*/
	/* Interrupt routines */
static uint16_t irq_rx(void) {
	rx_flag = 1;
	process_poll(&cc1100_radio_process);
	return 1;
}

/* Other Static Functions */
/*---------------------------------------------------------------------------*/
static void on(void) {
	ENERGEST_ON(ENERGEST_TYPE_LISTEN);
	receive_on = 1;

	// Disable interrupts
	cc1100_gdo2_int_disable();
	cc1100_gdo0_int_disable();

	// Go to idle and flush FIFOs
	cc1100_cmd_idle();
	cc1100_cmd_flush_rx();
	cc1100_cmd_flush_tx();

	// Configure GDO IRQ
	cc1100_cfg_fifo_thr(7); // 32 bytes in RX FIFO
	cc1100_cfg_gdo0(CC1100_GDOx_SYNC_WORD);
	cc1100_gdo0_int_set_rising_edge();
	cc1100_gdo0_register_callback(irq_rx);
	cc1100_gdo0_int_clear();
	cc1100_gdo0_int_enable();
	cc1100_cfg_gdo2(CC1100_GDOx_RX_FIFO);

	// Start RX
	cc1100_cmd_rx();
}

static void off(void) {
	receive_on = 0;

	// Disable interrupts
	cc1100_gdo0_int_disable();
	cc1100_gdo2_int_disable();

	// Go to idle and flush
	cc1100_cmd_idle();
	cc1100_cmd_flush_rx();
	cc1100_cmd_flush_tx();

	ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
}

static int rx(void) {
	uint8_t fifo_len;

	// Reset rx_buffer_pointer
	rx_buffer_ptr = 0;

	// Clear RX flag, because it's going to be handled
	rx_flag = 0;

	uint8_t rxb;
	rxb = cc1100_status_rxbytes();

	if (rxb == 0 && cc1100_gdo0_read() == 0) {
		on();
		PRINTF("rx: nothing in fifo !!\n");
		return 0;
	}

	// get the length byte
	cc1100_fifo_get(&rx_buffer_len, 1);

	// check the length byte
	if (rx_buffer_len > BUFFER_SIZE || rx_buffer_len == 0) {
		PRINTF("rx: error length (%d)\n", rx_buffer_len);
		rx_buffer_len = 0;
		on();
		return 0;
	}

	// Loop until end of packet
	while (cc1100_gdo0_read()) {
		// get the bytes in FIFO
		fifo_len = cc1100_status_rxbytes();

		if (fifo_len & 0x80) {
			on();
			PRINTF("rx: error rxfifo overflow (%d)\n", fifo_len);
			return 0;
		}

		if (fifo_len == 0) {
			on();
			PRINTF("rx: warning, fifo_len=0\n");
			return 0;
		}

		// Check for local overflow
		if (rx_buffer_ptr + fifo_len > BUFFER_SIZE + FOOTER_LEN) {
			on();
			PRINTF("rx: error local overflow\n");
			return 0;
		}

		// remove one byte
		fifo_len -= 1;
		cc1100_fifo_get(rx_buffer + rx_buffer_ptr, fifo_len);
		rx_buffer_ptr += fifo_len;

		// Wait until FIFO is filled above threshold, or EOP
		while (!cc1100_gdo2_read() && cc1100_gdo0_read()) {
			;
		}
	}

	// Packet complete, get the end
	fifo_len = cc1100_status_rxbytes();

	if (fifo_len & 0x80) {
		on();
		PRINTF("rx: error rxfifo overflow (%d)\n", fifo_len);
		return 0;
	}

	// Check for local overflow
	if (rx_buffer_ptr + fifo_len > BUFFER_SIZE + FOOTER_LEN) {
		on();
		PRINTF("rx: error local overflow\n");
		return 0;
	}
	// Get the bytes
	cc1100_fifo_get(rx_buffer + rx_buffer_ptr, fifo_len);
	rx_buffer_ptr += fifo_len;

	// check if we have the entire packet
	if ((rx_buffer_len + FOOTER_LEN) != rx_buffer_ptr) {
		on();
		PRINTF("rx: lengths don't match [%u!=%u]\n", rx_buffer_len, rx_buffer_ptr);
		return 0;
	}

	if (!(rx_buffer[rx_buffer_len + 1] & 0x80)) {
		on();
		PRINTF("rx: error CRC\n");
		RIMESTATS_ADD(badcrc);
		return 0;
	}

	on();
	PRINTF("rx: received %d bytes\n", rx_buffer_len);
	return 1;
}
