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

static uint16_t sfd_cb(void);
int rx_ack(void);

int putchar(int c)
{
    uart0_putchar((char) c);
    return c;
}

uint8_t rxframe[128];
volatile uint16_t flag = 0;

uint8_t txframe[128];
uint8_t txlength;

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
    
    LEDS_INIT();
    LEDS_ON();

    uart0_init(UART0_CONFIG_1MHZ_115200);
    printf("CC2420 TX test program with address recognition and acknowledge frames\r\n");
    
    cc2420_init();
    cc2420_io_sfd_register_cb(sfd_cb);
    cc2420_io_sfd_int_set_falling();
    cc2420_io_sfd_int_clear();
    cc2420_io_sfd_int_enable();
    
    uint8_t fcf[2] = {0x21, 0x88};  /* -> 00100001 10001000 -> reverse of bits for each byte -> 10000100 00010001 -> ack bit = 1 (6th bit), Frame type = 001 (don't forget to read from right to left) */
    uint8_t seq_numb = 0x01;
    uint8_t dest_pan_id[2] = {0x22, 0x00};
    uint8_t dest_addr[2] = {0x11, 0x11};

    uint8_t src_pan_id[2] = {0x22, 0x01};
    uint8_t src_addr[2] = {0x11, 0x12};

    while ( (cc2420_get_status() & 0x40) == 0 ); // waiting for xosc being stable

    cc2420_set_panid(src_pan_id); // save pan id in ram 
    cc2420_set_shortadr(src_addr); // save short address in ram

    printf("CC2420 initialized\r\n");

    LEDS_OFF();

    while (1)
    {
        cc2420_cmd_idle();
        cc2420_cmd_flushtx();

        txlength = sprintf((char *)txframe, "Hello World #%i", seq_numb);
        
        printf("Sent : %s of length %d\r\n", txframe,txlength);
	
        txlength += 13;
        
        cc2420_fifo_put(&txlength, 1);
        cc2420_fifo_put(fcf, 2);
        cc2420_fifo_put(&seq_numb, 1);
        cc2420_fifo_put(dest_pan_id, 2);
        cc2420_fifo_put(dest_addr, 2);
        cc2420_fifo_put(src_pan_id, 2);
        cc2420_fifo_put(src_addr, 2);
        cc2420_fifo_put(txframe, txlength-13);
        
	LED_BLUE_TOGGLE();

        cc2420_cmd_tx();

        micro_delay(0xFFFF);
	while (cc2420_io_sfd_read());

	printf("Waiting for acknowledge frame...\n");

	if (rx_ack())
	{
            seq_numb ++;
	}
	else
	{
	    printf("No Acknowledge frame received for frame number #%i - Retrying...\r\n\n", seq_numb);
	    LED_RED_TOGGLE();
	}
        micro_delay(0xFFFF);
        micro_delay(0xFFFF);
        micro_delay(0xFFFF);
        micro_delay(0xFFFF);
        micro_delay(0xFFFF);
        micro_delay(0xFFFF);
        micro_delay(0xFFFF);
        micro_delay(0xFFFF);
        micro_delay(0xFFFF);
        micro_delay(0xFFFF);
    }
    
    
    return 0;
}


static uint16_t sfd_cb(void)
{
    if (cc2420_io_fifop_read())
    {
        flag = 1;
    }
    return 0;
}


/**
 * Acknowledge waiting function.
 */
int rx_ack(void)
{
    uint8_t length;
    int retries = 0;

    while ( flag == 0 )
    {
        if( retries == 100 ) return 0;
        retries++;
    }

    flag = 0;      
    LED_GREEN_TOGGLE();
    cc2420_fifo_get(&length, 1);

    if ( length < 128 )
    {
        cc2420_fifo_get(rxframe, length);
        printf("Acknowledge frame received with sequence number #%i\r\n\n", rxframe[length-3]);
        LED_BLUE_TOGGLE();
    }

    return 1;
}

