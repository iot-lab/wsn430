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
#include <stdio.h>
#include <string.h>

#include "contiki.h"

#if defined(__AVR__)
#include <avr/io.h>
#elif defined(__MSP430__)
#include <io.h>
#endif

#include "dev/leds.h"
#include "dev/cc1100-radio.h"
#include "cc1100.h"

#include "net/rime/packetbuf.h"
#include "net/rime/rimestats.h"

#include "sys/timetable.h"

#define WITH_SEND_CCA 0


#if CC1100_CONF_TIMESTAMPS
#include "net/rime/timesynch.h"
#define TIMESTAMP_LEN 3
#else /* CC1100_CONF_TIMESTAMPS */
#define TIMESTAMP_LEN 0
#endif /* CC1100_CONF_TIMESTAMPS */

#ifndef CC1100_CONF_CHECKSUM
#define CC1100_CONF_CHECKSUM 0
#endif /* CC1100_CONF_CHECKSUM */

#if CC1100_CONF_CHECKSUM
#include "lib/crc16.h"
#define CHECKSUM_LEN 2
#else
#define CHECKSUM_LEN 0
#endif /* CC1100_CONF_CHECKSUM */

#define FOOTER_LEN 2
#define AUX_LEN (CHECKSUM_LEN + TIMESTAMP_LEN)

struct timestamp {
  uint16_t time;
  uint8_t authority_level;
};


#define FOOTER1_CRC_OK      0x80
#define FOOTER1_CORRELATION 0x7f

#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif

/* XXX hack: these will be made as Chameleon packet attributes */
rtimer_clock_t cc1100_radio_time_of_arrival, cc1100_radio_time_of_departure;

int cc1100_radio_authority_level_of_sender;

#if CC1100_CONF_TIMESTAMPS
static rtimer_clock_t setup_time_for_transmission;
static unsigned long total_time_for_transmission, total_transmission_len;
static int num_transmissions;
#endif /* CC1100_CONF_TIMESTAMPS */

/*---------------------------------------------------------------------------*/
PROCESS(cc1100_radio_process, "CC1100 driver");
/*---------------------------------------------------------------------------*/

static void (* receiver_callback)(const struct radio_driver *);

int cc1100_radio_on(void);
int cc1100_radio_off(void);
int cc1100_radio_read(void *buf, unsigned short bufsize);
int cc1100_radio_send(const void *data, unsigned short len);
void cc1100_radio_set_receiver(void (* recv)(const struct radio_driver *d));

static uint16_t irq_fifo(void);
static uint16_t irq_eop(void);
static int rx_begin(void);
static int rx(void);
static int rx_end(void);

signed char cc1100_radio_last_rssi;
uint8_t cc1100_radio_last_correlation;

#define EVENT_FIFO 0x12
#define EVENT_EOP  0x34
static uint16_t radio_event = 0;

#define BUFFER_SIZE 128
static uint8_t rx_buffer[BUFFER_SIZE];
static uint8_t rx_buffer_len = 0;
static uint8_t rx_buffer_ptr = 0;

const struct radio_driver cc1100_radio_driver =
  {
    cc1100_radio_send,
    cc1100_radio_read,
    cc1100_radio_set_receiver,
    cc1100_radio_on,
    cc1100_radio_off,
  };

static uint8_t receive_on=0;
/* Radio stuff in network byte order. */
static uint16_t pan_id;

static int channel;

/*---------------------------------------------------------------------------*/
static uint8_t locked, lock_on, lock_off;

#define RX_FIFO_THR 32

static void
on(void)
{
  ENERGEST_ON(ENERGEST_TYPE_LISTEN);
  PRINTF("on\n");
  receive_on = 1;

  cc1100_gdo2_int_disable();
  cc1100_gdo0_int_disable();
  
  cc1100_cmd_idle();
  cc1100_cmd_flush_rx();
  cc1100_cmd_flush_tx();
  cc1100_cmd_calibrate();

  cc1100_cfg_fifo_thr(7); // 32 bytes RX
  
  cc1100_cfg_gdo0(CC1100_GDOx_SYNC_WORD);
  cc1100_gdo0_int_set_falling_edge();
  cc1100_gdo0_register_callback(irq_eop);
  cc1100_gdo0_int_clear();
  cc1100_gdo0_int_enable();
  
  cc1100_cfg_gdo2(CC1100_GDOx_RX_FIFO);
  cc1100_gdo2_int_set_rising_edge();
  cc1100_gdo2_register_callback(irq_fifo);
  cc1100_gdo2_int_clear();
  cc1100_gdo2_int_enable();
  
  cc1100_cmd_rx();
}

static void
off(void)
{
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
#define GET_LOCK() locked = 1
static void RELEASE_LOCK(void) {
  if(lock_on) {
    on();
    lock_on = 0;
  }
  if(lock_off) {
    off();
    lock_off = 0;
  }
  locked = 0;
}
/*---------------------------------------------------------------------------*/
void
cc1100_radio_set_receiver(void (* recv)(const struct radio_driver *))
{
  receiver_callback = recv;
}
/*---------------------------------------------------------------------------*/
void
cc1100_radio_init(void)
{
  cc1100_init();
  cc1100_cmd_idle();

  cc1100_cfg_append_status(CC1100_APPEND_STATUS_DISABLE);
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

  uint8_t table[1];
  table[0] = 0x67; // -5dBm
  cc1100_cfg_patable(table, 1);
  cc1100_cfg_pa_power(0);
  
  cc1100_cmd_calibrate();
  
  cc1100_radio_set_pan_addr(0xffff, 0x0000, NULL);
  cc1100_radio_set_channel(26);

  process_start(&cc1100_radio_process, NULL);
}
/*---------------------------------------------------------------------------*/
int
cc1100_radio_send(const void *payload, unsigned short payload_len)
{
  uint8_t total_len;
#if CC1100_CONF_TIMESTAMPS
  struct timestamp timestamp;
#endif /* CC1100_CONF_TIMESTAMPS */
#if CC1100_CONF_CHECKSUM
  uint16_t checksum;
#endif /* CC1100_CONF_CHECKSUM */

  // check the length
  if (payload_len > BUFFER_SIZE) {
    return -1;
  }

  GET_LOCK();

  if(packetbuf_attr(PACKETBUF_ATTR_RADIO_TXPOWER) > 0) {
    cc1100_radio_set_txpower(packetbuf_attr(PACKETBUF_ATTR_RADIO_TXPOWER) - 1);
  } else {
    cc1100_radio_set_txpower(CC1100_TXPOWER_MAX);
  }
  
  PRINTF("cc1100: sending %d bytes\n", payload_len);
  
  RIMESTATS_ADD(lltx);

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

#if CC1100_CONF_CHECKSUM
  checksum = crc16_data(payload, payload_len, 0);
#endif /* CC1100_CONF_CHECKSUM */
  total_len = payload_len + AUX_LEN;
  
  uint16_t len, ptr;
  ptr = 0;
  len = (payload_len > 63)? 63: payload_len;
  ptr += len;
  
  /* Put the frame length byte */
  cc1100_fifo_put(&total_len, 1);
  
  PRINTF("cc1100: total_len = %d bytes\n", total_len);
  
  /* Put the maximum number of bytes */
  cc1100_fifo_put((uint8_t*)payload, len);
  //~ PRINTF("cc1100: put %d bytes\n", len);
  
  /* If CCA required, do it */
#if WITH_SEND_CCA
  cc1100_cmd_rx();
  micro_delay(1000);
  cc1100_cmd_tx();
#else /* WITH_SEND_CCA */
  cc1100_cmd_tx();
#endif /* WITH_SEND_CCA */

  /* Check if the radio is still in RX state */
  if ( cc1100_status_marcstate() != 0x0D) {
    
#if CC1100_CONF_TIMESTAMPS
    rtimer_clock_t txtime = timesynch_time();
#endif /* CC1100_CONF_TIMESTAMPS */

    if(receive_on) {
      ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
    }
    ENERGEST_ON(ENERGEST_TYPE_TRANSMIT);
    
    // wait until transfer started
    while ( cc1100_gdo0_read() == 0 );
    
    /* Now is time to transmit everything */
    while (ptr != payload_len) {
      
      /* wait until fifo threashold */
      while ( cc1100_gdo2_read() ) ;
      
      /* refill fifo */
      len = ( (payload_len-ptr) > 50)? 50: (payload_len-ptr);
      cc1100_fifo_put(&((uint8_t*)payload)[ptr], len);
      ptr += len;
      //~ PRINTF("cc1100: put %d bytes\n", len);
    }
    
    if (AUX_LEN>0) {
    
      while ( cc1100_status_txbytes() > (64-AUX_LEN) ) ;
#if CC1100_CONF_CHECKSUM
      cc1100_fifo_put(&checksum, CHECKSUM_LEN);
      //~ PRINTF("cc1100: put %d bytes\n", CHECKSUM_LEN);
#endif /* CC1100_CONF_CHECKSUM */
      
#if CC1100_CONF_TIMESTAMPS
      timestamp.authority_level = timesynch_authority_level();
      timestamp.time = timesynch_time();
      cc1100_fifo_put(&timestamp, TIMESTAMP_LEN);
      //~ PRINTF("cc1100: put %d bytes\n", TIMESTAMP_LEN);
#endif /* CC1100_CONF_TIMESTAMPS */
    
    }
    
    /* wait until frame is sent */
    while( cc1100_gdo0_read() );
    
    PRINTF("cc1100: done sending\n");

#if CC1100_CONF_TIMESTAMPS
    setup_time_for_transmission = txtime - timestamp.time;

    if(num_transmissions < 10000) {
      total_time_for_transmission += timesynch_time() - txtime;
      total_transmission_len += total_len;
      num_transmissions++;
    }

#endif /* CC1100_CONF_TIMESTAMPS */

#ifdef ENERGEST_CONF_LEVELDEVICE_LEVELS
    ENERGEST_OFF_LEVEL(ENERGEST_TYPE_TRANSMIT,cc1100_radio_get_txpower());
#endif
    ENERGEST_OFF(ENERGEST_TYPE_TRANSMIT);
    if(receive_on) {
      ENERGEST_ON(ENERGEST_TYPE_LISTEN);
    }

    on();
    RELEASE_LOCK();
    
    return 0;
  }
  
  
  /* If we are using WITH_SEND_CCA, we get here if the packet wasn't
     transmitted because of other channel activity. */
  RIMESTATS_ADD(contentiondrop);
  PRINTF("cc1100: do_send() transmission never started\n");
  on();
  RELEASE_LOCK();
  return -3;			/* Transmission never started! */
  
}
/*---------------------------------------------------------------------------*/
int
cc1100_radio_off(void)
{
  /* Don't do anything if we are already turned off. */
  if(receive_on == 0) {
    return 1;
  }

  /* If we are called when the driver is locked, we indicate that the
     radio should be turned off when the lock is unlocked. */
  if(locked) {
    lock_off = 1;
    return 1;
  }
  
  off();
  return 1;
}
/*---------------------------------------------------------------------------*/
int
cc1100_radio_on(void)
{
  if(receive_on) {
    return 1;
  }
  if(locked) {
    lock_on = 1;
    return 1;
  }

  on();
  return 1;
}
/*---------------------------------------------------------------------------*/
int
cc1100_radio_get_channel(void)
{
  return channel;
}
/*---------------------------------------------------------------------------*/
void
cc1100_radio_set_channel(int c)
{
  //~ uint16_t f;
  /*
   * Subtract the base channel (11), multiply by 5, which is the
   * channel spacing. 357 is 2405-2048 and 0x4000 is LOCK_THR = 1.
   */

  channel = c;
  
  //~ f = 5 * (c - 11) + 357 + 0x4000;
  //~ /*
   //~ * Writing RAM requires crystal oscillator to be stable.
   //~ */
  //~ while(!(status() & (BV(CC1100_XOSC16M_STABLE))));
//~ 
  //~ /* Wait for any transmission to end. */
  //~ while(status() & BV(CC1100_TX_ACTIVE));
//~ 
  //~ setreg(CC1100_FSCTRL, f);
//~ 
  //~ /* If we are in receive mode, we issue an SRXON command to ensure
     //~ that the VCO is calibrated. */
  //~ if(receive_on) {
    //~ strobe(CC1100_SRXON);
  //~ }

}
/*---------------------------------------------------------------------------*/
void
cc1100_radio_set_pan_addr(unsigned pan,
			   unsigned addr,
			   const uint8_t *ieee_addr)
{
  //~ uint16_t f = 0;
  /*
   * Writing RAM requires crystal oscillator to be stable.
   */
  //~ while(!(status() & (BV(CC1100_XOSC16M_STABLE))));

  pan_id = pan;
  //~ FASTSPI_WRITE_RAM_LE(&pan, CC1100RAM_PANID, 2, f);
  //~ FASTSPI_WRITE_RAM_LE(&addr, CC1100RAM_SHORTADDR, 2, f);
  //~ if(ieee_addr != NULL) {
    //~ FASTSPI_WRITE_RAM_LE(ieee_addr, CC1100RAM_IEEEADDR, 8, f);
  //~ }
}
/*---------------------------------------------------------------------------*/
/*
 * Interrupt leaves frame intact in FIFO.
 */
#if CC1100_CONF_TIMESTAMPS
static volatile rtimer_clock_t interrupt_time;
static volatile int interrupt_time_set;
#endif /* CC1100_CONF_TIMESTAMPS */
#if CC1100_TIMETABLE_PROFILING
#define cc1100_radio_timetable_size 16
TIMETABLE(cc1100_radio_timetable);
TIMETABLE_AGGREGATE(aggregate_time, 10);
#endif /* CC1100_TIMETABLE_PROFILING */

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(cc1100_radio_process, ev, data)
{
  PROCESS_BEGIN();

  PRINTF("cc1100_radio_process: started\n");
  
  while(1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
    
    if (radio_event == EVENT_EOP) {
      GET_LOCK();
      radio_event = 0;
      if ( !rx_begin() || !rx() || !rx_end() ) {
        on();
        RELEASE_LOCK();
        continue;
      }
      
    } else if (radio_event == EVENT_FIFO) {
      radio_event = 0;
      if ( !rx_begin() ) { // get length
        on();
        RELEASE_LOCK();
        continue;
      }
      
      if ( !rx() ) { // get what's available
        on();
        RELEASE_LOCK();
        continue;
      }
      
      while ( radio_event != EVENT_EOP ) {
        PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
        if (radio_event == EVENT_FIFO) {
          radio_event = 0;
          if ( !rx() ) {
            on();
            RELEASE_LOCK();
            break;
          }
        }
      }
      
      if (radio_event == EVENT_EOP) {
        if ( !rx() || !rx_end() ){
          on();
          RELEASE_LOCK();
          continue;
        }
      } else {
        on();
        RELEASE_LOCK();
        continue;
      }
      
    } else {
      on();
        RELEASE_LOCK();
      continue;
    }
    
#if CC1100_TIMETABLE_PROFILING
    TIMETABLE_TIMESTAMP(cc1100_radio_timetable, "poll");
#endif /* CC1100_TIMETABLE_PROFILING */
    
    if(receiver_callback != NULL) {
      PRINTF("cc1100_radio_process: calling receiver callback\n");
      receiver_callback(&cc1100_radio_driver);
#if CC1100_TIMETABLE_PROFILING
      TIMETABLE_TIMESTAMP(cc1100_radio_timetable, "end");
      timetable_aggregate_compute_detailed(&aggregate_time,
             &cc1100_radio_timetable);
      timetable_clear(&cc1100_radio_timetable);
#endif /* CC1100_TIMETABLE_PROFILING */
    } else {
      PRINTF("cc1100_radio_process not receiving function\n");
    }
    
    on();
    RELEASE_LOCK();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
int
cc1100_radio_read(void *buf, unsigned short bufsize)
{
  uint8_t footer[2];
#if CC1100_CONF_CHECKSUM
  uint16_t checksum;
#endif /* CC1100_CONF_CHECKSUM */
#if CC1100_CONF_TIMESTAMPS
  struct timestamp t;
#endif /* CC1100_CONF_TIMESTAMPS */
  
  // STITCH: if a packet has been received, but the process has not been
  // executed, execute it manually
  if (radio_event == EVENT_EOP && rx_buffer_len == 0 && rx_buffer_ptr == 0) {
    void (* old_callback)(const struct radio_driver *);
    
    old_callback = receiver_callback;
    // deactivate the callback that should happen at the end of the reception
    receiver_callback = NULL;
    
    struct process *p = &cc1100_radio_process;
    p->thread(&p->pt, PROCESS_EVENT_POLL, 0x0);
    
    // reactivate the callback
    receiver_callback = old_callback;
  }
  
  
  // check if there really is a packet in the buffer
  if (rx_buffer_len == 0 || rx_buffer_len != rx_buffer_ptr) {
    // if not return 0
    return 0;
  }

  
#if CC1100_CONF_TIMESTAMPS
  if(interrupt_time_set) {
    cc1100_radio_time_of_arrival = interrupt_time;
    interrupt_time_set = 0;
  } else {
    cc1100_radio_time_of_arrival = 0;
  }
  cc1100_radio_time_of_departure = 0;
#endif /* CC1100_CONF_TIMESTAMPS */

  if(rx_buffer_len - AUX_LEN > bufsize) {
    RIMESTATS_ADD(toolong);
    return 0;
  }
  
  memcpy(buf, rx_buffer, rx_buffer_len-AUX_LEN);
  
#if CC1100_CONF_CHECKSUM
  //~ getrxdata(&checksum, CHECKSUM_LEN);
#endif /* CC1100_CONF_CHECKSUM */

#if CC1100_CONF_TIMESTAMPS
  //~ getrxdata(&t, TIMESTAMP_LEN);
#endif /* CC1100_CONF_TIMESTAMPS */

  //~ getrxdata(footer, FOOTER_LEN);
  
#if CC1100_CONF_CHECKSUM
  if(checksum != crc16_data(rx_buffer, rx_buffer_len - AUX_LEN, 0)) {
    PRINTF("checksum failed 0x%04x != 0x%04x\n",
	   checksum, crc16_data(rx_buffer, rx_buffer_len - AUX_LEN, 0));
  }
  
  if(footer[1] & FOOTER1_CRC_OK &&
     checksum == crc16_data(rx_buffer, rx_buffer_len - AUX_LEN, 0)) {
#else
  //~ if(footer[1] & FOOTER1_CRC_OK) {
  if ( 1 ) {
#endif /* CC1100_CONF_CHECKSUM */

    cc1100_radio_last_rssi = footer[0];
    cc1100_radio_last_correlation = footer[1] & FOOTER1_CORRELATION;

    packetbuf_set_attr(PACKETBUF_ATTR_RSSI, cc1100_radio_last_rssi);
    packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, cc1100_radio_last_correlation);
    
    RIMESTATS_ADD(llrx);
    
#if CC1100_CONF_TIMESTAMPS
    cc1100_radio_time_of_departure =
      t.time +
      setup_time_for_transmission +
      (total_time_for_transmission * (len - 2)) / total_transmission_len;
  
    cc1100_radio_authority_level_of_sender = t.authority_level;

    packetbuf_set_attr(PACKETBUF_ATTR_TIMESTAMP, t.time);
#endif /* CC1100_CONF_TIMESTAMPS */
  
  } else {
    RIMESTATS_ADD(badcrc);
    rx_buffer_len = AUX_LEN;
  }
  
  int ret_len = rx_buffer_len - AUX_LEN;
  rx_buffer_len = 0;
  rx_buffer_ptr = 0;
  
  return ret_len;
}
/*---------------------------------------------------------------------------*/
static uint8_t tx_power;
void
cc1100_radio_set_txpower(uint8_t power)
{
  tx_power = power;
  GET_LOCK();
  
  cc1100_cfg_patable(&power, 1);
  cc1100_cfg_pa_power(0);
    
  RELEASE_LOCK();
}
/*---------------------------------------------------------------------------*/
int
cc1100_radio_get_txpower(void)
{
  return (int) tx_power;
}
/*---------------------------------------------------------------------------*/
int
cc1100_radio_rssi(void)
{
  int rssi;
  int radio_was_off = 0;
  
  if(!receive_on) {
    radio_was_off = 1;
    cc1100_radio_on();
  }
  //TODO wait a little

  rssi = (int)cc1100_status_rssi();

  if(radio_was_off) {
    cc1100_radio_off();
  }
  return rssi;
}
/*---------------------------------------------------------------------------*/
/* Interrupt routines */
static uint16_t irq_fifo(void)
{ 
  radio_event = EVENT_FIFO;
  process_poll(&cc1100_radio_process);
  return 1;
}
static uint16_t irq_eop(void)
{
#if CC1100_CONF_TIMESTAMPS
  interrupt_time = timesynch_time();
  interrupt_time_set = 1;
#endif /* CC1100_CONF_TIMESTAMPS */
  
  radio_event = EVENT_EOP;
  process_poll(&cc1100_radio_process);
#if CC1100_TIMETABLE_PROFILING
  timetable_clear(&cc1100_radio_timetable);
  TIMETABLE_TIMESTAMP(cc1100_radio_timetable, "interrupt");
#endif /* CC1100_TIMETABLE_PROFILING */
  return 1;
}


/* Other Static Functions */
static int rx_begin(void)
{
  rx_buffer_ptr = 0;
  
  if (cc1100_status_rxbytes() == 0)
  {
    PRINTF("rx_begin: nothing in fifo !!\n");
    return 0;
  }
  
  // get the length byte
  cc1100_fifo_get(&rx_buffer_len, 1);
  
  if (rx_buffer_len > BUFFER_SIZE || rx_buffer_len == 0)
  {
    PRINTF("rx_begin: error length (%d)\n", rx_buffer_len);
    rx_buffer_len = 0;
    return 0;
  }
  
  return 1;
}


static int rx(void)
{   
  uint8_t fifo_len;
  
  fifo_len = cc1100_status_rxbytes();
  
  if (fifo_len & 0x80)
  {
    PRINTF("rx: error rxfifo overflow (%d)\n", fifo_len);
    return 0;
  }
  
  if (fifo_len > 0)
  {
    if ( rx_buffer_ptr + fifo_len > BUFFER_SIZE)
    {
      PRINTF("rx: error local overflow\n");
      return 0;
    }
    
    cc1100_fifo_get(rx_buffer+rx_buffer_ptr, fifo_len);
    rx_buffer_ptr += fifo_len;
    
    // we should have emptied the FIFO, but since communication is slow...
    fifo_len = cc1100_status_rxbytes();
    
    // if there are more bytes than the threshold, do as if an interrupt occured
    if (fifo_len >= RX_FIFO_THR) {
      return rx();
    }
  }
  
  return 1;
}
static int rx_end(void)
{
  // check if we have the entire packet
  if (rx_buffer_len != rx_buffer_ptr)
  {
    PRINTF("rx_end: lengths don't match [%u!=%u]\n", rx_buffer_len, rx_buffer_ptr);
    return 0;
  }
  
  if ( ! (cc1100_status_crc_lqi() & 0x80 ) )
  {
    PRINTF("rx_end: error CRC\n");
    return 0;
  }

  PRINTF("rx_end: ok [%d bytes]\n", rx_buffer_len);
  
  return 1;
}
