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
 * \brief file defining the different frames involved with TDMA
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inrialpes.fr>
 * \date November 2009
 */

#ifndef _TDMA_FRAMES_H_
#define _TDMA_FRAMES_H_


typedef struct {
    uint8_t length, // length of the packet
            fctl; // frame control
} header_t;

#define HEADER_LENGTH sizeof(header_t)
/*
 * FCTL format:
 * | 7 6 5 4 3 2 1 0 |
 * |  type  |  addr  |
 * | frame  |  src   |
 * 
 */
#define HEADER_TYPE_MASK 0xF0
#define HEADER_ADDR_MASK 0x0F
#define HEADER_SET_TYPE(hdr, type) hdr.fctl= \
        ((hdr.fctl&HEADER_ADDR_MASK)|(type&HEADER_TYPE_MASK))
#define HEADER_GET_TYPE(hdr) (hdr.fctl&HEADER_TYPE_MASK)
#define HEADER_SET_ADDR(hdr, addr) hdr.fctl= \
        ((hdr.fctl&HEADER_TYPE_MASK)|(addr&HEADER_ADDR_MASK))
#define HEADER_GET_ADDR(hdr) (hdr.fctl&HEADER_ADDR_MASK)

typedef struct {
    uint8_t rssi, crc;
} footer_t;

#define FOOTER_LENGTH sizeof(footer_t)

typedef struct {
    header_t hdr;
    uint8_t seq;
    uint8_t ctl;
    uint8_t data;
} beacon_msg_t;

#define BEACON_LENGTH sizeof(beacon_msg_t)
#define BEACON_TYPE 0x10

typedef struct {
    header_t hdr;
    uint8_t ctl;
} control_msg_t;

#define CONTROL_LENGTH sizeof(beacon_msg_t)
#define CONTROL_TYPE       0x20
#define CONTROL_ATTACH_REQ 0xA0
#define CONTROL_ATTACH_OK  0xB0
#define CONTROL_ATTACH_ERR 0xC0

#define CONTROL_SET_TYPE(msg, type) msg.ctl=((msg.ctl&0x0F)|(type&0xF0))
#define CONTROL_GET_TYPE(msg) (msg.ctl&0xF0)
#define CONTROL_SET_ADDR(msg, data) msg.ctl=((msg.ctl&0xF0)|(data&0x0F))
#define CONTROL_GET_ADDR(msg) (msg.ctl&0x0F)

#define PACKET_SIZE_MAX 64
#define PAYLOAD_LENGTH_MAX PACKET_SIZE_MAX-(HEADER_LENGTH+FOOTER_LENGTH)

typedef struct {
    header_t hdr;
    uint8_t payload[PAYLOAD_LENGTH_MAX];
} data_msg_t;

#define DATA_LENGTH sizeof(data_msg_t)
#define DATA_TYPE 0x30


#endif
