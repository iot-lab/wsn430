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
 * \brief header file of the TDMA MAC node implementation
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inrialpes.fr>
 * \date November 2009
 */

#ifndef _TDMA_N_H_
#define _TDMA_N_H_

/**
 * This node MAC address
 */
extern uint8_t node_addr;

/**
 * Initialize the MAC layer. Look for a coordinator,
 * send an attach request, wait for an ACK.
 */
void mac_init(uint8_t channel);

/**
 * This function indicates the MAC layer that new data is available
 * in the \ref mac_data_payload buffer for sending to the coordinator.
 * Once this function is called, the mac_data_payload buffer should not
 * be accessed (written to) until the access_allowed callback has been
 * called.
 */
void mac_send(void);

/**
 * This function registers a callback function that will be called
 * everytime it is possible to write-access the mac_data_payload buffer.
 * \param cb the callback function to register
 */
void mac_set_access_allowed_cb(uint16_t (*cb)(void));

/**
 * Function that returns non-zero if it is possible to the mac_data_payload
 * buffer, or zero if it is not possible for now.
 * \return 1 if access allowed, 0 if not allowed
 */
int16_t mac_is_access_allowed(void);

/**
 * Pointer to the data payload. You can only write to this buffer
 * when mac_is_access_allowed returns 1.
 */
extern uint8_t* const mac_payload;

/**
 * The size of the mac_data_payload buffer.
 */
#define TDMA_PAYLOAD_SIZE 60

#endif
