/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
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
 * $Id: cc1100.h,v 1.7 2008/07/02 09:05:40 adamdunkels Exp $
 */

/**
 * \file
 *         CC1100 driver header file
 * \author
 *         Clément Burin des Roziers <clement.burin-des-roziers@inrialpes.fr>
 * 
 * This driver has been mostly copied from the cc2420... files from Adam Dunkels
 */

#ifndef __CC1100_H__
#define __CC1100_H__

#include "contiki.h"
#include "dev/radio.h"

void cc1100_radio_init(void);

#define CC1100_MAX_PACKET_LEN      127

void cc1100_radio_set_channel(int channel);
int cc1100_radio_get_channel(void);

void cc1100_radio_set_pan_addr(unsigned pan,
				unsigned addr,
				const uint8_t *ieee_addr);

extern signed char cc1100_radio_last_rssi;
extern uint8_t cc1100_radio_last_correlation;

int cc1100_radio_rssi(void);

extern const struct radio_driver cc1100_radio_driver;

/**
 * \param power Between 1 and 31.
 */
void cc1100_radio_set_txpower(uint8_t power);
int cc1100_radio_get_txpower(void);
#define CC1100_TXPOWER_MAX  31
#define CC1100_TXPOWER_MIN   0

/**
 * Interrupt function, called from the simple-cc1100-arch driver.
 *
 */
int cc1100_radio_interrupt(void);

/* XXX hack: these will be made as Chameleon packet attributes */
extern rtimer_clock_t cc1100_radio_time_of_arrival,
  cc1100_radio_time_of_departure;
extern int cc1100_radio_authority_level_of_sender;

int cc1100_radio_on(void);
int cc1100_radio_off(void);

#endif /* __CC1100_H__ */
