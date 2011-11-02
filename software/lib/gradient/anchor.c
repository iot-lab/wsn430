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
 * @file anchor.c
 * @brief 
 * @author Tony Ducrocq
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
static uint16_t my_seqnum = 0;
static uint8_t hop_threshold = 0;
static uint8_t my_hop = 99;
static uint16_t my_parent;

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
  return 0;
}

uint16_t packet_sent() {
  return 0;
}

void packet_received(uint16_t src_addr, uint8_t* data, uint16_t length,
		     int8_t rssi) {
  hello_t *hello = (hello_t *) (void *) data;
  beacon_t *beacon;
  report_t report;
  uint16_t beacon_seqnum;
  uint8_t crc;

  /********** if packet is a BEACON *********/
  if (length == sizeof(beacon_t) && hello->type == BEACON_T) {
    beacon = (beacon_t *) (void *) data;

    beacon_seqnum = beacon->seqnum[0];
    beacon_seqnum <<= 8;
    beacon_seqnum += beacon->seqnum[1];

    /* CRC verification */
    crc = crc8_bytes((uint8_t *) beacon->raw,
		     (uint16_t) ST_OFFSET(beacon_t, crc));

    if (my_seqnum < beacon_seqnum && crc == beacon->crc) {
      if (((my_hop >= (beacon->hop + 1) ) || hop_threshold >= 3)) {
      
	hop_threshold = 0;
	
	my_parent = src_addr;

	// resend beacon
	beacon->hop++;
	my_seqnum = beacon_seqnum;

	beacon->crc = crc8_bytes((uint8_t *) beacon->raw,
				 (uint16_t) ST_OFFSET(beacon_t, crc));
	mac_send(MAC_BROADCAST_ADDR, beacon->raw, sizeof(beacon_t), 0);
      }
    } else {
      hop_threshold++;
    }
  }

  /********** if packet is a HELLO *********/
  if (length == sizeof(hello_t) && hello->type == HELLO_T) {

    /* CRC verification */
    crc = crc8_bytes((uint8_t *) hello->raw,
		     (uint16_t) ST_OFFSET(hello_t, crc));

    if (crc == hello->crc) {
      report.type = REPORT_T;
      report.mobile_id[0] = (uint8_t) src_addr;
      report.mobile_id[1] = (uint8_t) (src_addr >> 8);
      report.anchor_id[0] = (uint8_t) mac_addr;
      report.anchor_id[1] = (uint8_t) (mac_addr >> 8);
      report.seqnum[0] = (uint8_t) hello->seqnum[0];
      report.seqnum[1] = (uint8_t) hello->seqnum[1];
	
      report.rssi = rssi;
      report.crc = crc8_bytes((uint8_t *) report.raw,
			      (uint16_t) ST_OFFSET(report_t, crc));
      
      mac_send(my_parent, report.raw, sizeof(report_t), 1);
    }
  }

  /********** if packet is a REPORT *********/
  if (length == sizeof(report_t) &&  hello->type == REPORT_T) {
    mac_send(my_parent, data, sizeof(report_t), 1);
  }

}

static void vSendingTask(void* pvParameters) {
  /* initialize variables */
  my_seqnum = 0;
  hop_threshold = 0;
  my_hop = 99;

  while (1) {
    vTaskDelay(20);
  }
}

void vApplicationIdleHook(void);
void vApplicationIdleHook(void) {
  _BIS_SR(LPM0_bits);
}
