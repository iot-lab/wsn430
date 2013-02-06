/*
 * Copyright (c) 2008, Swedish Institute of Computer Science.
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
 * This file is part of the Contiki operating system.
 *
 * $Id: shell-wsn430.c,v 1.13 2009/03/05 21:12:02 adamdunkels Exp $
 */

/**
 * \file
 *         Tmote Sky-specific Contiki shell commands
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "shell-wsn430.h"

#include "dev/watchdog.h"

#include "dev/leds.h"

#include "ds2411.h"
#include "ds1722.h"

#include <stdio.h>
#include <string.h>

/*---------------------------------------------------------------------------*/
PROCESS(shell_nodeid_process, "nodeid");
SHELL_COMMAND(nodeid_command,
	      "nodeid",
	      "nodeid: set node ID",
	      &shell_nodeid_process);
PROCESS(shell_sense_process, "sense");
SHELL_COMMAND(sense_command,
	      "sense",
	      "sense: print out sensor data",
	      &shell_sense_process);

/*---------------------------------------------------------------------------*/
#define MAX(a, b) ((a) > (b)? (a): (b))
#define MIN(a, b) ((a) < (b)? (a): (b))

/*---------------------------------------------------------------------------*/
struct sense_msg {
  uint16_t clock;
  uint16_t temp;
};
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(shell_sense_process, ev, data)
{
  struct sense_msg msg;
  PROCESS_BEGIN();

  msg.clock = clock_time();
  msg.temp = ds1722_read_MSB();
  msg.temp <<= 8;
  msg.temp += ds1722_read_LSB();

  char msg[32];
  snprintf(msg, sizeof(msg), "%u", clock_time());
  shell_output_str(&sense_command, "clock = ", msg);
  snprintf(msg, sizeof(msg), "%u.%u", ds1722_read_MSB(), (ds1722_read_LSB()>>7) * 5);
  shell_output_str(&sense_command, "T = ", msg);
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(shell_nodeid_process, ev, data)
{
  uint16_t nodeid;
  char buf[20];
  const char *newptr;
  PROCESS_BEGIN();

  nodeid = shell_strtolong(data, &newptr);

  /* If no node ID was given on the command line, we print out the
     current channel. Else we burn the new node ID. */
  if(newptr == data) {
    nodeid = ds2411_id.serial1;
    nodeid <<= 8;
    nodeid += ds2411_id.serial0;
  } else {
    nodeid = shell_strtolong(data, &newptr);
  }

  snprintf(buf, sizeof(buf), "%u", nodeid);
  shell_output_str(&nodeid_command, "Node ID: ", buf);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void
shell_wsn430_init(void)
{
  ds1722_set_res(10);
  ds1722_sample_cont();

  shell_register_command(&sense_command);
  shell_register_command(&nodeid_command);

}
/*---------------------------------------------------------------------------*/
