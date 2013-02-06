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
 * \addtogroup ds1722
 * @{
 */

/**
 * \file
 * \brief  DS1722 temperature sensor driver
 * \author Colin Chaballier
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
 * \date   2008
 **/

/**
 * @}
 */

#include <io.h>
#include "ds1722.h"
#include "spi1.h"

#define REG_CONF 0x0
#define REG_LSB  0x1
#define REG_MSB  0x2

#define WRITE_REG 0x80

#define CONF_MASK 0xE0
#define CONF_1SHOT 0x10
#define CONF_RES_MASK 0x0E
#define CONF_SD 0x1

static void inline micro_wait(register uint16_t n)
{
  /* MCLK is running 8MHz, 1 cycle = 125ns    */
  /* n=1 -> waiting = 4*125ns = 500ns         */

  /* MCLK is running 4MHz, 1 cycle = 250ns    */
  /* n=1 -> waiting = 4*250ns = 1000ns        */

    __asm__ __volatile__ (
    "1: \n"
    " dec  %[n] \n"      /* 2 cycles */
    " jne  1b \n"        /* 2 cycles */
        : [n] "+r"(n));
}

static uint8_t resolution = 0x2;

critical void ds1722_init(void)
{
    spi1_init();
}

void ds1722_set_res(uint16_t res)
{
  switch (res)
  {
    case 8: resolution = 0x0; break;
    case 9: resolution = 0x2; break;
    case 10:resolution = 0x4; break;
    case 11:resolution = 0x6; break;
    case 12:resolution = 0x8; break;
    default : resolution = 0x2;
  }
}

critical void ds1722_sample_1shot(void)
{
  uint8_t reg;
  reg = CONF_MASK | CONF_1SHOT | (resolution&CONF_RES_MASK) | CONF_SD;

  ds1722_write_cfg(reg);
}

critical void ds1722_sample_cont(void)
{
  uint8_t reg;
  reg = CONF_MASK | (resolution&CONF_RES_MASK);

  ds1722_write_cfg(reg);
}

critical void ds1722_stop(void)
{
  uint8_t reg;
  reg = CONF_MASK | (resolution&CONF_RES_MASK) | CONF_SD;
  reg = 0xE1 | resolution;

  ds1722_write_cfg(reg);
}

critical uint8_t ds1722_read_MSB(void)
{
  uint8_t temp;

  spi1_select(SPI1_DS1722);
  spi1_write_single(REG_MSB);
  temp = spi1_read_single();
  spi1_deselect(SPI1_DS1722);

  return temp;
}

critical uint8_t ds1722_read_LSB(void)
{
  uint8_t temp;

  spi1_select(SPI1_DS1722);
  spi1_write_single(REG_LSB);
  temp = spi1_read_single();
  spi1_deselect(SPI1_DS1722);

  return temp;
}


critical uint8_t ds1722_read_cfg(void)
{
  uint8_t temp;

  spi1_select(SPI1_DS1722);
  spi1_write_single(REG_CONF);
  temp = spi1_read_single();
  spi1_deselect(SPI1_DS1722);

  return temp;
}

critical void ds1722_write_cfg(uint8_t c)
{
  spi1_select(SPI1_DS1722);
  spi1_write_single(REG_CONF | WRITE_REG);
  spi1_write_single(c);
  spi1_deselect(SPI1_DS1722);
}
