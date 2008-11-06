/**
 *  \file   ds1722.c
 *  \brief  DS1722 temperature sensor driver (available on wsn430v1.3b et .4 boards)
 *  \author Colin Chaballier
 *  \date   2008
 **/

#include <io.h>
#include "ds1722.h"
#include "spi1.h"

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

void ds1722_init(void)
{
    SPI1_INIT();
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

void ds1722_sample_1shot(void)
{
  uint8_t reg;
  reg = 0xF1 | resolution;
  
  SPI1_DS1722_ENABLE();
  micro_wait(1);
  SPI1_TX(0x80);
  SPI1_TX(reg);
  SPI1_DS1722_DISABLE();  
}

void ds1722_sample_cont(void)
{
  uint8_t reg;
  reg = 0xF0 | resolution;
  
  SPI1_DS1722_ENABLE();
  micro_wait(1);
  SPI1_TX(0x80);
  SPI1_TX(reg);
  
  SPI1_DS1722_DISABLE();  
}

void ds1722_stop(void)
{
  uint8_t reg;
  reg = 0xE1 | resolution;
  
  SPI1_DS1722_ENABLE();
  micro_wait(1);
  SPI1_TX(0x80);
  SPI1_TX(reg);
  SPI1_DS1722_DISABLE();  
}

uint8_t ds1722_read_MSB(void)
{
  uint8_t temp;
  
  SPI1_DS1722_ENABLE();
  micro_wait(1);
  SPI1_TX(0x2);
  SPI1_RX(temp);
  
  SPI1_DS1722_DISABLE();
  
  return temp;
}

uint8_t ds1722_read_LSB(void)
{
  uint8_t temp;
  
  SPI1_DS1722_ENABLE();
  micro_wait(1);
  SPI1_TX(0x1);
  SPI1_RX(temp);
  SPI1_DS1722_DISABLE();
  
  return temp;
}


uint8_t ds1722_read_cfg(void)
{
  uint8_t c;
  SPI1_DS1722_ENABLE();
  micro_wait(1);
  SPI1_TX(0x00);
  SPI1_RX(c);
  SPI1_DS1722_DISABLE();
  return c;
}

void ds1722_write_cfg(uint8_t c)
{
  SPI1_DS1722_ENABLE();
  micro_wait(1);
  SPI1_TX(0x80);
  SPI1_TX(c);
  SPI1_DS1722_DISABLE();
}
