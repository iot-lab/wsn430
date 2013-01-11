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
 *  \brief  INA209 driver
 *  \author Colin Chaballier
 *  \author Clément Burin des Roziers
 *  \date   2008
 **/

#include <io.h>

#include "i2c0.h"
#include "ina209.h"

#define DC_INA209_ADDRESS    0x41
#define BATT_INA209_ADDRESS  0x40

#define CONFIGURATION_REGISTER  0x00
#define SHUNT_VOLTAGE_REGISTER  0x03
#define BUS_VOLTAGE_REGISTER    0x04
#define POWER_REGISTER          0x05
#define CURRENT_REGISTER        0x06
#define CALIBRATION_REGISTER    0x16

/*
 * Configuration is:
 * 16V Bus Full Scale Range
 * +/-160mV Shunt Voltage Range
 * 12bit sampling for Bus and Shunt ADCs, 64sample average
 * Continuous Sampling for both, 68ms averaging
 */
// MSB | LSB
static uint8_t config[] = { 0x17, 0x77 };

/*
 * Bus Voltage is 13bit aligned on left, and its LSB is 4mV.
 * Current LSB is 5uA
 * Power LSB is 100uW
 */
static uint8_t calib[] = { 0x20, 0x00 };

void ina209_init(void) {
	// Initialize the I2C
	i2c0_init();

	// Write default configuration
	i2c0_write(DC_INA209_ADDRESS, CONFIGURATION_REGISTER, 2, config);
	i2c0_write(DC_INA209_ADDRESS, CALIBRATION_REGISTER, 2, calib);

	i2c0_write(BATT_INA209_ADDRESS, CONFIGURATION_REGISTER, 2, config);
	i2c0_write(BATT_INA209_ADDRESS, CALIBRATION_REGISTER, 2, calib);
}

void ina209_set_sample_period(enum ina209_period period) {
#define ADC_MASK 0xF
#define BADC_SHIFT 7
#define SADC_SHIFT 3

	uint16_t new_config;
	uint8_t bytes[2];

	new_config = config[0];
	new_config <<= 8;
	new_config += config[1];

	// clear settings
	new_config &= ~( (ADC_MASK << SADC_SHIFT) | (ADC_MASK << BADC_SHIFT) );

	// add settings
	new_config |= ( (period << SADC_SHIFT) | (period | BADC_SHIFT) );

	bytes[0] = new_config >> 8;
	bytes[1] = new_config & 0xFF;

	// update registers
	i2c0_write(DC_INA209_ADDRESS, CONFIGURATION_REGISTER, 2, bytes);
	i2c0_write(BATT_INA209_ADDRESS, CONFIGURATION_REGISTER, 2, bytes);
}

uint16_t ina209_read_dc_voltage(void) {
	uint8_t voltage[2];
	i2c0_read(DC_INA209_ADDRESS, BUS_VOLTAGE_REGISTER, 2, voltage);
	return ((((uint16_t) voltage[0]) << 8) + (uint16_t) voltage[1]);
}
uint16_t ina209_read_batt_voltage(void) {
	uint8_t voltage[2];
	i2c0_read(BATT_INA209_ADDRESS, BUS_VOLTAGE_REGISTER, 2, voltage);
	return ((((uint16_t) voltage[0]) << 8) + (uint16_t) voltage[1]);
}

uint16_t ina209_read_dc_current(void) {
	uint8_t current[2];
	i2c0_read(DC_INA209_ADDRESS, CURRENT_REGISTER, 2, current);
	return ((((uint16_t) current[0]) << 8) + (uint16_t) current[1]);
}
uint16_t ina209_read_batt_current(void) {
	uint8_t current[2];
	i2c0_read(BATT_INA209_ADDRESS, CURRENT_REGISTER, 2, current);
	return ((((uint16_t) current[0]) << 8) + (uint16_t) current[1]);
}

uint16_t ina209_read_dc_power(void) {
	uint8_t power[2];
	i2c0_read(DC_INA209_ADDRESS, POWER_REGISTER, 2, power);
	return ((((uint16_t) power[0]) << 8) + (uint16_t) power[1]);
}
uint16_t ina209_read_batt_power(void) {
	uint8_t power[2];
	i2c0_read(BATT_INA209_ADDRESS, POWER_REGISTER, 2, power);
	return ((((uint16_t) power[0]) << 8) + (uint16_t) power[1]);
}

uint16_t ina209_read_dc_shunt_voltage(void) {
	uint8_t voltage[2];
	i2c0_read(DC_INA209_ADDRESS, SHUNT_VOLTAGE_REGISTER, 2, voltage);
	return ((((uint16_t) voltage[0]) << 8) + (uint16_t) voltage[1]);
}
uint16_t ina209_read_batt_shunt_voltage(void) {
	uint8_t voltage[2];
	i2c0_read(BATT_INA209_ADDRESS, SHUNT_VOLTAGE_REGISTER, 2, voltage);
	return ((((uint16_t) voltage[0]) << 8) + (uint16_t) voltage[1]);
}
