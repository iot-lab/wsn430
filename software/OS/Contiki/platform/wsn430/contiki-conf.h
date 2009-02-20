/* -*- C -*- */
/* @(#)$Id: contiki-conf.h,v 1.31 2008/11/06 20:45:06 nvt-se Exp $ */

#ifndef CONTIKI_CONF_H
#define CONTIKI_CONF_H

#define HAVE_STDINT_H
#include "msp430def.h"

#ifdef USE_CONF_APP
#include "contiki-conf-app.h"
#endif

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

#ifndef WITH_UIP
    #define WITH_UIP  0
#endif

#ifndef WITH_SLIP
    #define WITH_SLIP 0
#endif

#ifndef WITH_RIME
    #define WITH_RIME 0
#endif

#ifndef MAC_DRIVER
    #define MAC_DRIVER(...) nullmac_init(__VA_ARGS__)
#endif

#if WITH_SLIP && !WITH_UIP
    #error In order to enable SLIP, WITH_UIP must be set too
#endif

#if WITH_SLIP
#define UIP_CONF_IP_FORWARD 1
#endif

#define UIP_CONF_LLH_LEN         0
#define UIP_CONF_BUFFER_SIZE     128


#endif /* CONTIKI_CONF_H */
