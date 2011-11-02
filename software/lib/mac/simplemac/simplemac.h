/**
 * \file simplemac.h
 * \brief header file of a simple mac protocol
 */

#ifndef _SIMPLEMAC_H
#define _SIMPLEMAC_H

#define BROADCAST_ADDR 0xFF

/**
 * Function pointer prototype for callback.
 * \param packet pointer to the received packet
 * \param length number of received bytes
 * \param src_addr address of the source node
 */
typedef void (*mac_handler_t)(uint8_t packet[], uint16_t length, uint8_t src_addr);

/**
 * Initialize the MAC layer implementation.
 */
void mac_init(void);

/**
 * Send a packet to a node.
 * \param packet pointer to the packet
 * \param length number of bytes to send (max = 252)
 * \param dst_addr address of the destination node
 * \return 0
 */
uint16_t mac_send(uint8_t packet[], uint16_t length, uint8_t dst_addr);

/**
 * Register a function callback that'll be called
 * when a packet has been received.
 * \param cb function pointer
 */
void mac_register_rx_cb(mac_handler_t cb);

/**
 * Disable MAC layer. Stop transmitting.
 */
void mac_stop(void);
#endif
