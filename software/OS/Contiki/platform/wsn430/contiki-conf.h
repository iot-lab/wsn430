/* -*- C -*- */
/* @(#)$Id: contiki-conf.h,v 1.71 2010/02/08 22:12:29 adamdunkels Exp $ */

/*
 * Code mostly copied from the sky platform.
 * Ported by Clément Burin des Roziers <clement.burin-des-roziers@inrialpes.fr>
 */

#ifndef CONTIKI_CONF_H
#define CONTIKI_CONF_H

// #define WITH_NULLMAC 1

/* Specifies the default MAC driver */
#define MAC_CONF_CSMA               1

#define XMAC_CONF_COMPOWER          0
#define CXMAC_CONF_COMPOWER         1


#if WITH_UIP6
#define MAC_CONF_DRIVER             cxmac_driver
#define MAC_CONF_CHANNEL_CHECK_RATE 8
#define RIME_CONF_NO_POLITE_ANNOUCEMENTS 0
#define CXMAC_CONF_ANNOUNCEMENTS    0
#define XMAC_CONF_ANNOUNCEMENTS     0
#else /* WITH_UIP6 */
#define MAC_CONF_DRIVER             xmac_driver
#define MAC_CONF_CHANNEL_CHECK_RATE 4
#define TIMESYNCH_CONF_ENABLED 1
#define RIME_CONF_NO_POLITE_ANNOUCEMENTS 1
#define XMAC_CONF_ANNOUNCEMENTS     1
#define CXMAC_CONF_ANNOUNCEMENTS    1
#endif /* WITH_UIP6 */

#define QUEUEBUF_CONF_NUM          16

#define PACKETBUF_CONF_ATTRS_INLINE 1

#ifndef RF_CHANNEL
#define RF_CHANNEL              26
#endif /* RF_CHANNEL */

#define IEEE802154_CONF_PANID       0xABCD

#define SHELL_VARS_CONF_RAM_BEGIN 0x1100
#define SHELL_VARS_CONF_RAM_END 0x2000



#define CFS_CONF_OFFSET_TYPE	long

#define PROFILE_CONF_ON 0
#define ENERGEST_CONF_ON 1

#define HAVE_STDINT_H
#define MSP430_MEMCPY_WORKAROUND 1
#include "msp430def.h"


#define CCIF
#define CLIF

#define CC_CONF_INLINE inline

#define AODV_COMPLIANCE
#define AODV_NUM_RT_ENTRIES 32

#define TMOTE_SKY 1
#define WITH_ASCII 1

#define PROCESS_CONF_NUMEVENTS 8
#define PROCESS_CONF_STATS 1
/*#define PROCESS_CONF_FASTPOLL    4*/

/* CPU target speed in Hz */
#define F_CPU 8000000uL

/* Our clock resolution, this is the same as Unix HZ. */
#define CLOCK_CONF_SECOND 128

#define BAUD2UBR(baud) ((F_CPU/baud))

#ifdef WITH_UIP6

#define RIMEADDR_CONF_SIZE              8

#define UIP_CONF_LL_802154              1
#define UIP_CONF_LLH_LEN                0

#ifndef UIP_CONF_ROUTER
#define UIP_CONF_ROUTER			0
#endif

#define UIP_CONF_IPV6                   1
#define UIP_CONF_IPV6_QUEUE_PKT         1
#define UIP_CONF_IPV6_CHECKS            1
#define UIP_CONF_IPV6_REASSEMBLY        0
#define UIP_CONF_NETIF_MAX_ADDRESSES    3
#define UIP_CONF_ND6_MAX_PREFIXES       3
#define UIP_CONF_ND6_MAX_NEIGHBORS      4
#define UIP_CONF_ND6_MAX_DEFROUTERS     2
#define UIP_CONF_IP_FORWARD             0
#define UIP_CONF_BUFFER_SIZE		240

#define SICSLOWPAN_CONF_COMPRESSION_IPV6        0
#define SICSLOWPAN_CONF_COMPRESSION_HC1         1
#define SICSLOWPAN_CONF_COMPRESSION_HC01        2
#define SICSLOWPAN_CONF_COMPRESSION             SICSLOWPAN_CONF_COMPRESSION_HC01
#ifndef SICSLOWPAN_CONF_FRAG
#define SICSLOWPAN_CONF_FRAG                    1
#define SICSLOWPAN_CONF_MAXAGE                  8
#endif /* SICSLOWPAN_CONF_FRAG */
#define SICSLOWPAN_CONF_CONVENTIONAL_MAC	1
#define SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS       2
#else /* WITH_UIP6 */
#define UIP_CONF_IP_FORWARD      1
#define UIP_CONF_BUFFER_SIZE     108
#endif /* WITH_UIP6 */

#define UIP_CONF_ICMP_DEST_UNREACH 1

#define UIP_CONF_DHCP_LIGHT
#define UIP_CONF_LLH_LEN         0
#define UIP_CONF_RECEIVE_WINDOW  48
#define UIP_CONF_TCP_MSS         48
#define UIP_CONF_MAX_CONNECTIONS 4
#define UIP_CONF_MAX_LISTENPORTS 8
#define UIP_CONF_UDP_CONNS       12
#define UIP_CONF_FWCACHE_SIZE    30
#define UIP_CONF_BROADCAST       1
#define UIP_ARCH_IPCHKSUM        1
#define UIP_CONF_UDP             1
#define UIP_CONF_UDP_CHECKSUMS   1
#define UIP_CONF_PINGADDRCONF    0
#define UIP_CONF_LOGGING         0

#define UIP_CONF_TCP_SPLIT       0


typedef unsigned short uip_stats_t;
typedef unsigned short clock_time_t;


/* External flash definitions */
typedef unsigned long off_t;
#define ROM_ERASE_UNIT_SIZE  512
#define XMEM_ERASE_UNIT_SIZE (64*1024L)

/* Use the first 64k of external flash for node configuration */
#define NODE_ID_XMEM_OFFSET     (0 * XMEM_ERASE_UNIT_SIZE)

/* Use the second 64k of external flash for codeprop. */
#define EEPROMFS_ADDR_CODEPROP  (1 * XMEM_ERASE_UNIT_SIZE)

#define CFS_XMEM_CONF_OFFSET    (2 * XMEM_ERASE_UNIT_SIZE)
#define CFS_XMEM_CONF_SIZE      (1 * XMEM_ERASE_UNIT_SIZE)

#define CFS_RAM_CONF_SIZE 4096


#ifdef PROJECT_CONF_H
#include PROJECT_CONF_H
#endif /* PROJECT_CONF_H */



#endif /* CONTIKI_CONF_H */
