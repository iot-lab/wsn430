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
 * \defgroup tsl2550 TSL2550 light sensor driver
 * \ingroup wsn430
 * @{
 * The TSL2550 is a light sensor communicating with the MSP430
 * via I2C on the USART0 port.
 * 
 * Once powered-up, the chip continuously samples
 * the ambient light with two photodiodes, that need
 * to be further processed in order to get a light power
 * value in lux. See the datasheet for more details. 
 */

/**
 * \file
 * \brief  TSL2550 light sensor driver header
 * \author Colin Chaballier
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
 * \date   2008
 **/

#ifndef _TSL2550_H_
#define _TSL2550_H_

/**
 * \brief Configure IO pins and USART0 for I2C.
 * 
 * This should be called before any other driver function.
 */
void tsl2550_init(void);

/**
 * \brief Put the device in Power Down mode.
 */
void tsl2550_powerdown(void);

/**
 * \brief Put the device in Power Up mode and read the command register.
 * \return the command register, should be 0x03.
 */
uint8_t tsl2550_powerup(void);

/**
 * \brief Set extended conversion mode.
 */
void tsl2550_set_extended(void);

/**
 * \brief Reset the device to standard conversion mode.
 */
void tsl2550_set_standard(void);

/*
 * The ADC output values are composed as follows:
 * |  B7   |  B6  |  B5  |  B4  |  B3  |  B2  |  B1  |  B0  |
 * | valid |     CHORD bits     |        STEP bits          |
 */

/**
 * \brief Read the channel 0 conversion value. (Visible+Infrared)
 * \return the read value
 */
uint8_t tsl2550_read_adc0(void);

/**
 * \brief Read the channel 1 conversion value. (Infrared only)
 * \return the read value
 */
uint8_t tsl2550_read_adc1(void);
#endif

/**
 * @}
 */
