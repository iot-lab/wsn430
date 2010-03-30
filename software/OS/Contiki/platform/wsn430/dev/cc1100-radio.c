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

#define WITH_SEND_CCA 0
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

static void (* receiver_callback)(const struct radio_driver *);

int cc1100_radio_on(void);
int cc1100_radio_off(void);
int cc1100_radio_read(void *buf, unsigned short bufsize);
int cc1100_radio_send(const void *data, unsigned short len);
void cc1100_radio_set_receiver(void(* recv)(const struct radio_driver *d));

static uint16_t irq_rx(void);
static int rx(void);

// Local Buffer
#define BUFFER_SIZE 127
static uint8_t rx_buffer[BUFFER_SIZE + FOOTER_LEN];
static uint8_t rx_buffer_len = 0;
static uint8_t rx_buffer_ptr = 0;

const struct radio_driver cc1100_radio_driver = { cc1100_radio_send,
		cc1100_radio_read, cc1100_radio_set_receiver, cc1100_radio_on,
		cc1100_radio_off, };

static uint8_t receive_on = 0;

/*---------------------------------------------------------------------------*/
static void on(void) {
	ENERGEST_ON(ENERGEST_TYPE_LISTEN);
	PRINTF("on\n");
	receive_on = 1;

	cc1100_gdo2_int_disable();
	cc1100_gdo0_int_disable();

	cc1100_cmd_idle();
	cc1100_cmd_flush_rx();
	cc1100_cmd_flush_tx();
	cc1100_cmd_calibrate();

	cc1100_cfg_fifo_thr(0); // 4 bytes in RX FIFO

	cc1100_cfg_gdo0(CC1100_GDOx_SYNC_WORD);
	cc1100_cfg_gdo2(CC1100_GDOx_RX_FIFO);
	cc1100_gdo2_int_set_rising_edge();
	cc1100_gdo2_register_callback(irq_rx);
	cc1100_gdo2_int_clear();
	cc1100_gdo2_int_enable();

	rx_buffer_len = 0;
	rx_buffer_ptr = 0;
	cc1100_cmd_rx();
}

static void off(void) {
	PRINTF("off\n");
	receive_on = 0;

	cc1100_cmd_idle();
	cc1100_cmd_flush_rx();
	cc1100_cmd_flush_tx();
	cc1100_gdo0_int_disable();
	cc1100_gdo2_int_disable();

	ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
}
/*---------------------------------------------------------------------------*/
void cc1100_radio_set_receiver(void(* recv)(const struct radio_driver *)) {
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

	cc1100_cfg_fs_autocal(CC1100_AUTOCAL_NEVER);
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

	cc1100_cfg_chan(4);

	// Set the TX Power
	uint8_t table[1];
	table[0] = 0x67; // -5dBm
	cc1100_cfg_patable(table, 1);
	cc1100_cfg_pa_power(0);

	// Start the process
	process_start(&cc1100_radio_process, NULL);
}
/*---------------------------------------------------------------------------*/
int cc1100_radio_send(const void *payload, unsigned short payload_len) {
	uint16_t len, ptr;

	// check the length
	if (payload_len > BUFFER_SIZE) {
		return RADIO_TX_ERR;
	}

	PRINTF("cc1100: sending %u bytes\n", payload_len);

	int i;
	for (i=0;i<payload_len;i++) {
		PRINTF("%.2x ", ((uint8_t*)payload)[i]);
		if ((i+1) % 16 == 0) {
			PRINTF("\n");
		}
	}
	PRINTF("\n\n\n");

	/* Go to IDLE state and flush everything */
	cc1100_cmd_idle();

	cc1100_gdo0_int_disable();
	cc1100_gdo2_int_disable();

	cc1100_cmd_flush_rx();
	cc1100_cmd_flush_tx();

	/* Configure interrupts */
	cc1100_cfg_gdo0(CC1100_GDOx_SYNC_WORD);
	cc1100_cfg_gdo2(CC1100_GDOx_TX_FIFO);
	cc1100_cfg_fifo_thr(12); // 13 bytes in TX FIFO

	/* Write packet to TX FIFO. */
	ptr = 0;
	len = (payload_len > 63) ? 63 : payload_len;
	ptr += len;

	uint8_t totlen = (uint8_t)payload_len;
	/* Put the frame length byte */
	cc1100_fifo_put(&totlen, 1);

	PRINTF("cc1100: total_len = %u bytes\n", totlen);

	/* Put the maximum number of bytes */
	cc1100_fifo_put((uint8_t*) payload, len);

	/* If CCA required, do it */
#if WITH_SEND_CCA
	// TODO implement real CCA
	cc1100_cmd_tx();
#else /* WITH_SEND_CCA */
	cc1100_cmd_tx();
#endif /* WITH_SEND_CCA */

	/* Check if the radio is still in RX state */
	if (cc1100_status_marcstate() != 0x0D) {
		if (receive_on) {
			ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
		}
		ENERGEST_ON(ENERGEST_TYPE_TRANSMIT);

		// wait until transfer started
		while (cc1100_gdo0_read() == 0)
			;

		/* Now is time to transmit everything */
		while (ptr != payload_len) {

			/* wait until fifo threshold */
			while (cc1100_gdo2_read())
				;

			/* refill fifo */
			len = ((payload_len - ptr) > 50) ? 50 : (payload_len - ptr);
			cc1100_fifo_put(&((uint8_t*) payload)[ptr], len);
			ptr += len;
			//~ PRINTF("cc1100: put %d bytes\n", len);
		}

		/* wait until frame is sent */
		while (cc1100_gdo0_read())
			;

		PRINTF("cc1100: done sending\n");

		ENERGEST_OFF(ENERGEST_TYPE_TRANSMIT);

		if (receive_on) {
			ENERGEST_ON(ENERGEST_TYPE_LISTEN);
			on();
		}

		return RADIO_TX_OK;
	}

	/* If we are using WITH_SEND_CCA, we get here if the packet wasn't
	 transmitted because of other channel activity. */
	RIMESTATS_ADD(contentiondrop);
	PRINTF("cc1100: do_send() transmission never started\n");

	return RADIO_TX_ERR; /* Transmission never started! */

}
/*---------------------------------------------------------------------------*/
int cc1100_radio_off(void) {
	/* Don't do anything if we are already turned off. */
	if (receive_on == 0) {
		return 1;
	}

	off();
	return 1;
}
/*---------------------------------------------------------------------------*/
int cc1100_radio_on(void) {
	/* Don't do anything if we are already turned on. */
	if (receive_on) {
		return 1;
	}

	on();
	return 1;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(cc1100_radio_process, ev, data) {
	PROCESS_BEGIN();
		PRINTF("cc1100_radio_process: started\n");

		while(1) {
			PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
			// The only event we can receive is when packet RX has started

			if (rx() && (receiver_callback != NULL)) {
				PRINTF("cc1100_radio_process: calling receiver callback\n");
				receiver_callback(&cc1100_radio_driver);
			}
			on();
		}

		PROCESS_END();
	}
	/*---------------------------------------------------------------------------*/
int cc1100_radio_read(void *buf, unsigned short bufsize) {
	// check if there really is a packet in the buffer
	if (rx_buffer_len == 0) {
		// if not return 0
		PRINTF("cc11000_radio_read empty buffer\n");
		return 0;
	}

	memcpy(buf, rx_buffer, rx_buffer_len);

	packetbuf_set_attr(PACKETBUF_ATTR_RSSI, rx_buffer[rx_buffer_len]);
	packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, rx_buffer[rx_buffer_len+1] & (~CRC_OK));

	RIMESTATS_ADD(llrx);

	PRINTF("cc11000_radio_read len=%u\n", rx_buffer_len);
	return rx_buffer_len;
}
/*---------------------------------------------------------------------------*/
/* Interrupt routines */
static uint16_t irq_rx(void) {
	process_poll(&cc1100_radio_process);
	return 1;
}

/* Other Static Functions */
static int rx(void) {

	uint8_t fifo_len;
	rx_buffer_ptr = 0;

	if (cc1100_status_rxbytes() == 0) {
		PRINTF("rx_begin: nothing in fifo !!\n");
		return 0;
	}

	// get the length byte
	cc1100_fifo_get(&rx_buffer_len, 1);

	// check the length byte
	if (rx_buffer_len > BUFFER_SIZE || rx_buffer_len == 0) {
		PRINTF("rx_begin: error length (%d)\n", rx_buffer_len);
		rx_buffer_len = 0;
		return 0;
	}

	// Change the RX FIFO threshold (48B)
	cc1100_cfg_fifo_thr(11);

	// Loop until end of packet
	while (cc1100_gdo0_read()) {
		// get the bytes in FIFO
		fifo_len = cc1100_status_rxbytes();

		if (fifo_len & 0x80) {
			PRINTF("rx: error rxfifo overflow (%d)\n", fifo_len);
			return 0;
		}

		if (fifo_len == 0) {
			PRINTF("rx: warning, fifo_len=0\n");
			return 0;
		}

		// Check for local overflow
		if (rx_buffer_ptr + fifo_len > BUFFER_SIZE + FOOTER_LEN) {
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
		PRINTF("rx: error rxfifo overflow (%d)\n", fifo_len);
		return 0;
	}

	// Check for local overflow
	if (rx_buffer_ptr + fifo_len > BUFFER_SIZE + FOOTER_LEN) {
		PRINTF("rx: error local overflow\n");
		return 0;
	}
	// Get the bytes
	cc1100_fifo_get(rx_buffer + rx_buffer_ptr, fifo_len);
	rx_buffer_ptr += fifo_len;

	// check if we have the entire packet
	if ((rx_buffer_len + FOOTER_LEN) != rx_buffer_ptr) {
		PRINTF("rx_end: lengths don't match [%u!=%u]\n", rx_buffer_len, rx_buffer_ptr);
		return 0;
	}

	if (!(rx_buffer[rx_buffer_len + 1] & 0x80)) {
		PRINTF("rx_end: error CRC\n");
		RIMESTATS_ADD(badcrc);
		return 0;
	}

	PRINTF("cc1100: received %d bytes\n", rx_buffer_len);

	int i;
	for (i=0;i<rx_buffer_len;i++) {
		PRINTF("%.2x\ ", ((uint8_t*)rx_buffer)[i]);
		if ((i+1) % 16 == 0) {
			PRINTF("\n");
		}
	}
	PRINTF("\n\n\n");
	return 1;
}
