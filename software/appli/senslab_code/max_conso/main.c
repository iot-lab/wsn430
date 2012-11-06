/* General includes */
#include <io.h>
#include <signal.h>

/* Drivers includes */
#include "clock.h"
#include "timerA.h"
#include "leds.h"
#include "cc1100.h"
#include "ds1722.h"
#include "ds2411.h"
#include "m25p80.h"
#include "tsl2550.h"

void io_init(void);
void radio_start_max_tx(void);
uint16_t radio_check(void);

/**
 * The main function.
 */
int main( void )
{
	/* Stop the watchdog timer. */
	WDTCTL = WDTPW + WDTHOLD;
	
	/* Configure the IO ports */
	io_init();
	
	/* Setup MCLK 8MHz and SMCLK 1MHz */
	set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
	set_aclk_div(8); // ACKL is at 4096Hz
	
	/* Initialize the LEDs */
	LEDS_INIT();
	LEDS_ON();
	
	// radio
	radio_start_max_tx();
	
	// timerA to check
	timerA_init();
	timerA_start_ACLK_div(4);
	timerA_set_alarm_from_now(TIMERA_ALARM_CCR0, 4096, 4096);
	timerA_register_cb(TIMERA_ALARM_CCR0, radio_check);
	
	tsl2550_init();
	tsl2550_powerup();
	ds1722_init();
	ds1722_sample_cont();
	m25p80_init();
	m25p80_wakeup();
	
	eint();
	
	// Enter an infinite loop
	while (1) {
		LPM0; // do nothing
	}
	
	return 0;
}

void io_init(void) {
	P1SEL = 0;
	P2SEL = 0;
	P3SEL = 0;
	P4SEL = 0;
	P5SEL = 0;
	P6SEL = 0;
	
	P1DIR = BV(0)+BV(1)+BV(2)+BV(5)+BV(6)+BV(7);
	P1OUT = 0;
	
	P2DIR = BV(0)+BV(1)+BV(2)+BV(3)+BV(4)+BV(5)+BV(6)+BV(7);
	P2OUT = 0;
	
	P3DIR = BV(0)+BV(2)+BV(4)+BV(6)+BV(7);
	P3OUT = BV(2)+BV(4);
	
	P4DIR = BV(2)+BV(3)+BV(4)+BV(5)+BV(6)+BV(7);
	P4OUT = BV(2)+BV(4);
	
	P5DIR = BV(0)+BV(1)+BV(3)+BV(4)+BV(5)+BV(6)+BV(7);
	P5OUT = 0;
	
	P6DIR = BV(0)+BV(1)+BV(3)+BV(4)+BV(5)+BV(6)+BV(7);
	P6OUT = 0;
}

void radio_start_max_tx(void) {
	
	// Initialize the radio driver
	cc1100_init();
	cc1100_cmd_idle();


	cc1100_cfg_append_status(CC1100_APPEND_STATUS_DISABLE);
	cc1100_cfg_crc_autoflush(CC1100_CRC_AUTOFLUSH_DISABLE);
	cc1100_cfg_white_data(CC1100_DATA_WHITENING_ENABLE);
	cc1100_cfg_crc_en(CC1100_CRC_CALCULATION_ENABLE);
	cc1100_cfg_freq_if(0x0C);
	cc1100_cfg_fs_autocal(CC1100_AUTOCAL_NEVER);

	cc1100_cfg_mod_format(CC1100_MODULATION_MSK);

	cc1100_cfg_sync_mode(CC1100_SYNCMODE_30_32);

	cc1100_cfg_manchester_en(CC1100_MANCHESTER_DISABLE);
	
	cc1100_cfg_txoff_mode(CC1100_TXOFF_MODE_STAY_TX);
	
	// set channel bandwidth (560 kHz)
	cc1100_cfg_chanbw_e(0);
	cc1100_cfg_chanbw_m(2);

	// set data rate (0xD/0x2F is 250kbps)
	cc1100_cfg_drate_e(0x0D);
	cc1100_cfg_drate_m(0x2F);

	uint8_t table[1];
	table[0] = 0xC2; // 10dBm
	cc1100_cfg_patable(table, 1);
	cc1100_cfg_pa_power(0);
	
	cc1100_cmd_flush_tx();
	cc1100_cmd_tx();
}

uint16_t radio_check(void) {
	LED_RED_TOGGLE();
	if ( (cc1100_status() & 0x70) != 0x20) {
		LED_GREEN_TOGGLE();
		radio_start_max_tx();
	}
	
	return 0;
}
