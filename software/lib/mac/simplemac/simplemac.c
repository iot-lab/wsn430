/**
 * \file simplemac.c
 * \brief A complete implementation of a very simple MAC layer.
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
#include "mac.h"
#include "simplephy.h"
#include "ds2411.h"
#include "timerB.h"
#include "leds.h"

#define DATA_FRAME        0xAA

#define STATE_RX               0x0
#define STATE_TX_WAIT_EOP      0x1
#define STATE_TX_WAIT_EOP_TX   0x2
#define STATE_RX_WAIT_BACKOFF  0x3

typedef union
{
    uint8_t raw[255];
    struct
    {
        uint8_t src_addr;
        uint8_t dst_addr;
        uint8_t type;
        uint8_t payload[252];
    };
} mac_frame_t;

// node's MAC address
static uint8_t local_addr;

// callback for received packets
static mac_handler_t mac_cb;

// frame to send
static mac_frame_t txframe;
static uint16_t txframe_length;

// internal state
static uint16_t mac_state;

// prototypes
static void frame_sent(void);
static void frame_received(uint8_t frame[], uint16_t length);
static uint16_t try_sending(void);

void mac_init(void)
{
    // initialize the unique serial number chip and set node address accordingly
    ds2411_init();
    local_addr = ds2411_id.serial0;
    
    // seed the random number generator
    uint16_t seed;
    seed = ( ((uint16_t)ds2411_id.serial0) << 8) + (uint16_t)ds2411_id.serial1;
    srand(seed);
    
    // initialize the phy layer
    phy_init();
    phy_register_frame_sent_notifier(frame_sent);
    phy_register_frame_received_handler(frame_received);
    
    // clean callback
    mac_cb = 0x0;
    
    // initialize the timerB
    timerB_init();
    timerB_register_cb(TIMERB_ALARM_CCR0, try_sending);
    
    phy_start_rx();
    
    mac_state = STATE_RX;
}
   
void mac_register_rx_cb(mac_handler_t cb)
{
    mac_cb = cb;
}
 
uint16_t mac_send(uint8_t packet[], uint16_t length, uint8_t dst_addr)
{
    uint16_t i;
    
    if (length>252)
    {
        return -1;
    }
    
    // copy packet to local frame buffer
    txframe.src_addr = local_addr;
    txframe.dst_addr = BROADCAST_ADDR;
    txframe.type = DATA_FRAME;
    txframe_length = length + 3;
    
    for (i=0;i<length;i++)
    {
        txframe.payload[i] = packet[i];
    }
    switch (mac_state)
    {
        case STATE_RX:
            try_sending();
            break;
        case STATE_RX_WAIT_BACKOFF:
            break;
        case STATE_TX_WAIT_EOP:
            mac_state = STATE_TX_WAIT_EOP_TX;
            break;
        case STATE_TX_WAIT_EOP_TX:
            break;
    }
    return 0;
}

/**
 * Local function used to try to send a packet.
 * It does a CCA, and send or go back to rx depending on the result.
 */
static uint16_t try_sending(void)
{
    // do CCA
    if (phy_cca())
    {
        // channel is clear, send.
        phy_send_frame(txframe.raw, txframe_length);
        mac_state = STATE_TX_WAIT_EOP;
    }
    else
    {
        // channel is busy, pick a backoff
        timerB_start_SMCLK_div(TIMERB_DIV_8);
        uint32_t ticks;
        ticks  = rand() * 50000;
        ticks >>= 16;
        ticks += 10000;
        timerB_set_alarm_from_now(TIMERB_ALARM_CCR0, (uint16_t)ticks, 0);
        mac_state = STATE_RX_WAIT_BACKOFF;
    }
    return 0;
}

/**
 * Callback function passed to the PHY module, that is called when
 * a frame has been sent, indicating normal radio activity may be
 * continued.
 */
static void frame_sent(void)
{
    phy_start_rx();
    switch (mac_state)
    {
        case STATE_TX_WAIT_EOP:
            mac_state = STATE_RX;
            break;
        case STATE_TX_WAIT_EOP_TX:
            try_sending();
            break;
    }
}

/**
 * Callback function passed to the PHY module, that is called when
 * a frame has been received. The frame header is parsed, and the
 * payload is passed to the user application if a callback has been
 * registered.
 */
static void frame_received(uint8_t frame[], uint16_t length)
{
    mac_frame_t *rx_frame;
    rx_frame = (mac_frame_t*) frame;
    
    phy_start_rx();
    
    if (rx_frame->type == DATA_FRAME)
    {
        if (mac_cb != 0x0)
        {
            (*mac_cb)(rx_frame->payload, length-3, rx_frame->src_addr);
        }
    }
}
