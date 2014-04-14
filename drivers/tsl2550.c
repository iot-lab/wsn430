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
 * \addtogroup tsl2550
 * @{
 */

/**
 * \file   tsl2550.c
 * \brief  TSL2550 light sensor driver
 * \author Colin Chaballier
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
 * \date   2008
 **/

/**
 * @}
 */

#include <io.h>
#include "tsl2550.h"
#include "i2c0.h"

/**
 * \name TSL2550 commands constants
 * @{
 */
#define TSL_ADDR     0x39
#define TSL_CMD_PD   0x00 // power down
#define TSL_CMD_PU   0x03 // power up and read config.
#define TSL_CMD_EXT  0x1D // set extended mode
#define TSL_CMD_RST  0x18 // reset to standard mode
#define TSL_CMD_ADC0 0x43 // read ADC0 value
#define TSL_CMD_ADC1 0x83 // read ADC1 value
/**
 * @}
 */


critical void tsl2550_init(void)
{
    // configure TSL supply
    P4SEL &= ~(1<<5);
    P4DIR |=  (1<<5);
    P4OUT |=  (1<<5);

    i2c0_init();
}

critical uint8_t tsl2550_powerup(void)
{
    uint8_t readValue;
    i2c0_write_single(TSL_ADDR, TSL_CMD_PU);
    i2c0_read_single(TSL_ADDR, &readValue);
    return readValue;
}

critical void tsl2550_powerdown(void)
{
    i2c0_write_single(TSL_ADDR, TSL_CMD_PD);
}

critical void tsl2550_set_extended()
{
    i2c0_write_single(TSL_ADDR, TSL_CMD_EXT);
}

critical void tsl2550_set_standard()
{
    i2c0_write_single(TSL_ADDR, TSL_CMD_RST);
}

critical uint8_t tsl2550_read_adc0(void)
{
    uint8_t readValue;
    i2c0_write_single(TSL_ADDR, TSL_CMD_ADC0);
    i2c0_read_single(TSL_ADDR, &readValue);
    return readValue;
}

critical uint8_t tsl2550_read_adc1(void)
{
    uint8_t readValue;
    i2c0_write_single(TSL_ADDR, TSL_CMD_ADC1);
    i2c0_read_single(TSL_ADDR, &readValue);
    return readValue;
}
