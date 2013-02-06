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
 * \brief source file of the TDMA MAC coordinator implementation
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inrialpes.fr>
 * \date November 2009
 */

#include <io.h>
#include <stdlib.h>
#include <string.h>
#include "tdma_c.h"
#include "tdma_timings.h"
#include "tdma_frames.h"
#include "tdma_mgt.h"
#include "cc1101.h"
#include "ds2411.h"
#include "timerB.h"

/* MACROS */
#ifndef NULL
    #define NULL 0x0
#endif

#define LEDS_DEBUG
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

// ALARMS
#define ALARM_SLOTS  TIMERB_ALARM_CCR0

uint8_t node_addr=0x0;

/* STATIC PROTOTYPES */
static void set_rx(void);
static uint16_t slot_alarm(void);
static uint16_t beacon_send(void);
static uint16_t beacon_sent(void);
static uint16_t slot_data(void);
static uint16_t slot_control(void);

/* GLOBAL VARIABLES */
slot_t mac_slots[DATA_SLOT_MAX];

/* STATIC VARIABLES */
// frames
static beacon_msg_t beacon_msg;
static control_msg_t control_msg;
static data_msg_t data_msg;
static footer_t footer;

// times
static uint16_t beacon_eop_time;

// other
static uint16_t slot_count;
static uint16_t (*new_data_cb)(int16_t);


void mac_init(uint8_t channel) {
    int16_t i;

    // initialize the unique serial number chip and set node address accordingly
    ds2411_init();
    node_addr = ds2411_id.serial0 & HEADER_ADDR_MASK;

    // seed the random number generator
    srand((ds2411_id.serial0<<8)+ds2411_id.serial1);

    // initialize the timerB, with the beacon perdiod
    timerB_init();
    timerB_start_ACLK_div(TIMERB_DIV_1);
    timerB_set_alarm_from_now(ALARM_SLOTS, SLOT_LENGTH, SLOT_LENGTH);
    timerB_register_cb(ALARM_SLOTS, slot_alarm);

    // configure the radio
    cc1101_init();
    cc1101_cmd_idle();

    /* configure the radio behaviour */
    cc1101_cfg_append_status(CC1101_APPEND_STATUS_ENABLE);
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

    // go to RX after TX
    cc1101_cfg_rxoff_mode(CC1101_RXOFF_MODE_STAY_RX);
    cc1101_cfg_txoff_mode(CC1101_TXOFF_MODE_RX);

    uint8_t table[1];
    table[0] = 0x50; // 0dBm
    cc1101_cfg_patable(table, 1);
    cc1101_cfg_pa_power(0);

    // set IDLE state, flush everything
    cc1101_cmd_idle();
    cc1101_cmd_flush_rx();
    cc1101_cmd_flush_tx();

    // configure irq
    cc1101_cfg_gdo0(CC1101_GDOx_SYNC_WORD);
    cc1101_gdo0_int_set_falling_edge();

    // configure the beacon frame
    beacon_msg.hdr.length = BEACON_LENGTH-1;
    HEADER_SET_ADDR(beacon_msg.hdr, node_addr);
    HEADER_SET_TYPE(beacon_msg.hdr,BEACON_TYPE);
    beacon_msg.seq=0;

    // initialize the slot management service
    tdma_mgt_init();
    // reset slot count
    slot_count = -1;

    // init the data slots
    for (i=0;i<DATA_SLOT_MAX;i++) {
        mac_slots[i].ready=0;
        mac_slots[i].addr=0;
    }

    // reset the callback
    new_data_cb = 0x0;
}

void mac_set_new_data_cb(uint16_t (*cb)(int16_t)) {
    new_data_cb = cb;
}

static void set_rx(void) {
    // idle, flush
    cc1101_cmd_idle();
    cc1101_cmd_flush_rx();
    cc1101_cmd_flush_tx();
    // set RX
    cc1101_cmd_rx();

    // clear interrupt
    cc1101_gdo0_int_clear();
    cc1101_gdo2_int_clear();
}

static uint16_t slot_alarm(void) {
    slot_count++;
    if (slot_count>CTRL_SLOT) {
        // beacon
        slot_count = BEACON_SLOT;
        beacon_send();
    } else if (slot_count<=DATA_SLOT_MAX) {
        // dataslot
        cc1101_gdo0_register_callback(slot_data);

    } else {
        // controlslot
        LED_GREEN_OFF();
        LED_BLUE_ON();
        cc1101_gdo0_register_callback(slot_control);

    }
    return 0;
}

static uint16_t beacon_send(void) {
    LED_RED_ON();
    LED_GREEN_OFF();
    LED_BLUE_OFF();

    // flush
    cc1101_cmd_idle();
    cc1101_cmd_flush_rx();
    cc1101_cmd_flush_tx();

    // calibrate
    cc1101_cmd_calibrate();

    // config IRQ
    cc1101_gdo0_register_callback(beacon_sent);
    cc1101_gdo0_int_clear();
    cc1101_gdo0_int_enable();

    // start TX
    cc1101_cmd_tx();

    // put frame in FIFO
    cc1101_fifo_put((uint8_t*)&beacon_msg, beacon_msg.hdr.length+1);

    beacon_msg.ctl=0;
    beacon_msg.data=0;
    return 0;
}

static uint16_t beacon_sent(void) {
    beacon_eop_time = timerB_time();
    LED_RED_OFF();
    beacon_msg.seq++;

    cc1101_gdo0_register_callback(slot_data);
    LED_GREEN_ON();
    return 0;
}

static uint16_t slot_data(void) {
    uint8_t len, src;
    uint16_t now;
    now = timerB_time();

    // test CRC and bytes in FIFO
    len = cc1101_status_rxbytes();

    // check fifo length
    if (len==0) {
        // empty packet
        //~ printf("empty");
        return 0;
    } else if (len>64) {
        // overflow, flush
        //~ printf("over");
        set_rx();
        return 0;
    }

    // get length, and check
    cc1101_fifo_get(&data_msg.hdr.length, 1);
    if (data_msg.hdr.length>(DATA_LENGTH-1)) {
        // length too big, can't empty, flush
        //~ printf("big");
        set_rx();
        return 0;
    }
    // length is good
    // get data
    cc1101_fifo_get((uint8_t*)&data_msg+1, data_msg.hdr.length);
    // get status
    cc1101_fifo_get((uint8_t*)&footer, FOOTER_LENGTH);

    // check CRC
    if ( (footer.crc&0x80)==0 ) {
        // bad crc, exit
        //~ printf("crc");
        return 0;
    }

    // check type, destination
    if (HEADER_GET_TYPE(data_msg.hdr) != DATA_TYPE) {
        //~ printf("not_data");
        return 0;
    }

    // check source corresponds to timeslot
    src = HEADER_GET_ADDR(data_msg.hdr);

    if (tdma_mgt_getaddr(slot_count)!=src) {
        // src doesn't match slot
        return 0;
    }

    // check data has been read
    if (mac_slots[slot_count-1].ready==0) {
        memcpy(mac_slots[slot_count-1].data, data_msg.payload, MAC_PAYLOAD_SIZE);
        mac_slots[slot_count-1].ready = 1;

        // wake the CPU up by returning 1
        return 1;
    }
    return 0;
}

static uint16_t slot_control(void) {
    uint8_t len, src;

    // test bytes in FIFO
    len = cc1101_status_rxbytes();

    // check fifo length
    if (len==0) {
        // empty packet
        //~ printf("empty");
        return 0;
    } else if (len>64) {
        // overflow, flush
        //~ printf("over");
        set_rx();
        return 0;
    }

    // get length, and check
    cc1101_fifo_get(&control_msg.hdr.length, 1);
    if (control_msg.hdr.length<(CONTROL_LENGTH-1)) {
        // length too small, download packet and return
        cc1101_fifo_get((uint8_t*)&control_msg+1, control_msg.hdr.length);
        cc1101_fifo_get((uint8_t*)&footer, FOOTER_LENGTH);
        //~ printf("small");
        return 0;
    } else if (control_msg.hdr.length>(CONTROL_LENGTH-1)) {
        // length too big, can't empty, flush
        //~ printf("big");
        set_rx();
        return 0;
    }

    // length is good
    // get data+status
    cc1101_fifo_get((uint8_t*)&control_msg+1, CONTROL_LENGTH-1);
    cc1101_fifo_get((uint8_t*)&footer, FOOTER_LENGTH);

    // check CRC
    if ( (footer.crc&0x80)==0 ) {
        // bad crc, exit
        //~ printf("crc");
        return 0;
    }

    src = HEADER_GET_ADDR(control_msg.hdr);
    // it's valid data for me, check content
    if (CONTROL_GET_TYPE(control_msg) == CONTROL_ATTACH_REQ) {
        int16_t slot;
        slot = tdma_mgt_attach(src);
        if (slot>0) {
            CONTROL_SET_TYPE(beacon_msg, CONTROL_ATTACH_OK);
            CONTROL_SET_ADDR(beacon_msg, src);
            beacon_msg.data = (uint8_t)slot;
            mac_slots[slot-1].addr = src;
        } else {
            CONTROL_SET_TYPE(beacon_msg, CONTROL_ATTACH_ERR);
            CONTROL_SET_ADDR(beacon_msg, src);
            beacon_msg.data = 0xFF;
        }
    }
    return 0;
}
