/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
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
 * @(#)$Id: msp430.c,v 1.10 2009/02/04 18:28:44 joxe Exp $
 */
#include <io.h>
#include <signal.h>
#include <sys/unistd.h>
#include "msp430.h"
#include "dev/watchdog.h"
#include "net/uip.h"


/*---------------------------------------------------------------------------*/
void
msp430_init_dco(void)
{
    DCOCTL  = 0;
    BCSCTL1 = 0;
    BCSCTL2 = SELM_2 | SELS ;

    int i;
    do {
    IFG1 &= ~OFIFG;                  /* Clear OSCFault flag  */
    for (i = 0xff; i > 0; i--)       /* Time for flag to set */
        nop();                        /*                      */
    } while ((IFG1 & OFIFG) != 0);    /* OSCFault flag still set? */

}
/*---------------------------------------------------------------------------*/

static void
init_ports(void)
{
    /* Turn everything off, device drivers enable what is needed. */

    /* All configured for digital I/O */
    P1SEL = 0;
    P2SEL = 0;
    P3SEL = 0;
    P4SEL = 0;
    P5SEL = 0;
    P6SEL = 0;

    /* All inputs Except outputs*/
    P1DIR = 0;
    P1OUT = 0;
    P2DIR = 0;
    P2OUT = 0;
    P3DIR = 0;
    P3OUT = 0;
    P4DIR = 0;
    P4OUT = 0;
    P5DIR = 0;
    P5OUT = 0;
    P6DIR = 0;
    P6OUT = 0;

    /* Disable Interrupts */
    P1IE = 0;
    P2IE = 0;
}
/*---------------------------------------------------------------------------*/
/* msp430-ld may align _end incorrectly. Workaround in cpu_init. */
extern int _end;		/* Not in sys/unistd.h */
static char *cur_break = (char *)&_end;

void
msp430_cpu_init(void)
{
  dint();
  watchdog_init();
  init_ports();
  msp430_init_dco();
  eint();
  if((uintptr_t)cur_break & 1) { /* Workaround for msp430-ld bug! */
    cur_break++;
  }
}
/*---------------------------------------------------------------------------*/
#define asmv(arg) __asm__ __volatile__(arg)

#define STACK_EXTRA 32

/*
 * Allocate memory from the heap. Check that we don't collide with the
 * stack right now (some other routine might later). A watchdog might
 * be used to check if cur_break and the stack pointer meet during
 * runtime.
 */
void *
sbrk(int incr)
{
  char *stack_pointer;

  asmv("mov r1, %0" : "=r" (stack_pointer));
  stack_pointer -= STACK_EXTRA;
  if(incr > (stack_pointer - cur_break))
    return (void *)-1;		/* ENOMEM */

  void *old_break = cur_break;
  cur_break += incr;
  /*
   * If the stack was never here then [old_break .. cur_break] should
   * be filled with zeros.
  */
  return old_break;
}
/*---------------------------------------------------------------------------*/
/*
 * Mask all interrupts that can be masked.
 */
int
splhigh_(void)
{
  /* Clear the GIE (General Interrupt Enable) flag. */
  int sr;
  asmv("mov r2, %0" : "=r" (sr));
  asmv("bic %0, r2" : : "i" (GIE));
  return sr & GIE;		/* Ignore other sr bits. */
}
/*---------------------------------------------------------------------------*/
/*
 * Restore previous interrupt mask.
 */
void
splx_(int sr)
{
  /* If GIE was set, restore it. */
  asmv("bis %0, r2" : : "r" (sr));
}
/*---------------------------------------------------------------------------*/
/* this code will always start the TimerB if not already started */
void
msp430_sync_dco(void) {
  uint16_t last;
  uint16_t diff;
/*   uint32_t speed; */
  /* DELTA_2 assumes an ACLK of 32768 Hz */
#define DELTA_2    ((MSP430_CPU_SPEED) / 32768)

  /* Select SMCLK clock, and capture on ACLK for TBCCR6 */
  TBCTL = TBSSEL1 | TBCLR;
  TBCCTL6 = CCIS0 + CM0 + CAP;
  /* start the timer */
  TBCTL |= MC1;

  // wait for next Capture
  TBCCTL6 &= ~CCIFG;
  while(!(TBCCTL6 & CCIFG));
  last = TBCCR6;

  TBCCTL6 &= ~CCIFG;
  // wait for next Capture - and calculate difference
  while(!(TBCCTL6 & CCIFG));
  diff = TBCCR6 - last;

  /* Stop timer - conserves energy according to user guide */
  TBCTL = 0;

/*   speed = diff; */
/*   speed = speed * 32768; */
/*   printf("Last TAR diff:%d target: %ld ", diff, DELTA_2); */
/*   printf("CPU Speed: %lu DCOCTL: %d\n", speed, DCOCTL); */

  /* resynchronize the DCO speed if not at target */
  if(DELTA_2 < diff) {        /* DCO is too fast, slow it down */
    DCOCTL--;
    if(DCOCTL == 0xFF) {              /* Did DCO role under? */
      BCSCTL1--;
    }
  } else if (DELTA_2 > diff) {
    DCOCTL++;
    if(DCOCTL == 0x00) {              /* Did DCO role over? */
      BCSCTL1++;
    }
  }
}
/*---------------------------------------------------------------------------*/
