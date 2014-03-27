#ifndef _INA209_H_
#define _INA209_H_

void ina209_init(void);

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
