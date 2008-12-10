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
#define  writeByte(b)               I2CDR = b
#define  readByte()                 I2CDR

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
    
    /* Configure USART0 for I2C 
     * No DMA, 7bit address, normal mode */
    U0CTL &= ~(I2C | I2CEN | SYNC);
    U0CTL = SWRST;
    U0CTL |= SYNC | I2C;
    U0CTL &= ~I2CEN;
    
    /* Configure I2CTCTL 
     * byte mode, SMCLK */
    I2CTCTL = 0;
    I2CTCTL = ( I2CSSEL_2 );
    
    /* Configure I2C clock
     * prescaler = 1
     * SCLH = 5 periods
     * SCLL = 5 periods 
     * This gives a 100kHz frequency with 1MHz SMCLK*/
    I2CPSC = 0x0;
    I2CSCLH = 0x3;
    I2CSCLL = 0x3;
    
    /* Arbitrary own address */
    I2COA = 0xAAAA;
    
    /* Disable all I2C interrupts */
    I2CIE = 0;
    
    /* Enable I2C module */
    U0CTL |= I2CEN;
}

uint8_t i2c0_read_single (uint8_t slaveAddr, uint8_t *value )
{
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
    
    setMasterMode();
    setTransmitMode();
    unsetRepeatMode();
    
    setSlaveAddress( slaveAddr );
    
    clearTXReady();
    setTransmitLength(1);
    clearAccessReady();
    
    setStartBit();
    
    while ( !TXReady() ) ;
    
    writeByte(regAddr);
    
    while ( !AccessReady() );
    
    setReceiveMode();
    setTransmitLength(len);
    
    clearAccessReady();
    
    setStartBit();
    setStopBit();
    
    for ( pos=0; pos<len; pos++)
    {
        while (! RXReady() );
        
        buf[pos] = readByte();
    }
    
    while ( !AccessReady() );
    
    while ( I2CBusy() );
    
    return 1;
}

uint8_t i2c0_write_single( uint8_t slaveAddr, uint8_t value )
{   
    setMasterMode();
    setTransmitMode();
    
    setSlaveAddress( slaveAddr );
    setTransmitLength(1);
    
    setStartBit();
    setStopBit();
    
    while ( !TXReady() ) ;
    
    writeByte(value);

    while ( I2CBusy() );
    
    return 1;
}

uint8_t i2c0_write( uint8_t slaveAddr, uint8_t regAddr, uint16_t len, uint8_t *buf )
{
    uint16_t pos = 0;
    
    setMasterMode();
    setTransmitMode();
    
    setSlaveAddress( slaveAddr );
    setTransmitLength(len+1);
    
    setStartBit();
    setStopBit();
    
    while ( !TXReady() );
    
    writeByte(regAddr);
        
    for ( pos=0; pos<len; pos++)
    {
        while ( !TXReady() );
        
        writeByte(buf[pos]);
    }
    
    while ( !AccessReady() );
    
    while ( I2CBusy() );
    
    return 1;
}
