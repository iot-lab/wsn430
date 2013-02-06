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
 * \file
 * \brief MCP73861 Battery Charger driver
 * \author Eric Fleury
 * \author Antoine Fraboulet
 * \date September 06
 */

#include <io.h>
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
static void
milli_wait(register unsigned int n)
{
  unsigned int i;
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
