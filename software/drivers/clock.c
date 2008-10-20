/**
 *  \file   clock.c
 *  \brief  msp430 system clock 
 *  \author Antoine Fraboulet
 *  \date   2005
 **/

#include <io.h>
#include <signal.h>
#include <iomacros.h>
#include <stdio.h>

#include "clock.h"

/***************************************************************
 * we have to wait OFIFG to be sure the switch is ok 
 * slau049e.pdf page 4-12 [pdf page 124]
 ***************************************************************/

#define WAIT_CRISTAL()                                                 \
do {                                                                   \
  int i;                                                               \
  do {                                                                 \
    IFG1 &= ~OFIFG;                  /* Clear OSCFault flag  */        \
    for (i = 0xff; i > 0; i--)       /* Time for flag to set */        \
       nop();                        /*                      */        \
  }  while ((IFG1 & OFIFG) != 0);    /* OSCFault flag still set? */    \
} while (0)

/***************************************************************
 *
 ***************************************************************/

void set_mcu_speed_dco_mclk_4MHz_smclk_1MHz(void)
{
  /*
   * ACLK  = ??
   * MCLK  = dcoclk @ 4.16MHz
   * SMCLK = dcoclk / 2
   */
  
  // turn on XT1 
  
  //start up crystall oscillator XT2
  // DIVA_0 -> ACLK divider = 1
  // RSEL2 | RSEL1 | RSEL0 -> resistor select
  BCSCTL1 = XT2OFF | RSEL2 | RSEL1 | RSEL0;
  
  // SELM_0 = dcoclk
  // SELM_1 = dcoclk
  // SELM_2 = XT2CLK/LFXTCLK
  // SELM_3 = LFXTCLK
  // SELS   : SMCLK source XT2
  // DIVS_1 : SMCLK divider /2 
  BCSCTL2 = SELM_0 | DIVS_2;
  
  // dcox = 6, rsel = 7, mod = 0
  // according to "Bob L'éponge" the MCLK clock speed should
  // be set to 4.16MHz and SMCLK clock speed to 2.08Mhz
  DCOCTL  = DCO2 | DCO1;
  
  WAIT_CRISTAL();
}

/***************************************************************
 *
 ***************************************************************/

void set_mcu_speed_xt2_mclk_2MHz_smclk_1MHz(void)
{
  DCOCTL   = 0; /* dco */
  BCSCTL1  = 0; /* xt2 on + xts high + aclk full speed */
  BCSCTL2  = (SELM_2 | DIVM_2) | (SELS | DIVS_3); /* xt2/4, xt2/8 */

  WAIT_CRISTAL();
} 

/***************************************************************
 *
 ***************************************************************/

void set_mcu_speed_xt2_mclk_4MHz_smclk_1MHz(void)
{
  DCOCTL   = 0;
  BCSCTL1  = 0;
  BCSCTL2  = (SELM_2 | DIVM_1) | (SELS | DIVS_3);

  WAIT_CRISTAL();
} 

void set_aclk_div(uint16_t div)
{
  int f=0;
  switch (div)
    {
    case 1: f = DIVA_0; break;
    case 2: f = DIVA_1; break;
    case 4: f = DIVA_2; break;
    case 8: f = DIVA_3; break;
    default: f = DIVA_0; break;
    }
  BCSCTL1  &= ~(DIVA_3);
  BCSCTL1  |= f;
}

/***************************************************************
 *
 ***************************************************************/

void set_mcu_speed_xt2_mclk_8MHz_smclk_8MHz(void)
{
  DCOCTL  = 0;
  BCSCTL1 = 0;
  BCSCTL2 = SELM_2 | SELS;

  WAIT_CRISTAL();
}

/***************************************************************
 *
 ***************************************************************/

void set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz(void)
{
  DCOCTL  = 0;
  BCSCTL1 = 0;
  BCSCTL2 = SELM_2 | (SELS | DIVS_3) ;

  WAIT_CRISTAL();
}

/***************************************************************
 *
 ***************************************************************/


#if 0
/* Wait in us */
void spi_msp430_cc1100_micro_wait(register unsigned int n)
{
  /* MCLK is running 8MHz, 1 cycle = 125ns    */
  /* n=1 -> waiting = 4*125ns = 500ns         */

  /* MCLK is running 4MHz, 1 cycle = 250ns    */
  /* n=1 -> waiting = 4*250ns = 1000ns        */
  n = n * 1000;

    __asm__ __volatile__ (
		"1: \n"
		" dec	%[n] \n"      /* 2 cycles */
		" jne	1b \n"        /* 2 cycles */
        : [n] "+r"(n));
}

/* Wait in ms */
void spi_msp430_cc1100_wait(register unsigned int n)
{
  n = n * 1000;
  spi_msp430_cc1100_micro_wait(n);
}
#endif
