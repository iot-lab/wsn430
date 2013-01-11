#include <io.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "flood.h"
#include "mac.h"
#include "timerB.h"

/* ----DEFINES---- */
#define HEADER_LENGTH 7
#define MAX_DATA_LEN  25
#define MAX_ROUTE_LEN 10
#define MAX_KNOWN_PACKETS 4
#define STATE_RX 0x1
#define STATE_TX 0x2
#define ALARM_TXDELAY TIMERB_ALARM_CCR2


/* ----STRUCTURES---- */
typedef struct {
    uint8_t dst_addr[2]; // network destination
    uint8_t src_addr[2]; // network source
    uint8_t id; //  source packet id
    uint8_t data_len; // length of the data
    uint8_t route_len; // length of the route
    uint8_t data[45];
} packet_t;

typedef struct {
    uint8_t src_addr[2],
            id;
} packet_id_t;

/* ----PROTOTYPES---- */
static uint16_t frame_received(uint8_t packet[], uint16_t length, uint16_t src_addr, int16_t rssi);
static void delay_packet(void);
static uint16_t send(void);
static inline int is_packet_known(packet_t* pkt);


/* ----DATA---- */
static net_handler_t rx_cb;
static packet_t tx_pkt;
static uint16_t tx_length;
static uint16_t state;
static packet_id_t known_packets[MAX_KNOWN_PACKETS];
static uint8_t known_packet_id = 0;
static uint8_t my_packet_id = 0;

void net_init() {
    int i;
    
    //  initialize MAC layer, and timerB
    mac_init(4);
    
    // register mac callback
    mac_set_rx_cb(frame_received);
    
    // init callback
    rx_cb = 0x0;
    
    state = STATE_RX;
    
    for (i=0; i<MAX_KNOWN_PACKETS; i++) {
        known_packets[i].src_addr[0] = 0;
        known_packets[i].src_addr[1] = 0;
        known_packets[i].id = 0;
    }
}

uint16_t net_send(uint8_t packet[], uint16_t length, uint16_t dst_addr) {
    uint8_t* route;
    if (state == STATE_TX) {
        printf("net_send state error\n");
        return 0;
    }
    
    if (length > MAX_DATA_LEN) {
        printf("net_send length error\n");
        return 0;
    }
    state = STATE_TX;
    
    tx_pkt.dst_addr[0] = dst_addr>>8;
    tx_pkt.dst_addr[1] = dst_addr&0xFF;
    tx_pkt.src_addr[0] = node_addr>>8;
    tx_pkt.src_addr[1] = node_addr&0xFF;
    tx_pkt.id = my_packet_id++;
    
    tx_pkt.data_len = length;
    tx_pkt.route_len = 1;
    memcpy(tx_pkt.data, packet, length);
    
    route = tx_pkt.data + length;
    route[0] = node_addr>>8;
    route[1] = node_addr&0xFF;
    
    tx_length = HEADER_LENGTH + length + 2;
    
    // put packet to known packets
    known_packets[known_packet_id].src_addr[0] = tx_pkt.src_addr[0];
    known_packets[known_packet_id].src_addr[1] = tx_pkt.src_addr[1];
    known_packets[known_packet_id].id = tx_pkt.id;
    known_packet_id += 1;
    known_packet_id %= MAX_KNOWN_PACKETS;
    
    delay_packet();
    
    return 1;
}

void net_register_rx_cb(net_handler_t cb) {
    rx_cb = cb;
}

void net_stop() {
    mac_stop();
}

/* ----PRIVATE FUNCTIONS---- */
static uint16_t frame_received(uint8_t packet[], uint16_t length, uint16_t src_addr, int16_t rssi) {
    packet_t *rx_pkt;
    uint16_t dst;
    uint16_t ret_val = 0;
    
    // check min length
    if (length < HEADER_LENGTH) {
        return 0;
    }
    
    // cast the received packet
    rx_pkt = (packet_t*) packet;
    
    // ckeck the length
    if ( (HEADER_LENGTH + (rx_pkt->data_len) + (rx_pkt->route_len*2)) != length ) {
        return 0;
    }
    
    // if packet known, abort
    if (is_packet_known(rx_pkt)) {
        return 0;
    }
    
    
    // put packet to known packets
    known_packets[known_packet_id].src_addr[0] = rx_pkt->src_addr[0];
    known_packets[known_packet_id].src_addr[1] = rx_pkt->src_addr[1];
    known_packets[known_packet_id].id = rx_pkt->id;
    known_packet_id += 1;
    known_packet_id %= MAX_KNOWN_PACKETS;
    
    // check the destination, if for me call the callback
    dst = (rx_pkt->dst_addr[0]<<8) + (rx_pkt->dst_addr[1]);
    
    if ( (dst == node_addr || dst == MAC_BROADCAST) && rx_cb) {
        uint16_t src;
        src = (rx_pkt->src_addr[0]<<8) + (rx_pkt->src_addr[1]);
        ret_val = rx_cb(rx_pkt->data, rx_pkt->data_len, src);
        
        int i;
        uint8_t *route;
        uint16_t hop;
        route = rx_pkt->data + rx_pkt->data_len;
        printf("route = ");
        for (i=0; i<rx_pkt->route_len; i++) {
            hop = (route[2*i]<<8) + (route[2*i+1]);
            printf("%.4x-", hop);
        }
        printf("%.4x\n", node_addr);
    }
    
    // check if it needs to be forwarded
    if ( dst == MAC_BROADCAST || dst != node_addr ) {
        uint8_t *route;
        
        // check if state is correct
        if (state == STATE_TX) {
            // already a packet being sent, abort
            return ret_val;
        }
        
        // check if there is room for one more hop
        if (rx_pkt->route_len >= MAX_ROUTE_LEN) {
            // too big, drop
            printf("Too many hops!\n");
            return ret_val;
        }
        
        // insert my address
        route = rx_pkt->data + rx_pkt->data_len;
        route[rx_pkt->route_len*2] = node_addr>>8;
        route[rx_pkt->route_len*2+1] = node_addr & 0xFF;
        rx_pkt->route_len+=1;
        length +=2;
        
        // copy packet to send in a local buffer
        memcpy(&tx_pkt, rx_pkt, length);
        tx_length = length;
        
        // start delay to send
        delay_packet();
    }
    
    return ret_val;
}

static void delay_packet(void) {
    uint16_t delay;
    delay = rand(); // 16383 ticks max (0.5s)
    delay &= 0x3FFF;
    delay += 1;
    
    timerB_set_alarm_from_now(ALARM_TXDELAY, delay, 0);
    timerB_register_cb(ALARM_TXDELAY, send);
}

static uint16_t send(void) {
    if ( mac_send((uint8_t*)&tx_pkt, tx_length, MAC_BROADCAST) ) {
        delay_packet();
    } else {
        state = STATE_RX;
    }
    
    return 0;
}

static inline int is_packet_known(packet_t *pkt) {
    int i;
    for (i=0; i<MAX_KNOWN_PACKETS; i++)
        if ( (pkt->src_addr[0] == known_packets[i].src_addr[0]) &&
             (pkt->src_addr[1] == known_packets[i].src_addr[1]) &&
             (pkt->id == known_packets[i].id))
            return 1;
    return 0;
}
