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
 * \brief OS-free implementation of CSMA MAC protocol
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inrialpes.fr>
 * \date November 2009
 * 
 * The protocol implemented is basic CSMA: all the nodes are always
 * in RX mode. If a frame is received, the callback function provided
 * by the user application is called, with the received data passed
 * as argument. When the node wants to send some data, a CCA (clear
 * channel assessment) is done, and the frame is sent if the channel
 * is clear, otherwise a random backoff is waited before a new try
 * can occur.
 */

#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mac.h"
#include "cc1100.h"
#include "ds2411.h"
#include "timerB.h"

#define PACKET_LENGTH_MAX 58

#define HEADER_LENGTH   sizeof(ack_t)-1
#define TYPE_DATA 0xAA
#define TYPE_ACK  0xBB

#define DELAY_COUNT_MAX 6
#define ALARM_RETRY TIMERB_ALARM_CCR0
#define ACK_TIMEOUT 131 // 4ms

#if 0
    #define PRINTF(...) printf(__VA_ARGS__)
#else
    #define PRINTF(...) 
#endif

typedef struct {
    uint8_t length;
    uint8_t type;
    uint8_t dst_addr[2];
    uint8_t src_addr[2];
    uint8_t payload[PACKET_LENGTH_MAX];
} frame_t;

typedef struct {
    uint8_t length;
    uint8_t type;
    uint8_t dst_addr[2];
    uint8_t src_addr[2];
} ack_t;

// node's MAC address
uint16_t node_addr;

// callback for received packets
static mac_received_t received_cb;
static mac_sent_t sent_cb;
static mac_error_t error_cb;

// frame received
static frame_t rxframe, txframe;
static ack_t ack;

// retry count
static uint16_t delay_count;

// prototypes
static uint16_t rx_set(void);
static uint16_t rx_parse(void);
static uint16_t rx_ackdone(void);

static uint16_t tx_try(void);
static uint16_t tx_delay(void);
static uint16_t tx_done(void);
static uint16_t tx_ack(void);

void mac_init(uint8_t channel)
{
    // initialize the unique serial number chip and set node address accordingly
    ds2411_init();
    node_addr = (((uint16_t)ds2411_id.serial1)<<8) + (ds2411_id.serial0);
    
    // seed the random number generator
    srand(node_addr);
    
    // reset callbacks
    received_cb = 0x0;
    sent_cb = 0x0;
    error_cb = 0x0;
    
    // initialize the timerB
    timerB_init();
    timerB_start_ACLK_div(1);
    timerB_register_cb(ALARM_RETRY, tx_try);
    
    // configure the radio
    cc1100_init();
    cc1100_cmd_idle();
    
    /* configure the radio behaviour */
    cc1100_cfg_append_status(CC1100_APPEND_STATUS_ENABLE);
    cc1100_cfg_crc_autoflush(CC1100_CRC_AUTOFLUSH_DISABLE);
    cc1100_cfg_white_data(CC1100_DATA_WHITENING_ENABLE);
    cc1100_cfg_crc_en(CC1100_CRC_CALCULATION_ENABLE);
    cc1100_cfg_freq_if(0x0E);
    cc1100_cfg_fs_autocal(CC1100_AUTOCAL_IDLE_TO_TX_RX);
    cc1100_cfg_mod_format(CC1100_MODULATION_MSK);
    cc1100_cfg_sync_mode(CC1100_SYNCMODE_30_32);
    cc1100_cfg_manchester_en(CC1100_MANCHESTER_DISABLE);
    cc1100_cfg_cca_mode(CC1100_CCA_MODE_RSSI_PKT_RX);
    
    // freq = 860MHz
    cc1100_write_reg(CC1100_REG_FREQ2, 0x1F);
    cc1100_write_reg(CC1100_REG_FREQ1, 0xDA);
    cc1100_write_reg(CC1100_REG_FREQ0, 0x12);
    
    // configure the radio channel (300kHz spacing)
    cc1100_cfg_chanspc_e(0x3);
    cc1100_cfg_chanspc_m(0x6C);
    cc1100_cfg_chan(channel<<1); // channel x2 to get 600kHz spacing
    
    // rise CCA threshold
    cc1100_cfg_carrier_sense_abs_thr(5);
    
    // set channel bandwidth (560 kHz)
    cc1100_cfg_chanbw_e(0);
    cc1100_cfg_chanbw_m(2);

    // set data rate (0xD/0x2F is 250kbps)
    cc1100_cfg_drate_e(0x0D);
    cc1100_cfg_drate_m(0x2F);

    // go to RX after RX and TX
    cc1100_cfg_rxoff_mode(CC1100_RXOFF_MODE_IDLE);
    cc1100_cfg_txoff_mode(CC1100_TXOFF_MODE_RX);

    uint8_t table[1];
    //~ table[0] = 0x03; // -30dBm
    //~ table[0] = 0x0F; // -20dBm
    //~ table[0] = 0x1E; // -15dBm
    //~ table[0] = 0x27; // -10dBm
    //~ table[0] = 0x67; // -5dBm
    table[0] = 0x50; // 0dBm
    //~ table[0] = 0x81; // +5dBm
    //~ table[0] = 0xC2; // +10dBm
    cc1100_cfg_patable(table, 1);
    cc1100_cfg_pa_power(0);
    
    // set IDLE state, flush everything, and start rx
    cc1100_cmd_idle();
    cc1100_cmd_flush_rx();
    cc1100_cmd_flush_tx();
    cc1100_cmd_calibrate();
    
    // configure irq
    cc1100_cfg_gdo0(CC1100_GDOx_SYNC_WORD);
    cc1100_gdo0_int_set_falling_edge();
    cc1100_gdo0_int_clear();
    cc1100_gdo0_int_enable();
    cc1100_gdo0_register_callback(rx_parse);
    
    // start the machine
    rx_set();
    
    txframe.length = 0;
}
   
void mac_set_rx_cb(mac_received_t cb) {
    received_cb = cb;
}

void mac_set_sent_cb(mac_sent_t cb) {
    sent_cb = cb;
}

void mac_set_error_cb(mac_error_t cb) {
    error_cb = cb;
}

critical uint16_t mac_send(uint8_t packet[], uint16_t length, uint16_t dst_addr) {
    // check length
    if (length>PACKET_LENGTH_MAX) {
        PRINTF("mac_send length error\n");
        return 2;
    }
    
    // check state
    if (txframe.length != 0) {
        PRINTF("mac_send already sending\n");
        // already sending, can't do anything
        return 1;
    }
    // prepare header
    txframe.length = length + HEADER_LENGTH;
    txframe.type = TYPE_DATA;
    txframe.dst_addr[0] = dst_addr>>8;
    txframe.dst_addr[1] = dst_addr & 0xFF;
    txframe.src_addr[0] = node_addr>>8;
    txframe.src_addr[1] = node_addr & 0xFF;
    
    // copy packet to the local variable
    memcpy(txframe.payload, packet, length);
    
    // try to send
    delay_count = 0;
    tx_delay();
    return 0;
}

critical void mac_stop(void) {
    cc1100_cmd_idle();
    cc1100_gdo0_int_disable();
    cc1100_gdo2_int_disable();
    cc1100_cmd_pwd();
    
    timerB_stop();
}
critical void mac_restart(void) {
    rx_set();
}

static uint16_t rx_set(void) {
    /* we arrive here:
     * - at start up
     * - after having sent a frame
     * - after having failed to send a frame
     * - after having received a frame
     * therefore we check the possible states to repair
     * any erroneous situation.
     */
    uint8_t status;
    status = cc1100_status() & 0x70;
    
    if ( status != 0x10) {
        // not RX
        // flush everything
        cc1100_cmd_idle();
        cc1100_cmd_flush_rx();
        cc1100_cmd_flush_tx();
        cc1100_gdo0_int_clear();
        cc1100_cmd_rx();
    } else if ((cc1100_gdo0_read() == 0) & (cc1100_status_rxbytes()>0)) {
        // RX with data in FIFO
        // flush everything
        cc1100_cmd_idle();
        cc1100_cmd_flush_rx();
        cc1100_cmd_flush_tx();
        cc1100_gdo0_int_clear();
        cc1100_cmd_rx();
    }
    
    return 0;
}

static uint16_t tx_delay(void) {
    if (delay_count==0) {
        // if first try, quick
        timerB_set_alarm_from_now(ALARM_RETRY, 2, 0);
        timerB_register_cb(ALARM_RETRY, tx_try);
    } else if (delay_count >= DELAY_COUNT_MAX) {
        // to many tries, abort
        // delete packet
        txframe.length = 0;
        // reset callback
        cc1100_gdo0_register_callback(rx_parse);
        
        // reset rx
        cc1100_cmd_idle();
        rx_set();
        PRINTF("too many tries\n");
        // call the error callback
        if (error_cb) {
            return error_cb();
        }
    } else {
        uint16_t delay;
        // delay randomly between 1ms and 63ms
        delay = rand();
        delay &= ((1<<11)-1);
        delay += 32;
        
        timerB_set_alarm_from_now(ALARM_RETRY, delay, 0);
        timerB_register_cb(ALARM_RETRY, tx_try);
    }
    delay_count ++;
    
    return 0;
}

static uint16_t tx_try(void) {
    uint8_t status;
    
    if (txframe.length == 0) {
        PRINTF("tx_try no packet error\n");
        return rx_set();
    }
    
    // if radio not in RX, delay
    status = cc1100_status() & 0x70;
    if ( status != 0x10) {
        PRINTF("try sending radio state error (%x)\n", status);
        tx_delay();
        
        return rx_set();
    }
    
    // if there are some weird bytes in TX FIFO, flush everything
    if (cc1100_status_txbytes()!=0) {
        PRINTF("mac had to flush\n");
        cc1100_cmd_idle();
        cc1100_cmd_flush_tx();
        cc1100_cmd_flush_rx();
    }
    
    // try to send
    cc1100_cmd_tx();
    
    // get chip status
    status = cc1100_status() & 0x70;
    
    // if status is not RX
    if ( status != 0x10) {
        // put data in fifo
        cc1100_fifo_put((uint8_t*)&txframe, txframe.length+1);
        cc1100_gdo0_register_callback(tx_done);
    } else {
        tx_delay();
        rx_set();
    }
    
    return 0;
}

static uint16_t tx_done(void) {
    // if destination is broadcast, don't wait for ACK
    if ((txframe.dst_addr[0]==0xFF) && (txframe.dst_addr[1]==0xFF)) {
        cc1100_gdo0_register_callback(rx_parse);
        txframe.length = 0;
        rx_set();
        if (sent_cb) {
            return sent_cb();
        }
    } else {
        cc1100_gdo0_register_callback(tx_ack);
        timerB_set_alarm_from_now(ALARM_RETRY, ACK_TIMEOUT, 0);
        timerB_register_cb(ALARM_RETRY, tx_delay);
    }
    return 0;
}

static uint16_t tx_ack(void) {
    uint8_t status[2];
    uint16_t dst;
    uint16_t ret_val;
    
    /* Check if there are bytes in FIFO */
    if (cc1100_status_rxbytes() == 0) {
        return rx_set();
    }
    
    /* Check Length is correct */
    cc1100_fifo_get( (uint8_t*) &(ack.length), 1);
    
    // if too big, flush
    if ( ack.length != HEADER_LENGTH ) {
        return rx_set();
    }
    
    /* Get Data */
    cc1100_fifo_get( (uint8_t*) &(ack.length)+1, ack.length);
    
    /* Get Status Bytes */
    cc1100_fifo_get(status, 2);
    
    /* Check CRC */
    if ( (status[1] & 0x80) == 0 ) {
        return rx_set();
    }
    
    ret_val = 0;
    
    /* Compute addresses */
    dst = (((uint16_t)ack.dst_addr[0])<<8) + ack.dst_addr[1];
    
    /* Check addresses */
    if ( (dst==node_addr) && (ack.src_addr[0]==txframe.dst_addr[0]) \
                           && (ack.src_addr[1]==txframe.dst_addr[1]) ) {
        txframe.length = 0;
        timerB_unset_alarm(ALARM_RETRY);
        cc1100_gdo0_register_callback(rx_parse);
        rx_set();
        if (sent_cb) {
            return sent_cb();
        }
    }
    return rx_set();
}

static uint16_t rx_parse(void) {
    uint8_t status[2];
    uint16_t src, dst;
    uint16_t ret_val;
    int16_t rssi;
    
    /* Check if there are bytes in FIFO */
    if ( (cc1100_status_rxbytes() == 0) || (cc1100_status_rxbytes() > 64) ) {
        return rx_set();
    }
    
    /* Check Length is correct */
    cc1100_fifo_get( (uint8_t*) &(rxframe.length), 1);
    // if too big, flush
    if ( rxframe.length > sizeof(rxframe)-1 ) {
        return rx_set();
    }
    
    /* Get Data */
    cc1100_fifo_get( (uint8_t*) &(rxframe.length)+1, rxframe.length);
    
    /* Get Status Bytes */
    cc1100_fifo_get(status, 2);
    
    /* Check CRC */
    if ( (status[1] & 0x80) == 0 ) {
        return rx_set();
    }
    
    /* Check for min length */
    if ( rxframe.length < HEADER_LENGTH ) {
        return rx_set();
    }
    
    /* Compute addresses */
    dst = (((uint16_t)rxframe.dst_addr[0])<<8) + rxframe.dst_addr[1];
    src = (((uint16_t)rxframe.src_addr[0])<<8) + rxframe.src_addr[1];
    
    ret_val = 0;
    
    int len;
    len = rxframe.length;
    len -= HEADER_LENGTH;
    
    rssi = status[0] >= 128 ? status[0]-256 : status[0];
    rssi -= 140;
    
    // if for me, send an ACK
    if (dst==node_addr) {
        ack.length = HEADER_LENGTH;
        ack.type = TYPE_ACK;
        ack.dst_addr[0] = rxframe.src_addr[0];
        ack.dst_addr[1] = rxframe.src_addr[1];
        ack.src_addr[0] = rxframe.dst_addr[0];
        ack.src_addr[1] = rxframe.dst_addr[1];
        if (cc1100_status_txbytes()) {
            cc1100_cmd_flush_tx();
        }
        cc1100_cmd_tx();
        cc1100_fifo_put((uint8_t*)&ack, ack.length+1);
        cc1100_gdo0_register_callback(rx_ackdone);
        
        if (received_cb) {
            return received_cb(rxframe.payload, len, src, rssi);
        }
        
    } else if ( (dst==0xFFFF) && received_cb ) {
        /* Call the packet received function */
        ret_val = received_cb(rxframe.payload, len, src, rssi);
        ret_val |= rx_set();
        return ret_val;
    } else {
        return rx_set();
    }
    
    return 0;
}

static uint16_t rx_ackdone(void) {
    cc1100_gdo0_register_callback(rx_parse);
    return rx_set();
}
