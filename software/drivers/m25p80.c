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
  
  SPI1_INIT();
  
  return m25p80_get_signature();
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

critical uint8_t m25p80_get_signature()
{
  uint8_t ret = 0;
  SPI1_M25P80_ENABLE();
  
  SPI1_TX(OPCODE_RES);
  SPI1_TX(DUMMY);
  SPI1_TX(DUMMY);
  SPI1_TX(DUMMY);
  
  SPI1_RX(ret);
  
  SPI1_M25P80_DISABLE();
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
  SPI1_M25P80_ENABLE();
  
  SPI1_TX(OPCODE_RDSR);
  SPI1_RX(ret);
  
  SPI1_M25P80_DISABLE();
  return ret; 
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

critical void m25p80_wakeup(void)
{
  SPI1_M25P80_ENABLE();
  SPI1_TX(OPCODE_RES);
  SPI1_M25P80_DISABLE();
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

critical void m25p80_power_down(void)
{ 
  SPI1_M25P80_ENABLE();
  SPI1_TX(OPCODE_DP);
  SPI1_M25P80_DISABLE();
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

static void m25p80_block_wip()
{
  #define WIP 0x01
  
  uint8_t state;
  
  SPI1_M25P80_ENABLE();
  
  SPI1_TX(OPCODE_RDSR);
  SPI1_RX(state);
  
  while(state & WIP)
  {
    SPI1_TX(OPCODE_RDSR);
    SPI1_RX(state);
  }
  
  SPI1_M25P80_DISABLE();
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

static void m25p80_write_enable()
{
  SPI1_M25P80_ENABLE();
  SPI1_TX(OPCODE_WREN);
  SPI1_M25P80_DISABLE();
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

critical void m25p80_erase_sector(uint8_t index)
{
  m25p80_block_wip();
  m25p80_write_enable();
  SPI1_M25P80_ENABLE();
  SPI1_TX(OPCODE_SE);
  SPI1_TX(index & 0xff);
  SPI1_TX(0     & 0xff);
  SPI1_TX(0     & 0xff);
  SPI1_M25P80_DISABLE();
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

critical void m25p80_erase_bulk()
{
  uint8_t state;
  m25p80_block_wip();
  
  m25p80_write_enable();
  
  SPI1_M25P80_ENABLE();
  SPI1_TX(OPCODE_BE);
  SPI1_M25P80_DISABLE();
  
  SPI1_M25P80_ENABLE();
  SPI1_TX(OPCODE_RDSR);
  SPI1_RX(state);
  SPI1_M25P80_DISABLE();
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

critical void m25p80_save_page(uint16_t page, const uint8_t *buffer) 
{
  uint16_t i = 0;

  m25p80_block_wip();
  m25p80_write_enable();
  SPI1_M25P80_ENABLE();

  SPI1_TX(OPCODE_PP);
  SPI1_TX((page >> 8) & 0xff);
  SPI1_TX((page >> 0) & 0xff);
  SPI1_TX((        0) & 0xff);

  for(i = 0; i < M25P80_PAGE_SIZE; i++)
    {
      SPI1_TX(buffer[i]);
    }

  SPI1_M25P80_DISABLE();
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */


critical void m25p80_load_page(uint16_t page, uint8_t *buffer) 
{
  uint16_t i = 0;
  
  m25p80_block_wip();
  SPI1_M25P80_ENABLE();

  SPI1_TX(OPCODE_READ);
  SPI1_TX((page >> 8) & 0xff);
  SPI1_TX((page >> 0) & 0xff);
  SPI1_TX((        0) & 0xff);

  for(i = 0; i < M25P80_PAGE_SIZE; i++)
    {
       SPI1_RX(buffer[i]);
    }

  SPI1_M25P80_DISABLE();
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */
