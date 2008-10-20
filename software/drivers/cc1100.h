/**
 * \file cc1100.h
 * \brief CC1100 driver header
 * \author Clément Burin des Roziers
 * \date October 08
 */
#ifndef _CC1100_H
#define _CC1100_H

// other
void micro_delay(register unsigned int n);

// standard
/**
 * radio initialization procedure
 */
void cc1100_init(void);

uint16_t cc1100_status(void);

// commands

/**
 * force chip to reset all registers to default state
 */
void cc1100_cmd_reset(void);

/**
 * stop cristal oscillator
 */
void cc1100_cmd_xoff(void);

/**
 * calibrate frequency synthetizer
 */
void cc1100_cmd_calibrate(void);

/**
 * enable rx.
 */
void cc1100_cmd_rx(void);

/**
 * enable tx. if in rx with cca enabled, go to tx if channel clear
 */
void cc1100_cmd_tx(void);

/**
 * stop tx/rx/calibration/wor
 */
void cc1100_cmd_idle(void);

/**
 * start wake-on-radio : periodic channel sampling with RX polling
 */
void cc1100_cmd_wor(void);

/**
 * enter power down
 */
void cc1100_cmd_pwd(void);

/**
 * flush RX FIFO
 */
void cc1100_cmd_flush_rx(void);

/**
 * flush TX FIFO
 */
void cc1100_cmd_flush_tx(void);

/**
 * reset real time clock to Event1 value for WOR
 */
void cc1100_cmd_reset_wor(void);

/**
 * does nothing, update status byte
 */
void cc1100_cmd_nop(void);

// FIFO access

/**
 * copy a buffer to the radio TX FIFO
 * \param buffer a pointer to the buffer
 * \param length the number of bytes to copy
 */
void cc1100_fifo_put(uint8_t* buffer, uint16_t length);

/**
 * copy the content of the radio RX FIFO to a buffer
 * \param buffer a pointer to the buffer
 * \param length the number of bytes to copy
 **/
void cc1100_fifo_get(uint8_t* buffer, uint16_t length);


// Power Table Config
/**
 * configure the radio chip with the given power table
 * \param table pointer to the array containing the values
 * \param length number of elements (max = 8)
 */
void cc1100_cfg_patable(uint8_t table[], uint16_t length);

// radio config
#define CC1100_GDOx_RX_FIFO           0x00 /* assert above threshold, deassert when below         */
#define CC1100_GDOx_RX_FIFO_EOP       0x01 /* assert above threshold or EOP, deassert when empty  */
#define CC1100_GDOx_TX_FIFO           0x02 /* assert above threshold, deassert when below         */
#define CC1100_GDOx_TX_THR_FULL       0x03 /* asserts TX FIFO full. De-asserts when below thr     */
#define CC1100_GDOx_RX_OVER           0x04 /* asserts when RX overflow, deassert when flushed     */
#define CC1100_GDOx_TX_UNDER          0x05 /* asserts when RX underflow, deassert when flushed    */
#define CC1100_GDOx_SYNC_WORD         0x06 /* assert SYNC sent/recv, deasserts on EOP             */
					   /* In RX, de-assert on overflow or bad address         */
					   /* In TX, de-assert on underflow                       */
#define CC1100_GDOx_RX_OK             0x07 /* assert when RX PKT with CRC ok, de-assert on 1byte  */
                                           /* read from RX Fifo                                   */
#define CC1100_GDOx_PREAMB_OK         0x08 /* assert when preamble quality reached : PQI/PQT ok   */
#define CC1100_GDOx_CCA               0x09 /* Clear channel assessment. High when RSSI level is   */
					   /* below threshold (dependent on the current CCA_MODE) */
/**
 * gdo0 output pin configuration
 * example : use 0x06 for sync/eop or 0x0 for rx fifo threshold
 * \param cfg the configuration value
 */
void cc1100_cfg_gdo0(uint8_t cfg);

/**
 * gdo2 output pin configuration
 * example : use 0x06 for sync/eop or 0x0 for rx fifo threshold
 * \param cfg the configuration value
 */
void cc1100_cfg_gdo2(uint8_t cfg);

/**
 * Set the threshold for both RX and TX FIFOs
 * corresponding values are : 
 * 
 * value   0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | 15 \n
 * TX     61 | 57 | 53 | 49 | 45 | 41 | 37 | 33 | 29 | 25 | 21 | 17 | 13 |  9 |  5 | 1 \n
 * RX      4 |  8 | 12 | 16 | 20 | 24 | 28 | 32 | 36 | 40 | 44 | 48 | 52 | 56 | 60 | 64
 * 
 * \param cfg the configuration value
 */
void cc1100_cfg_fifo_thr(uint8_t cfg);

/**
 * Set the packet length in fixed packet length mode
 * or the maximum packet length in variable length mode
 * \param cfg the configuration value
 */
void cc1100_cfg_packet_length(uint8_t cfg);

/**
 * Set the preamble quality estimator threshold
 * (values are 0-7)
 * \param cfg the configuration value
 */
void cc1100_cfg_pqt(uint8_t cfg);

#define CC1100_CRC_AUTOFLUSH_ENABLE  0x1
#define CC1100_CRC_AUTOFLUSH_DISABLE 0x0
/**
 * enable/disable the automatic flush of RX FIFO when CRC is not OK
 * \param cfg the configuration value
 */
void cc1100_cfg_crc_autoflush(uint8_t cfg);

#define CC1100_APPEND_STATUS_ENABLE  0x1
#define CC1100_APPEND_STATUS_DISABLE 0x0
/**
 * enable/disable the appending of 2 information bytes at the end of
 * a received packet
 * \param cfg the configuration value
 */
void cc1100_cfg_append_status(uint8_t cfg);

#define CC1100_ADDR_NO_CHECK                 0x0
#define CC1100_ADDR_CHECK_NO_BROADCAST       0x1
#define CC1100_ADDR_CHECK_BROADCAST_0        0x2
#define CC1100_ADDR_CHECK_NO_BROADCAST_0_255 0x3
/**
 * control the address check mode
 * \param cfg the configuration value
 */
void cc1100_cfg_adr_check(uint8_t cfg);

#define CC1100_DATA_WHITENING_ENABLE  0x1
#define CC1100_DATA_WHITENING_DISABLE 0x0
/**
 * turn data whitening on/off
 * \param cfg the configuration value
 */
void cc1100_cfg_white_data(uint8_t cfg);


#define CC1100_CRC_CALCULATION_ENABLE  0x1
#define CC1100_CRC_CALCULATION_DISABLE 0x0
/**
 * turn CRC calculation on/off
 * \param cfg the configuration value
 */
void cc1100_cfg_crc_en(uint8_t cfg);

#define CC1100_PACKET_LENGTH_FIXED    0x0
#define CC1100_PACKET_LENGTH_VARIABLE 0x1
#define CC1100_PACKET_LENGTH_INFINITE 0x2
/**
 * configure the packet length mode
 * \param cfg the configuration value
 */
void cc1100_cfg_length_config(uint8_t cfg);

/**
 * Set the device address for packet filtration
 * \param cfg the configuration value
 */
void cc1100_cfg_device_addr(uint8_t cfg);

/**
 * Set the channel number.
 * \param cfg the configuration value
 */
void cc1100_cfg_chan(uint8_t cfg);

/**
 * Set the exponent of the channel bandwidth
 * (values are 0-3)
 * \param cfg the configuration value
 */
void cc1100_cfg_chanbw_e(uint8_t cfg);

/**
 * Set mantissa of the channel bandwidth
 * (values are 0-3)
 * \param cfg the configuration value
 */
void cc1100_cfg_chanbw_m(uint8_t cfg);

/**
 * Set the exponent of the data symbol rate
 * (values are 0-16)
 * \param cfg the configuration value
 */
void cc1100_cfg_drate_e(uint8_t cfg);

/**
 * Set the mantissa of the data symbol rate
 * (values are 0-255)
 * \param cfg the configuration value
 */
void cc1100_cfg_drate_m(uint8_t cfg);

#define CC1100_MODULATION_2FSK 0x00
#define CC1100_MODULATION_GFSK 0x01
#define CC1100_MODULATION_ASK  0x03
#define CC1100_MODULATION_MSK  0x07
/**
 * Set the signal modulation
 * \param cfg the configuration value
 */
void cc1100_cfg_mod_format(uint8_t cfg);

#define CC1100_MANCHESTER_ENABLE  0x1
#define CC1100_MANCHESTER_DISABLE 0x0
/**
 * Set manchester encoding on/off
 * \param cfg the configuration value
 */
void cc1100_cfg_manchester_en(uint8_t cfg);


#define CC1100_SYNCMODE_NO_PREAMB      0x0
#define CC1100_SYNCMODE_15_16          0x1
#define CC1100_SYNCMODE_16_16          0x2
#define CC1100_SYNCMODE_30_32          0x3
#define CC1100_SYNCMODE_NO_PREAMB_CS   0x4
#define CC1100_SYNCMODE_15_16_CS       0x5
#define CC1100_SYNCMODE_16_16_CS       0x6
#define CC1100_SYNCMODE_30_32_CS       0x7
/**
 * select the sync-word qualifier mode
 * \param cfg the configuration value
 */
void cc1100_cfg_sync_mode(uint8_t cfg);

#define CC1100_FEC_ENABLE  0x1
#define CC1100_FEC_DISABLE 0x0
/**
 * Set forward error correction on/off
 * supported in fixed packet length mode only
 * \param cfg the configuration value
 */
void cc1100_cfg_fec_en(uint8_t cfg);

/**
 * Set the minimum number of preamble bytes to be tramsitted \n
 * Setting :      0  |  1  |  2  |  3  |  4  |  5  |  6  |  7 \n
 * nb. of bytes : 2  |  3  |  4  |  6  |  8  |  12 |  16 |  24 
 * \param cfg the configuration value
 */
void cc1100_cfg_num_preamble(uint8_t cfg);

/**
 * Set the channel spacing exponent
 * (values are 0-3)
 * \param cfg the configuration value
 */
void cc1100_cfg_chanspc_e(uint8_t cfg);

/**
 * Set the channel spacing mantissa
 * (values are 0-255)
 * \param cfg the configuration value
 */
void cc1100_cfg_chanspc_m(uint8_t cfg);


#define CC1100_RX_TIME_RSSI_ENABLE  0x1
#define CC1100_RX_TIME_RSSI_DISABLE 0x0
/**
 * Set direct RX termination based on rssi measurement
 * \param cfg the configuration value
 */
void cc1100_cfg_rx_time_rssi(uint8_t cfg);

/**
 * Set timeout for syncword search in RX for WOR and normal op
 * (values are 0-7)
 * \param cfg the configuration value
 */
void cc1100_cfg_rx_time(uint8_t cfg);

#define CC1100_CCA_MODE_ALWAYS      0x0
#define CC1100_CCA_MODE_RSSI        0x1
#define CC1100_CCA_MODE_PKT_RX      0x2
#define CC1100_CCA_MODE_RSSI_PKT_RX 0x3
/**
 * Set the CCA mode reflected in CCA signal
 * \param cfg the configuration value
 */
void cc1100_cfg_cca_mode(uint8_t cfg);

#define CC1100_RXOFF_MODE_IDLE     0x00
#define CC1100_RXOFF_MODE_FSTXON   0x01 /* freq synth on, ready to Tx */
#define CC1100_RXOFF_MODE_TX       0x02 
#define CC1100_RXOFF_MODE_STAY_RX  0x03
/**
 * Set the behavior after a packet RX
 * \param cfg the configuration value
 */
void cc1100_cfg_rxoff_mode(uint8_t cfg);

#define CC1100_TXOFF_MODE_IDLE     0x00
#define CC1100_TXOFF_MODE_FSTXON   0x01 /* freq synth on, ready to Tx */
#define CC1100_TXOFF_MODE_STAY_TX  0x02
#define CC1100_TXOFF_MODE_RX       0x03
/**
 * Set the behavior after packet TX
 * \param cfg the configuration value
 */
void cc1100_cfg_txoff_mode(uint8_t cfg);


#define CC1100_AUTOCAL_NEVER             0x00
#define CC1100_AUTOCAL_IDLE_TO_TX_RX     0x01
#define CC1100_AUTOCAL_TX_RX_TO_IDLE     0x02
#define CC1100_AUTOCAL_4TH_TX_RX_TO_IDLE 0x03
/**
 * Set auto calibration policy
 * \param cfg the configuration value
 */
void cc1100_cfg_fs_autocal(uint8_t cfg);

/**
 * Set the relative threshold for asserting Carrier Sense \n
 * Setting :    0     |  1  |  2   |  3 \n
 * thr     : disabled | 6dB | 10dB | 14dB \n
 * \param cfg the configuration value
 */
void cc1100_cfg_carrier_sense_rel_thr(uint8_t cfg);

/**
 * Set the absolute threshold for asserting Carrier Sense
 * referenced to MAGN_TARGET \n
 * Setting :    -8    |  -7  |  -1  |       0        |  1  |   7 \n
 * thr     : disabled | -7dB | -1dB | at MAGN_TARGET | 1dB |  7dB \n
 * \param cfg the configuration value
 */
void cc1100_cfg_carrier_sense_abs_thr(uint8_t cfg);

/**
 * Set event0 timeout register for WOR operation
 * \param cfg the configuration value
 */
void cc1100_cfg_event0(uint16_t cfg);

#define CC1100_RC_OSC_ENABLE  0x0
#define CC1100_RC_OSC_DISABLE 0x1
/**
 * Set the RC oscillator on/off, needed by WOR
 * \param cfg the configuration value
 */
void cc1100_cfg_rc_pd(uint8_t cfg);

/**
 * Set the event1 timeout register
 * \param cfg the configuration value
 */
void cc1100_cfg_event1(uint8_t cfg);

/**
 * Set the WOR resolution
 * \param cfg the configuration value
 */
void cc1100_cfg_wor_res(uint8_t cfg);

/**
 * select the PA power setting, index of the patable
 * \param cfg the configuration value
 */
void cc1100_cfg_pa_power(uint8_t cfg);

// Status Registers access
/**
 * read the register containing the last CRC calculation match
 * and LQI estimate
 */
uint8_t cc1100_status_crc_lqi(void);

/**
 * read the RSSI
 */
uint8_t cc1100_status_rssi(void);

/**
 * read the main radio state machine state
 */
uint8_t cc1100_status_marcstate(void);

/**
 * read the high byte of the WOR timer
 */
uint8_t cc1100_status_wortime1(void);

/**
 * read the low byte of the WOR timer 
 */
uint8_t cc1100_status_wortime0(void);

/**
 * read the packet status register
 */
uint8_t cc1100_status_pktstatus(void);

/**
 * read the number of bytes in TX FIFO
 */
uint8_t cc1100_status_txbytes(void);

/**
 * read the number of bytes in RX FIFO
 */
uint8_t cc1100_status_rxbytes(void);


// GDOx int config & access

/**
 * enable interrupt for GDO0
 */
void cc1100_gdo0_int_enable(void);

/**
 * disable interrupt for GDO0
 */
void cc1100_gdo0_int_disable(void);
/**
 * clear interrupt for GDO0
 */
void cc1100_gdo0_int_clear(void);
/**
 * configure interrupt for GDO0 on high to low transition
 */
void cc1100_gdo0_int_set_falling_edge(void);
/**
 * configure interrupt for GDO0 on low to high transition
 */
void cc1100_gdo0_int_set_rising_edge(void);
/**
 * read the state of GDO0
 */
uint16_t cc1100_gdo0_read(void);
/**
 * register a callback function for GDO0 interrupt
 * \param cb a function pointer
 */
void cc1100_gdo0_register_callback(void (*cb)(void));

/**
 * enable interrupt for GDO2
 */
void cc1100_gdo2_int_enable(void);
/**
 * disable interrupt for GDO2
 */
void cc1100_gdo2_int_disable(void);
/**
 * clear interrupt for GDO2
 */
void cc1100_gdo2_int_clear(void);
/**
 * configure interrupt for GDO2 on high to low transition
 */
void cc1100_gdo2_int_set_falling_edge(void);
/**
 * configure interrupt for GDO2 on low to high transition
 */
void cc1100_gdo2_int_set_rising_edge(void);
/**
 * read the state of GDO2
 */
uint16_t cc1100_gdo2_read(void);
/**
 * register a callback function for GDO2 interrupt
 * \param cb a function pointer
 */
void cc1100_gdo2_register_callback(void (*cb)(void));

// direct access to registers
/**
 * read the content of any cc1100 register
 * \param addr the address of the register
 * \return the value of the register
 */
uint8_t cc1100_read_reg(uint8_t addr);

/**
 * write a value to any cc1100 register
 * \param addr the address of the register
 * \param value the value of to be written
 */
void cc1100_write_reg(uint8_t addr, uint8_t value);

#endif
