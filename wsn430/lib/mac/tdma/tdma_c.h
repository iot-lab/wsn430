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
 * \brief header file of the TDMA MAC coordinator implementation
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inrialpes.fr>
 * \date November 2009
 */

#ifndef _TDMA_C_H_
#define _TDMA_C_H_
#include "tdma_timings.h"
#include "tdma_frames.h"
/**
 * This node MAC address
 */
extern uint8_t node_addr;

/**
 * Initialize the MAC layer. Look for a coordinator,
 * send an attach request, wait for an ACK.
 */
void mac_init(uint8_t channel);

#define MAC_PAYLOAD_SIZE PAYLOAD_LENGTH_MAX
#define MAC_SLOT_NUMBER DATA_SLOT_MAX
/**
 * a structure containing all relevant information a on slot
 */
typedef struct {
    /**
     * The node address conrresponding to this slot.
     */
    uint8_t addr;
    /**
     * This is the lates received data from this slot.
     * It should be read only when ready is set to 1;
     */
    uint8_t data[MAC_PAYLOAD_SIZE];
    /**
     * Flag indicating that new data has been placed in the data buffer.
     * Set it to 0 when you have read it. New data won't be copied
     * if this flag is not cleared.
     * When this flag is set by the MAC layer, the CPU will be woken up
     * if it is in a LPM.
     */
    volatile uint8_t ready;
} slot_t;

/**
 * A variable containing all informations on all the slots
 */
extern slot_t mac_slots[MAC_SLOT_NUMBER];

/**
 * Function that registers a callback function that will be called
 * when new data has been received. The argument of the callback
 * function will be the slot number available.
 * \param cb the callback function to register
 */
void mac_set_new_data_cb(uint16_t (*cb)(int16_t slot));

#endif
