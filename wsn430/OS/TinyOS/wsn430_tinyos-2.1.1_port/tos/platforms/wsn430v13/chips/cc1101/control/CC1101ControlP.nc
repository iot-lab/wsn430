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
 * @author Jonathan Hui <jhui@archrock.com>
 * @author David Moss
 * @author Urs Hunkeler (ReadRssi implementation)
 * @version $Revision: 1.7 $ $Date: 2008/06/24 04:07:28 $
 */

#include "Timer.h"
#include "CC1101.h"

module CC1101ControlP @safe() {

  provides interface Init;
  provides interface Resource;
  provides interface CC2420Config;
  provides interface CC2420Power;
  provides interface Read<uint16_t> as ReadRssi;

  uses interface Alarm<T32khz,uint32_t> as StartupTimer;
  uses interface GeneralIO as CSN;
  uses interface GeneralIO as SOMI;
  uses interface ActiveMessageAddress;

  uses interface CC1101Register as IOCFG2;
  uses interface CC1101Register as IOCFG0;
  uses interface CC1101Register as FIFOTHR;
  uses interface CC1101Register as PKTCTRL1;
  uses interface CC1101Register as CHANNR;
  uses interface CC1101Register as FSCTRL1;
  uses interface CC1101Register as FREQ2;
  uses interface CC1101Register as FREQ1;
  uses interface CC1101Register as FREQ0;
  uses interface CC1101Register as MDMCFG4;
  uses interface CC1101Register as MDMCFG3;
  uses interface CC1101Register as MDMCFG2;
  uses interface CC1101Register as MDMCFG1;
  uses interface CC1101Register as MDMCFG0;
  uses interface CC1101Register as DEVIATN;
  uses interface CC1101Register as MCSM1;
  uses interface CC1101Register as MCSM0;
  
  uses interface CC1101Strobe as SRES;
  uses interface CC1101Strobe as SRX;
  uses interface CC1101Strobe as SFRX;
  uses interface CC1101Strobe as SFTX;
  uses interface CC1101Strobe as SIDLE;
  uses interface CC1101Strobe as SXOFF;
  uses interface CC1101Strobe as SPWD;
  
  uses interface CC1101Status as RSSI;
  uses interface CC1101Status as MARCSTATE;
  
  uses interface Resource as SpiResource;
  uses interface Resource as RssiResource;
  uses interface Resource as SyncResource;

  uses interface Leds;

}

implementation {

  typedef enum {
    S_VREG_STOPPED,
    S_VREG_STARTING,
    S_VREG_STARTED,
    S_XOSC_STARTING,
    S_XOSC_STARTED,
  } cc2420_control_state_t;

  uint8_t m_channel;
  
  uint8_t m_tx_power;
  
  uint16_t m_pan;
  
  uint16_t m_short_addr;
  
  bool m_sync_busy;
  
  /** TRUE if acknowledgments are enabled */
  bool autoAckEnabled;
  
  /** TRUE if acknowledgments are generated in hardware only */
  bool hwAutoAckDefault;
  
  /** TRUE if software or hardware address recognition is enabled */
  bool addressRecognition;
  
  /** TRUE if address recognition should also be performed in hardware */
  bool hwAddressRecognition;
  
  norace cc2420_control_state_t m_state = S_VREG_STOPPED;
  
  /***************** Prototypes ****************/

  task void sync();
  task void syncDone();
  
  void inline micro_delay(register unsigned int n)
	{
		__asm__ __volatile__ (
		"1: \n"
		" dec %[n] \n"
		" jne 1b \n"
					: [n] "+r"(n));
	}
	
	
  /***************** Init Commands ****************/
  command error_t Init.init() {
    call CSN.makeOutput();
    
    m_short_addr = call ActiveMessageAddress.amAddress();
    m_pan = call ActiveMessageAddress.amGroup();
    m_tx_power = CC2420_DEF_RFPOWER;
    m_channel = CC2420_DEF_CHANNEL;
    
    
#if defined(CC2420_NO_ADDRESS_RECOGNITION)
    addressRecognition = FALSE;
#else
    addressRecognition = TRUE;
#endif
    
#if defined(CC2420_HW_ADDRESS_RECOGNITION)
    hwAddressRecognition = TRUE;
#else
    hwAddressRecognition = FALSE;
#endif
    
    
#if defined(CC2420_NO_ACKNOWLEDGEMENTS)
    autoAckEnabled = FALSE;
#else
    autoAckEnabled = TRUE;
#endif
    
#if defined(CC2420_HW_ACKNOWLEDGEMENTS)
    hwAutoAckDefault = TRUE;
    hwAddressRecognition = TRUE;
#else
    hwAutoAckDefault = FALSE;
#endif
    
    
    return SUCCESS;
  }

  /***************** Resource Commands ****************/
  async command error_t Resource.immediateRequest() {
    error_t error = call SpiResource.immediateRequest();
    if ( error == SUCCESS ) {
      call CSN.clr();
    }
    return error;
  }

  async command error_t Resource.request() {
    return call SpiResource.request();
  }

  async command uint8_t Resource.isOwner() {
    return call SpiResource.isOwner();
  }

  async command error_t Resource.release() {
    atomic {
      call CSN.set();
      return call SpiResource.release();
    }
  }

  /***************** CC2420Power Commands ****************/
  async command error_t CC2420Power.startVReg() {
    atomic {
      if ( m_state != S_VREG_STOPPED ) {
        return FAIL;
      }
      m_state = S_VREG_STARTING;
    }
  
    call CSN.clr();
    call CSN.set();
    call CSN.clr();
    call CSN.set();
    micro_delay(80);
    call CSN.clr();
    
		//~ while (call SOMI.get());
		while (P5IN & (1<<2));
		call SRES.strobe();
		//~ while (call SOMI.get());
		while (P5IN & (1<<2));
		call CSN.set();
  	    
    //~ print_reg();
      
		m_state = S_VREG_STARTED;
    signal CC2420Power.startVRegDone();
    
    return SUCCESS;
  }

  async command error_t CC2420Power.stopVReg() {
    m_state = S_VREG_STOPPED;
    call CSN.clr();
    call SPWD.strobe();
    call CSN.set();
    return SUCCESS;
  }

  async command error_t CC2420Power.startOscillator() {
    atomic {
      if ( m_state != S_VREG_STARTED ) {
        return FAIL;
      }
        
      m_state = S_XOSC_STARTING;
      
    	call CSN.clr();
      call IOCFG2.write( CC1101_CONFIG_IOCFG2 );
			call CSN.set();
    	call CSN.clr();
      call IOCFG0.write( CC1101_CONFIG_IOCFG0 );
			call CSN.set();
    	call CSN.clr();
      call FIFOTHR.write( CC1101_CONFIG_FIFOTHR );
			call CSN.set();
    	call CSN.clr();
      call PKTCTRL1.write( CC1101_CONFIG_PKTCTRL1 );
			call CSN.set();
    	call CSN.clr();
      call CHANNR.write( (m_channel - 11)*3 );
      //~ call CHANNR.write( 0x0 );
			call CSN.set();
    	call CSN.clr();
      call FSCTRL1.write( CC1101_CONFIG_FSCTRL1 );
			call CSN.set();
    	call CSN.clr();
      call FREQ2.write( CC1101_CONFIG_FREQ2 );
			call CSN.set();
    	call CSN.clr();
      call FREQ1.write( CC1101_CONFIG_FREQ1 );
			call CSN.set();
    	call CSN.clr();
      call FREQ0.write( CC1101_CONFIG_FREQ0 );
			call CSN.set();
    	call CSN.clr();
      call MDMCFG4.write( CC1101_CONFIG_MDMCFG4 );
			call CSN.set();
    	call CSN.clr();
      call MDMCFG3.write( CC1101_CONFIG_MDMCFG3 );
			call CSN.set();
    	call CSN.clr();
      call MDMCFG2.write( CC1101_CONFIG_MDMCFG2 );
			call CSN.set();
    	call CSN.clr();
      call MDMCFG1.write( CC1101_CONFIG_MDMCFG1 );
			call CSN.set();
    	call CSN.clr();
      call MDMCFG0.write( CC1101_CONFIG_MDMCFG0 );
			call CSN.set();
    	call CSN.clr();
      call DEVIATN.write( CC1101_CONFIG_DEVIATN );
			call CSN.set();
    	call CSN.clr();
      call MCSM1.write( CC1101_CONFIG_MCSM1 );
			call CSN.set();
    	call CSN.clr();
      call MCSM0.write( CC1101_CONFIG_MCSM0 );
			call CSN.set();
      
      call CSN.clr();
      call SIDLE.strobe();
      call CSN.set();
      call CSN.clr();
      call SFRX.strobe();
      call CSN.set();
      call CSN.clr();
      call SFTX.strobe();
      call CSN.set();
      
			m_state = S_XOSC_STARTED;
      
      //~ print_reg();
      
      signal CC2420Power.startOscillatorDone();
    }
    return SUCCESS;
  }


  async command error_t CC2420Power.stopOscillator() {
    atomic {
      if ( m_state != S_XOSC_STARTED ) {
        return FAIL;
      }
      m_state = S_VREG_STARTED;
      call CSN.set();
      call CSN.clr();
      call SXOFF.strobe();
      call CSN.set();
    }
    return SUCCESS;
  }

  async command error_t CC2420Power.rxOn() {
  	uint8_t state;
    atomic {
      if ( m_state != S_XOSC_STARTED ) {
        return FAIL;
      }
      call CSN.set();
      call CSN.clr();
      call SRX.strobe();
      call CSN.set();
      
      do {
      	call CSN.clr();
      	call MARCSTATE.read(&state);
      	call CSN.set();
			} while (state != 0x0D); // state != RX
    }
    return SUCCESS;
  }

  async command error_t CC2420Power.rfOff() {
    atomic {  
      if ( m_state != S_XOSC_STARTED ) {
        return FAIL;
      }
      call CSN.set();
      call CSN.clr();
      call SIDLE.strobe();
      call CSN.set();
    }
    return SUCCESS;
  }

  
  /***************** CC2420Config Commands ****************/
  command uint8_t CC2420Config.getChannel() {
    atomic return m_channel;
  }

  command void CC2420Config.setChannel( uint8_t channel ) {
    atomic m_channel = channel;
  }

  async command uint16_t CC2420Config.getShortAddr() {
    atomic return m_short_addr;
  }

  command void CC2420Config.setShortAddr( uint16_t addr ) {
    atomic m_short_addr = addr;
  }

  async command uint16_t CC2420Config.getPanAddr() {
    atomic return m_pan;
  }

  command void CC2420Config.setPanAddr( uint16_t pan ) {
    atomic m_pan = pan;
  }

  /**
   * Sync must be called to commit software parameters configured on
   * the microcontroller (through the CC2420Config interface) to the
   * CC2420 radio chip.
   */
  command error_t CC2420Config.sync() {
    atomic {
      if ( m_sync_busy ) {
        return FAIL;
      }
      
      m_sync_busy = TRUE;
      if ( m_state == S_XOSC_STARTED ) {
        call SyncResource.request();
      } else {
        post syncDone();
      }
    }
    return SUCCESS;
  }

  /**
   * @param enableAddressRecognition TRUE to turn address recognition on
   * @param useHwAddressRecognition TRUE to perform address recognition first
   *     in hardware. This doesn't affect software address recognition. The
   *     driver must sync with the chip after changing this value.
   */
  command void CC2420Config.setAddressRecognition(bool enableAddressRecognition, bool useHwAddressRecognition) {
    atomic {
      addressRecognition = enableAddressRecognition;
      hwAddressRecognition = useHwAddressRecognition;
    }
  }
  
  /**
   * @return TRUE if address recognition is enabled
   */
  async command bool CC2420Config.isAddressRecognitionEnabled() {
    atomic return addressRecognition;
  }
  
  /**
   * @return TRUE if address recognition is performed first in hardware.
   */
  async command bool CC2420Config.isHwAddressRecognitionDefault() {
    atomic return hwAddressRecognition;
  }
  
  
  /**
   * Sync must be called for acknowledgement changes to take effect
   * @param enableAutoAck TRUE to enable auto acknowledgements
   * @param hwAutoAck TRUE to default to hardware auto acks, FALSE to
   *     default to software auto acknowledgements
   */
  command void CC2420Config.setAutoAck(bool enableAutoAck, bool hwAutoAck) {
    atomic autoAckEnabled = enableAutoAck;
    atomic hwAutoAckDefault = hwAutoAck;
  }
  
  /**
   * @return TRUE if hardware auto acks are the default, FALSE if software
   *     acks are the default
   */
  async command bool CC2420Config.isHwAutoAckDefault() {
    atomic return hwAutoAckDefault;    
  }
  
  /**
   * @return TRUE if auto acks are enabled
   */
  async command bool CC2420Config.isAutoAckEnabled() {
    atomic return autoAckEnabled;
  }
  
  /***************** ReadRssi Commands ****************/
  command error_t ReadRssi.read() { 
    return call RssiResource.request();
  }
  
  /***************** Spi Resources Events ****************/
  event void SyncResource.granted() {
    call CSN.clr();
    call SIDLE.strobe();
    call CSN.set();
    
    call CSN.clr();
    call CHANNR.write( (m_channel-11)*3 );
    call CSN.set();
    
    call CSN.clr();
    call SRX.strobe();
    call CSN.set();
    
    call SyncResource.release();
    post syncDone();
  }

  event void SpiResource.granted() {
    call CSN.clr();
    signal Resource.granted();
  }

  event void RssiResource.granted() { 
    uint8_t data;
    call CSN.clr();
    call RSSI.read(&data);
    call CSN.set();
    
    call RssiResource.release();
    signal ReadRssi.readDone(SUCCESS, (uint16_t)data); 
  }
  
  
  /***************** StartupTimer Events ****************/
  async event void StartupTimer.fired() {
    
  }

  /***************** ActiveMessageAddress Events ****************/
  async event void ActiveMessageAddress.changed() {
    atomic {
      m_short_addr = call ActiveMessageAddress.amAddress();
      m_pan = call ActiveMessageAddress.amGroup();
    }
    
    post sync();
  }
  
  /***************** Tasks ****************/
  /**
   * Attempt to synchronize our current settings with the CC2420
   */
  task void sync() {
    call CC2420Config.sync();
  }
  
  task void syncDone() {
    atomic m_sync_busy = FALSE;
    signal CC2420Config.syncDone( SUCCESS );
  }
  
  
  /***************** Functions ****************/
  
  /***************** Defaults ****************/
  default event void CC2420Config.syncDone( error_t error ) {
  }

  default event void ReadRssi.readDone(error_t error, uint16_t data) {
  }
  
}
