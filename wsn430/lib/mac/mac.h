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
 * \brief header file of any standard mac protocol
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inrialpes.fr>
 * \date November 2009
 */

#ifndef _MAC_H
#define _MAC_H

#define MAC_BROADCAST 0xFFFF

/**
 * Function pointer prototype for callback.
 * \param packet pointer to the received packet
 * \param length number of received bytes
 * \param src_addr address of the source node
 * \return 1 if the CPU should we waken up, 0 if it should stay in LPM
 */
typedef uint16_t (*mac_received_t)(uint8_t packet[], uint16_t length, uint16_t src_addr, int16_t rssi);
typedef uint16_t (*mac_sent_t)(void);
typedef uint16_t (*mac_error_t)(void);

/**
 * This node MAC address
 */
extern uint16_t node_addr;

/**
 * Initialize the MAC layer implementation.
 * \param channel the radio channel to use (values 0-20)
 */
void mac_init(uint8_t channel);

/**
 * Send a packet to a node.
 * \param packet pointer to the packet
 * \param length number of bytes to send
 * \param dst_addr address of the destination node
 * \return 0 if OK, 1 if a packet is being sent, 2 if length too big.
 */
uint16_t mac_send(uint8_t packet[], uint16_t length, uint16_t dst_addr);

/**
 * Register a function callback that'll be called
 * when a packet has been received.
 * \param cb function pointer
 */
void mac_set_rx_cb(mac_received_t cb);

/**
 * Register a function callback that'll be called
 * when a packet has been sent successfully.
 * \param cb function pointer
 */
void mac_set_sent_cb(mac_sent_t cb);

/**
 * Register a function callback that'll be called
 * when a packet has failed to be sent.
 * \param cb function pointer
 */
void mac_set_error_cb(mac_error_t cb);

/**
 * Disable MAC layer. Stop transmitting.
 */
void mac_stop(void);
/**
 * re-enable MAC layer.
 */
void mac_restart(void);

#endif
