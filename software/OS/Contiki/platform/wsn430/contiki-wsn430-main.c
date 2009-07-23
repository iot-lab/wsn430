/*
 * Copyright (c) 2006, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#)$Id: contiki-sky-main.c,v 1.40 2008/11/09 12:22:04 adamdunkels Exp $
 */

/*
 * Code mostly copied from the sky platform.
 * Ported by Clément Burin des Roziers <clement.burin-des-roziers@inrialpes.fr>
 */

#include <signal.h>
#include <stdio.h>
#include <string.h>

#include <io.h>

#include "contiki.h"
#include "contiki-wsn430.h"

#include "dev/leds.h"
#include "dev/watchdog.h"
#include "dev/xmem.h"
#include "dev/cc1100-radio.h"

#include "lib/random.h"

#include "cfs-coffee-arch.h"
#include "cfs/cfs-coffee.h"
#include "sys/autostart.h"
#include "sys/profile.h"

// WSN430 drivers
#include "uart0.h"
#include "ds2411.h"
#include "ds1722.h"

/*---------------------------------------------------------------------------*/
static void
print_processes(struct process * const processes[])
{
  /*  const struct process * const * p = processes;*/
  printf("Starting");
  while(*processes != NULL) {
    printf(" '%s'", (*processes)->name);
    processes++;
  }
  putchar('\n');
}
/*--------------------------------------------------------------------------*/
int
main(int argc, char **argv)
{
    /*
    * Initalize hardware.
    */
    msp430_cpu_init();
    clock_init();
    leds_init();
    uart0_init(UART0_CONFIG_1MHZ_115200);
    wsn430_slip_init();
    
    /*
     * Initialize Unique ID.
     */
    leds_on(LEDS_GREEN);
    ds2411_init();
    
    /* XXX hack: Fix it so that the 802.15.4 MAC address is compatible
     with an Ethernet MAC address - byte 0 (byte 2 in the DS ID)
     cannot be odd. */
    ds2411_id.raw[2] &= 0xfe;
    
    /*
     * Initialize External Flash.
     */
    leds_on(LEDS_BLUE);
    xmem_init();
    
    /*
     * Initialize realtime timer.
     */
    leds_off(LEDS_RED);
    rtimer_init();
    /*
     * Hardware initialization done!
     */

    random_init(ds2411_id.raw[6]);
    leds_off(LEDS_BLUE);
    
    /*
    * Initialize Contiki and our processes.
    */
    process_init();
    
    /* Start the event timer process */
    process_start(&etimer_process, NULL);
    ctimer_init();
    
    printf(CONTIKI_VERSION_STRING " started. ");
    wsn430_set_rime_addr();
    printf("MAC %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
        ds2411_id.raw[0], ds2411_id.raw[1], ds2411_id.raw[2], ds2411_id.raw[3],
        ds2411_id.raw[4], ds2411_id.raw[5], ds2411_id.raw[6], ds2411_id.raw[7]);
    
    /* Initialize the network */
    cc1100_radio_init();
    wsn430_network_init();
    
  #if PROFILE_CONF_ON
    profile_init();
  #endif /* PROFILE_CONF_ON */

    leds_off(LEDS_GREEN);

  #if TIMESYNCH_CONF_ENABLED
    timesynch_init();
    timesynch_set_authority_level(rimeaddr_node_addr.u8[0]);
  #endif /* TIMESYNCH_CONF_ENABLED */

    /*
     * Start energy management
     */
    energest_init();
    ENERGEST_ON(ENERGEST_TYPE_CPU);
    
    /* Start the application processes registered in the autostart */
    print_processes(autostart_processes);
    autostart_start(autostart_processes);

    /*
    * This is the scheduler loop.
    */
    watchdog_start();
    
    while(1) {
        int r;
      #if PROFILE_CONF_ON
        profile_episode_start();
      #endif /* PROFILE_CONF_ON */
        do {
            /* Reset watchdog. */
            watchdog_periodic();
            r = process_run();
        } while(r > 0);
      #if PROFILE_CONF_ON
        profile_episode_end();
      #endif /* PROFILE_CONF_ON */
        
        /*
        * Idle processing.
        */
        int s = splhigh();		/* Disable interrupts. */
        if(process_nevents() != 0)
        {
            splx(s);		/* Re-enable interrupts. */
        }
        else
        {
            static unsigned long irq_energest = 0;
            ENERGEST_OFF(ENERGEST_TYPE_CPU);
            ENERGEST_ON(ENERGEST_TYPE_LPM);
            /* We only want to measure the processing done in IRQs when we
             * are asleep, so we discard the processing time done when we
             * were awake. */
            energest_type_set(ENERGEST_TYPE_IRQ, irq_energest);
      
            /* Re-enable interrupts and go to sleep atomically. */
            
            watchdog_stop();
            eint();
            _BIS_SR(LPM1_bits);
            //~ _BIS_SR(GIE | SCG0 | SCG1 | CPUOFF); /* LPM3 sleep. This
                      //~ statement will block
                      //~ until the CPU is
                      //~ woken up by an
                      //~ interrupt that sets
                      //~ the wake up flag. */
                            
            /* We get the current processing time for interrupts that was
             * done during the LPM and store it for next time around.  */
            dint();
            irq_energest = energest_type_time(ENERGEST_TYPE_IRQ);
            eint();
            watchdog_start();
            ENERGEST_OFF(ENERGEST_TYPE_LPM);
            ENERGEST_ON(ENERGEST_TYPE_CPU);
            
        }
    }

    return 0;
}
/*---------------------------------------------------------------------------*/
#if LOG_CONF_ENABLED
void
log_message(char *m1, char *m2)
{
  printf("%s%s\n", m1, m2);
}
#endif /* LOG_CONF_ENABLED */
