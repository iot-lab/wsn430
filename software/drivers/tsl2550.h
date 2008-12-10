/**
 *  \file   tsl2550.h
 *  \brief  TSL2550 light sensor driver (available on wsn430v1.3b et .4 boards)
 *  \author Colin Chaballier
 *  \date   2008
 **/

#ifndef _TSL2550_H_
#define _TSL2550_H_

/**
 * Configure IO pins and USART0 for I2C.
 */
void tsl2550_init(void);

/**
 * Put the device in Power Down mode.
 */
void tsl2550_powerdown(void);

/**
 * Put the device in Power Up mode and read the command register.
 * \return the command register, should be 0x03.
 */
uint8_t tsl2550_powerup(void);

/**
 * Set extended conversion mode.
 */
void tsl2550_set_extended(void);

/**
 * Reset the device to standard conversion mode.
 */
void tsl2550_set_standard(void);

/*
 * The ADC output values are composed as follows:
 * |  B7   |  B6  |  B5  |  B4  |  B3  |  B2  |  B1  |  B0  |
 * | valid |     CHORD bits     |        STEP bits          |
 */

/**
 * Read the channel 0 conversion value. (Visible+Infrared)
 * \return the read value
 */
uint8_t tsl2550_read_adc0(void);

/**
 * Read the channel 1 conversion value. (Infrared only)
 * \return the read value
 */
uint8_t tsl2550_read_adc1(void);
#endif

