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
 * \defgroup ds2411 DS2411 unique serial number driver
 * \ingroup wsn430
 * @{
 * The DS2411 is a unique serial number chip.
 * It is factory written with a garanteed unique 64bit number, 
 * that can be used to provide network addresses for example.
 *
 */

/**
 * \file
 * \brief  DS2411 unique ID driver header
 * \author Antoine Fraboulet <antoine.fraboulet@insa-lyon.fr>
 * \author Colin Chaballier
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
 **/


#ifndef DS2411_H
#define DS2411_H

/*
 *   MSB                                  LSB
 *   CRC : S5 : S4 : S3 : S2 : S1 : S0 : FAMILY
 */

/**
 * \brief Serial number format.
 */
typedef
union
{
  uint8_t raw[8]; /**< \brief 8byte array */
  struct
  {
    uint8_t crc; /**< \brief CRC byte */     /* MSB */
    uint8_t serial5; /**< \brief byte 5 */
    uint8_t serial4; /**< \brief byte 4 */
    uint8_t serial3; /**< \brief byte 3 */
    uint8_t serial2; /**< \brief byte 2 */
    uint8_t serial1; /**< \brief byte 1 */
    uint8_t serial0; /**< \brief byte 0 */
    uint8_t family; /**< \brief Family byte */  /* LSB */
  };
} ds2411_serial_number_t;

/**
 * \brief Serial number variable.
 * 
 * Variable where the serial number is stored after call
 * to ds2411_init() is made.
 */
extern ds2411_serial_number_t ds2411_id;

/**
 * \brief Initialize ds2411 component and read its value.
 * 
 * The serial number is stored in ds2411_id.
 * \return 1 if ok, 0 if error
 */
uint16_t ds2411_init(void);

#endif

/**
 * @}
 */
