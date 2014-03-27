#include <io.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "source.h"
#include "mac.h"
#include "timerB.h"

/* ----DEFINES---- */
#define HEADER_LENGTH (sizeof(data_t)-(MAX_DATA_LEN+2*MAX_ROUTE_LEN))
#define MAX_DATA_LEN  25
#define MAX_ROUTE_LEN 10

#define STATE_RX    0x1 // waiting for data
#define STATE_TX    0x3 // sending data

#define TYPE_SOURCE_DATA 0x11

#define ALARM_DATADELAY TIMERB_ALARM_CCR2
#define ALARM_1MS 33

#define MAX_DATA_TRY 4

/* ----STRUCTURES---- */
typedef struct {
    uint8_t type;
    uint8_t dst_addr[2]; // network destination
    uint8_t src_addr[2]; // network source
    uint8_t id; // source packet id
    uint8_t payload_len; // length of the data
    uint8_t route_len; // length of the route
    uint8_t payload[MAX_DATA_LEN + 2*MAX_ROUTE_LEN];
} data_t;

/* ----PROTOTYPES---- */
static uint16_t data_received(uint8_t packet[], uint16_t length, uint16_t src_addr, int16_t rssi);
static uint16_t delay_data(void);
static uint16_t send_data(void);

static inline void reset_all(void);
static inline int16_t find_pos_in_route(uint8_t *route, uint16_t route_len);

/* ----DATA---- */
static net_handler_t rx_cb;
static data_t data;
static uint16_t data_length, data_addr;
static uint8_t *data_route;
static uint16_t data_try_count;
static uint16_t state;
static uint16_t packet_id;


static inline void reset_all(void) {
    state = STATE_RX;
    data_length = 0;
    mac_set_rx_cb(data_received);
}

void net_init() {
    //  initialize MAC layer, and timerB
    mac_init(5);

    // init variables
    rx_cb = 0x0;
    packet_id = 0;

    reset_all();
}

uint16_t net_send(uint8_t packet[], uint16_t length, uint16_t route[], uint16_t route_len) {
    uint16_t i;

    if (state != STATE_RX) {
        printf("net_send state not RX\n");
        return 0;
    }

    if (length > MAX_DATA_LEN) {
        printf("net_send length error\n");
        return 0;
    }

    if ((route_len==0) || (route_len > MAX_ROUTE_LEN)) {
        printf("net_send route length error\n");
        return 0;
    }

    /// prepare packet
    // set state
    state = STATE_TX;

    // set type
    data.type = TYPE_SOURCE_DATA;

    // set destination address
    data.dst_addr[0] = route[route_len-1]>>8;
    data.dst_addr[1] = route[route_len-1]&0xFF;
    data.src_addr[0] = node_addr>>8;
    data.src_addr[1] = node_addr&0xFF;
    data.id = packet_id++;

    // copy data
    data.payload_len = length;
    memcpy(data.payload, packet, length);

    // copy route (remove destination from route)
    data.route_len = route_len-1;
    data_route = data.payload + length;

    for (i=0; i<data.route_len; i++) {
        data_route[2*i]    = route[i] >> 8;
        data_route[2*i +1] = route[i] & 0xFF;
    }

    // global length
    data_length = HEADER_LENGTH + length + 2*data.route_len;

    // next hop
    data_addr = route[0];

    // send packet (a little later)
    data_try_count = 0;
    delay_data();

    printf("txpkt");
    for (i=0;i<data_length;i++) {
        printf(":%x", ((uint8_t*)&data)[i]);
    }
    printf("\n");
    return 1;
}

void net_register_rx_cb(net_handler_t cb) {
    rx_cb = cb;
}

void net_stop() {
    mac_stop();
}

/* ----PRIVATE FUNCTIONS---- */

static uint16_t delay_data(void) {
    if (data_try_count >= MAX_DATA_TRY) {
        printf("data send aborted\n");
        reset_all();
        return 0;
    }

    if (data_try_count == 0) {
        timerB_set_alarm_from_now(ALARM_DATADELAY, 2, 0);
    } else {
        uint16_t delay;
        delay = rand();
        //~ delay &= 0x3FFF; // 16383 ticks max (0.5s)
        delay &= 0x1FFF; // 8k ticks max (0.25s)
        delay += ALARM_1MS*10; // 10ms min

        timerB_set_alarm_from_now(ALARM_DATADELAY, delay, 0);
    }

    timerB_register_cb(ALARM_DATADELAY, send_data);
    data_try_count ++;
    return 0;
}

static uint16_t send_data(void) {
    if ( mac_send((uint8_t*)&data, data_length, data_addr) ) {
        delay_data();
    } else {
        state = STATE_RX;
    }

    return 0;
}


static uint16_t data_received(uint8_t packet[], uint16_t length, uint16_t src_addr, int16_t rssi) {
    data_t *rx_data;
    uint16_t dst;
    uint8_t *route;

    // check min length
    if (length < HEADER_LENGTH) {
        printf("data_frame_received packet to small\n");
        return 0;
    }

    // cast the received packet
    rx_data = (data_t*) packet;

    // check the type
    if (rx_data->type != TYPE_SOURCE_DATA) {
        printf("data_frame_received not DATA type\n");
        return 0;
    }

    // ckeck the length
    if ( (HEADER_LENGTH + (rx_data->payload_len) + (rx_data->route_len*2)) != length ) {
        printf("data_frame_received length doesn't match\n");
        return 0;
    }

    // set the route pointer
    route = rx_data->payload + rx_data->payload_len;

    // copy packet
    memcpy(&data, rx_data, length);
    data_route = data.payload + data.payload_len;

    // check the destination, if for me call the callback
    dst = (rx_data->dst_addr[0]<<8) + (rx_data->dst_addr[1]);

    // if for me, if for me call the callback
    if ( (dst == node_addr) && rx_cb) {
        uint16_t src;
        src = (data_route[0]<<8) + (data_route[1]);

        // clear the data indicator
        data_length = 0;

        return rx_cb(data.payload, data.payload_len, src);
    } else {
        // otherwise, forward
        int16_t pos;
        pos = find_pos_in_route(route, rx_data->route_len);

        if (pos<0) {
            // next hop not found
            printf("data_frame_received fw next hop not found\n");
            // clear the data indicator
            data_length = 0;
            return 0;
        } else if (pos==rx_data->route_len-1){
            data_addr = (((uint16_t)rx_data->dst_addr[0])<<8) + rx_data->dst_addr[1];
        } else {
            data_addr = (((uint16_t)route[2*pos])<<8) + route[2*pos+1];
            data_length = length;
        }

        // forward
        data_length = length;
        data_try_count = 1;
        delay_data();
        state = STATE_TX;
        printf("fw\n");
    }
    return 0;
}

static inline int16_t find_pos_in_route(uint8_t *route, uint16_t route_len) {
    uint8_t* p;
    for (p=route+2*(route_len-1); p-route>=0; p-=2) {
        if ((p[0]<<8)+p[1] == node_addr) {
            return (p-route)/2;
        }
    }
    return -1;
}
