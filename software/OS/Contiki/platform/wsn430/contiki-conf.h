/* -*- C -*- */
/* @(#)$Id: contiki-conf.h,v 1.31 2008/11/06 20:45:06 nvt-se Exp $ */

#ifndef CONTIKI_CONF_H
#define CONTIKI_CONF_H

#define HAVE_STDINT_H
#include "msp430def.h"

#define CCIF
#define CLIF

#define CC_CONF_INLINE inline

#define PROCESS_CONF_NUMEVENTS 8
#define PROCESS_CONF_STATS 1

/* CPU target speed in Hz */
#define F_CPU 8000000uL

/* Our clock resolution, this is the same as Unix HZ. */
#define CLOCK_CONF_SECOND 64

typedef unsigned short clock_time_t;


/* External flash definitions */
typedef unsigned long off_t;
#define XMEM_ERASE_UNIT_SIZE (64*1024L)

/// UIP SECTION ///
typedef unsigned short uip_stats_t;

#define WITH_UIP  1
#define WITH_SLIP 0

#if WITH_SLIP && !WITH_UIP
    #error In order to enable SLIP, WITH_UIP must be set too
#endif

#if WITH_SLIP
#define UIP_CONF_IP_FORWARD 1
#endif

#define UIP_CONF_LLH_LEN         0
#define UIP_CONF_BUFFER_SIZE     90


#endif /* CONTIKI_CONF_H */
