/*
 * Copyright  2008-2009 SensTools, INRIA
 *
 * <dev-team@sentools.info>
 *
 * This software is a set of libraries designed to develop applications
 * for the WSN430 embedded hardware platform.
 *
 * This software is governed by the CeCILL license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 */

/**
 * \brief Timer A example
 *        This example illustrates the use of timers in low power mode (LPM).
 *        Timer A is used with two alarms that drives LEDs. The red led is
 *        driven by a 0.5Hz alarm and the green one is triggered by a 1Hz alarm
 *        that periodically wakes the processor up. The main thread just toggle the
 *        blue LED and put the processor in LPM3 (so that timer are still active).
 *        As a consequence the blue LED blinks at half the frequency of the green LED
 *        i.e. 0.5Hz
 * \author Christophe Braillon <christophe.braillon@inria.fr>
 * \date April 08
 */
#include <io.h>
#include <signal.h>
#include "leds.h"
#include "timerA.h"
#include "clock.h"

uint16_t led_state = 0;
uint16_t sw = 1;

/** Toggles LED
 * @param n iIndex of the LED to toggle. Red LED as index 0, green one 1 and blue one 2.
 */
static void toggle_led( uint16_t n )
{
	led_state ^= (1 << n);

	if(led_state & 0x1) LED_RED_ON(); else LED_RED_OFF();
	if(led_state & 0x2) LED_GREEN_ON(); else LED_GREEN_OFF();
	if(led_state & 0x4) LED_BLUE_ON(); else LED_BLUE_OFF();
}

/** Alarm 0 callback
 *  Toggles the red LED and does not wake the processor up
 */
static uint16_t callback0()
{
	toggle_led(0);

	return 0;
}

/** Alarm 1 callback
 *  Toggles the green LED and wakes the processor up once out of two
 */
static uint16_t callback1()
{
	toggle_led(1);

	sw = !sw;

	return sw;
}

int main(void)
{
	// Stop the watchdog timer.
	WDTCTL = WDTPW | WDTHOLD;

	// Set the MCU clock to 8MHz
	set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();

	// Enable interrupts
	eint();

	// Swtich of LEDs
	LEDS_INIT();
	LEDS_OFF();

	/** Initialization of timer A
	 *  This has to be done before using any of the timer A manipulation routine
	 */
	timerA_init();

	/** Callbacks registration
	 *  Two callbacks are registered, one for alarm 0 and another one for alarm 1.
	 *  Alarm 2 and overflow are not used. Therefore a NULL callback is registers.
	 *  This disables the two interrupts
	 */
	timerA_register_cb(TIMERA_ALARM_CCR0, callback0);
	timerA_register_cb(TIMERA_ALARM_CCR1, callback1);
	timerA_register_cb(TIMERA_ALARM_CCR2, 0);
	timerA_register_cb(TIMERA_ALARM_OVER, 0);

	/** Alarm periods definition
	 *  Alarm 0 is triggered every 32768 ticks (i.e. every 1s as ACLK is 32768Hz
	 *  on WSN430). Its first call will happen 1 tick after the routine call.
	 *  Alarm 1 is triggered every 16384 ticks (i.e. every 500ms). Its first call
	 *  will happen 10 ticks after the routine call.
	 */
	timerA_set_alarm_from_now(TIMERA_ALARM_CCR0, 1, 32768);
	timerA_set_alarm_from_now(TIMERA_ALARM_CCR1, 10, 16384);

	/** Timer start
	 *  Starts the timer with a frequency of ACK/TIMERA_DIV_1, i.e. 32768Hz
	 */
	timerA_start_ACLK_div(TIMERA_DIV_1);

	// Main loop
	while (1)
	{
		// Put the processor in low power mode (only ACLK is active)
		LPM3;
		// Toggles the blue LED and go back to LPM
		toggle_led(2);
	}

	return 0;
}
