/**
 *  \file   uart1.c
 *  \brief  MSP430 uart1 driver (for use on wsn430v1.3b et .4 boards)
 *  \author Colin Chaballier
 *  \date   2008
 **/


#include <io.h>
#include <signal.h>
#include "uart1.h"

/**
 * Macro waiting the end of a transmission using UART1.
 */
#define  UART1_WAIT_FOR_EOTx() while ((U1TCTL & TXEPT) != TXEPT)

/**
 * Macro waiting the end of a reception using UART1.
 */
#define  UART1_WAIT_FOR_EORx() while ((IFG2 & URXIFG1) == 0)

/**
 * Macro sending a byte using UART1.
 * \param x the byte to send
 */
#define USART1_TX(x) \
do { \
    U1TXBUF = x; \
    UART1_WAIT_FOR_EOTx(); \
} while(0)

/**
 * Macro receiving a byte using UART1.
 * \param x the variable to store the received byte
 */
#define USART1_RX(x) \
do { \
    UART1_WAIT_FOR_EORx(); \
    x = U1RXBUF; \
} while(0)


/* Variables */
static uart1_cb_t rx_char_cb;

void uart1_init(uint16_t config){

  P3SEL |= ( (0x1<<6) | (0x1<<7) ) ;

  ME2   |= (UTXE1|URXE1);           //Enable USART0 transmiter and receiver (UART mode)
  U1CTL  = SWRST;                 //reset
  U1CTL  = CHAR ;                  //init

  U1TCTL = SSEL_SMCLK | TXEPT;      //use SMCLK 
  U1RCTL = 0;
  
  switch (config)
  {
    case UART1_CONFIG_8MHZ_115200:
      // 115200 baud & SMCLK @ 8MHZ
      U1BR0  = 0x45;
      U1BR1  = 0x00;
      U1MCTL = 0xAA;
      break;
    case UART1_CONFIG_1MHZ_4800:
      // 4800 baud & SMCLK @ 1MHZ
      U1BR0  = 0xD0;
      U1BR1  = 0x00;
      U1MCTL = 0x11;
      break;
    case UART1_CONFIG_1MHZ_38400:
      // 38400 baud & SMCLK @ 1MHZ
      U1BR0  = 0x1A;
      U1BR1  = 0x00;
      U1MCTL = 0x00;
      break;
    case UART1_CONFIG_1MHZ_57600:
      // 56700 baud & SMCLK @ 1MHZ
      U1BR0  = 0x11;
      U1BR1  = 0x00;
      U1MCTL = 0x52;
      break;
    case UART1_CONFIG_1MHZ_115200:
      // 115200 baud & SMCLK @ 1MHZ
      U1BR0  = 0x08;
      U1BR1  = 0x00;
      U1MCTL = 0x5B;
      break;
    default:
      // 38400 baud & SMCLK @ 1MHZ
      U1BR0  = 0x1B;
      U1BR1  = 0;
      U1MCTL = 0x03;
      break;
  }
  
  // Enable USART0 receive interrupts
  IE2  |= URXIE1;
  
  U1CTL &= ~SWRST;
  
  rx_char_cb = 0x0;
}


int uart1_getchar_polling(void)
{
  int c;
  USART1_RX(c);
  return c;
}

int uart1_putchar(int c)
{
  USART1_TX(c);
  return c;
}

void uart1_stop(void)
{
  P3SEL &= ~( (0x1<<6) | (0x1<<7));
  ME2  &= ~(UTXE1 | URXE1);
}

void uart1_register_callback(uart1_cb_t cb)
{
    rx_char_cb = cb;
}

interrupt(USART1RX_VECTOR) usart1irq(void) {
    uint8_t dummy;
  
    /* Check status register for receive errors. */
    if(U1RCTL & RXERR) {
        /* Clear error flags by forcing a dummy read. */
        dummy = U1RXBUF;
    } 
    else if (rx_char_cb != 0x0)
    {
        /* if a callback has been registered, call it. */
        dummy = U1RXBUF;
        (*rx_char_cb)(dummy);
    }
    return;
}
