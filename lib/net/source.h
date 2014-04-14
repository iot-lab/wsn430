#ifndef SOURCE_H
#define SOURCE_H

extern uint16_t node_addr;
#define NET_BROADCAST 0xFFFF

/**
 * Function pointer prototype for callback.
 * \param packet pointer to the received packet
 * \param length number of received bytes
 * \param src_addr address of the source node
 * \return 1 if the CPU should we waken up, 0 if it should stay in LPM
 */
typedef uint16_t (*net_handler_t)(uint8_t packet[], uint16_t length, uint16_t src_addr);

/**
 * Initialize the NET layer.
 */
void net_init();

/**
 * Send a packet to a remote node.
 * \param packet the data pointer
 * \param length the data length
 * \param route the route to follow, the first address should be the next hop
 * \param route_len the number of hops in the route
 * \return 0 if error, 1 if send should occur
 */
uint16_t net_send(uint8_t packet[], uint16_t length, uint16_t route[], uint16_t route_len);

/**
 * Register a callback function, called when a packet is received.
 */
void net_register_rx_cb(net_handler_t cb);

/**
 * Stop the NET layer.
 */
void net_stop();

#endif
