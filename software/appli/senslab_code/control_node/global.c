/*
 * Copyright  2008-2009 INRIA/SensLab
 * 
 * <dev-team@senslab.info>
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
 * \file
 * \brief Global data and methods
 * \author Clément Burin des Roziers
 * \date 2009
 */

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Drivers */
#include "uart1.h"
#include "tsl2550.h"
#include "ds1722.h"
#include "ina209.h"
#include "spi1.h"

#include "global.h"

xSemaphoreHandle xUSART0Mutex, xUSART1Mutex;
void* uart1_rx_cb = 0x0;
uint16_t leds_policy;
uint16_t myTime;

void vGetUART(void) // 1
{
	xSemaphoreTake( xUSART1Mutex, portMAX_DELAY );
}

void vReleaseUART(void) // 1
{
	xSemaphoreGive( xUSART1Mutex );
}

void vGetTSL2550(void) // 0
{
	xSemaphoreTake( xUSART0Mutex, portMAX_DELAY);
	tsl2550_init();
}

void vReleaseTSL2550(void) // 0
{
	xSemaphoreGive( xUSART0Mutex );
}

void vGetINA209(void) // 0
{
	xSemaphoreTake( xUSART0Mutex, portMAX_DELAY);
	tsl2550_init();
}

void vReleaseINA209(void) // 0
{
	xSemaphoreGive( xUSART0Mutex );
}

void vGetDS1722(void) // 1
{
	xSemaphoreTake( xUSART1Mutex, portMAX_DELAY);
	uart1_stop();
	ds1722_init();
}

void vReleaseDS1722(void) // 1
{
	xSemaphoreGive( xUSART1Mutex );
	uart1_init(UART_CONFIG);
	uart1_register_callback((uart1_cb_t) uart1_rx_cb);
}

void vGetCC1100(void) // 1
{
	xSemaphoreTake( xUSART1Mutex, portMAX_DELAY);
	uart1_stop();
	spi1_init();
}

void vReleaseCC1100(void) // 1
{
	xSemaphoreGive( xUSART1Mutex );
	uart1_init(UART_CONFIG);
	uart1_register_callback((uart1_cb_t) uart1_rx_cb);
}

void vGetCC2420(void) // 1
{
	xSemaphoreTake( xUSART1Mutex, portMAX_DELAY);
	uart1_stop();
	spi1_init();
}

void vReleaseCC2420(void) // 1
{
	xSemaphoreGive( xUSART1Mutex );
	uart1_init(UART_CONFIG);
	uart1_register_callback((uart1_cb_t) uart1_rx_cb);
}
