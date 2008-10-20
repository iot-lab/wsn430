/**
 *  \file   uart0.h
 *  \brief  MSP430 uart0 driver (for use on wsn430v1.3b et .4 boards)
 *  \author Colin Chaballier
 *  \date   2008
 **/


#ifndef _UART0_H_
#define _UART0_H_

#include <io.h>
#include <iomacros.h>
#include <stdio.h>

#define	UART0_WAIT_FOR_EOTx() while ((U0TCTL & TXEPT) != TXEPT)
#define	UART0_WAIT_FOR_EORx() while ((IFG1 & URXIFG0) == 0)

#define USART0_TX(x)           \
do {                           \
	U0TXBUF = x;           \
	UART0_WAIT_FOR_EOTx(); \
} while(0)

#define USART0_RX(x)            \
do {                            \
	UART0_WAIT_FOR_EORx();  \
	x = U0RXBUF;            \
} while(0)


extern void uart0_init();
extern int uart0_getchar_polling();
extern int uart0_putchar(int c);
extern void uart0_stop();


#endif

