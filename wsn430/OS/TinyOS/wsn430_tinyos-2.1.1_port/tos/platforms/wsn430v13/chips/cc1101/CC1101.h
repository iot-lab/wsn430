/*
 * Copyright (c) 2005-2006 Rincon Research Corporation
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
 * - Neither the name of the Rincon Research Corporation nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * RINCON RESEARCH OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE
 */

/**
 * 500 kBaud
 */
 
/**
 * All frequency settings assume a 27 MHz crystal.
 * If you have a 27 MHz crystal, you'll need to fix the defined FREQ registers
 * 
 * @author Jared Hill
 * @author David Moss
 * @author Roland Hendel
 */
 
#ifndef CC1101_H
#define CC1101_H

//~ #warning "*** INCLUDING CC1101 RADIO ***"

enum {
  CC1101_RADIO_ID = unique( "Unique_CC1101_Radio" ),
};


enum{
  CC1101_PA_PLUS_10 = 0xC3,
  CC1101_PA_PLUS_5 = 0x85,
  CC1101_PA_PLUS_0 = 0x8E,
  CC1101_PA_MINUS_5 = 0x57,
  CC1101_PA_MINUS_10 = 0x34,
};

enum CC1101_config_reg_state_enums {
  CC1101_CONFIG_IOCFG2 = 0x01, // data in RX FIFO or EOP
  CC1101_CONFIG_IOCFG0 = 0x06, // SYNCWORD / EOP
  CC1101_CONFIG_FIFOTHR = 0x0F, // FIFO threshold max
  CC1101_CONFIG_PKTCTRL1 = 0x24, // append status, no addr check
  CC1101_CONFIG_PKTCTRL0 = 0x45, // Data whitening, crc computation, variable length
  CC1101_CONFIG_CHANNR = 0x00,
  CC1101_CONFIG_FSCTRL1 = 0x06, // freq_if
  CC1101_CONFIG_FREQ2 = 0x1F, // default freq = 860MHz
  CC1101_CONFIG_FREQ1 = 0xDA,
  CC1101_CONFIG_FREQ0 = 0x12,
  CC1101_CONFIG_MDMCFG4 = 0x2D, // 500kHz BW
  CC1101_CONFIG_MDMCFG3 = 0x2F, // datarate: 250kbps
  CC1101_CONFIG_MDMCFG2 = 0x73, // MSK, 30/32bit sync word
  CC1101_CONFIG_MDMCFG1 = 0x43, // 8 bytes preamble
  CC1101_CONFIG_MDMCFG0 = 0x94, // 333kHz channel spacing
  CC1101_CONFIG_DEVIATN = 0x00, // deviation for MSK
  CC1101_CONFIG_MCSM1 = 0x33, // RX after TX, Idle after RX
  CC1101_CONFIG_MCSM0 = 0x18, // Calibrate when going from idle to RX/TX
   
};

enum cc1101_config_reg_addr_enums {

  CC1101_IOCFG2 = 0x00,
  CC1101_IOCFG1 = 0x01,
  CC1101_IOCFG0 = 0x02,
  CC1101_FIFOTHR = 0x03,
  CC1101_SYNC1 = 0x04,
  CC1101_SYNC0 = 0x05,
  CC1101_PKTLEN = 0x06,
  CC1101_PKTCTRL1 = 0x07,
  CC1101_PKTCTRL0 = 0x08,
  CC1101_ADDR = 0x09,
  CC1101_CHANNR = 0x0A,
  CC1101_FSCTRL1 = 0x0B,
  CC1101_FSCTRL0 = 0x0C,
  CC1101_FREQ2 = 0x0D,
  CC1101_FREQ1 = 0x0E,
  CC1101_FREQ0 = 0x0F,
  CC1101_MDMCFG4 = 0x10,
  CC1101_MDMCFG3 = 0x11,
  CC1101_MDMCFG2 = 0x12,
  CC1101_MDMCFG1 = 0x13,
  CC1101_MDMCFG0 = 0x14,
  CC1101_DEVIATN = 0x15,
  CC1101_MCSM2 = 0x16,
  CC1101_MCSM1 = 0x17,
  CC1101_MCSM0 = 0x18,
  CC1101_FOCCFG = 0x19,
  CC1101_BSCFG = 0x1A,
  CC1101_AGCTRL2 = 0x1B,
  CC1101_AGCTRL1 = 0x1C,
  CC1101_AGCTRL0 = 0x1D,
  CC1101_WOREVT1 = 0x1E,
  CC1101_WOREVT0 = 0x1F,
  CC1101_WORCTRL = 0x20,
  CC1101_FREND1 = 0x21,
  CC1101_FREND0 = 0x22,
  CC1101_FSCAL3 = 0x23,
  CC1101_FSCAL2 = 0x24,
  CC1101_FSCAL1 = 0x25,
  CC1101_FSCAL0 = 0x26,
  CC1101_RCCTRL1 = 0x27, 
  CC1101_RCCTRL0 = 0x28,
  CC1101_FSTEST = 0x29,
  CC1101_PTEST = 0x2A,
  CC1101_AGCTEST = 0x2B,
  CC1101_TEST2 = 0x2C,
  CC1101_TEST1 = 0x2D,
  CC1101_TEST0 = 0x2E,
  CC1101_PARTNUM = 0x30,
  CC1101_VERSION = 0x31,
  CC1101_FREQEST = 0x32,
  CC1101_LQI = 0x33,
  CC1101_RSSI = 0x34,
  CC1101_MARCSTATE = 0x35,
  CC1101_WORTIME1 = 0x36,
  CC1101_WORTIME0 = 0x37,
  CC1101_PKTSTATUS = 0x38,
  CC1101_VCO_VC_DAC = 0x39,
  CC1101_TXBYTES = 0x3A,
  CC1101_RXBYTES = 0x3B,
};


enum cc1101_cmd_strobe_enums {

  CC1101_SRES = 0x30,
  CC1101_SFSTXON = 0x31,
  CC1101_SXOFF = 0x32,
  CC1101_SCAL = 0x33,
  CC1101_SRX = 0x34,
  CC1101_STX = 0x35,
  CC1101_SIDLE = 0x36,
  CC1101_SWOR = 0x38,
  CC1101_SPWD = 0x39,
  CC1101_SFRX = 0x3A,
  CC1101_SFTX = 0x3B,
  CC1101_SWORRST = 0x3C,
  CC1101_SNOP = 0x3D,
  
};

enum cc1101_addr_enums {

  CC1101_PATABLE = 0x3E,
  CC1101_TXFIFO = 0x3F,
  CC1101_RXFIFO = 0xBF,

};


#endif



