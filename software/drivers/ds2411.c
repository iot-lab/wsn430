
/*
  part of this code is inspired from the tinos1.x driver
  for DS2411 that is included in the file DS2411M.nc 
  original author Cory Sharp <cssharp@eecs.berkeley.edu>
*/


/* "Copyright (c) 2000-2003 The Regents of the University of California.  
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement
 * is hereby granted, provided that the above copyright notice, the following
 * two paragraphs and the author appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY
 * OF CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 */

//@author Cory Sharp <cssharp@eecs.berkeley.edu>

#include <io.h>
#include <signal.h>
#include <iomacros.h>
#include <stdio.h>
#include <string.h>
#include "ds2411.h"


/*

  The 1-wire timings suggested by the DS2411 data sheet are incorrect,
  incomplete, or unclear.  The timings provided the app note 522 work:

    http://www.maxim-ic.com/appnotes.cfm/appnote_number/522

*/


static uint8_t ds2411_id[ 8 ];

/***************************************************/
/***************************************************/
/***************************************************/

/* adjust timing loop according to wait loop */

// 8MHz -> *2
// 4MHz -> *1

#define MICRO  *2

enum ds2411_timing_t // micro seconds
  {
    STD_A = 6   MICRO,  // t_W1L / t_RL
    STD_B = 64  MICRO,  // t_SLOT - t_W1L
    STD_C = 60  MICRO,  // t_W0L
    STD_D = 10  MICRO,  // t_SLOT - t_W0L

    STD_E = 6   MICRO,  // near-max t_MSR

    STD_F = 55  MICRO,  // t_REC
    STD_G = 0   MICRO,  // unused
    STD_H = 480 MICRO,  // t_RSTL 
    STD_I = 70  MICRO,  // t_MSP
    STD_J = 410 MICRO   // t_REC
  };


static void __inline__ micro_wait(register unsigned int n)
{
  /* MCLK is running 8MHz, 1 cycle = 125ns    */
  /* n=1 -> waiting = 4*125ns = 500ns         */

  /* MCLK is running 4MHz, 1 cycle = 250ns    */
  /* n=1 -> waiting = 4*250ns = 1000ns        */

    __asm__ __volatile__ (
		"1: \n"
		" dec	%[n] \n"      /* 2 cycles */
		" jne	1b \n"        /* 2 cycles */
        : [n] "+r"(n));
}


/***************************************************/
/***************************************************/
/***************************************************/

static uint8_t ds2411_crc8_byte( uint8_t crc, uint8_t byte )
{
  int i;
  crc ^= byte;
  for( i=0; i<8; i++ )
    {
      if( crc & 1 )
	crc = (crc >> 1) ^ 0x8c;
      else
	crc >>= 1;
    }
  return crc;
}

/***************************************************/
/***************************************************/
/***************************************************/

#define _SI_   static inline

#define BIT(b) (1 << (b))

/* n name, p port, b bit */
#define MSP430_ASSIGN_PIN(n, p, b)                                             \
_SI_ void MSP430_DIR_##n##_INPUT()        { P##p##DIR &= ~BIT(b); }            \
_SI_ void MSP430_DIR_##n##_OUTPUT()       { P##p##DIR |=  BIT(b); }            \
_SI_ void MSP430_SEL_##n##_IO()           { P##p##SEL &= ~BIT(b); }            \
_SI_ void MSP430_SEL_##n##_FUN()          { P##p##SEL |=  BIT(b); }            \
_SI_ void MSP430_SET_##n##_LOW()          { P##p##OUT &= ~BIT(b); }            \
_SI_ void MSP430_SET_##n##_HIGH()         { P##p##OUT |=  BIT(b); }            \
_SI_ int  MSP430_READ_##n()               { return ((P##p##IN >> (b)) & 1); }  \
_SI_ void MSP430_WRITE_##n(int val)        \
   {                                       \
       if (val)                            \
         P##p##OUT |=  BIT(b);             \
       else                                \
         P##p##OUT &= ~BIT(b);             \
   }

MSP430_ASSIGN_PIN(ds2411,2,4)

/***************************************************/
/***************************************************/
/***************************************************/

/* DS2411 Pin to MSP430 Pin */

#define  DS2411Pin_init()                   \
do {                                        \
  MSP430_SEL_ds2411_IO();                   \
  MSP430_DIR_ds2411_INPUT();                \
  MSP430_SET_ds2411_LOW();                  \
} while(0)

#define DS2411Pin_output_low()              \
do {                                        \
  MSP430_DIR_ds2411_OUTPUT();               \
  MSP430_SET_ds2411_LOW();                  \
} while (0)

#define DS2411Pin_output_high()             \
do {                                        \
  MSP430_DIR_ds2411_OUTPUT();               \
  MSP430_SET_ds2411_HIGH();                 \
} while (0)

#define DS2411Pin_prepare_read()            MSP430_DIR_ds2411_INPUT()
#define DS2411Pin_read()                    MSP430_READ_ds2411()

/***************************************************/
/* INIT ********************************************/
/***************************************************/

static int ds2411_reset(void) // >= 960us
{
  int present;
  DS2411Pin_output_low();
  micro_wait(STD_H);            // t_RSTL
  DS2411Pin_output_high();
  DS2411Pin_prepare_read();
  micro_wait(STD_I);            // t_MSP
  present = DS2411Pin_read();
  micro_wait(STD_J);            // t_REC
  return (present == 0);
}

/***************************************************/
/* WRITE BIT ***************************************/
/***************************************************/

static void ds2411_write_bit_one(void) // >= 70us
{
  DS2411Pin_output_low();
  micro_wait(STD_A);  //t_W1L
  DS2411Pin_output_high();
  micro_wait(STD_B);  //t_SLOT - t_W1L
}

static void ds2411_write_bit_zero(void) // >= 70us
{
  DS2411Pin_output_low();
  micro_wait(STD_C);  //t_W0L
  DS2411Pin_output_high();
  micro_wait(STD_D);  //t_SLOT - t_W0L
}

/***************************************************/
/* WRITE *******************************************/
/***************************************************/

static void ds2411_write_bit( int is_one ) // >= 70us
{
  if(is_one)
    ds2411_write_bit_one();
  else
    ds2411_write_bit_zero();
}

static void ds2411_write_byte( uint8_t byte ) // >= 560us
{
  uint8_t bit;
  for( bit=0x01; bit!=0; bit<<=1 )
    ds2411_write_bit( byte & bit );
}

/***************************************************/
/* READ ********************************************/
/***************************************************/

static uint8_t ds2411_read_bit(void) // >= 70us
{
  int bit;
  DS2411Pin_output_low();
  micro_wait(STD_A);  //t_RL
  DS2411Pin_output_high();
  DS2411Pin_prepare_read();
  micro_wait(STD_E);  //near-max t_MSR
  bit = DS2411Pin_read();
  micro_wait(STD_F);  //t_REC
  return bit;
}

static uint8_t ds2411_read_byte(void) // >= 560us
{
  uint8_t byte = 0;
  uint8_t bit;
  for( bit=0x01; bit!=0; bit<<=1 )
    {
      if( ds2411_read_bit() )
	byte |= bit;
    }
  return byte;
}

/***************************************************/
/***************************************************/
/***************************************************/

static enum ds2411_result_t DS2411_init(void) // >= 6000us
{
  int retry = 5;
  uint8_t id[8];
  
  bzero( ds2411_id, 8 );
  DS2411Pin_init();
  DS2411Pin_output_high();
  ds2411_reset();

  while( retry-- > 0 )
    {
      int crc = 0;
      if( ds2411_reset() )
	{
	  uint8_t* byte;
	  ds2411_write_byte(0x33);
	  for( byte=id+7; byte!=id-1; byte-- )
	    {
	      *byte = ds2411_read_byte();
	      crc = ds2411_crc8_byte( crc, *byte );
	    }

	  if( crc == 0 )
	    {
	      memcpy( ds2411_id, id, 8 );
	      return DS2411_SUCCESS;
	    }
	}
    }
  return DS2411_ERROR;
}


/***************************************************/
/***************************************************/
/***************************************************/

#if defined(PRINT_DS2411)
static void print_hex2(int x)
{
  if (x < 0x10)
    printf("0");
  printf("%x",x);
}
#endif /* DEBUG_DS2411 */

#if defined(PRINT_DS2411)
void ds2411_print_id(ds2411_serial_number_t *id)
{
  int i;
  printf(" crc %x :",id->fields.crc);
  for(i=1; i < 7; i++)
    {
      print_hex2(id->raw[i]);
      printf(":");
    }
  printf(" family %x\n",id->fields.family);
}
#endif /* DEBUG_DS2411 */

enum ds2411_result_t ds2411_init(void)
{
  return DS2411_init();
}

void ds2411_get_id(ds2411_serial_number_t *id)
{
  memcpy( id->raw, ds2411_id, 8 );
}

/***************************************************/
/***************************************************/
/***************************************************/
