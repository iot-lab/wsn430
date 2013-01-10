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
 *  \file
 *  \brief  INA209 driver header
 *  \author Colin Chaballier
 *  \author Clément Burin des Roziers
 *  \date   2008
 **/

#ifndef _INA209_H_
#define _INA209_H_

/**
 * Initialize the INA209 with default configuration
 */
void ina209_init(void);

enum ina209_period {
	PERIOD_1MS = 0x9,
	PERIOD_2MS = 0xA,
	PERIOD_4MS = 0xB,
	PERIOD_8MS = 0xC,
	PERIOD_17MS = 0xD,
	PERIOD_34MS = 0xE,
	PERIOD_68MS = 0xF
};

void ina209_set_sample_period(enum ina209_period period);

/**
 * Read the bus voltage.
 * \return the bus voltage in 0.032mV
 */
uint16_t ina209_read_dc_voltage(void);
uint16_t ina209_read_batt_voltage(void);

/**
 * Read the shunt voltage.
 * \return the shunt voltage in x10µV
 */
uint16_t ina209_read_dc_shunt_voltage(void);
uint16_t ina209_read_batt_shunt_voltage(void);

/**
 * Read the current.
 * \return the current in x10µA
 */
uint16_t ina209_read_dc_current(void);
uint16_t ina209_read_batt_current(void);

/**
 * Read the instant power.
 * \return the power in x0.2mW
 */
uint16_t ina209_read_dc_power(void);
uint16_t ina209_read_batt_power(void);

#endif
