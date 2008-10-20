/**
 *  \file   uart0.c
 *  \brief  MSP430 uart0 driver (for use on wsn430v1.3b et .4 boards)
 *  \author Colin Chaballier
 *  \date   2008
 **/



#include "uart0.h"


void uart0_init(){

  P3SEL |= (0x10 | 0x20);

  ME1  |= UTXE0|URXE0;           //Enable USART0 transmiter and receiver (UART mode)
  U0CTL  = SWRST;                 //reset
  U0CTL  = CHAR ;                  //init

  U0TCTL = SSEL_SMCLK|TXEPT;      //use SMCLK 
  U0RCTL = 0;

// 115200 baud & SMCLK @ 1MHZ
  //~ U0BR1  = 0;
  //~ U0BR0  = 0x09;
  //~ U0MCTL = 0x10;

// 38400 baud & SMCLK @ 1MHZ
  U0BR1  = 0;
  U0BR0  = 0x1B;
  U0MCTL = 0x03;
  
  U0CTL &= ~SWRST;
}


int uart0_getchar_polling()
{
  int c;
  USART0_RX(c);
  return c;
}

int uart0_putchar(int c)
{
  USART0_TX(c);
  return (unsigned char)c;
}

void uart0_stop()
{
  P3SEL &= ~(0x10 | 0x20);
  ME1  &= ~(UTXE0 | URXE0);
}

