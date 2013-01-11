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
 * \addtogroup wsn430
 * @{
 */

/**
 * \addtogroup i2c0
 * @{
 */

/**
 * \file
 * \brief I2C0 driver
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
 * \date November 08
 */

/**
 * @}
 */

/**
 * @}
 */


#include <io.h>
#include "i2c0.h"

#define  setMasterMode()            U0CTL |= MST
#define  enableI2C()                U0CTL |= I2CEN
#define  disableI2C()               U0CTL &= ~I2CEN
#define  setTransmitMode()          I2CTCTL |= I2CTRX
#define  setReceiveMode()           I2CTCTL &= ~(I2CTRX)
#define  setSlaveAddress( addr )    I2CSA = addr
#define  setTransmitLength( l )     I2CNDAT = l
#define  setStopBit()               I2CTCTL |= I2CSTP
#define  setStartBit()              I2CTCTL |= I2CSTT
#define  writeByte(b)               I2CDRB = b
#define  readByte()                 I2CDRB

#define  setRepeatMode()            I2CTCTL |= I2CRM
#define  unsetRepeatMode()          I2CTCTL &= ~(I2CRM)

#define  clearIFG()                 I2CIFG = 0

#define  I2CBusy()                  (I2CDCTL & I2CBUSY)
#define  TXReady()                  (I2CIFG & TXRDYIFG)
#define  clearTXReady()             I2CIFG &= ~(TXRDYIFG)
#define  RXReady()                  (I2CIFG & RXRDYIFG)
#define  AccessReady()              (I2CIFG & ARDYIFG)
#define  clearAccessReady()         I2CIFG &= ~(ARDYIFG)
#define  NACKError()                (I2CIFG & NACKIFG)

void i2c0_init(void)
{
    /* Configure IO pins for I2C */
    P3DIR &= ~( (0x1 << 3) | (0x1 << 1) );
    P3SEL |= ( (0x1 << 3) | (0x1 << 1) );
    
    /* Force UART Reset and I2C mode*/
    U0CTL |= SWRST;
    
    /* Configure USART0 for I2C 
     * No DMA, 7bit address, normal mode */
    // Enable I2C
    U0CTL |= I2C | SYNC;
    
    // Disable I2C
    U0CTL &= ~I2CEN;
    
    // Configure U0CTL
    U0CTL &= ~(RXDMAEN | TXDMAEN | XA);
    
    /* Configure I2CTCTL 
     * byte mode, SMCLK */
    I2CTCTL = 0;
    I2CTCTL = ( I2CSSEL_2 );
    
    /* Configure I2C clock
     * prescaler = 2
     * SCLH = 39 periods
     * SCLL = 39 periods 
     * This gives a 70kHz frequency with 8MHz SMCLK*/
#ifdef I2C_FREQ_70K
    I2CPSC  = 0x0;
    I2CSCLH = 0x37;//0x25; //config 100kHz with 8MHz SMCLK
    I2CSCLL = 0x37;//0x25;
#else // config 100kHz with 1MHz SMCLK
    I2CPSC  = 0x0;
    I2CSCLH = 0x3;
    I2CSCLL = 0x3;
#endif

    /* Arbitrary own address */
    I2COA = 0xAAAA;
    
    /* Disable all I2C interrupts */
    I2CIE = 0;
    I2CIFG = 0;
    
    /* Enable I2C module */
    U0CTL |= I2CEN;
}

uint8_t i2c0_read_single (uint8_t slaveAddr, uint8_t *value )
{
    // Reset State Machine
    U0CTL &= ~I2CEN;
    U0CTL |= I2CEN;
    
    setReceiveMode();
    setMasterMode();
    setTransmitLength(1);
    setSlaveAddress( slaveAddr );
    setStartBit();
    setStopBit();
    
    while ( !RXReady() ) ;
    *value = readByte();
    
    while ( I2CBusy() );
    
    return 1;
}

uint8_t i2c0_read (uint8_t slaveAddr, uint8_t regAddr, uint16_t len, uint8_t* buf )
{
    uint16_t pos = 0;
    
    // Reset State Machine
    U0CTL &= ~I2CEN;
    U0CTL |= I2CEN;
    
    setMasterMode();
    setTransmitMode();
    //~ unsetRepeatMode();
    
    setSlaveAddress( slaveAddr );
    
    clearTXReady();
    clearAccessReady();
    setTransmitLength(1);
    
    setStartBit();
    setStopBit();
    
    while ( (!TXReady()) && (!NACKError()) )
        ;
    
    if (NACKError()) {
		return 0;
	}
	
    writeByte(regAddr);
    
    while ( I2CBusy() );
    
    setReceiveMode();
    setMasterMode();
    setTransmitLength(len);
    setSlaveAddress( slaveAddr );
    
    clearAccessReady();
    
    setStartBit();
    setStopBit();
    
    for ( pos=0; pos<len; pos++)
    {
        while (! RXReady() );
        
        buf[pos] = readByte();
    }
    
    while ( I2CBusy() );
    
    return 1;
}

uint8_t i2c0_write_single( uint8_t slaveAddr, uint8_t value )
{   
    // Reset State Machine
    U0CTL &= ~I2CEN;
    U0CTL |= I2CEN;
    
    setMasterMode();
    setTransmitMode();
    
    setSlaveAddress( slaveAddr );
    setTransmitLength(1);
    
    setStartBit();
    setStopBit();
    
    while ( (!TXReady()) && (!NACKError()) )
        ;
    
    if (NACKError()) {
		return 0;
	}
	
    writeByte(value);

    while ( I2CBusy() );
    
    return 1;
}

uint8_t i2c0_write( uint8_t slaveAddr, uint8_t regAddr, uint16_t len, uint8_t *buf )
{
    uint16_t pos = 0;
    
    // Reset State Machine
    U0CTL &= ~I2CEN;
    U0CTL |= I2CEN;
    
    setMasterMode();
    setTransmitMode();
    
    setSlaveAddress( slaveAddr );
    setTransmitLength(len+1);
    
    setStartBit();
    setStopBit();
    
    while ( (!TXReady()) && (!NACKError()) )
        ;
    
    if (NACKError()) {
		return 0;
	}
    
    writeByte(regAddr);
        
    for ( pos=0; pos<len; pos++)
    {
        while ( !TXReady() && (!NACKError()) )
			;
        
		if (NACKError()) {
			return 0;
		}
        writeByte(buf[pos]);
    }
    
    while ( I2CBusy() );
    
    return 1;
}


uint8_t i2c0_raw_write( uint8_t slaveAddr, uint16_t len, uint8_t *buf )
{
    uint16_t pos = 0;
    
    // Reset State Machine
    U0CTL &= ~I2CEN;
    U0CTL |= I2CEN;
    
    setMasterMode();
    setTransmitMode();
    
    setSlaveAddress( slaveAddr );
    setTransmitLength(len);
    
    setStartBit();
    setStopBit();
    
    for ( pos=0; pos<len; pos++)
    {
        while ( !TXReady() && (!NACKError()) )
			;
    
		if (NACKError()) {
			return 0;
		}
        writeByte(buf[pos]);
    }
    
    while ( I2CBusy() );
    
    return 1;
}
