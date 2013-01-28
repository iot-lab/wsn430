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
 * \addtogroup m25p80
 * @{
 */

/**
 * \file
 * \brief M25P80 flash memory driver.
 * \author Antoine Fraboulet <antoine.fraboulet@insa-lyon.fr>
 * \author Colin Chaballier
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
 * \date November 08
 */

/**
 * @}
 */

#include <io.h>
#include <signal.h>

#include "m25p80.h"
#include "spi1.h"

#define DUMMY 0x0

/**************************************************/
/** MSP430 <> M25P80 PORT *************************/
/**************************************************/

#define M25P80_PORT_WPR  P1OUT /* 1.2 write enable */
#define M25P80_PORT_H    P4OUT /* 4.7 Hold         */

/* on MSP430_PORT 1                 */
#define M25P80_PIN_PROTECT    2

/* on MSP430_PORT 4                 */
#define M25P80_PIN_CS         4
#define M25P80_PIN_HOLD       7

#define BITMASK(n)      (1 << n)

#define M25P80_BIT_HOLD    BITMASK(M25P80_PIN_HOLD)
#define M25P80_BIT_PROTECT BITMASK(M25P80_PIN_PROTECT)

/* hold is negated : active low */
/* Bit : 0 = hold               */
/*       1 = open               */
#define M25P80_HOLD_ENABLE()   do { M25P80_PORT_H &= ~M25P80_BIT_HOLD; } while(0)
#define M25P80_HOLD_DISABLE()  do { M25P80_PORT_H |=  M25P80_BIT_HOLD; } while(0)

/* write protect negated : active low */
/* Bit : 0 = Write protect on         */
/*       1 = Write protect off        */
#define M25P80_PROTECT_ENABLE()  do { M25P80_PORT_WPR &= ~M25P80_BIT_PROTECT; } while(0)
#define M25P80_PROTECT_DISABLE() do { M25P80_PORT_WPR |=  M25P80_BIT_PROTECT; } while(0)

/**************************************************/
/** M25P80 Instructions ****************************/
/**************************************************/

#define OPCODE_WREN      0x06u /* write enable         */
#define OPCODE_WRDI      0x04u /* */
#define OPCODE_RDSR      0x05u /* read status register */
#define OPCODE_WRSR      0x01u /* */
#define OPCODE_READ      0x03u
#define OPCODE_FAST_READ 0x0Bu
#define OPCODE_PP        0x02u
#define OPCODE_SE        0xd8u
#define OPCODE_BE        0xc7u
#define OPCODE_DP        0xb9u
#define OPCODE_RES       0xabu

#define M25P80_ELECTRONIC_SIGNATURE 0x13

/* msp430 is little endian */
struct __attribute__ ((packed)) m25p80_sr {
  uint8_t
    wip:1,     /* b0 */
    wel:1,
    bp0:1,
    bp1:1,
    bp2:1,
    unused1:1,
    unused2:1, 
    srwd:1;    /* b7 */
};
typedef struct m25p80_sr m25p80_sr_t;
 
/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

critical uint8_t m25p80_init()
{
  // Configure as GPIO outputs(CSn,Hold)
  P4DIR  |=  (M25P80_BIT_HOLD); 
  P4SEL  &= ~(M25P80_BIT_HOLD);

  // Configure as GPIO outputs(write)
  P1DIR  |=  (M25P80_BIT_PROTECT);
  P1SEL  &= ~(M25P80_BIT_PROTECT);
  
  M25P80_HOLD_DISABLE();
  M25P80_PROTECT_DISABLE();
  
  spi1_init();
  
  return m25p80_get_signature();
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

critical uint8_t m25p80_get_signature()
{
  uint8_t ret = 0;
  spi1_select(SPI1_M25P80);
  
  spi1_write_single(OPCODE_RES);
  spi1_write_single(DUMMY);
  spi1_write_single(DUMMY);
  spi1_write_single(DUMMY);
  
  ret = spi1_read_single();
  
  spi1_deselect(SPI1_M25P80);
  return ret;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

/* M25p80 is programmed using Hold, Write and Clock 
 * other information is data or command 
 */

critical uint8_t m25p80_get_state()
{ 
  uint8_t ret = 0;
  spi1_select(SPI1_M25P80);
  
  spi1_write_single(OPCODE_RDSR);
  ret = spi1_read_single();
  
  spi1_deselect(SPI1_M25P80);
  return ret; 
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

critical void m25p80_wakeup(void)
{
  spi1_select(SPI1_M25P80);
  spi1_write_single(OPCODE_RES);
  spi1_deselect(SPI1_M25P80);
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

critical void m25p80_power_down(void)
{ 
  spi1_select(SPI1_M25P80);
  spi1_write_single(OPCODE_DP);
  spi1_deselect(SPI1_M25P80);
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

static void m25p80_block_wip()
{
  #define WIP 0x01
  
  uint8_t state;
  
  spi1_select(SPI1_M25P80);
  
  spi1_write_single(OPCODE_RDSR);
  state = spi1_read_single();
  
  while(state & WIP)
  {
    spi1_write_single(OPCODE_RDSR);
    state = spi1_read_single();
  }
  
  spi1_deselect(SPI1_M25P80);
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

static void m25p80_write_enable()
{
  spi1_select(SPI1_M25P80);
  spi1_write_single(OPCODE_WREN);
  spi1_deselect(SPI1_M25P80);
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

critical void m25p80_erase_sector(uint8_t ix)
{
  m25p80_block_wip();
  m25p80_write_enable();
  spi1_select(SPI1_M25P80);
  spi1_write_single(OPCODE_SE);
  spi1_write_single(ix & 0xff);
  spi1_write_single(0  & 0xff);
  spi1_write_single(0  & 0xff);
  spi1_deselect(SPI1_M25P80);
  
  m25p80_block_wip();
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

critical void m25p80_erase_bulk()
{
  m25p80_block_wip();
  
  m25p80_write_enable();
  
  spi1_select(SPI1_M25P80);
  spi1_write_single(OPCODE_BE);
  spi1_deselect(SPI1_M25P80);
  
  m25p80_block_wip();
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

critical void m25p80_write(uint32_t addr, uint8_t *buffer, uint16_t size)
{
    
    uint8_t *p = buffer, *_end;
    uint32_t end = addr + size;
    uint32_t i, next_page;
    int16_t len;
    
    for(i = addr; i < end;) {
        next_page = (i | 0xff) + 1;
        if(next_page > end)
        {
            next_page = end;
        }
        
        _end = p + (next_page-i);
        
        m25p80_block_wip();
        m25p80_write_enable();
        spi1_select(SPI1_M25P80);
        spi1_write_single(OPCODE_PP);
        spi1_write_single((i >> 16) & 0xff);
        spi1_write_single((i >>  8) & 0xff);
        spi1_write_single((i >>  0) & 0xff);
        
        len = (_end-p);
        spi1_write(p, len);
        
        p = _end;
        spi1_deselect(SPI1_M25P80);
        
        i = next_page;
    }
    
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */


critical void m25p80_read(uint32_t addr, uint8_t *buffer, uint16_t size)
{
  m25p80_block_wip();
  spi1_select(SPI1_M25P80);

  spi1_write_single(OPCODE_READ);
  spi1_write_single((addr >> 16) & 0xff);
  spi1_write_single((addr >>  8) & 0xff);
  spi1_write_single((addr >>  0) & 0xff);

  spi1_read(buffer, size);
  
  spi1_deselect(SPI1_M25P80);
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */
