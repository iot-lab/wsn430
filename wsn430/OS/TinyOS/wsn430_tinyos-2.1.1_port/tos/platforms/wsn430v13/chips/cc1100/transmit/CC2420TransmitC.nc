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
 * Implementation of the transmit path for the ChipCon CC2420 radio.
 *
 * @author Jonathan Hui <jhui@archrock.com>
 * @version $Revision: 1.2 $ $Date: 2008/06/17 07:28:24 $
 */

#include "IEEE802154.h"

configuration CC2420TransmitC {

  provides {
    interface StdControl;
    interface CC2420Transmit;
    interface RadioBackoff;
    interface ReceiveIndicator as EnergyIndicator;
    interface ReceiveIndicator as ByteIndicator;
  }
}

implementation {

  components CC1100TransmitP;
  StdControl = CC1100TransmitP;
  CC2420Transmit = CC1100TransmitP;
  RadioBackoff = CC1100TransmitP;
  EnergyIndicator = CC1100TransmitP.EnergyIndicator;
  ByteIndicator = CC1100TransmitP.ByteIndicator;

  components MainC;
  MainC.SoftwareInit -> CC1100TransmitP;
  MainC.SoftwareInit -> Alarm;
  
  components AlarmMultiplexC as Alarm;
  CC1100TransmitP.BackoffTimer -> Alarm;

  components HplCC1100PinsC as Pins;
  CC1100TransmitP.CSN -> Pins.CSN;
  CC1100TransmitP.SFD -> Pins.GDO0;

  components HplCC1100InterruptsC as Interrupts;
  CC1100TransmitP.InterruptSFD -> Interrupts.InterruptGDO0;
  
  //components HplCC1100AlarmC as Counter32khz32;
  //CC1100Transmit.CounterSFD -> Counter32khz32.Counter;

  components new CC1100SpiC() as Spi;
  CC1100TransmitP.SpiResource -> Spi;
  CC1100TransmitP.ChipSpiResource -> Spi;
  CC1100TransmitP.SNOP        -> Spi.SNOP;
  CC1100TransmitP.STX         -> Spi.STX;
  CC1100TransmitP.SRX         -> Spi.SRX;
  CC1100TransmitP.SIDLE       -> Spi.SIDLE;
  CC1100TransmitP.SFTX        -> Spi.SFTX;
  CC1100TransmitP.SFRX        -> Spi.SFRX;
  CC1100TransmitP.TXFIFO      -> Spi.TXFIFO;
  CC1100TransmitP.PKTSTATUS   -> Spi.PKTSTATUS;
  CC1100TransmitP.MARCSTATE   -> Spi.MARCSTATE;
  CC1100TransmitP.VERSION     -> Spi.VERSION;
  CC1100TransmitP.TXBYTES     -> Spi.TXBYTES;
  
  CC1100TransmitP.MCSM1      -> Spi.MCSM1;
  CC1100TransmitP.IOCFG0      -> Spi.IOCFG0;
  CC1100TransmitP.IOCFG2      -> Spi.IOCFG2;
  
  components CC2420ReceiveC;
  CC1100TransmitP.CC2420Receive -> CC2420ReceiveC;
  
  components CC2420PacketC;
  CC1100TransmitP.CC2420Packet -> CC2420PacketC;
  CC1100TransmitP.CC2420PacketBody -> CC2420PacketC;
  CC1100TransmitP.PacketTimeStamp -> CC2420PacketC;
  CC1100TransmitP.PacketTimeSyncOffset -> CC2420PacketC;

  components LedsC;
  CC1100TransmitP.Leds -> LedsC;
}
