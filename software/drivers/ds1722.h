
/**
 *  \file   ds1722.h
 *  \brief  DS1722 temperature sensor driver (available on wsn430v1.3b et .4 boards)
 *  \author Colin Chaballier
 *  \date   2008
 **/

#ifndef _DS1722_H_
#define _DS1722_H_

/**
 * Initialize the SPI1 for transfer.
 */
void ds1722_init(void);

/**
 * Set the temperature sensor resolution.
 * \param res the resolution, should be 8/9/10/11/12
 */
void ds1722_set_res(uint16_t res);

/**
 * Command a single sampling to the sensor.
 */
void ds1722_sample_1shot(void);

/** 
 * Command a continuous sampling to the sensor.
 */
void ds1722_sample_cont(void);

/** 
 * Stop conversions and the sensor.
 */
void ds1722_stop(void);

/**
 * Read the MSB of the latest sensor conversion.
 * \return the 8bit MSB of the conversion
 */
uint8_t ds1722_read_MSB(void);

/**
 * Read the LSB of the latest sensor conversion.
 * \return the 8bit LSB of the conversion
 */
uint8_t ds1722_read_LSB(void);

/**
 * Read the configuration register.
 * \return the 8bit value of the register
 */
uint8_t ds1722_read_cfg(void);

/**
 * Write a value to the configuration register.
 * \param c the 8bit value
 */
void ds1722_write_cfg(uint8_t c);

#endif

