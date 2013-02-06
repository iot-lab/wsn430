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
 */

/**
 * Implementation for configuring a ChipCon CC2420 radio.
 *
 * @author Jonathan Hui <jhui@archrock.com>
 * @version $Revision: 1.3 $ $Date: 2008/05/14 21:33:07 $
 */

#include "CC2420.h"
#include "IEEE802154.h"

configuration CC2420ControlC {

  provides interface Resource;
  provides interface CC2420Config;
  provides interface CC2420Power;
  provides interface Read<uint16_t> as ReadRssi;
  
}

implementation {
  
  components CC1101ControlP;
  Resource = CC1101ControlP;
  CC2420Config = CC1101ControlP;
  CC2420Power = CC1101ControlP;
  ReadRssi = CC1101ControlP;

  components MainC;
  MainC.SoftwareInit -> CC1101ControlP;
  
  components AlarmMultiplexC as Alarm;
  CC1101ControlP.StartupTimer -> Alarm;

  components HplCC1101PinsC as Pins;
  CC1101ControlP.CSN -> Pins.CSN;
  CC1101ControlP.SOMI -> Pins.SOMI;

  // components HplCC1101InterruptsC as Interrupts;
  // CC2110ControlP.InterruptCCA -> Interrupts.InterruptCCA;

  components new CC1101SpiC() as Spi;
  CC1101ControlP.SpiResource -> Spi;
  
  CC1101ControlP.IOCFG2 -> Spi.IOCFG2;
  CC1101ControlP.IOCFG0 -> Spi.IOCFG0;
  CC1101ControlP.FIFOTHR -> Spi.FIFOTHR;
  CC1101ControlP.PKTCTRL1 -> Spi.PKTCTRL1;
  CC1101ControlP.CHANNR -> Spi.CHANNR;
  CC1101ControlP.FSCTRL1 -> Spi.FSCTRL1;
  CC1101ControlP.FREQ2 -> Spi.FREQ2;
  CC1101ControlP.FREQ1 -> Spi.FREQ1;
  CC1101ControlP.FREQ0 -> Spi.FREQ0;
  CC1101ControlP.MDMCFG4 -> Spi.MDMCFG4;
  CC1101ControlP.MDMCFG3 -> Spi.MDMCFG3;
  CC1101ControlP.MDMCFG2 -> Spi.MDMCFG2;
  CC1101ControlP.MDMCFG1 -> Spi.MDMCFG1;
  CC1101ControlP.MDMCFG0 -> Spi.MDMCFG0;
  CC1101ControlP.DEVIATN -> Spi.DEVIATN;
  CC1101ControlP.MCSM1 -> Spi.MCSM1;
  CC1101ControlP.MCSM0 -> Spi.MCSM0;
  
  CC1101ControlP.SRES -> Spi.SRES;
  CC1101ControlP.SRX -> Spi.SRX;
  CC1101ControlP.SFRX -> Spi.SFRX;
  CC1101ControlP.SFTX -> Spi.SFTX;
  CC1101ControlP.SIDLE -> Spi.SIDLE;
  CC1101ControlP.SXOFF -> Spi.SXOFF;
  CC1101ControlP.SPWD -> Spi.SPWD;
  
  CC1101ControlP.RSSI -> Spi.RSSI;
  CC1101ControlP.MARCSTATE -> Spi.MARCSTATE;


  components new CC1101SpiC() as SyncSpiC;
  CC1101ControlP.SyncResource -> SyncSpiC;

  components new CC1101SpiC() as RssiResource;
  CC1101ControlP.RssiResource -> RssiResource;
  
  components ActiveMessageAddressC;
  CC1101ControlP.ActiveMessageAddress -> ActiveMessageAddressC;

}

