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
 * Implementation of the receive path for the ChipCon CC2420 radio.
 *
 * @author Jonathan Hui <jhui@archrock.com>
 * @version $Revision: 1.4 $ $Date: 2009/08/14 20:33:43 $
 */

configuration CC2420ReceiveC {

  provides interface StdControl;
  provides interface CC2420Receive;
  provides interface Receive;
  provides interface ReceiveIndicator as PacketIndicator;

}

implementation {
  components MainC;
  components CC1100ReceiveP;
  components CC2420PacketC;
  components new CC1100SpiC() as Spi;
  components CC2420ControlC;
  
  components HplCC1100PinsC as Pins;
  components HplCC1100InterruptsC as InterruptsC;

  components LedsC as Leds;
  CC1100ReceiveP.Leds -> Leds;

  StdControl = CC1100ReceiveP;
  CC2420Receive = CC1100ReceiveP;
  Receive = CC1100ReceiveP;
  PacketIndicator = CC1100ReceiveP.PacketIndicator;

  MainC.SoftwareInit -> CC1100ReceiveP;
  
  CC1100ReceiveP.CSN -> Pins.CSN;
  CC1100ReceiveP.FIFO -> Pins.GDO2;
  CC1100ReceiveP.InterruptFIFO -> InterruptsC.InterruptGDO2;
  
  
  CC1100ReceiveP.SpiResource -> Spi;
  CC1100ReceiveP.RXFIFO -> Spi.RXFIFO;
  CC1100ReceiveP.SFRX -> Spi.SFRX;
  CC1100ReceiveP.SRX -> Spi.SRX;
  CC1100ReceiveP.SFTX -> Spi.SFTX;
  CC1100ReceiveP.SIDLE -> Spi.SIDLE;
  
  CC1100ReceiveP.PKTSTATUS -> Spi.PKTSTATUS;
  CC1100ReceiveP.MARCSTATE -> Spi.MARCSTATE;
  CC1100ReceiveP.RXBYTES -> Spi.RXBYTES;
  
  CC1100ReceiveP.CC2420Packet -> CC2420PacketC;
  CC1100ReceiveP.CC2420PacketBody -> CC2420PacketC;
  CC1100ReceiveP.PacketTimeStamp -> CC2420PacketC;
  CC1100ReceiveP.CC2420Config -> CC2420ControlC;

}
