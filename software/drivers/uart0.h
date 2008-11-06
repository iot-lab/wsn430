/**
 *  \file   uart0.h
 *  \brief  MSP430 uart0 driver (for use on wsn430v1.3b et .4 boards)
 *  \author Colin Chaballier
 *  \date   2008
 **/


#ifndef _UART0_H_
#define _UART0_H_

typedef void (*uart0_cb_t)(uint8_t c);

#define UART0_CONFIG_8MHZ_115200 0
#define UART0_CONFIG_1MHZ_38400  1

/**
 * Configure the USART0 for UART use.
 * \param config configuration to use depending
 * on SMCLK speed and desired baudrate
 */
void uart0_init(uint16_t config);

/**
 * Wait until a character is read and return it.
 * \return the character read
 */
int uart0_getchar_polling();

/**
 * Send a character.
 * \param c the character to send
 * \return the same sent character
 */
int uart0_putchar(int c);

/**
 * Stop the peripheral.
 */
void uart0_stop();

/**
 * Register a callback that will be called every time
 * a char is received.
 * \param the callback function pointer
 */
void uart0_register_callback(uart0_cb_t cb);

#endif

