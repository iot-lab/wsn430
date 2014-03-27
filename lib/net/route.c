#include <io.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "route.h"
#include "mac.h"
#include "timerB.h"

/* ----DEFINES---- */
#define MAX_DATA_LEN  25
#define MAX_ROUTE_LEN 10
#define HEADER_LENGTH (sizeof(data_t)-(MAX_DATA_LEN+2*MAX_ROUTE_LEN))

#define TYPE_SOURCE_DATA 0x11
#define TYPE_FLOOD_DATA  0x22

#define ALARM_DELAY TIMERB_ALARM_CCR2
#define ALARM_1MS 33

#define ROUTE_NUMBER 3
#define KNOWN_PACKET_NUMBER 4

#define ADDR_FROM_BYTES(a) (((a)[0]<<8)+((a)[1]))
#define INSERT_ADDR_AT(addr, at) (at)[0]=(addr)>>8;(at)[1]=(addr)&0xFF
#define ADDR_COPY(dst, src) (dst)[0]=(src)[0];(dst)[1]=(src)[1]

#define PRINT_PACKET(p) do { \
printf("t=%x, dst=%.2x%.2x, src=%.2x%.2x, id=%u\n", \
        (p)->type, (p)->dst_addr[0], (p)->dst_addr[1], (p)->src_addr[0], (p)->src_addr[1], (p)->id); \
printf("pay[%u]", (p)->payload_len); \
int i; \
for (i=0;i<(p)->payload_len;i++)printf(":%x", (p)->payload[i]);\
printf("\nroute[%u]=", (p)->route_len);\
for (i=0;i<2*(p)->route_len;i++) printf(":%x", (p)->payload[(p)->payload_len+i]);\
printf("\n");\
} while (0)


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

typedef struct {
    uint8_t number;
    uint8_t hops[2*(MAX_ROUTE_LEN+2)];
} route_t;

typedef struct {
    uint8_t src_addr[2],
            id;
} packet_id_t;

/* ----PROTOTYPES---- */
static uint16_t data_received(uint8_t packet[], uint16_t length, uint16_t src_addr, int16_t rssi);
static uint16_t send_data(void);
static uint16_t rx_flood_handle(void);
static uint16_t rx_source_handle(void);

static inline int16_t set_route_to_host(uint16_t addr);
static inline int16_t find_pos_in_route(uint16_t addr, uint8_t *route, uint16_t route_len);
static inline int16_t is_packet_known(data_t* pkt);
static inline void store_route(void);


/* ----DATA---- */
static net_handler_t rx_cb;
static data_t data;
static uint16_t data_length, data_addr;
static uint8_t *data_route;
static uint16_t packet_id;
static packet_id_t known_packets[KNOWN_PACKET_NUMBER];
static uint16_t known_packet_id = 0;
static route_t known_routes[ROUTE_NUMBER];
static uint16_t known_route_id = 0;

void net_init() {
    int16_t i;

    // initialize MAC layer, and timerB
    mac_init(6);

    // init variables
    rx_cb = 0x0;
    packet_id = 0;

    // init
    data_length = 0;
    mac_set_rx_cb(data_received);
    for (i=0; i<KNOWN_PACKET_NUMBER; i++) {
        known_packets[i].src_addr[0] = 0;
        known_packets[i].src_addr[1] = 0;
        known_packets[i].id = 0;
    }
    known_packet_id = 0;
    for (i=0; i<ROUTE_NUMBER; i++) {
        known_routes[i].number = 0;
    }

    known_route_id = 0;
}

uint16_t net_send(uint8_t packet[], uint16_t length, uint16_t dst_addr) {

    if (length > MAX_DATA_LEN) {
        printf("net_send length error\n");
        return 0;
    }

    if (data_length != 0) {
        printf("net_send, buffer busy error\n");
        return 0;
    }

    /// prepare packet
    data_length = HEADER_LENGTH + length;
    INSERT_ADDR_AT(dst_addr, data.dst_addr);
    INSERT_ADDR_AT(node_addr, data.src_addr);
    data.id = packet_id++;
    data.payload_len = length;
    memcpy(data.payload, packet, length);
    data_route = data.payload + data.payload_len;

    if (set_route_to_host(dst_addr)) {
        data.type = TYPE_SOURCE_DATA;
    } else {
        data.type = TYPE_FLOOD_DATA;
        data_addr = MAC_BROADCAST;
    }

    data_length = HEADER_LENGTH + length + 2*data.route_len;

    // send
    timerB_set_alarm_from_now(ALARM_DELAY, 2*ALARM_1MS, 0);
    timerB_register_cb(ALARM_DELAY, send_data);

    return 1;
}

void net_register_rx_cb(net_handler_t cb) {
    rx_cb = cb;
}

void net_stop() {
    mac_stop();
}

/* ----STANDARD PACKET HANDLING---- */

static uint16_t send_data(void) {
    if (data_addr && data_length) {
        if (mac_send((uint8_t*)&data, data_length, data_addr)) {}
    } else {
        printf("send_data, no data\n");
    }
    //~ printf("SENT:\n");
    //~ PRINT_PACKET(&data);

    // clear the buffer
    data_length = 0;
    return 0;
}


static uint16_t data_received(uint8_t packet[], uint16_t length, uint16_t src_addr, int16_t rssi) {
    data_t *rx_data;

    // check min length
    if (length < HEADER_LENGTH) {
        printf("data_frame_received packet to small\n");
        return 0;
    }

    // cast the received packet
    rx_data = (data_t*) packet;

    //~ printf("RECEIVED, [%u]:\n", length);
    //~ PRINT_PACKET(rx_data);

    // ckeck the length
    if ( (HEADER_LENGTH + (rx_data->payload_len) + (rx_data->route_len*2)) != length ) {
        printf("data_frame_received length doesn't match\n");
        return 0;
    }

    if ( (rx_data->type != TYPE_FLOOD_DATA) && (rx_data->type != TYPE_SOURCE_DATA) ) {
        // unknown type, abort
        printf("data_frame_received type error (%x)\n", rx_data->type);
        return 0;
    }

    // if packet is mine or known, abort
    if ( (ADDR_FROM_BYTES(rx_data->src_addr) == node_addr) || is_packet_known(rx_data)) {
        return 0;
    }

    // packet is valid, copy packet
    memcpy(&data, rx_data, length);
    data_length = length;
    data_route = data.payload + data.payload_len;
    data_addr = 0x0;


    // check the type
    if (data.type==TYPE_SOURCE_DATA) {
        return rx_source_handle();
    } else {
        return rx_flood_handle();
    }

    return 0;
}

/*----FLOODING----*/
static uint16_t rx_flood_handle(void) {
    uint16_t dst;
    uint16_t ret_val=0;
    printf("flood ");
    // check the destination
    dst = ADDR_FROM_BYTES(data.dst_addr);

    if ( dst == node_addr || dst == NET_BROADCAST) {

        // store the route
        store_route();

        // call the callback
        ret_val = rx_cb ? rx_cb(data.payload, data.payload_len, ADDR_FROM_BYTES(data.src_addr)) \
                        : 0;

        //~ int i;
        //~ printf("route = %.4x-", ADDR_FROM_BYTES(data.src_addr));
        //~ for (i=0; i<data.route_len; i++) {
            //~ printf("%.4x-", ADDR_FROM_BYTES(data_route+2*i));
        //~ }
        //~ printf("%.4x\n", node_addr);
    }

    // check if it needs to be forwarded
    if ( dst == MAC_BROADCAST || dst != node_addr ) {

        // check if there is room for one more hop
        if (data.route_len >= MAX_ROUTE_LEN) {
            // too big, drop
            printf("Too many hops!\n");
            return ret_val;
        }

        // insert my address
        INSERT_ADDR_AT( node_addr, data_route + data.route_len*2 );
        data.route_len+=1;
        data_length +=2;
        data_addr = MAC_BROADCAST;

        // start delay to send
        timerB_set_alarm_from_now(ALARM_DELAY, 2*ALARM_1MS, 0);
        timerB_register_cb(ALARM_DELAY, send_data);
        printf("fw\n");
    } else {
        data_length = 0;
    }

    return ret_val;
}




/*----SOURCE ROUTING----*/
static uint16_t rx_source_handle(void) {
    uint16_t dst;
    uint16_t ret_val=0;

    printf("source ");

    // check the destination
    dst = ADDR_FROM_BYTES(data.dst_addr);

    // if for me, if for me call the callback
    if ( (dst == node_addr) && rx_cb) {

        // clear the data indicator
        data_length = 0;

        // store the route
        store_route();

        ret_val = rx_cb(data.payload, data.payload_len, ADDR_FROM_BYTES(data.src_addr));
    } else {
        // otherwise, forward
        int16_t pos;
        pos = find_pos_in_route(node_addr, data_route, data.route_len);

        if (pos<0) {
            // next hop not found
            printf("data_frame_received fw next hop not found\n");
            // clear the data indicator
            data_length = 0;
            data_addr = 0x0;
            return ret_val;

        } else if (pos==data.route_len-1){
            data_addr = dst;
        } else {
            data_addr = ADDR_FROM_BYTES(data_route+2*pos);
        }

        // forward
        timerB_set_alarm_from_now(ALARM_DELAY, 2*ALARM_1MS, 0);
        timerB_register_cb(ALARM_DELAY, send_data);
        printf("fw\n");
    }

    return ret_val;
}


/*----OTHER----*/

/**
 * Function called when first sending a packet.
 * It should try to see if the destination node is in any known route.
 * Otherwise it declares flooding.
 * \param addr the destination address
 */
static inline int16_t set_route_to_host(uint16_t addr) {
    int i,j;
    int pos_dst, pos_src;

    printf("find_route\n");

    data.route_len = 0;
    if (addr==MAC_BROADCAST) {
        printf("broadcast, flooding\n");
        data_addr = MAC_BROADCAST;
        return 0;
    }

    for (i=0;i<ROUTE_NUMBER;i++) {

        pos_dst = find_pos_in_route(addr, known_routes[i].hops, known_routes[i].number);
        if (pos_dst != -1) {
            // we've found a route !
            pos_src = find_pos_in_route(node_addr, known_routes[i].hops, MAX_ROUTE_LEN);
            if (pos_src == -1) {
                // massive bug, a route without me!
                printf("bug route without me!!\n");
                continue;
            }
            printf("route found\n");

            int r=0;
            uint16_t hop;
            uint8_t* r_addr;

            if (pos_dst > pos_src) {
                data.route_len = pos_dst-pos_src-1;

                // recopy the route
                r=0;
                for (j=pos_src+1; j<pos_dst;j++) {
                    hop = ADDR_FROM_BYTES(known_routes[i].hops+2*j);
                    r_addr = data_route + 2*r;
                    //~ printf("hop=%.4x, addr=%p\n", hop, r_addr);
                    INSERT_ADDR_AT(hop, r_addr);
                    //~ INSERT_ADDR_AT(known_routes[i].hops[j], data_route + 2*(j-pos_src));

                    r+=1;
                }
            } else {
                data.route_len = pos_src-pos_dst-1;

                // recopy the route
                for (j=pos_src-1; j>pos_dst;j--) {
                    hop = ADDR_FROM_BYTES(known_routes[i].hops +2*j);
                    r_addr = data_route + 2*r;
                    //~ printf("hop=%x\n", hop);
                    INSERT_ADDR_AT(hop, r_addr);
                    //~ INSERT_ADDR_AT(known_routes[i].hops[j], data_route + 2*(pos_src-1-j));

                    r+=1;
                }
            }

            // say we found it
            if (data.route_len>0) {
                data_addr = ADDR_FROM_BYTES(data_route);
            } else {
                data_addr = ADDR_FROM_BYTES(data.dst_addr);
            }
            return 1;
        }
    }

    printf("no route, flooding\n");
    data_addr = MAC_BROADCAST;
    return 0;
}

static inline int16_t is_packet_known(data_t* pkt) {
    int i;

    // look for packet id
    for (i=0; i<KNOWN_PACKET_NUMBER; i++)
        if ( (ADDR_FROM_BYTES(pkt->src_addr) == ADDR_FROM_BYTES(known_packets[i].src_addr)) &&
             (pkt->id == known_packets[i].id))
            return 1;


    // if not known, put packet to known packets
    INSERT_ADDR_AT(ADDR_FROM_BYTES(pkt->src_addr), known_packets[known_packet_id].src_addr);
    known_packets[known_packet_id].id = pkt->id;
    known_packet_id += 1;
    known_packet_id %= KNOWN_PACKET_NUMBER;

    return 0;
}

static inline int16_t find_pos_in_route(uint16_t addr, uint8_t *route, uint16_t route_len) {
    uint8_t* p;

    //~ int i;
    //~ printf("route [%u]=", route_len);
    //~ for (i=0;i<route_len;i++) printf("%.2x%.2x ", route[2*i], route[2*i+1]);
    //~ printf("\n");


    for (p=route+2*(route_len-1); p-route>=0; p-=2) {

        //~ printf("matching %.4x with %.4x...", addr, ADDR_FROM_BYTES(p));

        if (ADDR_FROM_BYTES(p) == addr) {
            //~ printf("found\n");
            return (p-route)/2;
        }
        //~ printf("not found\n");
    }
    return -1;
}

static inline void store_route(void) {

    int i;
    uint8_t* hops;

    if (data.route_len > MAX_ROUTE_LEN) {
        // too big, error
        return;
    }

    // check if we already have a route to this node
    for (i=0;i<ROUTE_NUMBER;i++) {
        if (find_pos_in_route(ADDR_FROM_BYTES(data.src_addr), \
                                        known_routes[i].hops, \
                                        known_routes[i].number)!=-1) {
            // route exists
            return;
        }
    }

    // easy pointer
    hops = known_routes[known_route_id].hops;

    // first hop, the sender
    ADDR_COPY(hops, data.src_addr);
    hops += 2;

    // then the route
    memcpy(hops, data_route, 2*data.route_len);
    hops += (2*data.route_len);

    // then me
    INSERT_ADDR_AT(node_addr, hops);

    // update size
    known_routes[known_route_id].number = 2+data.route_len;


    printf("new route!\n");
    for (i=0; i<known_routes[known_route_id].number; i++) {
        printf("%.4x-", ADDR_FROM_BYTES(known_routes[known_route_id].hops+2*i));
    }

    // increment id
    known_route_id += 1;
    known_route_id %= ROUTE_NUMBER;

}

