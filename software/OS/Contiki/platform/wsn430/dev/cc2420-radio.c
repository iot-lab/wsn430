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
 * @(#)$Id: cc2420.c,v 1.30 2009/04/29 11:38:50 adamdunkels Exp $
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
#include "dev/cc2420-radio.h"
#include "cc2420.h"

#include "net/rime/packetbuf.h"
#include "net/rime/rimestats.h"
#include "rtimer.h"

#define WITH_SEND_CCA 0

#define CCA_WAIT_TIME (5 * RTIMER_ARCH_SECOND / 10000)

#define FOOTER_LEN 2
#define FOOTER1_CRC_OK      0x80
#define FOOTER1_CORRELATION 0x7f

#define AUX_LEN (FOOTER_LEN)

#define CC2420_MAX_PACKET_LEN 127

#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif

/*---------------------------------------------------------------------------*/
PROCESS(cc2420_radio_process, "CC2420 driver")
;
/*---------------------------------------------------------------------------*/
// Interface Methods
static int cc2420_on(void);
static int cc2420_off(void);
static int cc2420_read(void *buf, unsigned short bufsize);
static int cc2420_send(const void *data, unsigned short len);
static void cc2420_set_receiver(void(* recv)(const struct radio_driver *d));
const struct radio_driver cc2420_radio_driver = { cc2420_send, cc2420_read,
		cc2420_set_receiver, cc2420_on, cc2420_off, };

// Helpful Functions
/**
 * Callback function for the CC1100 driver, called with FIFO threshold or EOP
 * \return 1 to wake the CPU up
 */
static uint16_t irq_rx(void);

/**
 * Actually start RX
 */
static void on(void);
/**
 * Force the radio to IDLE, flush FIFOs.
 */
static void off(void);

static uint8_t receive_on = 0;
static volatile int16_t rx_flag = 0;

static void (* receiver_callback)(const struct radio_driver *);
static uint8_t locked, lock_on, lock_off;
#define GET_LOCK() locked = 1
static void RELEASE_LOCK(void) {
	if (lock_on) {
		on();
		lock_on = 0;
	}
	if (lock_off) {
		off();
		lock_off = 0;
	}
	locked = 0;
}
/*---------------------------------------------------------------------------*/
static void cc2420_set_receiver(void(* recv)(const struct radio_driver *)) {
	receiver_callback = recv;
}
/*---------------------------------------------------------------------------*/
void cc2420_radio_init(void) {

	cc2420_init();
	cc2420_set_frequency(2405 + 5 * 26);
	cc2420_set_fifopthr(127);
	cc2420_io_fifop_int_clear();
	cc2420_io_fifop_int_enable();
	cc2420_io_fifop_int_set_rising();
	cc2420_io_fifop_register_cb(irq_rx);

	cc2420_on();

	// Start the process
	process_start(&cc2420_radio_process, NULL);
}
/*---------------------------------------------------------------------------*/
static int cc2420_off(void) {
	/* Don't do anything if we are already turned off. */
	if (receive_on == 0) {
		return 1;
	}

	/* If we are called when the driver is locked, we indicate that the
	 radio should be turned off when the lock is unlocked. */
	if (locked) {
		lock_off = 1;
		return 1;
	}

	/* If we are currently receiving a packet (indicated by SFD == 1),
	 we don't actually switch the radio off now, but signal that the
	 driver should switch off the radio once the packet has been
	 received and processed, by setting the 'lock_off' variable. */
	if (cc2420_io_sfd_read()) {
		lock_off = 1;
		return 1;
	}

	off();
	return 1;
}
/*---------------------------------------------------------------------------*/
static int cc2420_on(void) {
	PRINTF("cc2420_on\n");
	if (receive_on) {
		return 1;
	}
	if (locked) {
		lock_on = 1;
		return 1;
	}

	on();
	return 1;
}
/*---------------------------------------------------------------------------*/
static int cc2420_send(const void *payload, unsigned short payload_len) {

	int i;
	uint8_t total_len;
	GET_LOCK();

	PRINTF("cc2420: sending %d bytes\n", payload_len);

	RIMESTATS_ADD(lltx);

	/* Wait for any previous transmission to finish. */
	while (cc2420_get_status() & CC2420_STATUS_TX_ACTIVE)
		;

	/* Write packet to TX FIFO. */
	cc2420_cmd_flushtx();

	total_len = payload_len + AUX_LEN;
	cc2420_fifo_put(&total_len, 1);
	cc2420_fifo_put((uint8_t*) payload, payload_len);

	/* The TX FIFO can only hold one packet. Make sure to not overrun
	 * FIFO by waiting for transmission to start here and synchronizing
	 * with the CC2420_TX_ACTIVE check in cc2420_send.
	 *
	 * Note that we may have to wait up to 320 us (20 symbols) before
	 * transmission starts.
	 */
#define LOOP_20_SYMBOLS 400	/* 326us (msp430 @ 2.4576MHz) */

#if WITH_SEND_CCA
	cc2420_cmd_rx();
	while(!(cc2420_get_status() & CC2420_STATUS_TX_ACTIVE));
	cc2420_cmd_txoncca();
#else /* WITH_SEND_CCA */
	cc2420_cmd_tx();
#endif /* WITH_SEND_CCA */

	for (i = LOOP_20_SYMBOLS; i > 0; i--) {
		if (cc2420_io_sfd_read()) {
			if (!(cc2420_get_status() & CC2420_STATUS_TX_ACTIVE)) {
				/* SFD went high yet we are not transmitting!
				 * => We started receiving a packet right now */
				return RADIO_TX_ERR;
			}

			if (receive_on) {
				ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
			}
			ENERGEST_ON(ENERGEST_TYPE_TRANSMIT);

			/* We wait until transmission has ended so that we get an
			 accurate measurement of the transmission time.*/
			while (cc2420_get_status() & CC2420_STATUS_TX_ACTIVE)
				;

			ENERGEST_OFF(ENERGEST_TYPE_TRANSMIT);
			if (receive_on) {
				ENERGEST_ON(ENERGEST_TYPE_LISTEN);
			} else {
				/* We need to explicitly turn off the radio,
				 * since STXON[CCA] -> TX_ACTIVE -> RX_ACTIVE */
				off();
			}

			RELEASE_LOCK();
			PRINTF("cc2420: sending done\n");
			return RADIO_TX_OK;
		}
	}

	/* If we are using WITH_SEND_CCA, we get here if the packet wasn't
	 transmitted because of other channel activity. */
	RIMESTATS_ADD(contentiondrop);
	PRINTF("cc2420: do_send() transmission never started\n");

	RELEASE_LOCK();
	return RADIO_TX_ERR; /* Transmission never started! */
}
/*---------------------------------------------------------------------------*/
static int cc2420_read(void *buf, unsigned short bufsize) {
	uint8_t footer[2];
	uint8_t len;

	if (!cc2420_io_fifop_read()) {
		/* If FIFO is 0, there is no packet in the RXFIFO. */
		return 0;
	}

	GET_LOCK();

	cc2420_fifo_get(&len, 1);

	if (len > CC2420_MAX_PACKET_LEN) {
		/* Oops, we must be out of sync. */
		cc2420_cmd_flushrx();
		RIMESTATS_ADD(badsynch);
		RELEASE_LOCK();
		return 0;
	}

	if (len <= AUX_LEN) {
		cc2420_cmd_flushrx();
		RIMESTATS_ADD(tooshort);
		RELEASE_LOCK();
		return 0;
	}

	if (len - AUX_LEN > bufsize) {
		cc2420_cmd_flushrx();
		RIMESTATS_ADD(toolong);
		RELEASE_LOCK();
		return 0;
	}

	cc2420_fifo_get(buf, len - AUX_LEN);
	cc2420_fifo_get(footer, FOOTER_LEN);

	if (footer[1] & FOOTER1_CRC_OK) {
		uint8_t cc2420_last_rssi = footer[0];
		uint8_t cc2420_last_correlation = footer[1] & FOOTER1_CORRELATION;

		packetbuf_set_attr(PACKETBUF_ATTR_RSSI, cc2420_last_rssi);
		packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, cc2420_last_correlation);

		RIMESTATS_ADD(llrx);

	} else {
		RIMESTATS_ADD(badcrc);
		len = AUX_LEN;
	}

	/* Clean up in case of FIFO overflow!  This happens for every full
	 * length frame and is signaled by FIFOP = 1 and FIFO = 0.
	 */
	if (cc2420_io_fifop_read() && !cc2420_io_fifo_read()) {
		/*    printf("cc2420_read: FIFOP_IS_1 1\n");*/
		cc2420_cmd_flushrx();
	} else if (cc2420_io_fifop_read()) {
		/* Another packet has been received and needs attention. */
		process_poll(&cc2420_radio_process);
	}

	RELEASE_LOCK();

	if (len < AUX_LEN) {
		return 0;
	}

	return len - AUX_LEN;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(cc2420_radio_process, ev, data) {

	PROCESS_BEGIN();

		PRINTF("cc2420_process: started\n");

		while(1) {
			PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

			if(receiver_callback != NULL) {
				PRINTF("cc2420_process: calling receiver callback\n");
				receiver_callback(&cc2420_radio_driver);
			} else {
				PRINTF("cc2420_process not receiving function\n");
				cc2420_cmd_flushrx();
			}
		}

		PROCESS_END();
	}

	/*---------------------------------------------------------------------------*/
	/* Interrupt routines */
static uint16_t irq_rx(void) {
	rx_flag = 1;
	process_poll(&cc2420_radio_process);
	return 1;
}

/* Other Static Functions */
/*---------------------------------------------------------------------------*/
static void on(void) {
	ENERGEST_ON(ENERGEST_TYPE_LISTEN);
	PRINTF("on\n");
	receive_on = 1;

	cc2420_io_fifop_int_enable();
	cc2420_cmd_rx();
	cc2420_cmd_flushrx();
}

static void off(void) {
	PRINTF("off\n");
	receive_on = 0;

	/* Wait for transmission to end before turning radio off. */
	while (cc2420_get_status() & CC2420_STATUS_TX_ACTIVE)
		;

	cc2420_cmd_idle();
	cc2420_io_fifop_int_disable();
	ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
}
