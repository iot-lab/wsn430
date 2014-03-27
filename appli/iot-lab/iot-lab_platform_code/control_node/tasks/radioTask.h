/*
 * Copyright  2008-2009 INRIA/SensLab
 *
 * <dev-team@senslab.info>
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
 * \brief Radio task header
 * \author Clément Burin des Roziers
 * \date 2009
 */

#ifndef _RADIOTASK_H
#define _RADIOTASK_H

/**
 * \brief Create and start the radio task.
 * \param priority the priority the task should run at.
 */
void radioTask_create(int priority);

/**
 * \brief Perform an RSSI measurement.
 *
 * \return The read value.
 *
 * \note the radio must not be injecting noise to perform an RSSI.
 */
int radio_getrssi(uint8_t * data);

/**
 * \brief Enable/Disable radio noise injection.
 * \param on non-zero for injection, 0 for no injection
 * \return 1 if request accepted, 0 if an error occured.
 */
int radio_setnoise(uint16_t on);

// CC1101 specific configuration
/**
 * \brief Update the CC1101 base frequency.
 * \param freq the frequency register value (22 bits).
 * \return 1 if OK, 0 if error.
 */
int radio_cc1101_setfreq(uint32_t freq);

/**
 * \brief Set the CC1101 modulation format.
 * \param mof_format the mod_format register value to set.
 * \return 1 if OK, 0 if error.
 */
int radio_cc1101_setmod(uint8_t mod_format);

/**
 * \brief Set the CC1101 TX power.
 * \param patable the PATABLE value to set.
 * \return 1 if OK, 0 if error.
 */
int radio_cc1101_settxpower(uint8_t patable);

/**
 * \brief Set the CC1101 channel bandwidth.
 * \param chanbw_e the chanbw_e register value to set.
 * \param chanbw_m the chanbw_m register value to set.
 * \return 1 if OK, 0 if error.
 */
int radio_cc1101_setchanbw(uint8_t chanbw_e, uint8_t chanbw_m);

/**
 * \brief Set the CC1101 data rate.
 * \param drate_e the drate_e register value to set.
 * \param drate_m the drate_m register value to set.
 * \return 1 if OK, 0 if error.
 */
int radio_cc1101_setdatarate(uint8_t drate_e, uint8_t drate_m);


// CC2420 specific configuration
/**
 * \brief Update the CC2420 base frequency.
 * \param freq the frequency in MHz.
 * \return 1 if OK, 0 if error.
 */
int radio_cc2420_setfreq(uint16_t freq);

/**
 * \brief Set the CC2420 TX power.
 * \param palavel the PA_LEVEL value to set (5bits).
 * \return 1 if OK, 0 if error.
 */
int radio_cc2420_settxpower(uint8_t palevel);
#endif
