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


void ina209_init(void)
{
    // MANDATORY - Set P2.1 as input for SMBus-Alert signal from INA209 chips
    P2SEL &= ~0x02; // P2.1 as GPIO
    P2DIR &= ~0x02; // P2.1 as input

    /* Initialize the I2C */
    i2c0_init();
                        // MSB | LSB
    uint8_t config[]  = {0x14, 0xCF};
    /*
     * Configuration is:
     * 16V Bus Full Scale Range
     * +/-160mV Shunt Voltage Range
     * 12bit sampling for Bus and Shunt ADCs, 2sample average
     * Continuous Sampling for both
     */

    //~ uint8_t calib[]   = {0x28, 0x00};
    uint8_t calib[]   = {0x28, 0x5F};
    /*
     * Theoretical calibration is: 0x2800
     * Corrected calibration (after measure) is 0x285F (for 103mA)
     */

    /*
     * Bus Voltage is 13bit aligned on left, and its LSB is 4mV.
     * Current LSB is 4uA
     * Power LSB is 80uW
     */

    i2c0_write(DC_INA209_ADDRESS, CONFIGURATION_REGISTER, 2, config);
    i2c0_write(DC_INA209_ADDRESS, CALIBRATION_REGISTER, 2, calib);

    i2c0_write(BATT_INA209_ADDRESS, CONFIGURATION_REGISTER, 2, config);
    i2c0_write(BATT_INA209_ADDRESS, CALIBRATION_REGISTER, 2, calib);
}

uint16_t ina209_read_dc_voltage(void)
{
    uint8_t voltage[2];
    i2c0_read(DC_INA209_ADDRESS, BUS_VOLTAGE_REGISTER, 2, voltage);
    return ( (((uint16_t)voltage[0])<<8) + (uint16_t)voltage[1]);
}
uint16_t ina209_read_batt_voltage(void)
{
    uint8_t voltage[2];
    i2c0_read(BATT_INA209_ADDRESS, BUS_VOLTAGE_REGISTER, 2, voltage);
    return ( (((uint16_t)voltage[0])<<8) + (uint16_t)voltage[1]);
}

uint16_t ina209_read_dc_current(void)
{
    uint8_t current[2];
    i2c0_read(DC_INA209_ADDRESS, CURRENT_REGISTER, 2, current);
    return ( (((uint16_t)current[0])<<8) + (uint16_t)current[1]);
}
uint16_t ina209_read_batt_current(void)
{
    uint8_t current[2];
    i2c0_read(BATT_INA209_ADDRESS, CURRENT_REGISTER, 2, current);
    return ( (((uint16_t)current[0])<<8) + (uint16_t)current[1]);
}

uint16_t ina209_read_dc_power(void)
{
    uint8_t power[2];
    i2c0_read(DC_INA209_ADDRESS, POWER_REGISTER, 2, power);
    return ( (((uint16_t)power[0])<<8) + (uint16_t)power[1]);
}
uint16_t ina209_read_batt_power(void)
{
    uint8_t power[2];
    i2c0_read(BATT_INA209_ADDRESS, POWER_REGISTER, 2, power);
    return ( (((uint16_t)power[0])<<8) + (uint16_t)power[1]);
}

uint16_t ina209_read_dc_shunt_voltage(void)
{
    uint8_t voltage[2];
    i2c0_read(DC_INA209_ADDRESS, SHUNT_VOLTAGE_REGISTER, 2, voltage);
    return ( (((uint16_t)voltage[0])<<8) + (uint16_t)voltage[1]);
}
uint16_t ina209_read_batt_shunt_voltage(void)
{
    uint8_t voltage[2];
    i2c0_read(BATT_INA209_ADDRESS, SHUNT_VOLTAGE_REGISTER, 2, voltage);
    return ( (((uint16_t)voltage[0])<<8) + (uint16_t)voltage[1]);
}
