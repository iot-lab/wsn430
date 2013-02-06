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
 * \addtogroup cc1101
 * @{
 */

/**
 * \file
 * \brief CC1101 driver implementation
 * \author Guillaume Chelius <guillaume.chelius@inria.fr>
 * \author Antoine Fraboulet <antoine.fraboulet@insa-lyon.fr>
 * \author Colin Chaballier
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
 * \date October 08
 */

/**
 * @}
 */

#include <io.h>
#include <signal.h>

#include "spi1.h"

#include "cc1101.h"
#include "cc1101_gdo.h"
#include "cc1101_globals.h"

static uint16_t (*gdo0_cb)(void);
static uint16_t (*gdo2_cb)(void);

void inline micro_delay(register unsigned int n)
{
    __asm__ __volatile__ (
  "1: \n"
  " dec %[n] \n"
  " jne 1b \n"
        : [n] "+r"(n));
}

void cc1101_reinit(void)
{
    spi1_init();
}

critical void cc1101_init(void)
{
  gdo0_cb = 0x0;
  gdo2_cb = 0x0;
  
  spi1_init();
  GDO_INIT();
  
  spi1_select(SPI1_CC1101);
  spi1_deselect(SPI1_CC1101);
  spi1_select(SPI1_CC1101);
  spi1_deselect(SPI1_CC1101);
  micro_delay(80);
  spi1_select(SPI1_CC1101);
  while (spi1_read_somi()) ;
  spi1_write_single(CC1101_STROBE_SRES | CC1101_ACCESS_STROBE);
  while (spi1_read_somi()) ;
  spi1_deselect(SPI1_CC1101);
  
  
  // write default frequency : 868MHz
  cc1101_write_reg(CC1101_REG_FREQ2, 0x20);
  cc1101_write_reg(CC1101_REG_FREQ1, 0x25);
  cc1101_write_reg(CC1101_REG_FREQ0, 0xED);
  
  // value from SmartRF
  cc1101_write_reg(CC1101_REG_DEVIATN, 0x0);
}

critical uint8_t cc1101_read_reg(uint8_t addr)
{
  uint8_t reg;
  spi1_select(SPI1_CC1101);
  spi1_write_single(addr | CC1101_ACCESS_READ);
  reg = spi1_read_single();
  spi1_deselect(SPI1_CC1101);
  return reg;
}

critical void cc1101_write_reg(uint8_t addr, uint8_t value)
{
  spi1_select(SPI1_CC1101);
  spi1_write_single(addr | CC1101_ACCESS_WRITE);
  spi1_write_single(value);
  spi1_deselect(SPI1_CC1101);
}

critical uint8_t cc1101_strobe_cmd(uint8_t cmd)
{
  uint8_t ret;
  spi1_select(SPI1_CC1101);
  ret = spi1_write_single(cmd | CC1101_ACCESS_STROBE);
  spi1_deselect(SPI1_CC1101);
  return ret;
}

critical void cc1101_fifo_put(uint8_t* buffer, uint16_t length)
{
  spi1_select(SPI1_CC1101);
  spi1_write_single(CC1101_DATA_FIFO_ADDR | CC1101_ACCESS_WRITE_BURST);
  spi1_write(buffer, length);
  spi1_deselect(SPI1_CC1101);
}

critical void cc1101_fifo_get(uint8_t* buffer, uint16_t length)
{
  spi1_select(SPI1_CC1101);
  spi1_write_single(CC1101_DATA_FIFO_ADDR | CC1101_ACCESS_READ_BURST);
  spi1_read(buffer, length);
  spi1_deselect(SPI1_CC1101);
}

critical uint8_t cc1101_read_status(uint8_t addr)
{
  return cc1101_read_reg(addr | CC1101_ACCESS_STATUS);
}


void cc1101_gdo0_register_callback(uint16_t (*cb)(void))
{
  gdo0_cb = cb;
}

void cc1101_gdo2_register_callback(uint16_t (*cb)(void))
{
  gdo2_cb = cb;
}

#define WAIT_STATUS(status) \
    while ( (cc1101_cmd_nop() & CC1101_STATUS_MASK) != status) ;

void cc1101_cmd_calibrate(void)
{
  cc1101_cmd_idle();
  cc1101_strobe_cmd(CC1101_STROBE_SCAL);

  WAIT_STATUS(CC1101_STATUS_IDLE);
}

void cc1101_cmd_idle(void)
{
  uint8_t state;

  state = cc1101_cmd_nop() & CC1101_STATUS_MASK;
  switch (state)
  {
    case CC1101_STATUS_RXFIFO_OVERFLOW:
      cc1101_cmd_flush_rx();
      break;
    case CC1101_STATUS_TXFIFO_UNDERFLOW:
      cc1101_cmd_flush_tx();
      break;
    default:
      cc1101_strobe_cmd(CC1101_STROBE_SIDLE);
  }

  while ((cc1101_cmd_nop() & CC1101_STATUS_MASK) != CC1101_STATUS_IDLE) {
    cc1101_strobe_cmd(CC1101_STROBE_SIDLE);
  }

}


void port1irq(void);
/**
 * Interrupt service routine for PORT1.
 * Used for handling CC1101 interrupts triggered on
 * the GDOx pins.
 */
interrupt(PORT1_VECTOR) port1irq(void)
{
  if (P1IFG & GDO0_PIN)
  {
    GDO0_INT_CLEAR();
    if (gdo0_cb != 0x0)
    {
      if (gdo0_cb())
      {
          LPM4_EXIT;
      }
    }
  }

  if (P1IFG & GDO2_PIN)
  {
    GDO2_INT_CLEAR();
    if (gdo2_cb != 0x0)
    {
      if (gdo2_cb())
      {
          LPM4_EXIT;
      }
    }
  } 
}

