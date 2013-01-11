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
 * \brief OS-free implementation of XMAC MAC protocol
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inrialpes.fr>
 * \date November 2009
 * 
 */
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include "mac.h"
#include "cc1100.h"
#include "ds2411.h"
#include "timerB.h"
#include "leds.h"

#define PACKET_LENGTH_MAX 56

#define HEADER_LENGTH   0x5

#define STATE_WOR       0x0
#define STATE_RX        0x10
#define STATE_TX        0x20

#define TYPE_PREAMBLE 0x1
#define TYPE_ACK      0x2
#define TYPE_DATA     0x3
#define TYPE_DATAACK  0x4

#define MAX_DELAY_COUNT 0x5

// timer alarms
#define ALARM_PREAMBLE TIMERB_ALARM_CCR0
#define ALARM_TIMEOUT TIMERB_ALARM_CCR1
#define ALARM_RETRY TIMERB_ALARM_CCR2

// timing
#define SEND_PERIOD 108
#define MAX_PREAMBLE_COUNT 64
#define ACK_TIMEOUT 131

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
} preamble_t;

// node's MAC address
uint16_t node_addr;

// callback for received packets
static mac_received_t received_cb;
static mac_sent_t sent_cb;
static mac_error_t error_cb;

// frames
static frame_t frame, txframe;

// internal state
static uint16_t state;
static uint16_t frame_to_send=0;

// count
static uint16_t delay_count;
static uint16_t preamble_count;

// prototypes
static uint16_t set_wor(void);
static uint16_t read_frame(void);
static uint16_t ack_sent(void);
static uint16_t try_send(void);
static uint16_t medium_clear(void);
static uint16_t medium_busy(void);
static uint16_t send_preamble(void);
static uint16_t read_ack(void);
static uint16_t send_data(void);
static uint16_t send_done(void);
static uint16_t read_dataack(void);
static uint16_t ack_timeout(void);
static uint16_t delay_send(void);


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
    timerB_start_ACLK_div(TIMERB_DIV_1);
    
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
    
    // freq = 860MHz
    cc1100_write_reg(CC1100_REG_FREQ2, 0x1F);
    cc1100_write_reg(CC1100_REG_FREQ1, 0xDA);
    cc1100_write_reg(CC1100_REG_FREQ0, 0x12);
    
    // configure the radio channel (300kHz spacing)
    cc1100_cfg_chanspc_e(0x3);
    cc1100_cfg_chanspc_m(0x6C);
    cc1100_cfg_chan(channel<<1); // channel x2 to get 600kHz spacing
    
    // rise CCA threshold
    cc1100_cfg_carrier_sense_abs_thr(3);
    
    // set channel bandwidth (560 kHz)
    cc1100_cfg_chanbw_e(0);
    cc1100_cfg_chanbw_m(2);

    // set data rate (0xD/0x2F is 250kbps)
    cc1100_cfg_drate_e(0x0D);
    cc1100_cfg_drate_m(0x2F);

    // go to IDLE after RX and TX
    cc1100_cfg_rxoff_mode(CC1100_RXOFF_MODE_IDLE);
    cc1100_cfg_txoff_mode(CC1100_TXOFF_MODE_IDLE);
    
    // configure WOR
    cc1100_cfg_rx_time(2); ///FIXME
    cc1100_cfg_wor_res(0);
    cc1100_cfg_event0(4208);
    cc1100_cfg_event1(4);
    cc1100_cfg_rc_pd(CC1100_RC_OSC_ENABLE);

    uint8_t table[1];
    //~ table[0] = 0x81; // +5dBm
    //~ table[0] = 0x67; // -5dBm
    table[0] = 0x27; // -10dBm
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
    
    // start the machine
    set_wor();
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
 
uint16_t mac_send(uint8_t packet[], uint16_t length, uint16_t dst_addr) {
    // check length
    if (length>PACKET_LENGTH_MAX) {
        printf("mac_send, packet length error\n");
        return 2;
    }
    
    // check state
    if (frame_to_send) {
        // there is already a frame about to be sent
        printf("mac_send, frame_to_send=1 error\n");
        return 1;
    }
    
    // prepare header
    txframe.length = length + HEADER_LENGTH;
    txframe.type = TYPE_DATA;
    txframe.dst_addr[0] = dst_addr>>8;
    txframe.dst_addr[1] = dst_addr & 0xFF;
    txframe.src_addr[0] = node_addr>>8;
    txframe.src_addr[1] = node_addr & 0xFF;
    
    // copy payload
    uint16_t i;
    for (i = 0; i < length; i++)
        txframe.payload[i] = packet[i];
    
    // update frame to send flag
    frame_to_send = 1;
    delay_count = 0;
    
    // call try_send to start TX procedure
    try_send();
    return 0;
}

static uint16_t set_wor(void) {
    cc1100_cmd_idle();
    
    cc1100_cfg_txoff_mode(CC1100_TXOFF_MODE_IDLE);
    cc1100_cfg_rxoff_mode(CC1100_RXOFF_MODE_IDLE);
    cc1100_cfg_cca_mode(CC1100_CCA_MODE_RSSI_PKT_RX);
    cc1100_cfg_rc_pd(CC1100_RC_OSC_ENABLE);
    cc1100_cfg_fs_autocal(CC1100_AUTOCAL_IDLE_TO_TX_RX);

    cc1100_gdo0_register_callback(read_frame);
    cc1100_gdo0_int_clear();
    
    cc1100_cmd_wor();
    
    state = STATE_WOR;
    
    return 0;
}

static uint16_t try_send(void) {
    //~ printf("try\n");
    if (state == STATE_WOR) {
        state = STATE_TX;
        
        // start RX
        cc1100_cmd_idle();
        cc1100_cmd_rx();
        
        // set a timer to check if the channel is free, the time to have a valid RSSI
        timerB_set_alarm_from_now(ALARM_PREAMBLE, SEND_PERIOD, 0);
        timerB_register_cb(TIMERB_ALARM_CCR0, medium_clear);
        cc1100_cfg_rc_pd(CC1100_RC_OSC_DISABLE);
        
        // change GDO to SyncWord, and its callback
        cc1100_gdo0_int_disable();
        cc1100_gdo0_int_set_rising_edge();
        cc1100_gdo0_int_clear();
        cc1100_gdo0_int_enable();
        cc1100_gdo0_register_callback(medium_busy);
    } else {
        if (cc1100_status_marcstate() == 0x01) {
            // this is an error situation
            set_wor();
            printf("try_send, bad state for sending (%u)\n", state);
        }
        return delay_send();
    }
    return 0;
}

static uint16_t delay_send(void) {
    //~ printf("delay\n");
    if (delay_count >= MAX_DELAY_COUNT) {
        // abort
        frame_to_send = 0;
        if (error_cb) 
            return error_cb();
    } else {
        // set a long random delay to send the packet
        uint16_t delay;
        delay = rand();
        delay &= 0x7FFF; // 0<=delay<32768
        delay += 1638; // 1638<=delay<=34406 (50ms <= delay < 1.05s)
        timerB_set_alarm_from_now(ALARM_RETRY, delay, 0);
        timerB_register_cb(ALARM_RETRY, try_send);
        
        delay_count++;
    }
    return 0;
}

static uint16_t medium_clear(void) {
    // the timer elapsed, thus the channel is clear
    cc1100_cmd_idle();
    cc1100_cfg_fs_autocal(CC1100_AUTOCAL_NEVER);
    cc1100_cfg_txoff_mode(CC1100_TXOFF_MODE_RX);
    cc1100_cfg_cca_mode(CC1100_CCA_MODE_ALWAYS);
    
    // change GDO to EoP, and its callback
    cc1100_gdo0_int_disable();
    cc1100_gdo0_int_set_falling_edge();
    cc1100_gdo0_int_clear();
    cc1100_gdo0_int_enable();
    cc1100_gdo0_register_callback(read_ack);
    
    // remove irq in case it happened
    cc1100_gdo0_int_clear();
    
    // prepare frame
    frame.length = HEADER_LENGTH;
    frame.type = TYPE_PREAMBLE;
    frame.dst_addr[0] = txframe.dst_addr[0];
    frame.dst_addr[1] = txframe.dst_addr[1];
    frame.src_addr[0] = node_addr>>8;
    frame.src_addr[1] = node_addr & 0xFF;
    
    // set periodic alarm for sending preambles
    timerB_set_alarm_from_now(ALARM_PREAMBLE, SEND_PERIOD, SEND_PERIOD);
    timerB_register_cb(ALARM_PREAMBLE, send_preamble);
    preamble_count = 0;
    
    send_preamble();
        
    return 0;
}

static uint16_t send_preamble(void) {
    preamble_count ++;
    
    if (preamble_count >= MAX_PREAMBLE_COUNT) {
        send_data();
        return 0;
    }
    cc1100_cmd_idle();
    cc1100_cmd_flush_rx();
    cc1100_cmd_flush_tx();
    cc1100_cmd_tx();
    
    cc1100_fifo_put((uint8_t*)(&frame.length), frame.length+1);
    return 0;
}

static uint16_t read_ack(void) {
    static struct {
        uint8_t length, type, dst_addr[2], src_addr[2], status[2];
    } ack;
    
    // check if radio state is idle
    if ( (cc1100_status() & 0x70) != 0x0) {
        // if not this means a preamble has been sent
        return 0;
    }
    /* Check CRC */
    if ( !(cc1100_status_crc_lqi() & 0x80) ) {
        // CRC error, abort
        return 0;
    }
    
    // we got a frame
    // Check Length is correct
    cc1100_fifo_get( (uint8_t*) &(ack.length), 1);
    if (ack.length != HEADER_LENGTH) {
        // length doesn't match the frame
        return 0;
    }
    
    // Get Frame bytes and status
    cc1100_fifo_get( (uint8_t*) &(ack.length)+1, ack.length+2);
    
    // check type
    if (ack.type != TYPE_ACK) {
        // type error
        return 0;
    }
    
    // Check address
    if ( (ack.dst_addr[0] != (node_addr>>8)) || (ack.dst_addr[1] != (node_addr&0xFF)) ||
        (ack.src_addr[0] != txframe.dst_addr[0]) || (ack.src_addr[1] != txframe.dst_addr[1]) ) {
        // addresses don't match
        return 0;
    }
    
    // everything's good, send data
    send_data();
    return 0;
}

static uint16_t send_data(void) {
    // unset alarm
    timerB_unset_alarm(ALARM_PREAMBLE);
    cc1100_cmd_idle();
    cc1100_cmd_flush_rx();
    cc1100_cmd_flush_tx();
    
    cc1100_cfg_txoff_mode(CC1100_TXOFF_MODE_IDLE);
    
    cc1100_cmd_tx();
    cc1100_fifo_put((uint8_t*)(&txframe.length), txframe.length+1);
    cc1100_gdo0_register_callback(send_done);
    return 0;
}

static uint16_t send_done(void) {
    //~ printf("sent\n");
    // data has been sent
    // if broadcast, don't wait for ACK
    if ((txframe.dst_addr[0]==0xFF) && (txframe.dst_addr[1])==0xFF) {
        //~ printf("send_done, after %u preambles\n", preamble_count);
        frame_to_send = 0;
        set_wor();
        if (sent_cb) return sent_cb();
    } else {
        // we need an ACK
        // set RX
        cc1100_cmd_idle();
        cc1100_cmd_flush_rx();
        cc1100_cmd_rx();
            
        // change GDO to EoP, and its callback
        cc1100_gdo0_int_disable();
        cc1100_gdo0_int_set_falling_edge();
        cc1100_gdo0_int_clear();
        cc1100_gdo0_int_enable();
        cc1100_gdo0_register_callback(read_dataack);
    
        // set a timer alarm in case we never get an ACK
        timerB_set_alarm_from_now(ALARM_TIMEOUT, ACK_TIMEOUT, 0);
        timerB_register_cb(ALARM_TIMEOUT, ack_timeout);
        
    }
    
    
    return 0;
}

static uint16_t read_dataack(void) {
    //~ printf("ack\n");
    static struct {
        uint8_t length, type, dst_addr[2], src_addr[2];
    } ack;
    
    // Check CRC
    if ( !(cc1100_status_crc_lqi() & 0x80) ) {
        // CRC error, abort
        cc1100_cmd_flush_rx();
        cc1100_cmd_rx();
        return 0;
    }
    
    // we got a frame
    // Check Length is correct
    cc1100_fifo_get( (uint8_t*) &(ack.length), 1);
    if (ack.length != HEADER_LENGTH) {
        // length doesn't match the frame
        cc1100_cmd_flush_rx();
        cc1100_cmd_rx();
        return 0;
    }
    
    // Get Frame bytes
    cc1100_fifo_get( (uint8_t*) &(ack.length)+1, ack.length);
    
    // check type
    if (ack.type != TYPE_DATAACK) {
        // type error
        cc1100_cmd_flush_rx();
        cc1100_cmd_rx();
        return 0;
    }
    
    // Check address
    if ( (ack.dst_addr[0] != (node_addr>>8)) || (ack.dst_addr[1] != (node_addr&0xFF)) ||
        (ack.src_addr[0] != txframe.dst_addr[0]) || (ack.src_addr[1] != txframe.dst_addr[1]) ) {
        // addresses don't match
        cc1100_cmd_flush_rx();
        cc1100_cmd_rx();
        return 0;
    }
    
    // everything's good, send is really done
    timerB_unset_alarm(ALARM_TIMEOUT);
    set_wor();
    frame_to_send = 0;
    if (sent_cb) return sent_cb();
    
    return 0;
}

static uint16_t ack_timeout(void) {
    set_wor();
    delay_send();
    return 0;
}

static uint16_t medium_busy(void) {
    // stop the timer
    timerB_unset_alarm(ALARM_PREAMBLE);
    
    // change GDO to EOP, and its callback
    cc1100_gdo0_int_disable();
    cc1100_gdo0_int_set_falling_edge();
    cc1100_gdo0_int_clear();
    cc1100_gdo0_int_enable();
    cc1100_gdo0_register_callback(read_frame);
    
    // check if frame received
    if ( !cc1100_gdo0_read() ) {
        cc1100_gdo0_int_clear();
        read_frame();
    }
    
    // set a delay for a retry
    delay_send();
    
    return 0;
}

/*------------------------RX--------------------------*/

static uint16_t read_frame(void) {
    uint16_t src, dst;
    uint8_t status[2];
    int16_t rssi;
    
    state = STATE_RX;
    
    // deactivate timeout alarm
    timerB_unset_alarm(ALARM_TIMEOUT);
    
    // Check CRC
    if ( !(cc1100_status_crc_lqi() & 0x80) ) {
        // CRC error, abort
        //~ printf("read_frame, crc error\n");
        set_wor();
        return 0;
    }
    
    // Check Length is correct
    cc1100_fifo_get( (uint8_t*) &(frame.length), 1);
    if (frame.length < HEADER_LENGTH || frame.length > PACKET_LENGTH_MAX) {
        // length error
        //~ printf("read_frame, length error\n");
        set_wor();
        return 0;
    }
    
    // Get Frame
    cc1100_fifo_get( (uint8_t*) &(frame.length)+1, frame.length);
    
    // Get Status
    cc1100_fifo_get( (uint8_t*) status, 2);
    
    // Compute addresses
    dst = (((uint16_t)frame.dst_addr[0])<<8) + frame.dst_addr[1];
    src = (((uint16_t)frame.src_addr[0])<<8) + frame.src_addr[1];
    
    // Check Frame Type
    if (frame.type == TYPE_PREAMBLE) {
        // preamble
        if ( dst == node_addr) {
            // for me !
            // Prepare ACK Frame
            frame.length = HEADER_LENGTH;
            frame.type = TYPE_ACK;
            frame.dst_addr[0] = frame.src_addr[0];
            frame.dst_addr[1] = frame.src_addr[1];
            frame.src_addr[0] = node_addr>>8;
            frame.src_addr[1] = node_addr & 0xFF;
            
            // configure auto switch
            cc1100_cfg_txoff_mode(CC1100_TXOFF_MODE_RX);
            cc1100_cfg_cca_mode(CC1100_CCA_MODE_ALWAYS);
            cc1100_cfg_rc_pd(CC1100_RC_OSC_DISABLE);
            cc1100_cfg_fs_autocal(CC1100_AUTOCAL_NEVER);
            cc1100_cmd_flush_rx();
            cc1100_cmd_flush_tx();
            
            // update callback
            cc1100_gdo0_register_callback(ack_sent);
            
            // Start TX
            cc1100_cmd_tx();
            
            // Put frame in TX FIFO
            cc1100_fifo_put((uint8_t*)&frame.length, frame.length+1);
            
            return 0;
            
        } else if (dst == 0xFFFF) {
            // for broadcast
            // we keep listening until we get data
            cc1100_cmd_flush_rx();
            cc1100_cmd_flush_tx();
            cc1100_cmd_rx();
            
            // set a timer alarm in case we never get a data frame
            timerB_set_alarm_from_now(ALARM_TIMEOUT, SEND_PERIOD*MAX_PREAMBLE_COUNT, 0);
            timerB_register_cb(ALARM_TIMEOUT, set_wor);
            return 0;
        } else {
            // not for me
            set_wor();
            return 0;
        }
        
    } else if (frame.type == TYPE_DATA) {
        // data
        int len = frame.length-HEADER_LENGTH;
        
        // if for me, send ACK
        if (dst == node_addr) {
            // send DATAACK
            frame.length = HEADER_LENGTH;
            frame.type = TYPE_DATAACK;
            frame.dst_addr[0] = frame.src_addr[0];
            frame.dst_addr[1] = frame.src_addr[1];
            frame.src_addr[0] = node_addr>>8;
            frame.src_addr[1] = node_addr & 0xFF;
            
            // update callback
            cc1100_gdo0_register_callback(set_wor);
            
            // Start TX
            cc1100_cmd_tx();
            
            // Put frame in TX FIFO
            cc1100_fifo_put((uint8_t*)&frame.length, frame.length+1);
        } else {
            set_wor();
        }
        
        if (dst == node_addr || dst == 0xFFFF) {
            // for me, call handler
            if (received_cb) {
                rssi = status[0] >= 128 ? status[0]-256 : status[0];
                rssi -= 140;
                return received_cb(frame.payload, len, src, rssi);
            }
        }
    } else {
        // type error
        //~ printf("read_frame, type error (%.2x)\n", frame.type);
        set_wor();
        return 0;
    }
    
    return 0;
}

static uint16_t ack_sent(void) {
    // now we wait for data
    cc1100_gdo0_register_callback(read_frame);
    
    // set a timer alarm in case we never get a data frame
    timerB_set_alarm_from_now(ALARM_TIMEOUT, SEND_PERIOD*4, 0);
    timerB_register_cb(ALARM_TIMEOUT, set_wor);
    return 0;
}

