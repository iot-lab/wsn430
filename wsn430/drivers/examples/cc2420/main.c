/*
 * Copyright  2008-2009 SensTools, INRIA
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


#include <io.h>
#include <signal.h>
#include <stdio.h>

/* Project includes */
#include "clock.h"
#include "uart0.h"
#include "cc2420.h"
#include "leds.h"

static uint16_t rx_ok(void);
static uint16_t char_cb(uint8_t c);

int putchar(int c) {
	uart0_putchar((char) c);
	return c;
}

uint8_t frame[128];
uint16_t frameseq = 0;
uint8_t length;
volatile uint16_t send = 0, receive = 0;

/**
 * The main function.
 */
int main( void )
{
	/* Stop the watchdog timer. */
	WDTCTL = WDTPW + WDTHOLD;
	
	/* Setup MCLK 8MHz and SMCLK 1MHz */
	set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
	
	/* Enable Interrupts */
	eint();
	
	uart0_init(UART0_CONFIG_1MHZ_115200);
	uart0_register_callback(char_cb);
	printf("CC2420 RXTX test program\r\n");
	
	LEDS_INIT();
	LEDS_OFF();
	
	
	cc2420_init();
	
	
	printf("CC2420 initialized\r\nType 's' to send a message\r\n");
	
	while(1)
	{
		// Enter RX
		LED_RED_ON();
		cc2420_cmd_idle();
		cc2420_cmd_flushrx();
		cc2420_cmd_rx();
		
		cc2420_io_sfd_register_cb(rx_ok);
		cc2420_io_sfd_int_set_falling();
		cc2420_io_sfd_int_clear();
		cc2420_io_sfd_int_enable();
		
		// Low Power Mode
		LPM0;
		
		// Check for send flag
		if (send == 1) {
			send = 0;
			LED_RED_OFF();
			cc2420_cmd_idle();
			cc2420_cmd_flushtx();
			cc2420_io_sfd_int_disable();
			
			frameseq ++;
			
			length = sprintf((char *)frame, "Hello World #%i", frameseq);
			
			printf("Sent : %s \r\n", frame);
			
			length += 2;
			
			cc2420_fifo_put(&length, 1);
			cc2420_fifo_put(frame, length-2);
			
			cc2420_cmd_tx();
			
			// Wait for SYNC word sent
			while (cc2420_io_sfd_read() == 0);
			
			// Wait for end of packet
			while (cc2420_io_sfd_read() != 0);
		}
		
		// Check for receive flag
		if (receive == 1) {
			receive = 0;
			uint8_t i;

			cc2420_fifo_get(&length, 1);
		
			if ( length >= 128 ) {
				continue;
			}
			
			cc2420_fifo_get(frame, length);
			
			// check CRC
			if ( (frame[length-1] & 0x80) == 0 ) {
				continue;
			}
			int8_t rssi = ((signed char)(frame[length-2]))-45;
			printf("Frame received with RSSI=%d dBm: ", rssi);
			for (i=0; i<length-2; i++) {
				printf("%c",frame[i]);
			}
			printf("\r\n");
			
			LED_GREEN_TOGGLE();
		}
	}
	
	return 0;
}

static uint16_t char_cb(uint8_t c) {
	if (c == 's') {
		send = 1;
		return 1;
	}
	
	return 0;
}

static uint16_t rx_ok(void) {
	receive = 1;
	return 1;
}
