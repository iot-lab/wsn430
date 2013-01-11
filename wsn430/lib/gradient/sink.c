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
 * @file sink.c
 * @brief 
 * @author Tony Ducrocq, Clément Burin des Roziers 
 * @version 0.1
 * @date November 29, 2010
 *
 */

/**
 * Standard header files
 */
#include <io.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/**
 * WSN430 drivers
 */
#include "clock.h"
#include "uart0.h"
#include "leds.h"

/**
 * MAC layer: CSMA is used
 */
#include "mac.h"

/**
 * SINGLE header file
 */
#include "gradient.h"
#include "crc8.h"

/* Global Variables */
static xSemaphoreHandle xSPIMutex;
static beacon_t beacon;
static uint16_t seqnum;

/**********************************************************************/

// Hardware initialization
static void prvSetupHardware(void);
// Task that sends packets
static void vSendingTask(void* pvParameters);

/**********************************************************************/

int putchar(int c) {
	return uart0_putchar(c);
}

/**********************************************************************/

int main(void) {
	/* Setup the hardware. */
	prvSetupHardware();

	/* Create the SPI mutex */
	xSPIMutex = xSemaphoreCreateMutex();

	/* Initialize the MAC layer */
	mac_init(xSPIMutex, packet_received, CHANNEL);

	/* Add the local task */
	xTaskCreate( vSendingTask, (const signed char*) "sender", configMINIMAL_STACK_SIZE, NULL, 1, NULL );
	
	/* Start the scheduler. */
	vTaskStartScheduler();

	/* As the scheduler has been started we should never get here! */
	return 0;
}

/**********************************************************************/

/**
 * Initialize the main hardware parameters.
 */
static void prvSetupHardware(void) {
	/* Stop the watchdog timer. */
	WDTCTL = WDTPW + WDTHOLD;

	/* Setup MCLK 8MHz and SMCLK 1MHz */
	set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();

	LEDS_INIT();
	LEDS_OFF();

	uart0_init(UART0_CONFIG_1MHZ_115200);
	uart0_register_callback(char_rx);

	/* Enable Interrupts */
	eint();
}

/**********************************************************************/

uint16_t char_rx(uint8_t c) {
	return 0;
}

/**********************************************************************/

uint16_t send_packet() {
  seqnum++;
  beacon.seqnum[0] = seqnum >> 8;
  beacon.seqnum[1] = seqnum;
  beacon.hop = 0;
  
  beacon.crc = crc8_bytes(beacon.raw, ST_OFFSET(beacon_t, crc) );
  
  mac_send(MAC_BROADCAST_ADDR, beacon.raw, sizeof(beacon_t), 0);
  return 0;
}

uint16_t packet_sent() {
	return 0;
}

static void process_report(report_t *report){
  /* Process your report here */
  

}

void packet_received(uint16_t src_addr, uint8_t* data,
		uint16_t length, int8_t rssi) {
  hello_t *hello = (hello_t *)(void *) data;
  report_t *report;

  /********** if packet is a REPORT *********/
  if( length == sizeof(report_t) && hello->type == REPORT_T ){
    report = (report_t *)hello;

    process_report(report);
  }

}

static void vSendingTask(void* pvParameters) {
  
  /* initialize variables */
  beacon.type = BEACON_T;
  seqnum = 0;
  
  while (1) {
    send_packet();
    vTaskDelay(50);
  }
}

void vApplicationIdleHook(void);
void vApplicationIdleHook(void) {
	_BIS_SR(LPM0_bits);
}
