/**
 * \file simplephy.c
 * \brief Simple PHY layer implementation
 */
#include <io.h>
#include "simplephy.h"
#include "cc1101.h"

#ifndef NULL
#define NULL 0x0
#endif

#define RADIO_STATUS_IDLE   0x0
#define RADIO_STATUS_RX     0x1
#define RADIO_STATUS_RX_PKT 0x2
#define RADIO_STATUS_TX     0x3

#define RADIO_FIFO_SIZE      64
#define RADIO_FIFOTHR_SETING 8
#define RADIO_TX_THR         29
#define RADIO_RX_THR         36

#define DELAY_UPDATE_RSSI    900

// file variables
static phy_handler_t  rx_handler;
static phy_notifier_t tx_notifier;
static uint16_t       radio_status;
static uint8_t        rx_buffer[255];
static uint16_t       rx_length;
static uint16_t       rx_index;
static uint8_t       *tx_buffer;
static uint16_t       tx_length;
static uint16_t       tx_index;

// prototypes
static uint16_t rx_ok(void);
static uint16_t tx_ok(void);
static uint16_t rx_unfill(void);
static uint16_t tx_refill(void);

void phy_init(void)
{
  // Initialize the radio driver
  cc1101_init();
  cc1101_cmd_idle();


  cc1101_cfg_append_status(CC1101_APPEND_STATUS_DISABLE);
  cc1101_cfg_crc_autoflush(CC1101_CRC_AUTOFLUSH_DISABLE);
  cc1101_cfg_white_data(CC1101_DATA_WHITENING_ENABLE);
  cc1101_cfg_crc_en(CC1101_CRC_CALCULATION_ENABLE);
  cc1101_cfg_freq_if(0x0C);
  cc1101_cfg_fs_autocal(CC1101_AUTOCAL_NEVER);

  cc1101_cfg_mod_format(CC1101_MODULATION_MSK);

  cc1101_cfg_sync_mode(CC1101_SYNCMODE_30_32);

  cc1101_cfg_manchester_en(CC1101_MANCHESTER_DISABLE);

  // set channel bandwidth (560 kHz)
  cc1101_cfg_chanbw_e(0);
  cc1101_cfg_chanbw_m(2);

  // set data rate (0xD/0x2F is 250kbps)
  cc1101_cfg_drate_e(0x0D);
  cc1101_cfg_drate_m(0x2F);

  uint8_t table[1];
  table[0] = 0xC2; // 10dBm
  cc1101_cfg_patable(table, 1);
  cc1101_cfg_pa_power(0);

  // reset function pointers
  rx_handler = NULL;
  tx_notifier = NULL;

  // reset status
  radio_status = RADIO_STATUS_IDLE;
}

uint16_t phy_send_frame(uint8_t frame[], uint16_t length)
{
  // simple phy doesn't handle larger packet size
  if (length > 255)
  {
    return 1;
  }

  cc1101_cmd_idle();
  cc1101_cmd_flush_tx();
  cc1101_cmd_calibrate();

  cc1101_cfg_gdo0(CC1101_GDOx_SYNC_WORD);
  cc1101_gdo0_int_set_falling_edge();
  cc1101_gdo0_int_clear();
  cc1101_gdo0_int_enable();
  cc1101_gdo0_register_callback(tx_ok);

  tx_buffer = frame;
  tx_length = length;

  if (tx_length > 63)
  {
    // set TX fifo threshold to 29
    cc1101_cfg_fifo_thr(8);

    cc1101_cfg_gdo2(CC1101_GDOx_TX_FIFO);
    cc1101_gdo2_int_set_falling_edge();
    cc1101_gdo2_int_clear();
    cc1101_gdo2_int_enable();
    cc1101_gdo2_register_callback(tx_refill);

    tx_index  = 63;

    cc1101_fifo_put((uint8_t*)(&tx_length), 1);
    cc1101_fifo_put(tx_buffer, 63);
  }
  else
  {
    cc1101_gdo2_int_disable();

    tx_index = tx_length;

    cc1101_fifo_put((uint8_t*)(&tx_length), 1);
    cc1101_fifo_put(frame, length);
  }

  cc1101_cmd_tx();

  radio_status = RADIO_STATUS_TX;

  return 0;
}

uint16_t phy_start_rx(void)
{
  if (radio_status == RADIO_STATUS_TX)
  {
    return 1;
  }

  cc1101_cmd_idle();
  cc1101_cmd_flush_rx();
  cc1101_cmd_flush_tx();
  cc1101_cmd_calibrate();

  cc1101_cfg_gdo0(CC1101_GDOx_SYNC_WORD);
  cc1101_gdo0_int_set_falling_edge();
  cc1101_gdo0_int_clear();
  cc1101_gdo0_int_enable();
  cc1101_gdo0_register_callback(rx_ok);

  // set RX fifo threshold to 36
  cc1101_cfg_fifo_thr(RADIO_FIFOTHR_SETING);

  cc1101_cfg_gdo2(CC1101_GDOx_RX_FIFO);
  cc1101_gdo2_int_set_rising_edge();
  cc1101_gdo2_int_clear();
  cc1101_gdo2_int_enable();
  cc1101_gdo2_register_callback(rx_unfill);

  cc1101_cmd_rx();

  // reset indicators
  rx_length = 0;
  rx_index = 0;

  radio_status = RADIO_STATUS_RX;

  return 0;
}


uint16_t phy_stop_rx(void)
{
  if (radio_status != RADIO_STATUS_RX)
  {
    return 1;
  }
  cc1101_cmd_idle();
  cc1101_gdo0_int_disable();
  cc1101_gdo2_int_disable();

  radio_status = RADIO_STATUS_IDLE;
  return 0;
}

uint16_t phy_cca(void)
{
  uint8_t pktstatus;
  switch (radio_status)
  {
    case RADIO_STATUS_IDLE:
      // set rx
      cc1101_cmd_rx();
      // give some time to update rssi measurment
      micro_delay(DELAY_UPDATE_RSSI);
      pktstatus = cc1101_status_pktstatus();
      // go back to idle
      cc1101_cmd_idle();
      break;
    case RADIO_STATUS_RX:
      pktstatus = cc1101_status_pktstatus();
      break;
    default:
      pktstatus = 0;
      break;
  }
  return pktstatus & 0x10;
}

uint8_t phy_rssi(void)
{
  uint8_t pktstatus;
  switch (radio_status)
  {
    case RADIO_STATUS_IDLE:
      // set rx
      cc1101_cmd_rx();
      // give some time to update rssi measurment
      micro_delay(DELAY_UPDATE_RSSI);
      pktstatus = cc1101_status_rssi();
      // go back to idle
      cc1101_cmd_idle();
      break;
    case RADIO_STATUS_RX:
      pktstatus = cc1101_status_rssi();
      break;
    default:
      pktstatus = 0x7F;
      break;
  }
  return pktstatus;
}

void phy_register_frame_received_handler(phy_handler_t cb)
{
  rx_handler = cb;
}

void phy_register_frame_sent_notifier(phy_notifier_t cb)
{
  tx_notifier = cb;
}

// Local Functions

/**
 * Function called when a packet has been received. (End Of Packet IRQ)
 * It first checks the CRC, then gets the final bytes from FIFO.
 * It calls then the notifier provided by the MAC layer
 */
static uint16_t rx_ok(void) {
  uint16_t len;
  radio_status = RADIO_STATUS_IDLE;

  // verify CRC result
  if ( !(cc1101_status_crc_lqi() & 0x80) )
  {
    cc1101_cmd_flush_rx();
    cc1101_cmd_rx();
    return 1;
  }

  // if the packet length is less than FIFO_THR
  // it's the first time we hear about it

  if (rx_index == 0)
  {
    cc1101_fifo_get((uint8_t*)&rx_length, 1);
  }

  // read the remaining bytes from the FIFO
  len = cc1101_status_rxbytes();

  // if length mismatch, restart rx
  if (len != (rx_length - rx_index) )
  {
    phy_start_rx();
    return 1;
  }

  cc1101_fifo_get(rx_buffer+rx_index, len);

  if (rx_handler != NULL)
  {
    (*rx_handler)(rx_buffer, rx_length);
  }

  return 1;
}

/**
 * Function called when a packet has been sent. (End Of Packet IRQ)
 * It calls then the notifier provided by the MAC layer
 */
static uint16_t tx_ok(void) {
  radio_status = RADIO_STATUS_IDLE;

  if (tx_notifier != NULL)
  {
    (*tx_notifier)();
  }

  return 1;
}

/**
 * Function called on FIFO RX above threshold interrupt.
 * It copies the amount of bytes from the FIFO to a local buffer,
 * and increment the corresponding index.
 */
static uint16_t rx_unfill(void)
{
  // check we are in RX
  if (radio_status != RADIO_STATUS_RX)
  {
    return 1;
  }

  // check the number of bytes in RX FIFO is really above threshold
  if (cc1101_status_rxbytes() < RADIO_RX_THR)
  {
    return 1;
  }

  // is it the first interrupt on rx fifo ?
  if (rx_index == 0)
  {
    // yes, read packet length
    cc1101_fifo_get((uint8_t*)&rx_length, 1);
  }

  // okay, let's unfill

  // prevent buffer overflow: restart rx
  if (rx_index + RADIO_RX_THR > 255)
  {
    phy_start_rx();
    return 1;
  }

  // read the bytes present in FIFO
  cc1101_fifo_get(rx_buffer+rx_index, RADIO_RX_THR);

  // update index
  rx_index += RADIO_RX_THR;

  return 1;
}

/**
 * Function called on TX FIFO below threshold interrupt.
 * It copies the amount of bytes from the local buffer to the FIFO,
 * and increments the corresponding index.
 */
static uint16_t tx_refill(void)
{
  // check we are in TX
  if (radio_status != RADIO_STATUS_TX)
  {
    return 1;
  }

  // check if everything has been put in FIFO
  if (tx_index == tx_length)
  {
    return 1;
  }

  // check the number of bytes in TX FIFO is really low
  if (cc1101_status_txbytes() > RADIO_TX_THR)
  {
    return 1;
  }

  // okay, let's refill
  uint16_t len;

  len = (tx_length - tx_index) > (RADIO_FIFO_SIZE-RADIO_TX_THR) ? (RADIO_FIFO_SIZE-RADIO_TX_THR) : (tx_length - tx_index);

  cc1101_fifo_put(tx_buffer+tx_index, len);
  tx_index += len;

  return 1;
}
