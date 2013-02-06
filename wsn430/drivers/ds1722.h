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
 * \defgroup ds1722 DS1722 temperature sensor driver
 * \ingroup wsn430
 * @{
 * The DS1722 chip is a digital thermometer from MAXIM.
 * It communicates with the MSP430 via an SPI bus.
 *
 * This temperature sensor has two main modes of sampling:
 * one-shot and continuous; and a temperature resolution
 * between 8 and 12 bits.
 *
 * There are two 8bit registers in the chip (the LSB and MSB)
 * containing the latest sampled value that can be read from SPI.
 * When asked for continuous sampling,
 * the chip samples and updates the two register repeatedly,
 * whereas for a one-shot conversion it is done only once.
 *
 * \sa http://www.maxim-ic.com/quick_view2.cfm/qv_pk/2766
 */


/**
 * \file
 * \brief  DS1722 temperature sensor driver header
 * \author Colin Chaballier
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
 * \date   2008
 **/




#ifndef _DS1722_H_
#define _DS1722_H_

/**
 * \brief Initialize the SPI1 for transfer.
 *
 * This function must be called first before using any other one.
 */
void ds1722_init(void);

/**
 * \brief Set the temperature sensor resolution.
 * \param res the resolution, should be 8/9/10/11/12
 */
void ds1722_set_res(uint16_t res);

/**
 * \brief Command a single sampling to the sensor.
 */
void ds1722_sample_1shot(void);

/**
 * \brief Command a continuous sampling to the sensor.
 */
void ds1722_sample_cont(void);

/**
 * \brief Stop conversions and the sensor.
 */
void ds1722_stop(void);

/**
 * \brief Read the MSB of the latest sensor conversion.
 * \return the 8bit MSB of the conversion
 */
uint8_t ds1722_read_MSB(void);

/**
 * \brief Read the LSB of the latest sensor conversion.
 * \return the 8bit LSB of the conversion
 */
uint8_t ds1722_read_LSB(void);

/**
 * \brief Read the configuration register.
 * \return the 8bit value of the register
 */
uint8_t ds1722_read_cfg(void);

/**
 * \brief Write a value to the configuration register.
 * \param c the 8bit value
 */
void ds1722_write_cfg(uint8_t c);

#endif


/**
 * @}
 */
