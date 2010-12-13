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

#include "net/packetbuf.h"
#include "net/rime/rimestats.h"
#include "net/netstack.h"

#include "sys/timetable.h"

#define WITH_SEND_CCA 1

#define FOOTER_LEN 2

#ifndef CC2420_CONF_CHECKSUM
#define CC2420_CONF_CHECKSUM 0
#endif /* CC2420_CONF_CHECKSUM */

#ifndef CC2420_CONF_AUTOACK
#define CC2420_CONF_AUTOACK 0
#endif /* CC2420_CONF_AUTOACK */

#if CC2420_CONF_CHECKSUM
#include "lib/crc16.h"
#define CHECKSUM_LEN 2
#else
#define CHECKSUM_LEN 0
#endif /* CC2420_CONF_CHECKSUM */

#define AUX_LEN (CHECKSUM_LEN + FOOTER_LEN)

#define FOOTER1_CRC_OK      0x80
#define FOOTER1_CORRELATION 0x7f

#define CC2420_MAX_PACKET_LEN 127

#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif

static uint8_t volatile pending;

#define BUSYWAIT_UNTIL(cond, max_time)                                  \
  do {                                                                  \
    rtimer_clock_t t0;                                                  \
    t0 = RTIMER_NOW();                                                  \
    while(!(cond) && RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + (max_time)));   \
  } while(0)

/*---------------------------------------------------------------------------*/
PROCESS(cc2420_radio_process, "CC2420 driver")
;
/*---------------------------------------------------------------------------*/
// Interface Methods
static int cc2420_on(void);
static int cc2420_off(void);

static int cc2420_read(void *buf, unsigned short bufsize);

static int cc2420_prepare(const void *data, unsigned short len);
static int cc2420_transmit(unsigned short len);
static int cc2420_send(const void *data, unsigned short len);

static int cc2420_receiving_packet(void);
static int pending_packet(void);
static int cc2420_cca(void);
static int detected_energy(void);

const struct radio_driver cc2420_radio_driver = { cc2420_radio_init,
		cc2420_prepare, cc2420_transmit, cc2420_send, cc2420_read, cc2420_cca,
		cc2420_receiving_packet, pending_packet, cc2420_on, cc2420_off };

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

static uint8_t locked, lock_on, lock_off;
#define GET_LOCK() locked++
static void RELEASE_LOCK(void) {
	if (locked == 1) {
		if (lock_on) {
			on();
			lock_on = 0;
		}
		if (lock_off) {
			off();
			lock_off = 0;
		}
	}
	locked--;
}
/*---------------------------------------------------------------------------*/
int cc2420_radio_init(void) {

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

	return 1;
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

	GET_LOCK();
	/* If we are currently receiving a packet (indicated by SFD == 1),
	 we don't actually switch the radio off now, but signal that the
	 driver should switch off the radio once the packet has been
	 received and processed, by setting the 'lock_off' variable. */
	if (cc2420_io_sfd_read()) {
		lock_off = 1;
	} else {
		off();
	}
	RELEASE_LOCK();
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

	GET_LOCK();
	on();
	RELEASE_LOCK();
	return 1;
}
/*---------------------------------------------------------------------------*/
int cc2420_receiving_packet(void) {
	return cc2420_io_sfd_read();
}
/*---------------------------------------------------------------------------*/
static int cc2420_cca(void) {
	int cca;
	int radio_was_off = 0;

	/* If the radio is locked by an underlying thread (because we are
	 being invoked through an interrupt), we preted that the coast is
	 clear (i.e., no packet is currently being transmitted by a
	 neighbor). */
	if (locked) {
		return 1;
	}

	GET_LOCK();
	if (!receive_on) {
		radio_was_off = 1;
		cc2420_on();
	}

	/* Make sure that the radio really got turned on. */
	if (!receive_on) {
		RELEASE_LOCK();
		return 1;
	}

	BUSYWAIT_UNTIL(cc2420_get_status() & CC2420_STATUS_RSSI_VALID, RTIMER_SECOND / 100);

	cca = cc2420_io_cca_read();

	if (radio_was_off) {
		cc2420_off();
	}
	RELEASE_LOCK();
	return cca;
}
/*---------------------------------------------------------------------------*/
static int pending_packet(void) {
	return cc2420_io_fifop_read();
}
/*---------------------------------------------------------------------------*/
static int cc2420_transmit(unsigned short payload_len) {

	int i;
	uint8_t total_len;
#if CC2420_CONF_CHECKSUM
	uint16_t checksum;
#endif /* CC2420_CONF_CHECKSUM */

	GET_LOCK();

	total_len = payload_len + AUX_LEN;

	/* The TX FIFO can only hold one packet. Make sure to not overrun
	 * FIFO by waiting for transmission to start here and synchronizing
	 * with the CC2420_TX_ACTIVE check in cc2420_send.
	 *
	 * Note that we may have to wait up to 320 us (20 symbols) before
	 * transmission starts.
	 */
#ifndef CC2420_CONF_SYMBOL_LOOP_COUNT
#error CC2420_CONF_SYMBOL_LOOP_COUNT needs to be set!!!
#else
#define LOOP_20_SYMBOLS CC2420_CONF_SYMBOL_LOOP_COUNT
#endif

#if WITH_SEND_CCA
	cc2420_cmd_rx();
	BUSYWAIT_UNTIL(cc2420_get_status() & CC2420_STATUS_TX_ACTIVE, RTIMER_SECOND / 10);
	cc2420_cmd_txoncca();
#else /* WITH_SEND_CCA */
	cc2420_cmd_tx();
#endif /* WITH_SEND_CCA */

	for (i = LOOP_20_SYMBOLS; i > 0; i--) {
		if (cc2420_io_sfd_read()) {
			if (!(cc2420_get_status() & CC2420_STATUS_TX_ACTIVE)) {
				/* SFD went high yet we are not transmitting!
				 * => We started receiving a packet right now */
				RELEASE_LOCK();
				return RADIO_TX_COLLISION;
			}

			if (receive_on) {
				ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
			}
			ENERGEST_ON(ENERGEST_TYPE_TRANSMIT);

			/* We wait until transmission has ended so that we get an
			 accurate measurement of the transmission time.*/
			BUSYWAIT_UNTIL(!(cc2420_get_status() & CC2420_STATUS_TX_ACTIVE), RTIMER_SECOND / 10);

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
	return RADIO_TX_COLLISION; /* Transmission never started! */
}
/*---------------------------------------------------------------------------*/
static int cc2420_prepare(const void *payload, unsigned short payload_len) {
	uint8_t total_len;
#if CC2420_CONF_CHECKSUM
	uint16_t checksum;
#endif /* CC2420_CONF_CHECKSUM */
	GET_LOCK();

	PRINTF("cc2420: sending %d bytes\n", payload_len);

	RIMESTATS_ADD(lltx);

	/* Write packet to TX FIFO. */
	cc2420_cmd_flushtx();

#if CC2420_CONF_CHECKSUM
	checksum = crc16_data(payload, payload_len, 0);
#endif /* CC2420_CONF_CHECKSUM */

	total_len = payload_len + AUX_LEN;

	cc2420_fifo_put(&total_len, 1);
	cc2420_fifo_put((uint8_t*) payload, payload_len);
#if CC2420_CONF_CHECKSUM
	cc2420_fifo_put((uint8_t*) &checksum, CHECKSUM_LEN);
#endif /* CC2420_CONF_CHECKSUM */

	RELEASE_LOCK();
	return 0;
}
/*---------------------------------------------------------------------------*/
static int cc2420_send(const void *payload, unsigned short payload_len) {
	cc2420_prepare(payload, payload_len);
	return cc2420_transmit(payload_len);
}
/*---------------------------------------------------------------------------*/
static int cc2420_read(void *buf, unsigned short bufsize) {
	uint8_t footer[2];
	uint8_t len;
#if CC2420_CONF_CHECKSUM
	uint16_t checksum;
#endif /* CC2420_CONF_CHECKSUM */

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
#if CC2420_CONF_CHECKSUM
	cc2420_fifo_get((uint8_t*)&checksum, CHECKSUM_LEN);
#endif /* CC2420_CONF_CHECKSUM */
	cc2420_fifo_get(footer, FOOTER_LEN);

#if CC2420_CONF_CHECKSUM
	if(checksum != crc16_data(buf, len - AUX_LEN, 0)) {
		PRINTF("checksum failed 0x%04x != 0x%04x\n",
				checksum, crc16_data(buf, len - AUX_LEN, 0));
	}

	if(footer[1] & FOOTER1_CRC_OK &&
			checksum == crc16_data(buf, len - AUX_LEN, 0)) {
#else
	if (footer[1] & FOOTER1_CRC_OK) {
#endif /* CC2420_CONF_CHECKSUM */
		uint8_t cc2420_last_rssi = footer[0];
		uint8_t cc2420_last_correlation = footer[1] & FOOTER1_CORRELATION;

		packetbuf_set_attr(PACKETBUF_ATTR_RSSI, cc2420_last_rssi);
		packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, cc2420_last_correlation);

		RIMESTATS_ADD(llrx);

	} else {
		RIMESTATS_ADD(badcrc);
		len = AUX_LEN;
	}

	if (cc2420_io_fifop_read()) {
		if (!cc2420_io_fifo_read()) {
			/* Clean up in case of FIFO overflow!  This happens for every full
			 * length frame and is signaled by FIFOP = 1 and FIFO = 0.
			 */
			cc2420_cmd_flushrx();
		} else {
			/* Another packet has been received and needs attention. */
			process_poll(&cc2420_radio_process);
		}
	}
	RELEASE_LOCK();

	if (len < AUX_LEN) {
		return 0;
	}

	return len - AUX_LEN;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(cc2420_radio_process, ev, data) {

	int len;
	PROCESS_BEGIN();

		PRINTF("cc2420_process: started\n");

		while(1) {
			PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

			PRINTF("cc2420_process: calling receiver callback\n");

			packetbuf_clear();
			len = cc2420_read(packetbuf_dataptr(), PACKETBUF_SIZE);
			if(len > 0) {
				packetbuf_set_datalen(len);

				NETSTACK_RDC.input();
			}}

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
	BUSYWAIT_UNTIL(!(cc2420_get_status() & CC2420_STATUS_TX_ACTIVE), RTIMER_SECOND / 10);

	cc2420_cmd_idle();
	cc2420_io_fifop_int_disable();
	ENERGEST_OFF(ENERGEST_TYPE_LISTEN);

	if (!cc2420_io_fifop_read()) {
		cc2420_cmd_flushrx();
	}
}
