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

int putchar(int c)
{
    uart0_putchar((char) c);
    return c;
}

uint8_t rxframe[128];

volatile uint16_t flag = 0;

/**
 * The main function.
 */
int main( void )
{
    uint8_t i;
    uint8_t length;
    
    /* Stop the watchdog timer. */
    WDTCTL = WDTPW + WDTHOLD;
    
    /* Setup MCLK 8MHz and SMCLK 1MHz */
    set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
    
    /* Enable Interrupts */
    eint();
    
    LEDS_INIT();
    LEDS_ON();

    uart0_init(UART0_CONFIG_1MHZ_115200);
    printf("CC2420 RX test program with address recognition and acknowledge frames\r\n");
    
    cc2420_init();
    cc2420_io_sfd_register_cb(sfd_cb);
    cc2420_io_sfd_int_set_falling();
    cc2420_io_sfd_int_clear();
    cc2420_io_sfd_int_enable();
    
    uint8_t src_pan_id[2] = {0x22,0x00};
    uint8_t src_addr[2] = {0x11,0x11};

    while ( (cc2420_get_status() & 0x40) == 0 ); // waiting for xosc being stable

    cc2420_set_panid(src_pan_id); // save pan id in ram 
    cc2420_set_shortadr(src_addr); // save short address in ram

    printf("CC2420 initialized\r\n");

    LEDS_OFF();

    while(1)
    {
        cc2420_cmd_idle();
        cc2420_cmd_flushrx();
        cc2420_cmd_rx();

        while (flag == 0) ;
        micro_delay(0xFFFF);
        flag = 0;      
	LED_GREEN_TOGGLE();
        cc2420_fifo_get(&length, 1);
        if ( length < 128 )
        {
            cc2420_fifo_get(rxframe, length);
            // check CRC
            if ( (rxframe[length-1] & 0x80) != 0 )
            {
                printf("Frame received with rssi=%ddBm:\r\n", ((signed int)((signed char)(rxframe[length-2])))-45);
                LED_BLUE_TOGGLE();
		// ignore 11 first bytes (fcf,seq,addr) and the 2 last ones (crc)
                for (i=11; i<length-2; i++)
                {
                    printf("%c",rxframe[i]);
                }
                printf("\r\n\n");
            }
	    else {
		printf("CRC non OK, erreur de transmission?\n");
                printf("\r\n");
                LED_RED_TOGGLE();
	    }
        }
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
