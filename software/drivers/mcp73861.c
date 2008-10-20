/* Copyright (c) 2006 by CITI INSA de Lyon.  All Rights Reserved */

/***
   NAME
     mcp73861
   PURPOSE
     
   NOTES
     
   HISTORY
     efleury - Sep 17, 2006: Created.
     $Log: mcp73861.c,v $
     Revision 1.2  2006-09-21 14:38:10  afraboul
     - driver modification, must invert input

     Revision 1.1  2006-09-21 11:58:23  efleury
     testing the LI-Polymer Charge Managment Controller

***/
#include <io.h>
//#include <iomacros.h>
#include "mcp73861.h"

/* adjust timing loop according to wait loop */

// 8MHz -> *2
// 4MHz -> *1

#define MICRO  *2
#define MILLI  *1


#define  MCP73861_SAMPLING_PERIODE (1300 MILLI) /* 0.3 sec */


static void __inline__ 
micro_wait(register unsigned int n)
{
  /* MCLK is running 8MHz, 1 cycle = 125ns    */
  /* n=1 -> waiting = 4*125ns = 500ns         */

  /* MCLK is running 4MHz, 1 cycle = 250ns    */
  /* n=1 -> waiting = 4*250ns = 1000ns        */

    __asm__ __volatile__ (
		"1: \n"
		" dec	%[n] \n" /* 2 cycles */
		" jne	1b   \n" /* 2 cycles */
        : [n] "+r"(n));
} /* micro_wait */


/* static void __inline__*/
void
milli_wait(register unsigned int n)
{
  int i;
  for(i=0; i<n; i++)
    micro_wait(1000 MICRO);
}

#define _SI_   static inline

#define BIT(b) (1 << (b))

/* n name, p port, b bit */
#define MSP430_ASSIGN_PIN(n, p, b)                                             \
  _SI_ void MSP430_DIR_##n##_INPUT()        { P##p##DIR &= ~BIT(b); }	\
  _SI_ void MSP430_DIR_##n##_OUTPUT()       { P##p##DIR |=  BIT(b); }	\
  _SI_ void MSP430_SEL_##n##_IO()           { P##p##SEL &= ~BIT(b); }	\
  _SI_ void MSP430_SEL_##n##_FUN()          { P##p##SEL |=  BIT(b); }	\
  _SI_ void MSP430_SET_##n##_LOW()          { P##p##OUT &= ~BIT(b); }	\
  _SI_ void MSP430_SET_##n##_HIGH()         { P##p##OUT |=  BIT(b); }	\
  _SI_ int  MSP430_READ_##n()               { return ((P##p##IN >> (b)) & 1); }	\
  _SI_ void MSP430_WRITE_##n(int val)					\
  {									\
    if (val)								\
      P##p##OUT |=  BIT(b);						\
    else								\
      P##p##OUT &= ~BIT(b);						\
  }

MSP430_ASSIGN_PIN(mcp73861_stat1,4,1); /* schematic_capnet */
MSP430_ASSIGN_PIN(mcp73861_stat2,6,3); /* schematic_capnet */

#define  MCP73861Pin_init()                 \
  do {					    \
    MSP430_SEL_mcp73861_stat1_IO();	    \
    MSP430_SEL_mcp73861_stat2_IO();	    \
    MSP430_DIR_mcp73861_stat1_INPUT();	    \
    MSP430_DIR_mcp73861_stat2_INPUT();	    \
    MSP430_WRITE_mcp73861_stat1(0);         \
    MSP430_WRITE_mcp73861_stat2(0);         \
  } while(0)


void 
mcp73861_init()
{
  MCP73861Pin_init();
} /* mcp73861_init */


enum mcp73861_result_t 
mcp73861_get_status()
{
  int stat1 = 0;
  int stat2 = 0;
  
  stat1 += ! MSP430_READ_mcp73861_stat1();
  stat2 += ! MSP430_READ_mcp73861_stat2();

  milli_wait(MCP73861_SAMPLING_PERIODE);
  stat1 += ! MSP430_READ_mcp73861_stat1();
  stat2 += ! MSP430_READ_mcp73861_stat2();

  milli_wait(MCP73861_SAMPLING_PERIODE);
  stat1 += ! MSP430_READ_mcp73861_stat1();
  stat2 += ! MSP430_READ_mcp73861_stat2();

  switch (stat1){
  case 0:
    stat1 = MCP73861_OFF;
    break;
  case 3:
    stat1 = MCP73861_ON;
    break;
  default:
    stat1 = MCP73861_FLASHING;
  }
  switch (stat2){
  case 0:
    stat2 = MCP73861_OFF;
    break;
  case 3:
    stat2 = MCP73861_ON;
    break;
  default:
    stat2 = MCP73861_FLASHING;
  }
  return MCP73861_STAT(stat1, stat2);
} /* mcp73861_get_status */
