/*
 * phy_cc1100_regs.h
 *
 *  Created on: Jan 21, 2011
 *      Author: burindes
 */

#ifndef PHY_CC1100_REGS_H_
#define PHY_CC1100_REGS_H_

// Chipcon
// Product = CC1101
// Chip version = A   (VERSION = 0x04)
// Crystal accuracy = 10 ppm
// X-tal frequency = 27 MHz
// RF output power = 0 dBm
// RX filterbandwidth = 562.500000 kHz
// Deviation = 132 kHz
// Datarate = 249.664307 kBaud
// Modulation = (1) GFSK
// Manchester enable = (0) Manchester disabled
// RF Frequency = 860 MHz
// Channel spacing = 200 kHz
// Channel number = 0
// Optimization = Sensitivity
// Sync mode = (3) 30/32 sync word bits detected
// Format of RX/TX data = (0) Normal mode, use FIFOs for RX and TX
// CRC operation = (1) CRC calculation in TX and CRC check in RX enabled
// Forward Error Correction = (0) FEC disabled
// Length configuration = (1) Variable length packets, packet length configured by the first received byte after sync word.
// Packetlength = 255
// Preamble count = (2)  4 bytes
// Append status = 1
// Address check = (0) No address check
// FIFO autoflush = 0
// Device address = 0
static const uint8_t cc1100_regs[] = {
    0x0B, 0x0C,   // FSCTRL1   Frequency synthesizer control.
    0x0C, 0x00,   // FSCTRL0   Frequency synthesizer control.
    0x0D, 0x1F,   // FREQ2     Frequency control word, high byte.
    0x0E, 0xDA,   // FREQ1     Frequency control word, middle byte.
    0x0F, 0x12,   // FREQ0     Frequency control word, low byte.
    0x10, 0x2D,   // MDMCFG4   Modem configuration.
    0x11, 0x2F,   // MDMCFG3   Modem configuration.
    0x12, 0x13,   // MDMCFG2   Modem configuration.
    0x13, 0x22,   // MDMCFG1   Modem configuration.
    0x14, 0xE5,   // MDMCFG0   Modem configuration.
    0x15, 0x62,   // DEVIATN   Modem deviation setting (when FSK modulation is enabled).
    0x21, 0xB6,   // FREND1    Front end RX configuration.
    0x22, 0x10,   // FREND0    Front end TX configuration.
    0x18, 0x18,   // MCSM0     Main Radio Control State Machine configuration.
    0x19, 0x1D,   // FOCCFG    Frequency Offset Compensation Configuration.
    0x1A, 0x1C,   // BSCFG     Bit synchronization Configuration.
    0x1B, 0xC7,   // AGCCTRL2  AGC control.
    0x1C, 0x00,   // AGCCTRL1  AGC control.
    0x1D, 0xB0,   // AGCCTRL0  AGC control.
    0x23, 0xEA,   // FSCAL3    Frequency synthesizer calibration.
    0x24, 0x2A,   // FSCAL2    Frequency synthesizer calibration.
    0x25, 0x00,   // FSCAL1    Frequency synthesizer calibration.
    0x26, 0x1F,   // FSCAL0    Frequency synthesizer calibration.
    0x29, 0x59,   // FSTEST    Frequency synthesizer calibration.
    0x2C, 0x88,   // TEST2     Various test settings.
    0x2D, 0x31,   // TEST1     Various test settings.
    0x2E, 0x0B,   // TEST0     Various test settings.
    0x03, 0x07,   // FIFOTHR   RXFIFO and TXFIFO thresholds.
    0x07, 0x04,   // PKTCTRL1  Packet automation control.
    0x08, 0x05,   // PKTCTRL0  Packet automation control.
    0x06, 0xFF    // PKTLEN    Packet length.
};


#endif /* PHY_CC1100_REGS_H_ */
