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
#include "cc2420.h"
#include "ds2411.h"
#include "timerB.h"

#define DELAY_COUNT_MAX 6
#define ALARM_RETRY TIMERB_ALARM_CCR0
#define ACK_TIMEOUT 131 // 4ms

#if 0
    #define PRINTF(...) printf(__VA_ARGS__)
#else
    #define PRINTF(...) 
#endif

#define PAYLOAD_LENGTH_MAX 100
#define EMPTY_FRAME_LENGTH 7
#define ACK_FRAME_LENGTH 7

#define TYPE_DATA 0xAA
#define TYPE_ACK  0xBB

typedef struct {
    uint8_t length;
    uint8_t type;
    uint8_t dst_addr[2];
    uint8_t src_addr[2];
    uint8_t payload[PAYLOAD_LENGTH_MAX];
    uint8_t fcf[2];
} frame_t;

typedef struct {
    uint8_t length;
    uint8_t type;
    uint8_t dst_addr[2];
    uint8_t src_addr[2];
    uint8_t fcf[2];
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
    
    // initialize the radio, set the correct channel
    cc2420_init();
    cc2420_set_frequency(2405+5*channel);
    
    // configure irq
    cc2420_io_sfd_int_set_falling();
    cc2420_io_sfd_int_clear();
    cc2420_io_sfd_int_enable();
    cc2420_io_sfd_register_cb(rx_parse);
    
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
    if (length>PAYLOAD_LENGTH_MAX) {
        return 2;
    }
    
    // check state
    if (txframe.length != 0) {
        // already sending, can't do anything
        return 1;
    }
    // prepare header
    txframe.length = length + EMPTY_FRAME_LENGTH;
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
    cc2420_cmd_idle();
    cc2420_cmd_xoscoff();
    
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
	// flush everything
	cc2420_cmd_idle();
	cc2420_cmd_flushrx();
	cc2420_cmd_flushtx();
	cc2420_io_sfd_int_clear();
	cc2420_cmd_rx();
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
        cc2420_io_sfd_register_cb(rx_parse);
        
        // reset rx
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
    if (txframe.length == 0) {
        return rx_set();
    }
    
    // try to send
    cc2420_fifo_put((uint8_t*)&txframe, txframe.length+1);
    cc2420_cmd_txoncca();
    
    // wait a little bit
    micro_delay(1000);
    
    // if status is not RX
    if ( cc2420_io_sfd_read()) {
        // tx started
        cc2420_io_sfd_register_cb(tx_done);
    } else {
		// retry next time
        tx_delay();
        rx_set();
    }
    
    return 0;
}

static uint16_t tx_done(void) {
    // if destination is broadcast, don't wait for ACK
    if ((txframe.dst_addr[0]==0xFF) && (txframe.dst_addr[1]==0xFF)) {
        cc2420_io_sfd_register_cb(rx_parse);
        txframe.length = 0;
        rx_set();
        if (sent_cb) {
            return sent_cb();
        }
    } else {
        cc2420_io_sfd_register_cb(tx_ack);
        timerB_set_alarm_from_now(ALARM_RETRY, ACK_TIMEOUT, 0);
        timerB_register_cb(ALARM_RETRY, tx_delay);
    }
    return 0;
}

static uint16_t tx_ack(void) {
    uint16_t dst;
    
    /* Check if there a complete packet FIFO */
    if (!(cc2420_io_fifo_read() && cc2420_io_fifop_read())) {
        return rx_set();
    }
    
    /* Check Length is correct */
    cc2420_fifo_get( (uint8_t*) &(ack.length), 1);
    
    // if too big, flush
    if ( ack.length != ACK_FRAME_LENGTH ) {
        return rx_set();
    }
    
    /* Get Data */
    cc2420_fifo_get( (uint8_t*) &(ack.length)+1, ack.length);
    
    /* Check CRC */
    if ( (ack.fcf[1] & 0x80) == 0 ) {
        return rx_set();
    }
    
    /* Compute addresses */
    dst = (((uint16_t)ack.dst_addr[0])<<8) + ack.dst_addr[1];
    
    /* Check addresses */
    if ( (dst==node_addr) && (ack.src_addr[0]==txframe.dst_addr[0]) \
                           && (ack.src_addr[1]==txframe.dst_addr[1]) ) {
        txframe.length = 0;
        timerB_unset_alarm(ALARM_RETRY);
        cc2420_io_sfd_register_cb(rx_parse);
        rx_set();
        if (sent_cb) {
            return sent_cb();
        }
    }
    return rx_set();
}

static uint16_t rx_parse(void) {
    uint8_t* status;
    uint16_t src, dst;
    uint16_t ret_val;
    int16_t rssi;
    
    /* Check if there a complete packet FIFO */
    if (!(cc2420_io_fifo_read() && cc2420_io_fifop_read())) {
        return rx_set();
    }
    
    /* Check Length is correct */
    cc2420_fifo_get( (uint8_t*) &(rxframe.length), 1);
    
    // if too big, flush
    if ( rxframe.length > sizeof(rxframe)-1 ) {
        return rx_set();
    }
    
    /* Get Data */
    cc2420_fifo_get( (&rxframe.length)+1, rxframe.length);
    status = &rxframe.length + rxframe.length -1;
    
    /* Check CRC */
    if ( (status[1] & 0x80) == 0 ) {
        return rx_set();
    }
    
    /* Check for min length */
    if ( rxframe.length < (sizeof(ack_t)-1) ) {
        return rx_set();
    }
    
    /* Compute addresses */
    dst = (((uint16_t)rxframe.dst_addr[0])<<8) + rxframe.dst_addr[1];
    src = (((uint16_t)rxframe.src_addr[0])<<8) + rxframe.src_addr[1];
    
    ret_val = 0;
    
    int len;
    len = rxframe.length - (sizeof(ack_t)-1);
    
    rssi = (int8_t) status[0];
    rssi -= 45;
    
    // if for me, send an ACK
    if (dst==node_addr) {
        ack.length = ACK_FRAME_LENGTH;
        ack.type = TYPE_ACK;
        ack.dst_addr[0] = rxframe.src_addr[0];
        ack.dst_addr[1] = rxframe.src_addr[1];
        ack.src_addr[0] = rxframe.dst_addr[0];
        ack.src_addr[1] = rxframe.dst_addr[1];
        cc2420_fifo_put((uint8_t*)&ack, ack.length+1);
        cc2420_cmd_tx();
        cc2420_io_sfd_register_cb(rx_ackdone);
        
        if (received_cb) {
            return received_cb(rxframe.payload, len, src, rssi);
        }
        
    } else if ( (dst==0xFFFF) && received_cb ) {
		rx_set();
		
        /* Call the packet received function */
        return received_cb(rxframe.payload, len, src, rssi);
    } else {
        rx_set();
        return 0;
    }
    return 0;
}

static uint16_t rx_ackdone(void) {
    cc2420_io_sfd_register_cb(rx_parse);
    return rx_set();
}
