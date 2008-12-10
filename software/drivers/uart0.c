/**
 *  \file   uart0.c
 *  \brief  MSP430 uart0 driver (for use on wsn430v1.3b et .4 boards)
 *  \author Colin Chaballier
 *  \date   2008
 **/


#include <io.h>
#include <signal.h>
#include "uart0.h"

/**
 * Macro waiting the end of a transmission using UART0.
 */
#define  UART0_WAIT_FOR_EOTx() while ((U0TCTL & TXEPT) != TXEPT)

/**
 * Macro waiting the end of a reception using UART0.
 */
#define  UART0_WAIT_FOR_EORx() while ((IFG1 & URXIFG0) == 0)

/**
 * Macro sending a byte using UART0.
 * \param x the byte to send
 */
#define USART0_TX(x) \
do { \
    U0TXBUF = x; \
    UART0_WAIT_FOR_EOTx(); \
} while(0)

/**
 * Macro receiving a byte using UART0.
 * \param x the variable to store the received byte
 */
#define USART0_RX(x) \
do { \
    UART0_WAIT_FOR_EORx(); \
    x = U0RXBUF; \
} while(0)


/* Variables */
static uart0_cb_t rx_char_cb;

void uart0_init(uint16_t config){

  P3SEL |= (0x10 | 0x20);

  ME1   |= (UTXE0|URXE0);           //Enable USART0 transmiter and receiver (UART mode)
  U0CTL  = SWRST;                 //reset
  U0CTL  = CHAR ;                  //init

  U0TCTL = SSEL_SMCLK|TXEPT;      //use SMCLK 
  U0RCTL = 0;
  
  switch (config)
  {
    case UART0_CONFIG_8MHZ_115200:
      // 115200 baud & SMCLK @ 8MHZ
      U0BR0  = 0x45;
      U0BR1  = 0x00;
      U0MCTL = 0xAA;
      break;
    case UART0_CONFIG_1MHZ_38400:
      // 38400 baud & SMCLK @ 1MHZ
      U0BR0  = 0x1A;
      U0BR1  = 0x00;
      U0MCTL = 0x00;
      break;
    case UART0_CONFIG_1MHZ_115200:
      // 115200 baud & SMCLK @ 1MHZ
      U0BR0  = 0x08;
      U0BR1  = 0x00;
      UMCTL0 = 0x5B;
      break;
    default:
      // 38400 baud & SMCLK @ 1MHZ
      U0BR0  = 0x1B;
      U0BR1  = 0;
      U0MCTL = 0x03;
      break;
  }
  
  // Enable USART0 receive interrupts
  IE1  |= URXIE0;   
  
  U0CTL &= ~SWRST;
  
  rx_char_cb = 0x0;
}


int uart0_getchar_polling(void)
{
  int c;
  USART0_RX(c);
  return c;
}

int uart0_putchar(int c)
{
  USART0_TX(c);
  return c;
}

void uart0_stop(void)
{
  P3SEL &= ~(0x10 | 0x20);
  ME1  &= ~(UTXE0 | URXE0);
}

void uart0_register_callback(uart0_cb_t cb)
{
    rx_char_cb = cb;
}

interrupt(USART0RX_VECTOR) usart0irq(void) {
    uint8_t dummy;
  
    /* Check status register for receive errors. */
    if(U0RCTL & RXERR) {
        /* Clear error flags by forcing a dummy read. */
        dummy = U0RXBUF;
    } 
    else if (rx_char_cb != 0x0)
    {
        /* if a callback has been registered, call it. */
        dummy = U0RXBUF;
        (*rx_char_cb)(dummy);
    }
    return;
}
