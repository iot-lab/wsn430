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

#ifndef _MAC_H
#define _MAC_H

#ifndef MAC_TX_POWER
#define MAC_TX_POWER PHY_TX_m10dBm
#endif

#define MAC_BROADCAST_ADDR 0xFFFF
#define MAC_TX_QUEUE_LENGTH 6



typedef void (*mac_rx_callback_t)(uint16_t src_addr, uint8_t* data,
		uint16_t length, int8_t rssi);

/**
 * This node address
 */
extern uint16_t mac_addr;

/**
 * Initialize and create the MAC task.
 * \param spi_m the radio spi mutex.
 * \param cb the packet received callback.
 * \param channel the radio channel to use.
 */
void mac_init(xSemaphoreHandle xSPIMutex, mac_rx_callback_t rx_cb, uint8_t channel);

/**
 * Send a packet to a node.
 * \param dest_addr the destination node address
 * \param data a pointer to the data to send
 * \param length the number of bytes to send
 * \return 1 if the packet will be sent, 0 if it won't
 */
uint16_t mac_send(uint16_t dest_addr, uint8_t* data, uint16_t length);

#endif
