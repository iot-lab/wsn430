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

/**
 * \file
 * \brief source file of the TDMA MAC node implementation
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inrialpes.fr>
 * \date November 2009
 */

#include <io.h>
#include <stdlib.h>
#include <string.h>
#include "tdma_n.h"
#include "tdma_frames.h"
#include "tdma_timings.h"
#include "cc1101.h"
#include "ds2411.h"
#include "timerB.h"

/* MACROS */
#ifndef NULL
    #define NULL 0x0
#endif


#ifdef LEDS_DEBUG
#include "leds.h"
#else
#define LED_GREEN_OFF()
#define LED_GREEN_ON()
#define LED_BLUE_OFF()
#define LED_BLUE_ON()
#define LED_RED_OFF()
#define LED_RED_ON()
#endif

#define PRINT_DEBUG
#ifdef PRINT_DEBUG
#include <stdio.h>
#else
#define printf(...)
#endif

enum {
    STATE_BEACON_SEARCH, // looking for a beacon
    STATE_ATTACHING_WAIT_TX, // beacon found, waiting random time to send attach
    STATE_ATTACHING_WAIT_RX, // attach sent, waiting for response
    STATE_ATTACHED
};


#define ALARM_BEACON    TIMERB_ALARM_CCR0
#define ALARM_TIMEOUT TIMERB_ALARM_CCR1
#define ALARM_SEND      TIMERB_ALARM_CCR2

uint8_t node_addr=0x0;
static volatile uint8_t send_ready=0;

/* STATIC PROTOTYPES */
static void set_rx(void);
static uint16_t beacon_rx(void);
static uint16_t beacon_received(void);
static uint16_t beacon_timeout(void);
static uint16_t sync_detected(void);
static uint16_t control_send(void);
static uint16_t control_sent(void);
static uint16_t slot_send(void);
static uint16_t slot_sent(void);

/* STATIC VARIABLES */
static beacon_msg_t beacon_msg;
static uint16_t beacon_sync_time;
static uint16_t beacon_eop_time;
static uint16_t beacon_timeout_count;

static control_msg_t control_msg;
static uint16_t attach_backoff;

static data_msg_t data_msg;
uint8_t* const mac_payload=data_msg.payload;

static uint16_t sync_time;
static uint8_t coord_addr;
static uint8_t my_slot;
static uint8_t state;

static uint16_t (*access_allowed_cb)(void);

void mac_init(uint8_t channel) {
    // initialize the unique serial number chip and set node address accordingly
    ds2411_init();
    node_addr = ds2411_id.serial0 & HEADER_ADDR_MASK;

    // seed the random number generator
    srand((ds2411_id.serial0<<8)+ds2411_id.serial1);

    // initialize the timerB
    timerB_init();
    timerB_start_ACLK_div(TIMERB_DIV_1);

    // configure the radio
    cc1101_init();
    cc1101_cmd_idle();

    /* configure the radio behaviour */
    cc1101_cfg_append_status(CC1101_APPEND_STATUS_DISABLE);
    cc1101_cfg_crc_autoflush(CC1101_CRC_AUTOFLUSH_DISABLE);
    cc1101_cfg_white_data(CC1101_DATA_WHITENING_ENABLE);
    cc1101_cfg_crc_en(CC1101_CRC_CALCULATION_ENABLE);
    cc1101_cfg_freq_if(0x0E);
    cc1101_cfg_fs_autocal(CC1101_AUTOCAL_NEVER);
    cc1101_cfg_mod_format(CC1101_MODULATION_MSK);
    cc1101_cfg_sync_mode(CC1101_SYNCMODE_30_32);
    cc1101_cfg_manchester_en(CC1101_MANCHESTER_DISABLE);
    cc1101_cfg_cca_mode(CC1101_CCA_MODE_RSSI_PKT_RX);

    // freq = 860MHz
    cc1101_write_reg(CC1101_REG_FREQ2, 0x1F);
    cc1101_write_reg(CC1101_REG_FREQ1, 0xDA);
    cc1101_write_reg(CC1101_REG_FREQ0, 0x12);

    // configure the radio channel (300kHz spacing)
    cc1101_cfg_chanspc_e(0x3);
    cc1101_cfg_chanspc_m(0x6C);
    cc1101_cfg_chan(channel<<1); // channel x2 to get 600kHz spacing

    // set channel bandwidth (560 kHz)
    cc1101_cfg_chanbw_e(0);
    cc1101_cfg_chanbw_m(2);

    // set data rate (0xD/0x2F is 250kbps)
    cc1101_cfg_drate_e(0x0D);
    cc1101_cfg_drate_m(0x2F);

    // go to IDLE after RX and TX
    cc1101_cfg_rxoff_mode(CC1101_RXOFF_MODE_IDLE);
    cc1101_cfg_txoff_mode(CC1101_TXOFF_MODE_IDLE);

    uint8_t table[1];
    table[0] = 0x50; // 0dBm
    cc1101_cfg_patable(table, 1);
    cc1101_cfg_pa_power(0);

    // set IDLE state, flush everything
    cc1101_cmd_idle();
    cc1101_cmd_flush_rx();
    cc1101_cmd_flush_tx();

    // configure irq eop
    cc1101_cfg_gdo0(CC1101_GDOx_SYNC_WORD);
    cc1101_gdo0_int_set_falling_edge();
    cc1101_gdo0_int_clear();
    cc1101_gdo0_int_enable();
    cc1101_gdo0_register_callback(beacon_received);

    // configure irq sync
    cc1101_cfg_gdo2(CC1101_GDOx_SYNC_WORD);
    cc1101_gdo2_int_set_rising_edge();
    cc1101_gdo2_int_clear();
    cc1101_gdo2_int_enable();
    cc1101_gdo2_register_callback(sync_detected);

    // start RX
    state = STATE_BEACON_SEARCH;
    printf("BEACON_SEARCH\n");
    my_slot = -1;
    set_rx();

    // initialize the flag
    send_ready = 0;
    // reset the callback
    access_allowed_cb = 0x0;
}

int16_t mac_is_access_allowed(void) {
    return (send_ready==0);
}

void mac_send(void) {
    send_ready = 1;
}

void mac_set_access_allowed_cb(uint16_t (*cb)(void)) {
    access_allowed_cb = cb;
}

static void set_rx(void) {
    // idle, flush, calibrate
    cc1101_cmd_idle();
    cc1101_cmd_flush_rx();
    cc1101_cmd_flush_tx();
    cc1101_cmd_calibrate();

    // set RX
    cc1101_cmd_rx();
    LED_RED_ON();
}

static uint16_t beacon_received() {
    uint8_t coord, seq;
    uint16_t now;
    now = timerB_time();

    // test CRC and bytes in FIFO
    if ( ((cc1101_status_crc_lqi()&0x80)==0) ||
         (cc1101_status_rxbytes()!=BEACON_LENGTH) ) {
        set_rx();
        return 0;
    }

    // data
    cc1101_fifo_get((uint8_t*)&beacon_msg, BEACON_LENGTH);

    // check length, type
    if ( (beacon_msg.hdr.length != (BEACON_LENGTH-1)) ||
         (HEADER_GET_TYPE(beacon_msg.hdr) != BEACON_TYPE) ) {
        set_rx();
        return 0;
    }

    // unset timeout alarm
    timerB_unset_alarm(ALARM_TIMEOUT);

    coord = HEADER_GET_ADDR(beacon_msg.hdr);
    seq = beacon_msg.seq;
    beacon_eop_time = now;

    if (state==STATE_BEACON_SEARCH) {
        // we were looking for a beacon, store the coordinator addr
        coord_addr = coord;
    } else if (coord != coord_addr) {
        // beacon from unknown coordinator
        set_rx();
        return 0;
    }

    LED_RED_OFF();

    // reset timeout count
    beacon_timeout_count = 0;

    // save beacon time
    beacon_sync_time = sync_time-BEACON_OVERHEAD;

    // set alarm to receive beacon
    timerB_set_alarm_from_time(ALARM_BEACON,  // alarm #
                            BEACON_PERIOD,  // ticks
                            0,  // no period
                            beacon_sync_time-SAFETY_TIME); // reference
    timerB_register_cb(ALARM_BEACON, beacon_rx);

    // check state
    switch (state) {
        case STATE_BEACON_SEARCH:
        // pick a random backoff between 1 and 16
        attach_backoff = 1+(rand()&0xF);

        // update state
        state = STATE_ATTACHING_WAIT_TX;
        break;
    case STATE_ATTACHING_WAIT_TX:
        // decrement count
        attach_backoff--;

        if (attach_backoff==0) {
            // prepare attach request frame
            control_msg.hdr.length = CONTROL_LENGTH-1;
            HEADER_SET_ADDR(control_msg.hdr, node_addr);
            HEADER_SET_TYPE(control_msg.hdr, CONTROL_TYPE);
            CONTROL_SET_TYPE(control_msg, CONTROL_ATTACH_REQ);
            CONTROL_SET_ADDR(control_msg, coord_addr);

            // set timer to send attach request
            timerB_set_alarm_from_time(ALARM_SEND,
                            CTRL_SLOT*SLOT_LENGTH, // ticks
                            0,
                            beacon_sync_time);
            timerB_register_cb(ALARM_SEND, control_send);

            // update state
            state = STATE_ATTACHING_WAIT_RX;
            attach_backoff = 0;
        }
        break;
    case STATE_ATTACHING_WAIT_RX:
        if ( (CONTROL_GET_TYPE(beacon_msg)==CONTROL_ATTACH_OK) && \
                (CONTROL_GET_ADDR(beacon_msg)==node_addr) ) {
            // store my_slot
            my_slot = beacon_msg.data;
            state = STATE_ATTACHED;
        } else {
            // attach failed, retry at next beacon
            state = STATE_BEACON_SEARCH;
        }
        break;
    case STATE_ATTACHED:
        if (send_ready) {
            // prepare data frame
            data_msg.hdr.length = DATA_LENGTH-1;
            HEADER_SET_TYPE(data_msg.hdr, DATA_TYPE);
            HEADER_SET_ADDR(data_msg.hdr, node_addr);

            timerB_set_alarm_from_time(ALARM_SEND, // alarm #
                                    my_slot*SLOT_LENGTH, // ticks
                                    0, // period
                                    beacon_sync_time); // ref
            // set alarm callback
            timerB_register_cb(ALARM_SEND, slot_send);

            // put the data in the FIFO
            cc1101_fifo_put((uint8_t*)&data_msg, data_msg.hdr.length+1);
            send_ready=0;
            if (access_allowed_cb && access_allowed_cb()) {
                // if wanted we return 1 to wake the CPU up
                return 1;
            }
        }
        break;
    }

    return 0;
}

static uint16_t sync_detected(void) {
    sync_time = timerB_time();
    return 0;
}

static uint16_t beacon_rx(void) {
    // wake up radio
    set_rx();

    // set callback
    cc1101_gdo0_register_callback(beacon_received);
    cc1101_gdo0_int_clear();
    cc1101_gdo0_int_enable();

    // set alarm for beacon timeout
    timerB_set_alarm_from_now(ALARM_TIMEOUT,  // alarm #
                            TIMEOUT_TIME,  // ticks
                            0);  // no period
    timerB_register_cb(ALARM_TIMEOUT, beacon_timeout);
    return 0;
}

static uint16_t beacon_timeout(void) {
    // put radio to sleep
    cc1101_cmd_pwd();
    LED_RED_OFF();

    // increase timeout count
    beacon_timeout_count++;

    // if too many timeouts, reset state
    if (beacon_timeout_count >= TIMEOUT_COUNT_MAX) {
        set_rx();
        state = STATE_BEACON_SEARCH;
        printf("BEACON_SEARCH\n");
        return 0;
    }

    // reset alarm to receive beacon
    timerB_set_alarm_from_time(ALARM_BEACON,  // alarm #
                            BEACON_PERIOD*(beacon_timeout_count+1),  // ticks
                            0,  // period (same)
                            beacon_sync_time-(SAFETY_TIME*(beacon_timeout_count+1))); // reference

    return 0;
}

static uint16_t control_send(void) {
    LED_BLUE_ON();
    cc1101_gdo0_register_callback(control_sent);
    cc1101_gdo0_int_clear();

    cc1101_cmd_idle();
    cc1101_cmd_flush_tx();

    cc1101_cmd_tx();
    cc1101_fifo_put((uint8_t*)&control_msg, control_msg.hdr.length+1);

    return 0;
}

static uint16_t control_sent(void) {
    LED_BLUE_OFF();
    return 0;
}

static uint16_t slot_send(void) {
    LED_GREEN_ON();
    cc1101_cmd_tx();
    cc1101_gdo0_register_callback(slot_sent);
    return 0;
}
static uint16_t slot_sent(void) {
    // put radio to sleep
    cc1101_cmd_pwd();
    LED_GREEN_OFF();
    return 0;
}
