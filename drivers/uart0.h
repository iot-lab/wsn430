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
 * \defgroup uart0 UART0 driver
 * \ingroup wsn430
 * @{
 *
 * The UART0 driver enables serial communications between the WSN430
 * and another device (e.g. a PC) using the MSP430 hardware USART0 port.
 *
 */

/**
 * \file
 * \brief MSP430 uart0 driver header
 * \author Antoine Fraboulet <antoine.fraboulet@insa-lyon.fr>
 * \author Colin Chaballier
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
 * \date   2008
 *
 **/


#ifndef _UART0_H_
#define _UART0_H_

/**
 * \brief UART0 callback type.
 * \param c the received character.
 * \return 1 if any low power mode (LPM) must be exited, 0 otherwise.
 */
typedef uint16_t (*uart0_cb_t)(uint8_t c);
/**
 * \brief UART0 DMA callback type.
 * \return 1 if any low power mode (LPM) must be exited, 0 otherwise.
 */
typedef uint16_t (*uart0_dma_cb_t)(void);

/**
 * \name Baudrate configuration
 * @{
 */
/** \brief Config value for having 115200 baud with SMCLK at 8MHz */
#define UART0_CONFIG_8MHZ_115200 0
/** \brief Config value for having 38400 baud with SMCLK at 1MHz */
#define UART0_CONFIG_1MHZ_38400  1
/** \brief Config value for having 115200 baud with SMCLK at 1MHz */
#define UART0_CONFIG_1MHZ_115200 2
/** \brief Config value for having 230400 baud with SMCLK at 8MHz */
#define UART0_CONFIG_8MHZ_230400 3
/** \brief Config value for having 500000 baud with SMCLK at 8MHz */
#define UART0_CONFIG_8MHZ_500000 4
/**
 * @}
 */

/**
 * \brief Configure the USART0 for UART use.
 * \param config the configuration to use depending
 *        on SMCLK speed and desired baudrate
 *
 * Initialization function that should be called before
 * any attempt to use the other functions.
 * The \a config parameter should be one of the constants defined above,
 * depending on the SMCLK speed and the desired baudrate.
 */
void uart0_init(uint16_t config);

/**
 * \brief Wait until a character is read and return it.
 * \return the read character.
 * This function waits until a character is received from the serial link,
 * and returns it. It will wait forever if no character is received.
 */
int uart0_getchar_polling(void);

/**
 * \brief Send a character.
 * \param c the character to send
 * \return the sent character
 * This function sends a single character on the serial link.
 * It is a blocking function that returns once the character is sent.
 */
int uart0_putchar(int c);

/**
 * \brief Stop the peripheral.
 * This function stops the USART0 module if not needed anymore.
 * It saves energy to stop it.
 */
void uart0_stop(void);

/**
 * \brief Register a received char callback .
 * \param cb the callback function pointer
 * This function registers a callback function as defined
 * by the \b uart0_cb_t type that will be called every time
 * a character is received, passing it as argument.
 */
void uart0_register_callback(uart0_cb_t cb);

/**
 * Send a block of data to the UART using a DMA channel.
 * \param data a pointer to the first byte of data to send
 * \param length the number of bytes to send
 * \return 1 if transfer started, 0 if error
 */
int uart0_dma_putchars(uint8_t* data, int16_t length);

/**
 * \brief Register a DMA end transfer callback .
 * \param cb the callback function pointer
 * This function registers a callback function as defined
 * by the \b uart0_dma_cb_t type that will be called
 * when a DMA transfer is done
 */
void uart0_register_dma_callback(uart0_dma_cb_t cb);
#endif

/**
 * @}
 */
