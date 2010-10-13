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

#ifndef _TDMA_NODE_H
#define _TDMA_NODE_H

/**
 * This node MAC address.
 */
extern uint16_t mac_addr;
/**
 * Create the MAC task.
 * \param xSPIMutex the mutex to access the radio SPI
 */
void mac_create_task(xSemaphoreHandle xSPIMutex);

/**
 * The different events used.
 */
enum mac_event {
	MAC_ASSOCIATED, MAC_LOST, MAC_DISASSOCIATED
};

/**
 * The different commands available.
 */
enum mac_command {
	MAC_ASSOCIATE, MAC_DISASSOCIATE
};

// Management
/**
 * Send a command to the MAC layer. Available commands are associate and disassociate.
 * The associate command should be called at least once to enable communicating.
 * \param cmd the command to execute.
 */
void mac_send_command(enum mac_command cmd);
/**
 * Set a callback function pointer to a specific event, in order to be able to handle it.
 * \param evt the event to set a callback to
 * \param handler the function pointer to be called
 */
void mac_set_event_handler(enum mac_event evt, void(*handler)(void));
/**
 * Set a callback function called when data has been received
 * \param handler the data received callback function pointer
 */
void mac_set_data_received_handler(void(*handler)(uint8_t* data,
		uint16_t length));
/**
 * Set a callback function called every time a beacon is received, useful for synchronization.
 * \param handler the beacon callback function pointer
 */
void mac_set_beacon_handler(void(*handler)(uint8_t id, uint16_t beacon_time));
/**
 * Send data to the coordinator. It is recommended to call this function right after the MAC_TX_READY
 * event is notified.
 * \param data a pointer to the data to send
 * \param length the number of bytes to send
 */
void mac_send(uint8_t* data, uint16_t length);

#endif
