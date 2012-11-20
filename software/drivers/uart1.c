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
 * \addtogroup uart1
 * @{
 */

/**
 * \file
 * \brief MSP430 uart1 driver header
 * \author Antoine Fraboulet <antoine.fraboulet@insa-lyon.fr>
 * \author Colin Chaballier
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
 * \date   2008
 **/

/**
 * @}
 */

#include <io.h>
#include <signal.h>
#include "uart1.h"

/**
 * \brief Macro waiting the end of a transmission using UART1.
 */
#define  UART1_WAIT_FOR_EOTx() while ((U1TCTL & TXEPT) != TXEPT)

/**
 * \brief Macro waiting the end of a reception using UART1.
 */
#define  UART1_WAIT_FOR_EORx() while ((IFG2 & URXIFG1) == 0)

/**
 * \brief Macro sending a byte using UART1.
 * \param x the byte to send
 */
#define USART1_TX(x) \
do { \
    U1TXBUF = x; \
    UART1_WAIT_FOR_EOTx(); \
} while(0)

/**
 * \brief Macro receiving a byte using UART1.
 * \param x the variable to store the received byte
 */
#define USART1_RX(x) \
do { \
    UART1_WAIT_FOR_EORx(); \
    x = U1RXBUF; \
} while(0)


/* Variables */
/**
 * \brief the callback pointer variable.
 */
static uart1_cb_t rx_char_cb;

critical void uart1_init(uint16_t config){

  P3SEL |= ( (0x1<<6) | (0x1<<7) ) ;

  ME2   |= (UTXE1|URXE1);          //Enable USART0 transmiter and receiver (UART mode)
  U1CTL  = SWRST;                  //reset
  U1CTL  = CHAR ;                  //init 8bit 1 bit stop No parity

  U1TCTL = SSEL1 | TXEPT;      //use SMCLK 
  U1RCTL = 0;
  
  switch (config)
  {
    case UART1_CONFIG_8MHZ_9600:
      // 9600 baud & SMCLK @ 8MHZ
      U1BR0  = 0x41;
      U1BR1  = 0x03;
      U1MCTL = 0x09;
      break;
    case UART1_CONFIG_8MHZ_19200:
      // 19200 baud & SMCLK @ 8MHZ
      U1BR0  = 0xA0;
      U1BR1  = 0x01;
      U1MCTL = 0x5B;
      break;
    case UART1_CONFIG_8MHZ_57600:
      // 19200 baud & SMCLK @ 8MHZ
      U1BR0  = 0x8A;
      U1BR1  = 0x00;
      U1MCTL = 0xEF;
      break;
    case UART1_CONFIG_8MHZ_115200:
      // 115200 baud & SMCLK @ 8MHZ
      U1BR0  = 0x45;
      U1BR1  = 0x00;
      U1MCTL = 0xAA;
      break;
    case UART1_CONFIG_1MHZ_38400:
      // 38400 baud & SMCLK @ 1MHZ
      U1BR0  = 0x1A;
      U1BR1  = 0x00;
      U1MCTL = 0x00;
      break;
    case UART1_CONFIG_1MHZ_115200:
      // 115200 baud & SMCLK @ 1MHZ
      U1BR0  = 0x08;
      U1BR1  = 0x00;
      U1MCTL = 0x5B;
      break;
    case UART1_CONFIG_1MHZ_4800:
      // 4800 baud & SMCLK @ 1MHZ
      U1BR0  = 0xD0;
      U1BR1  = 0x00;
      U1MCTL = 0x11;
      break;
    case UART1_CONFIG_1MHZ_57600:
      // 57600 baud & SMCLK @ 1MHZ
      U1BR0  = 0x11;
      U1BR1  = 0x00;
      U1MCTL = 0x52;
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

critical int uart1_putchar(int c)
{
  USART1_TX(c);
  return c;
}

critical void uart1_stop(void)
{
  P3SEL &= ~( (0x1<<6) | (0x1<<7));
  ME2  &= ~(UTXE1 | URXE1);
}

void uart1_register_callback(uart1_cb_t cb)
{
    rx_char_cb = cb;
}

void usart1irq(void);
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
        if ( rx_char_cb(dummy) )
        {
            LPM4_EXIT;
        }
    }
}
