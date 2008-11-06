/**
 *  \file   tsl2550.c
 *  \brief  TSL2550 light sensor driver (available on wsn430v1.3b et .4 boards)
 *  \author Colin Chaballier
 *  \date   2008
 **/

#include <io.h>
#include "tsl2550.h"

#define TSL_ADDR     0x39
#define TSL_CMD_PD   0x00 // power down
#define TSL_CMD_PU   0x03 // power up and read config.
#define TSL_CMD_EXT  0x1D // set extended mode
#define TSL_CMD_RST  0x18 // reset to standard mode
#define TSL_CMD_ADC0 0x43 // read ADC0 value
#define TSL_CMD_ADC1 0x83 // read ADC1 value


static void inline send_I2C_byte(char c)
{	// set master
	U0CTL |= MST;
	
	// slave address
	I2CSA = TSL_ADDR;
	
	// 1 byte to send
	I2CNDAT = 1;
	
	// set TX transfer with start and stop bits
	I2CTCTL |= I2CTRX | I2CSTT | I2CSTP;
	
	// wait for ready
	while ( (I2CIFG & TXRDYIFG) == 0) {}
	
	// send command : read channel 0
	I2CDR = c;
	
	// wait end of transmission
	while (I2CDCTL & I2CBUSY) ;
}

static uint8_t inline read_I2C_byte()
{
	I2CTCTL &= ~I2CTRX;         // I2CTRX=0 => Receive Mode (R/W bit = 1)
	
	U0CTL |= MST;
	I2CNDAT = 1;           // number of bytes to receive

	I2CIFG &= ~ARDYIFG;         // clear Access ready interrupt flag
	I2CTCTL |= I2CSTT+I2CSTP;   // start receiving and finally generate
                                //  start and stop condition
	// wait end of transmission
	while (I2CDCTL & I2CBUSY) ;

	return I2CDR;
}

void tsl2550_init(void)
{
	// configure I2C on USART0
	P3SEL |=  0xA;
	P3DIR &= ~0x0A;
	
	// configure TSL supply
	P4SEL &= ~(1<<5);
	P4DIR |=  (1<<5);
	P4OUT |=  (1<<5);
	
	// configure U0CTL for I2C
	U0CTL = (I2C | SYNC) ;
	U0CTL &= ~(I2CEN);
	
	// clock from SMCLK
	I2CTCTL = I2CSSEL_2;

	// clock config
	I2CPSC = 0x0;
	I2CSCLH = 0x7;
	I2CSCLL = 0x7;
	
	// disable all interrupts
	I2CIE = 0;
	
	// enable I2C
	U0CTL |= I2CEN;
	
}

uint8_t tsl2550_powerup(void)
{
	send_I2C_byte(TSL_CMD_PU);
	return read_I2C_byte();
}

void tsl2550_powerdown(void)
{
	send_I2C_byte(TSL_CMD_PD);
}

void tsl2550_set_extended()
{
	send_I2C_byte(TSL_CMD_EXT);
}

void tsl2550_set_standard()
{
	send_I2C_byte(TSL_CMD_RST);
}

uint8_t tsl2550_read_adc0(void)
{
	send_I2C_byte(TSL_CMD_ADC0);
	return read_I2C_byte();
}

uint8_t tsl2550_read_adc1(void)
{
	send_I2C_byte(TSL_CMD_ADC1);
	return read_I2C_byte();
}
