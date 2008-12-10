/**
 *  \file   uart1.h
 *  \brief  MSP430 uart1 driver (for use on wsn430v1.3b et .4 boards)
 *  \author Colin Chaballier
 *  \date   2008
 **/


#ifndef _UART1_H_
#define _UART1_H_

typedef void (*uart1_cb_t)(uint8_t c);

#define UART1_CONFIG_8MHZ_115200 0
#define UART1_CONFIG_1MHZ_38400  1
#define UART1_CONFIG_1MHZ_115200 2
#define UART1_CONFIG_1MHZ_4800   3
#define UART1_CONFIG_1MHZ_57600  4

/**
 * Configure the USART0 for UART use.
 * \param config configuration to use depending
 * on SMCLK speed and desired baudrate
 */
void uart1_init(uint16_t config);

/**
 * Wait until a character is read and return it.
 * \return the character read
 */
int uart1_getchar_polling(void);

/**
 * Send a character.
 * \param c the character to send
 * \return the same sent character
 */
int uart1_putchar(int c);

/**
 * Stop the peripheral.
 */
void uart1_stop(void);

/**
 * Register a callback that will be called every time
 * a char is received.
 * \param the callback function pointer
 */
void uart1_register_callback(uart1_cb_t cb);

#endif

