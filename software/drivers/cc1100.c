/**
 * \file cc1100.c
 * \brief CC1100 driver implementation
 * \author Clément Burin des Roziers
 * \date October 08
 */
#include <io.h>
#include <signal.h>

#include "cc1100.h"

#include "spi1.h"

#include "cc1100_gdo.h"
#include "cc1100_spi.h"
#include "cc1100_globals.h"

static void (*gdo0_cb)(void);
static void (*gdo2_cb)(void);

void inline micro_delay(register unsigned int n)
{
    __asm__ __volatile__ (
  "1: \n"
  " dec %[n] \n"
  " jne 1b \n"
        : [n] "+r"(n));
}

critical uint16_t cc1100_status(void)
{
  CC1100_STROBE_CMD(CC1100_STROBE_SNOP | CC1100_ACCESS_READ);
  return spi1_tx_return_value;
}

#define STATE_IDLE 0x0
#define STATE_RX   0x1
#define STATE_TX   0x2
#define STATE_CAL  0x8

static void wait_status(uint8_t state) {
  eint();
  uint8_t s;
  do {
      s = (cc1100_status() >> 4) & 0x07;
  } while (s != state);
}

critical void cc1100_init(void)
{
  gdo0_cb = 0x0;
  gdo2_cb = 0x0;
  
  SPI1_INIT();
  GDO_INIT();
  CC1100_RESET_CHIP();
  
  // write default frequency : 868MHz
  CC1100_WRITE_REG(CC1100_REG_FREQ2, 0x20);
  CC1100_WRITE_REG(CC1100_REG_FREQ1, 0x25);
  CC1100_WRITE_REG(CC1100_REG_FREQ0, 0xED);
  
  // value from SmartRF
  CC1100_WRITE_REG(CC1100_REG_DEVIATN, 0x0);
}

critical void cc1100_cmd_reset(void)
{
  CC1100_STROBE_CMD(CC1100_STROBE_SRES);
}

critical void cc1100_cmd_xoff(void)
{
  CC1100_STROBE_CMD(CC1100_STROBE_SXOFF);
}

critical void cc1100_cmd_calibrate(void)
{
  cc1100_cmd_idle();
  CC1100_STROBE_CMD(CC1100_STROBE_SCAL);
  wait_status(STATE_IDLE);
}

critical void cc1100_cmd_rx(void)
{
  CC1100_STROBE_CMD(CC1100_STROBE_SRX);
}

critical void cc1100_cmd_tx(void)
{
  CC1100_STROBE_CMD(CC1100_STROBE_STX);
}

critical void cc1100_cmd_idle(void)
{
  CC1100_STROBE_CMD(CC1100_STROBE_SIDLE);
  wait_status(STATE_IDLE);
}

critical void cc1100_cmd_wor(void)
{
  CC1100_STROBE_CMD(CC1100_STROBE_SWOR);
}

critical void cc1100_cmd_pwd(void)
{
  CC1100_STROBE_CMD(CC1100_STROBE_SPWD);
}

critical void cc1100_cmd_flush_rx(void)
{
  cc1100_cmd_idle();
  CC1100_STROBE_CMD(CC1100_STROBE_SFRX);
  wait_status(STATE_IDLE);
}

critical void cc1100_cmd_flush_tx(void)
{
  cc1100_cmd_idle();
  CC1100_STROBE_CMD(CC1100_STROBE_SFTX);
  wait_status(STATE_IDLE);
}

critical void cc1100_cmd_reset_wor(void)
{
  CC1100_STROBE_CMD(CC1100_STROBE_SWORRST);
}

critical void cc1100_cmd_nop(void)
{
  CC1100_STROBE_CMD(CC1100_STROBE_SNOP);
}

critical void cc1100_fifo_put(uint8_t* buffer, uint16_t length)
{
  CC1100_WRITE_REG_BURST(CC1100_DATA_FIFO_ADDR, buffer, length);
}

critical void cc1100_fifo_get(uint8_t* buffer, uint16_t length)
{
  CC1100_READ_REG_BURST(CC1100_DATA_FIFO_ADDR, buffer, length);
}

critical void cc1100_cfg_patable(uint8_t table[], uint16_t length)
{
  length = (length > 8) ? 8 : length;
  CC1100_WRITE_REG_BURST(CC1100_PATABLE_ADDR, table, length);
}

critical void cc1100_cfg_gdo0(uint8_t cfg)
{
  CC1100_WRITE_REG(CC1100_REG_IOCFG0, (cfg&0x3F));
}

critical void cc1100_cfg_gdo2(uint8_t cfg)
{
  CC1100_WRITE_REG(CC1100_REG_IOCFG2, (cfg&0x3F));
}

critical void cc1100_cfg_fifo_thr(uint8_t cfg)
{
  CC1100_WRITE_REG(CC1100_REG_FIFOTHR, (cfg&0x0F));
}

critical void cc1100_cfg_packet_length(uint8_t cfg)
{
  CC1100_WRITE_REG(CC1100_REG_PKTLEN, (cfg));
}

critical void cc1100_cfg_pqt(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_PKTCTRL1, reg);
  reg  = (reg & 0x1F) | ((cfg << 5) & 0xE0);
  CC1100_WRITE_REG(CC1100_REG_PKTCTRL1, reg);
}

critical void cc1100_cfg_crc_autoflush(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_PKTCTRL1, reg);
  reg  = (reg & 0xF7) | ((cfg << 3) & 0x08);
  CC1100_WRITE_REG(CC1100_REG_PKTCTRL1, reg);
}

critical void cc1100_cfg_append_status(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_PKTCTRL1, reg);
  reg  = (reg & 0xFB) | ((cfg << 2) & 0x04);
  CC1100_WRITE_REG(CC1100_REG_PKTCTRL1, reg);
}

critical void cc1100_cfg_adr_check(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_PKTCTRL1, reg);
  reg  = (reg & 0xFC) | ((cfg << 0) & 0x03);
  CC1100_WRITE_REG(CC1100_REG_PKTCTRL1, reg);
}

critical void cc1100_cfg_white_data(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_PKTCTRL0, reg);
  reg = (reg & 0xBF) | ((cfg << 6) & 0x40);
  CC1100_WRITE_REG(CC1100_REG_PKTCTRL0, reg);
}

critical void cc1100_cfg_crc_en(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_PKTCTRL0, reg);
  reg = (reg & 0xFB) | ((cfg << 2) & 0x04);
  CC1100_WRITE_REG(CC1100_REG_PKTCTRL0, reg);
}

critical void cc1100_cfg_length_config(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_PKTCTRL0, reg);
  reg = (reg & 0xFC) | ((cfg << 0) & 0x03);
  CC1100_WRITE_REG(CC1100_REG_PKTCTRL0, reg);
}

critical void cc1100_cfg_device_addr(uint8_t cfg)
{
  CC1100_WRITE_REG(CC1100_REG_ADDR, cfg);
}

critical void cc1100_cfg_chan(uint8_t cfg)
{
  CC1100_WRITE_REG(CC1100_REG_CHANNR, cfg);
}

critical void cc1100_cfg_freq_if(uint8_t cfg)
{
  CC1100_WRITE_REG(CC1100_REG_FSCTRL1, (cfg & 0x1F));
}


critical void cc1100_cfg_chanbw_e(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_MDMCFG4, reg);
  reg = (reg & 0x3F) | ((cfg << 6) & 0xC0);
  CC1100_WRITE_REG(CC1100_REG_MDMCFG4, reg);
}

critical void cc1100_cfg_chanbw_m(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_MDMCFG4, reg);
  reg = (reg & 0xCF) | ((cfg<<4) & 0x30);
  CC1100_WRITE_REG(CC1100_REG_MDMCFG4, reg);
}

critical void cc1100_cfg_drate_e(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_MDMCFG4, reg);
  reg = (reg & 0xF0) | ((cfg) & 0x0F);
  CC1100_WRITE_REG(CC1100_REG_MDMCFG4, reg);
}

critical void cc1100_cfg_drate_m(uint8_t cfg)
{
  CC1100_WRITE_REG(CC1100_REG_MDMCFG3, cfg);
}

critical void cc1100_cfg_mod_format(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_MDMCFG2, reg);
  reg  = (reg & 0x8F) | ((cfg << 4) & 0x70);
  CC1100_WRITE_REG(CC1100_REG_MDMCFG2, reg);
}

critical void cc1100_cfg_manchester_en(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_MDMCFG2, reg);
  reg  = (reg & 0xF7) | ((cfg << 3) & 0x08);
  CC1100_WRITE_REG(CC1100_REG_MDMCFG2, reg);
}

critical void cc1100_cfg_sync_mode(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_MDMCFG2, reg);
  reg  = (reg & 0xF8) | ((cfg << 0) & 0x07);
  CC1100_WRITE_REG(CC1100_REG_MDMCFG2, reg);
}

critical void cc1100_cfg_fec_en(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_MDMCFG1, reg);
  reg  = (reg & 0x7F) | ((cfg << 7) & 0x80);
  CC1100_WRITE_REG(CC1100_REG_MDMCFG1, reg);
}

critical void cc1100_cfg_num_preamble(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_MDMCFG1, reg);
  reg  = (reg & 0x8F) | ((cfg << 4) & 0x70);
  CC1100_WRITE_REG(CC1100_REG_MDMCFG1, reg);
}

critical void cc1100_cfg_chanspc_e(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_MDMCFG1, reg);
  reg  = (reg & 0xFE) | ((cfg << 0) & 0x01);
  CC1100_WRITE_REG(CC1100_REG_MDMCFG1, reg);
}

critical void cc1100_cfg_chanspc_m(uint8_t cfg)
{
  CC1100_WRITE_REG(CC1100_REG_MDMCFG0, cfg);
}

critical void cc1100_cfg_rx_time_rssi(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_MCSM2, reg);
  reg  = (reg & 0xEF) | ((cfg << 4) & 0x10);
  CC1100_WRITE_REG(CC1100_REG_MCSM2, reg);
}

critical void cc1100_cfg_rx_time(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_MCSM2, reg);
  reg  = (reg & 0xF8) | ((cfg << 0) & 0x07);
  CC1100_WRITE_REG(CC1100_REG_MCSM2, reg);
}

critical void cc1100_cfg_cca_mode(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_MCSM1, reg);
  reg  = (reg & 0xCF) | ((cfg << 4) & 0x30);
  CC1100_WRITE_REG(CC1100_REG_MCSM1, reg);
}

critical void cc1100_cfg_rxoff_mode(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_MCSM1, reg);
  reg  = (reg & 0xF3) | ((cfg << 2) & 0x0C);
  CC1100_WRITE_REG(CC1100_REG_MCSM1, reg);
}

critical void cc1100_cfg_txoff_mode(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_MCSM1, reg);
  reg  = (reg & 0xFC) | ((cfg << 0) & 0x03);
  CC1100_WRITE_REG(CC1100_REG_MCSM1, reg);
}

critical void cc1100_cfg_fs_autocal(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_MCSM0, reg);
  reg  = (reg & 0xCF) | ((cfg << 4) & 0x30);
  CC1100_WRITE_REG(CC1100_REG_MCSM0, reg);
}

critical void cc1100_cfg_carrier_sense_rel_thr(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_AGCCTRL1, reg);
  reg  = (reg & 0xCF) | ((cfg << 4) & 0x30);
  CC1100_WRITE_REG(CC1100_REG_AGCCTRL1, reg);
}

critical void cc1100_cfg_carrier_sense_abs_thr(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_AGCCTRL1, reg);
  reg  = (reg & 0xF0) | ((cfg << 0) & 0x0F);
  CC1100_WRITE_REG(CC1100_REG_AGCCTRL1, reg);
}

critical void cc1100_cfg_event0(uint16_t cfg)
{
  uint8_t reg;
  reg = (uint8_t)((cfg >> 8) & 0x00FF);
  CC1100_READ_REG(CC1100_REG_WOREVT1, cfg);
  reg = (uint8_t)((cfg) & 0x00FF);
  CC1100_READ_REG(CC1100_REG_WOREVT0, cfg);
}

critical void cc1100_cfg_rc_pd(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_WORCTRL, reg);
  reg  = (reg & 0x7F) | ((cfg << 7) & 0x80);
  CC1100_WRITE_REG(CC1100_REG_WORCTRL, reg);
}

critical void cc1100_cfg_event1(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_WORCTRL, reg);
  reg  = (reg & 0x8F) | ((cfg << 4) & 0x70);
  CC1100_WRITE_REG(CC1100_REG_WORCTRL, reg);
}

critical void cc1100_cfg_wor_res(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_WORCTRL, reg);
  reg  = (reg & 0xFC) | ((cfg << 0) & 0x03);
  CC1100_WRITE_REG(CC1100_REG_WORCTRL, reg);
}

critical void cc1100_cfg_pa_power(uint8_t cfg)
{
  uint8_t reg;
  CC1100_READ_REG(CC1100_REG_FREND0, reg);
  reg  = (reg & 0xF8) | ((cfg << 0) & 0x07);
  CC1100_WRITE_REG(CC1100_REG_FREND0, reg);
}

critical uint8_t cc1100_status_crc_lqi(void)
{
  uint8_t reg;
  CC1100_READ_STATUS_REG(CC1100_REG_LQI, reg);
  return reg;
}

critical uint8_t cc1100_status_rssi(void)
{
  uint8_t reg;
  CC1100_READ_STATUS_REG(CC1100_REG_RSSI, reg);
  return reg;
}

critical uint8_t cc1100_status_marcstate(void)
{
  uint8_t reg;
  CC1100_READ_STATUS_REG(CC1100_REG_MARCSTATE, reg);
  return reg;
}

critical uint8_t cc1100_status_wortime1(void)
{
  uint8_t reg;
  CC1100_READ_STATUS_REG(CC1100_REG_WORTIME1, reg);
  return reg;
}

critical uint8_t cc1100_status_wortime0(void)
{
  uint8_t reg;
  CC1100_READ_STATUS_REG(CC1100_REG_WORTIME0, reg);
  return reg;
}

critical uint8_t cc1100_status_pktstatus(void)
{
  uint8_t reg;
  CC1100_READ_STATUS_REG(CC1100_REG_PKTSTATUS, reg);
  return reg;
}

critical uint8_t cc1100_status_txbytes(void)
{
  uint8_t reg;
  CC1100_READ_STATUS_REG(CC1100_REG_TXBYTES, reg);
  return reg;
}

critical uint8_t cc1100_status_rxbytes(void)
{
  uint8_t reg;
  CC1100_READ_STATUS_REG(CC1100_REG_RXBYTES, reg);
  return reg;
}

critical void cc1100_gdo0_int_enable(void)
{
  GDO0_INT_ENABLE();
}

critical void cc1100_gdo0_int_disable(void)
{
  GDO0_INT_DISABLE();
}

critical void cc1100_gdo0_int_clear(void)
{
  GDO0_INT_CLEAR();
}

critical void cc1100_gdo0_int_set_falling_edge(void)
{
  GDO0_INT_SET_FALLING();
}

critical void cc1100_gdo0_int_set_rising_edge(void)
{
  GDO0_INT_SET_RISING();
}

critical uint16_t cc1100_gdo0_read(void)
{
  return GDO0_READ();
}

critical void cc1100_gdo0_register_callback(void (*cb)(void))
{
  gdo0_cb = cb;
}


critical void cc1100_gdo2_int_enable(void)
{
  GDO2_INT_ENABLE();
}

critical void cc1100_gdo2_int_disable(void)
{
  GDO2_INT_DISABLE();
}

critical void cc1100_gdo2_int_clear(void)
{
  GDO2_INT_CLEAR();
}

critical void cc1100_gdo2_int_set_falling_edge(void)
{
  GDO2_INT_SET_FALLING();
}

critical void cc1100_gdo2_int_set_rising_edge(void)
{
  GDO2_INT_SET_RISING();
}

critical uint16_t cc1100_gdo2_read(void)
{
  return GDO2_READ();
}

critical void cc1100_gdo2_register_callback(void (*cb)(void))
{
  gdo2_cb = cb;
}

#undef interrupt
/**
 * Interrupt service routine for PORT1.
 * Used for handling CC1100 interrupts triggered on
 * the GDOx pins.
 */
void __attribute__((interrupt (PORT1_VECTOR))) port1irq(void)  
{
  if (P1IFG & GDO0_PIN)
  {
    GDO0_INT_CLEAR();
    if (gdo0_cb != 0x0)
    {
      (*gdo0_cb)();
    }
  }

  if (P1IFG & GDO2_PIN)
  {
    GDO2_INT_CLEAR();
    if (gdo2_cb != 0x0)
    {
      (*gdo2_cb)();
    }
  } 
}

critical uint8_t cc1100_read_reg(uint8_t addr)
{
  uint8_t reg;
  CC1100_READ_REG(addr, reg);
  return reg;
}

critical void cc1100_write_reg(uint8_t addr, uint8_t value)
{
  CC1100_WRITE_REG(addr, value);
}
