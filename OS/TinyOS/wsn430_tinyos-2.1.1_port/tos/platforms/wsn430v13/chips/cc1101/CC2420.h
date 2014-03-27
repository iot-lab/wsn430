/*
 * Copyright (c) 2005-2006 Arch Rock Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 * - Neither the name of the Arch Rock Corporation nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * ARCHED ROCK OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE
 *
 * @author Jonathan Hui <jhui@archrock.com>
 * @author David Moss
 * @version $Revision: 1.19 $ $Date: 2009/09/17 23:36:36 $
 */

#ifndef __CC2420_H__
#define __CC2420_H__

typedef uint8_t cc2420_status_t;

#if defined(TFRAMES_ENABLED) && defined(IEEE154FRAMES_ENABLED)
#error "Both TFRAMES and IEEE154FRAMES enabled!"
#endif

/**
 * CC2420 header definition.
 *
 * An I-frame (interoperability frame) header has an extra network
 * byte specified by 6LowPAN
 *
 * Length = length of the header + payload of the packet, minus the size
 *   of the length byte itself (1).  This is what allows for variable
 *   length packets.
 *
 * FCF = Frame Control Field, defined in the 802.15.4 specs and the
 *   CC2420 datasheet.
 *
 * DSN = Data Sequence Number, a number incremented for each packet sent
 *   by a particular node.  This is used in acknowledging that packet,
 *   and also filtering out duplicate packets.
 *
 * DestPan = The destination PAN (personal area network) ID, so your
 *   network can sit side by side with another TinyOS network and not
 *   interfere.
 *
 * Dest = The destination address of this packet. 0xFFFF is the broadcast
 *   address.
 *
 * Src = The local node ID that generated the message.
 *
 * Network = The TinyOS network ID, for interoperability with other types
 *   of 802.15.4 networks.
 *
 * Type = TinyOS AM type.  When you create a new AMSenderC(AM_MYMSG),
 *   the AM_MYMSG definition is the type of packet.
 *
 * TOSH_DATA_LENGTH defaults to 28, it represents the maximum size of
 * the payload portion of the packet, and is specified in the
 * tos/types/message.h file.
 *
 * All of these fields will be filled in automatically by the radio stack
 * when you attempt to send a message.
 */
typedef nx_struct cc2420_header_t {
  nxle_uint8_t length;
  nxle_uint16_t fcf;
  nxle_uint8_t dsn;
  nxle_uint16_t destpan;
  nxle_uint16_t dest;
  nxle_uint16_t src;

#ifndef TFRAMES_ENABLED
  /** I-Frame 6LowPAN interoperability byte */
  nxle_uint8_t network;
#endif

  nxle_uint8_t type;
} cc2420_header_t;

/**
 * CC2420 Packet Footer
 */
typedef nx_struct cc2420_footer_t {
} cc2420_footer_t;

/**
 * CC2420 Packet metadata. Contains extra information about the message
 * that will not be transmitted.
 *
 * Note that the first two bytes automatically take in the values of the
 * FCS when the payload is full. Do not modify the first two bytes of metadata.
 */
typedef nx_struct cc2420_metadata_t {
  nx_uint8_t rssi;
  nx_uint8_t lqi;
  nx_uint8_t tx_power;
  nx_bool crc;
  nx_bool ack;
  nx_bool timesync;
  nx_uint32_t timestamp;
  nx_uint16_t rxInterval;

  /** Packet Link Metadata */
#ifdef PACKET_LINK
  nx_uint16_t maxRetries;
  nx_uint16_t retryDelay;
#endif
} cc2420_metadata_t;


typedef nx_struct cc2420_packet_t {
  cc2420_header_t packet;
  nx_uint8_t data[];
} cc2420_packet_t;


#ifndef TOSH_DATA_LENGTH
#define TOSH_DATA_LENGTH 28
#endif

#ifndef CC2420_DEF_CHANNEL
#define CC2420_DEF_CHANNEL 26
#endif

#ifndef CC2420_DEF_RFPOWER
#define CC2420_DEF_RFPOWER 31
#endif

/**
 * Ideally, your receive history size should be equal to the number of
 * RF neighbors your node will have
 */
#ifndef RECEIVE_HISTORY_SIZE
#define RECEIVE_HISTORY_SIZE 4
#endif

/**
 * The 6LowPAN NALP ID for a TinyOS network is 63 (TEP 125).
 */
#ifndef TINYOS_6LOWPAN_NETWORK_ID
#define TINYOS_6LOWPAN_NETWORK_ID 0x3f
#endif

enum {
  // size of the header not including the length byte
  MAC_HEADER_SIZE = sizeof( cc2420_header_t ) - 1,
  // size of the footer (FCS field)
  MAC_FOOTER_SIZE = sizeof( uint16_t ),
  // MDU
  MAC_PACKET_SIZE = MAC_HEADER_SIZE + TOSH_DATA_LENGTH + MAC_FOOTER_SIZE,

  CC2420_SIZE = MAC_HEADER_SIZE + MAC_FOOTER_SIZE,

  AM_OVERHEAD = 2,
};

enum cc2420_enums {
  CC2420_TIME_ACK_TURNAROUND = 7, // jiffies
  CC2420_TIME_VREN = 20,          // jiffies
  CC2420_TIME_SYMBOL = 2,         // 2 symbols / jiffy
  CC2420_BACKOFF_PERIOD = ( 20 / CC2420_TIME_SYMBOL ), // symbols
  CC2420_MIN_BACKOFF = ( 20 / CC2420_TIME_SYMBOL ),  // platform specific?
  CC2420_ACK_WAIT_DELAY = 256,    // jiffies
};



enum
{
  CC2420_INVALID_TIMESTAMP  = 0x80000000L,
};

#endif
