/*
 * Copyright (c) 2009, Swedish Institute of Computer Science.
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
 * $Id: uip6-bridge-tap.c,v 1.3 2010/06/14 19:19:17 adamdunkels Exp $
 *
 */

/**
 * \file
 *         A brief description of what this file is
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "contiki.h"
#include "net/uip.h"
#include "dev/slip.h"
#include "dev/leds.h"
#include "sicslow_ethernet.h"

#include "net/packetbuf.h"

#include <stdio.h>
#include <string.h>

#define UIP_IP_BUF ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

PROCESS(uip6_bridge, "IPv6/6lowpan TAP bridge");
PROCESS(tcpip_process, "tcpip dummy");
AUTOSTART_PROCESSES(&uip6_bridge);

/*---------------------------------------------------------------------------*/
static uint8_t (* outputfunc)(uip_lladdr_t *a);
uint8_t
tcpip_output(uip_lladdr_t *a)
{
  if(outputfunc != NULL) {
    outputfunc(a);
    /*    printf("pppp o %u tx %u rx %u\n", UIP_IP_BUF->proto,
	   packetbuf_attr(PACKETBUF_ATTR_TRANSMIT_TIME),
	   packetbuf_attr(PACKETBUF_ATTR_LISTEN_TIME));*/
    leds_invert(LEDS_GREEN);
  }
  return 0;
}
void
tcpip_ipv6_output(void)
{
}
void
tcpip_set_outputfunc(uint8_t (*f)(uip_lladdr_t *))
{
  outputfunc = f;
}
PROCESS_THREAD(tcpip_process, ev, data)
{
  PROCESS_BEGIN();
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/* Packet from SICSLoWPAN */
void
tcpip_input(void)
{
  if(uip_len > 0) {
    mac_LowpanToEthernet();
    if(uip_len > 0) {
      /*      printf("pppp i %u tx %u rx %u\n", UIP_IP_BUF->proto,
	     packetbuf_attr(PACKETBUF_ATTR_TRANSMIT_TIME),
	     packetbuf_attr(PACKETBUF_ATTR_LISTEN_TIME));*/
      slip_write(uip_buf, uip_len);
      leds_invert(LEDS_RED);
      uip_len = 0;
    }
  }
}
/*---------------------------------------------------------------------------*/
/* Packet from SLIP */
static void
slip_tcpip_input(void)
{
  // BUF changed to uip_buf in slip.c no need to memmove
  // memmove(uip_buf, &uip_buf[UIP_LLH_LEN], uip_len);
  mac_ethernetToLowpan(uip_buf);
}
/*---------------------------------------------------------------------------*/
static void
slip_activity(void)
{
  leds_invert(LEDS_BLUE);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(uip6_bridge, ev, data)
{
  PROCESS_BEGIN();

  printf("Setting up SLIP\n");

  mac_ethernetSetup();

  slip_arch_init(0);
  slip_set_input_callback(slip_activity);
  slip_set_tcpip_input_callback(slip_tcpip_input);
  process_start(&slip_process, NULL);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
int
putchar(int c)
{
#define SLIP_END     0300
  static char debug_frame = 0;

  if(!debug_frame) {		/* Start of debug output */
    slip_arch_writeb(SLIP_END);
    slip_arch_writeb('\r');	/* Type debug line == '\r' */
    debug_frame = 1;
  }

  slip_arch_writeb((char)c);

  /*
   * Line buffered output, a newline marks the end of debug output and
   * implicitly flushes debug output.
   */
  if(c == '\n') {
    slip_arch_writeb(SLIP_END);
    debug_frame = 0;
  }
  return c;
}
/*---------------------------------------------------------------------------*/
