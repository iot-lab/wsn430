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
 * \addtogroup uart0
 * @{
 */
/**
 * \file
 * \brief MSP430 uart0 driver
 * \author Antoine Fraboulet <antoine.fraboulet@insa-lyon.fr>
 * \author Colin Chaballier
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
 * \date   2008
 * 
 **/

/**
 * @}
 */


#include <io.h>
#include <signal.h>
#include "uart0.h"

/**
 * \brief Macro waiting the end of a transmission using UART0.
 */
#define  UART0_WAIT_FOR_EOTx() \
  while ((U0TCTL & TXEPT) != TXEPT)

/**
 * \brief Macro waiting the end of a reception using UART0.
 */
#define  UART0_WAIT_FOR_EORx() \
  while ((IFG1 & URXIFG0) == 0)

/**
 * \brief Macro sending a byte using UART0.
 * \param x the byte to send
 */
#define USART0_TX(x) \
do { \
    U0TXBUF = x; \
    UART0_WAIT_FOR_EOTx(); \
} while(0)

/**
 * \brief Macro receiving a byte using UART0.
 * \param x the variable to store the received byte
 */
#define USART0_RX(x) \
do { \
    UART0_WAIT_FOR_EORx(); \
    x = U0RXBUF; \
} while(0)


/* Variables */
/**
 * \brief the callback pointer variable.
 */
static uart0_cb_t rx_char_cb;
static uart0_dma_cb_t dma_cb;
static volatile int16_t uart_tx_busy;

critical void uart0_init(uint16_t config){

  P3SEL |= (0x10 | 0x20);

  ME1   |= (UTXE0|URXE0);         //Enable USART0 transmiter and receiver (UART mode)
  U0CTL  = SWRST;                 //reset
  U0CTL  = CHAR | SWRST;          //init

  U0TCTL = SSEL1 | TXEPT;      //use SMCLK 
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
    case UART0_CONFIG_8MHZ_230400:
      // 230400 baud & SMCLK @ 8MHZ
      U0BR0  = 0x22;
      U0BR1  = 0x00;
      U0MCTL = 0xDD;
      break;
    case UART0_CONFIG_8MHZ_500000:
      // 500000 baud & SMCLK @ 8MHZ
      U0BR0  = 0x10;
      U0BR1  = 0x00;
      U0MCTL = 0x00;
      break;
    default:
      // 38400 baud & SMCLK @ 1MHZ
      U0BR0  = 0x1B;
      U0BR1  = 0;
      U0MCTL = 0x03;
      break;
  }
  
  U0CTL &= ~SWRST;
  
  // Enable USART0 receive interrupts
  IE1  |= URXIE0;   
  
  rx_char_cb = 0x0;
  dma_cb = 0x0;
  uart_tx_busy = 0;
}


int uart0_getchar_polling(void)
{
  int c;
  USART0_RX(c);
  return c;
}

critical int uart0_putchar(int c)
{
  // wait until tx not busy
  while (uart_tx_busy) ;
  
  uart_tx_busy = 1;
  USART0_TX(c);
  uart_tx_busy = 0;
  return c;
}

critical void uart0_stop(void)
{
  // wait until tx not busy
  while (uart_tx_busy) ;
  
  P3SEL &= ~(0x10 | 0x20);
  ME1  &= ~(UTXE0 | URXE0);
}

void uart0_register_callback(uart0_cb_t cb)
{
    rx_char_cb = cb;
}

void usart0irq(void);
/**
 * \brief the interrupt function
 */
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
        if ( rx_char_cb(dummy) )
        {
            LPM4_EXIT;
        }
    }
}

/* DMA section */
int uart0_dma_putchars(uint8_t* data, int16_t length) {
    if (uart_tx_busy) {
        // busy, can't start transfer
        return 0;
    }
    uart_tx_busy = 1;
    
    // configure DMA: channel 0 is UART TX
    DMACTL0 = DMA0TSEL_4;
    DMACTL1 = 0;
    
    // configure channel 0:
    DMA0CTL = DMADT_0 | // single transfer
                DMADSTINCR_0 | // destination does not increment (UTX BUFFER)
                DMASRCINCR_3 | // source address is incremented (user's buffer)
                DMADSTBYTE | // destination is byte
                DMASRCBYTE | // source is byte
                DMAIE; // interrupt enable on DMA transfer end
    
    // configure source address: user's buffer
    DMA0SA = (uint16_t)(data+1);
    // configure destination address: UART TX BUF
    DMA0DA = U0TXBUF_;
    // configure length
    DMA0SZ = length-1;
    
    // enable DMA
    DMA0CTL |= DMAEN;
    
    // write first byte to launch
    U0TXBUF = data[0];
    
    return 1;
}

void uart0_register_dma_callback(uart0_dma_cb_t cb) {
    dma_cb = cb;
}

void dmairq(void);
interrupt(DACDMA_VECTOR) dmairq(void) {
    if (DMA0CTL&DMAIFG) {
        DMA0CTL = 0;
        uart_tx_busy = 0;
        
        // DMA channel 0 transfer finished!
        if (dma_cb && dma_cb()) {
            LPM4_EXIT;
        }
    }
}
