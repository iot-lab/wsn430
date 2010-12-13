#ifndef __PLATFORM_CONF_H__
#define __PLATFORM_CONF_H__

#define WITH_CC2420


/* the low-level radio driver */
#ifdef WITH_CC1100
#define NETSTACK_CONF_RADIO cc1100_radio_driver
#else
#define NETSTACK_CONF_RADIO cc2420_radio_driver
#endif


#define CC2420_CONF_SYMBOL_LOOP_COUNT 800

/* Our clock resolution, this is the same as Unix HZ. */
#define CLOCK_CONF_SECOND 128UL

#define BAUD2UBR(baud) ((F_CPU/baud))

#define CCIF
#define CLIF

#define CC_CONF_INLINE inline

#define HAVE_STDINT_H
#define MSP430_MEMCPY_WORKAROUND 1
#include "msp430def.h"


/* Types for clocks and uip_stats */
typedef unsigned short uip_stats_t;
typedef unsigned long clock_time_t;
typedef unsigned long off_t;


#define ROM_ERASE_UNIT_SIZE  512
#define XMEM_ERASE_UNIT_SIZE (64*1024L)

#endif /* __PLATFORM_CONF_H__ */
