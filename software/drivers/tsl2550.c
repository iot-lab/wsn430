/**
 *  \file   tsl2550.c
 *  \brief  TSL2550 light sensor driver (available on wsn430v1.3b et .4 boards)
 *  \author Colin Chaballier
 *  \date   2008
 **/

#include <io.h>
#include "tsl2550.h"
#include "i2c0.h"

#define TSL_ADDR     0x39
#define TSL_CMD_PD   0x00 // power down
#define TSL_CMD_PU   0x03 // power up and read config.
#define TSL_CMD_EXT  0x1D // set extended mode
#define TSL_CMD_RST  0x18 // reset to standard mode
#define TSL_CMD_ADC0 0x43 // read ADC0 value
#define TSL_CMD_ADC1 0x83 // read ADC1 value

void tsl2550_init(void)
{
    // configure TSL supply
    P4SEL &= ~(1<<5);
    P4DIR |=  (1<<5);
    P4OUT |=  (1<<5);
    
    i2c0_init();

}

uint8_t tsl2550_powerup(void)
{
    uint8_t readValue;
    i2c0_write_single(TSL_ADDR, TSL_CMD_PU);
    i2c0_read_single(TSL_ADDR, &readValue);
    return readValue;
}

void tsl2550_powerdown(void)
{
    i2c0_write_single(TSL_ADDR, TSL_CMD_PD);
	//~ send_I2C_byte(TSL_CMD_PD);
}

void tsl2550_set_extended()
{
    i2c0_write_single(TSL_ADDR, TSL_CMD_EXT);
	//~ send_I2C_byte(TSL_CMD_EXT);
}

void tsl2550_set_standard()
{
    i2c0_write_single(TSL_ADDR, TSL_CMD_RST);
	//~ send_I2C_byte(TSL_CMD_RST);
}

uint8_t tsl2550_read_adc0(void)
{
    uint8_t readValue;
    i2c0_write_single(TSL_ADDR, TSL_CMD_ADC0);
    i2c0_read_single(TSL_ADDR, &readValue);
    return readValue;
    
	//~ send_I2C_byte(TSL_CMD_ADC0);
	//~ return read_I2C_byte();
}

uint8_t tsl2550_read_adc1(void)
{
    uint8_t readValue;
    i2c0_write_single(TSL_ADDR, TSL_CMD_ADC1);
    i2c0_read_single(TSL_ADDR, &readValue);
    return readValue;
    
	//~ send_I2C_byte(TSL_CMD_ADC1);
	//~ return read_I2C_byte();
}
