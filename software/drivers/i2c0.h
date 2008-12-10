#ifndef _I2C0_H_
#define _I2C0_H_

/**
 * Initialize the I2C USART module.
 * Should be called fisrt.
 */
void i2c0_init(void);

/**
 * Read procedure. It sends a register address to a slave device, then receives
 * a given number of bytes from it.
 * \param slaveAddr the slave address
 * \param regAddr the register start address
 * \param len the number of bytes to read
 * \param buf an array pointer for storing the read values
 * \return 1
 */
uint8_t i2c0_read (uint8_t slaveAddr, uint8_t regAddr, uint16_t len, uint8_t* buf );

/**
 * Write procedure. It sends a register address to a slave device,
 * then sends a given number of bytes to it.
 * \param slaveAddr the slave address
 * \param regAddr the register start address
 * \param len the number of bytes to write
 * \param buf an array pointer with the values to send
 * \return 1
 */
uint8_t i2c0_write(uint8_t slaveAddr, uint8_t regAddr, uint16_t len, uint8_t* buf );

/**
 * Single write procedure. It sends one byte to a device.
 * \param slaveAddr the slave address
 * \param value the byte to send
 * \return 1
 */
uint8_t i2c0_write_single( uint8_t slaveAddr, uint8_t value );
/**
 * Single read procedure. It reads one byte from a device.
 * \param slaveAddr the slave address
 * \param value a pointer to the variable to store the value
 * \return 1
 */
uint8_t i2c0_read_single ( uint8_t slaveAddr, uint8_t *value );


#endif
