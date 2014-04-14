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
  components CC1101ReceiveP;
  components CC2420PacketC;
  components new CC1101SpiC() as Spi;
  components CC2420ControlC;

  components HplCC1101PinsC as Pins;
  components HplCC1101InterruptsC as InterruptsC;

  components LedsC as Leds;
  CC1101ReceiveP.Leds -> Leds;

  StdControl = CC1101ReceiveP;
  CC2420Receive = CC1101ReceiveP;
  Receive = CC1101ReceiveP;
  PacketIndicator = CC1101ReceiveP.PacketIndicator;

  MainC.SoftwareInit -> CC1101ReceiveP;

  CC1101ReceiveP.CSN -> Pins.CSN;
  CC1101ReceiveP.FIFO -> Pins.GDO2;
  CC1101ReceiveP.InterruptFIFO -> InterruptsC.InterruptGDO2;


  CC1101ReceiveP.SpiResource -> Spi;
  CC1101ReceiveP.RXFIFO -> Spi.RXFIFO;
  CC1101ReceiveP.SFRX -> Spi.SFRX;
  CC1101ReceiveP.SRX -> Spi.SRX;
  CC1101ReceiveP.SFTX -> Spi.SFTX;
  CC1101ReceiveP.SIDLE -> Spi.SIDLE;

  CC1101ReceiveP.PKTSTATUS -> Spi.PKTSTATUS;
  CC1101ReceiveP.MARCSTATE -> Spi.MARCSTATE;
  CC1101ReceiveP.RXBYTES -> Spi.RXBYTES;

  CC1101ReceiveP.CC2420Packet -> CC2420PacketC;
  CC1101ReceiveP.CC2420PacketBody -> CC2420PacketC;
  CC1101ReceiveP.PacketTimeStamp -> CC2420PacketC;
  CC1101ReceiveP.CC2420Config -> CC2420ControlC;

}
