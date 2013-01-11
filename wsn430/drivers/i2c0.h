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
 * \defgroup i2c0 I2C0 driver
 * @{
 * 
 * The I2C0 driver enables I2C communications between the WSN430
 * and other devices using the MSP430 hardware USART0 port.
 *
 */

/**
 * \file
 * \brief I2C_0 driver header
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
 * \date November 08
 */

/**
 * @}
 */

/**
 * @}
 */

#ifndef _I2C0_H_
#define _I2C0_H_

/**
 * \brief Initialize the I2C USART module.
 * 
 * This function should be called before any other i2c function.
 */
void i2c0_init(void);

/**
 * \brief Read procedure.
 * 
 * It sends a register address to a slave device, then receives
 * a given number of bytes from it.
 * \param slaveAddr the slave address.
 * \param regAddr the register start address.
 * \param len the number of bytes to read.
 * \param buf an array pointer for storing the read values.
 * \return 1
 */
uint8_t i2c0_read (uint8_t slaveAddr, uint8_t regAddr, uint16_t len, uint8_t* buf );

/**
 * \brief Write procedure.
 * 
 * It sends a register address to a slave device,
 * then sends a given number of bytes to it.
 * \param slaveAddr the slave address
 * \param regAddr the register start address
 * \param len the number of bytes to write
 * \param buf an array pointer with the values to send
 * \return 1
 */
uint8_t i2c0_write(uint8_t slaveAddr, uint8_t regAddr, uint16_t len, uint8_t* buf );
uint8_t i2c0_raw_write(uint8_t slaveAddr, uint16_t len, uint8_t* buf );
/**
 * \brief Single write procedure.
 * 
 * It sends one byte to a device.
 * \param slaveAddr the slave address
 * \param value the byte to send
 * \return 1
 */
uint8_t i2c0_write_single( uint8_t slaveAddr, uint8_t value );
/**
 * \brief Single read procedure.
 * 
 * It reads one byte from a device.
 * \param slaveAddr the slave address
 * \param value a pointer to the variable to store the value
 * \return 1
 */
uint8_t i2c0_read_single ( uint8_t slaveAddr, uint8_t *value );


#endif
