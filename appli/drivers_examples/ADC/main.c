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
 *  \brief  MSP430 ADC12 driver example
 *  \author Christophe Braillon
 *  \date   May 09
 **/

#include <stdio.h>
#include <signal.h>
#include <io.h>
#include "ADC.h"
#include "leds.h"
#include "uart0.h"
#include "clock.h"

// For printf
int putchar(int c)
{
	uart0_putchar(c);

	return 0;
}

// ADC convertion callback
static uint16_t adc_callback(uint8_t index, uint16_t value)
{
	// Prints the index and value:
	// Normally we shouldn't do printf in an interrupt service routine
	// doing it just for the example
	printf("%d %d\n\r", index, value);

	return 0;
}

int main(void)
{
	// Stop the watchdog timer.
	WDTCTL = WDTPW | WDTHOLD;

	set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();

	// Enables interrupts
	eint();

	// UART0 intialization (for printf in ADC callback)
	uart0_init(UART0_CONFIG_1MHZ_115200);

	LEDS_INIT();
	LEDS_OFF();

	// In this example we only use A0, A1 and A4
	ADC12_init();
	ADC12_enable(ADC(0) | ADC(1) | ADC(4));

	// We set 3 convertion up (0 to 2)
	ADC12_set_start_index(0);
	ADC12_set_stop_index(2);

	// First convertion is A0 with AVCC and AVSS as references
	ADC12_configure_index(0, ADC12_CHANNEL0, ADC12_AVCC_AVSS);
	// Second convertion is A1 with AVCC and AVSS as references
	ADC12_configure_index(1, ADC12_CHANNEL1, ADC12_AVCC_AVSS);
	// Third convertion is A4 with AVCC and AVSS as references
	ADC12_configure_index(2, ADC12_CHANNEL4, ADC12_AVCC_AVSS);

	// We convert from all 3 ADC and do it repeatedly
	ADC12_set_sequence_mode(REPEAT_SEQUENCE);

	// Turn the reference generator off (we neither use internal 2.5V nor 1.5V one)
	ADC12_set_reference_generator(GEN_OFF);

	// Register the same callback for all three conversions
	ADC12_register_cb(0, adc_callback);
	ADC12_register_cb(1, adc_callback);
	ADC12_register_cb(2, adc_callback);

	// Turn on ADC and start conversion
	ADC12_on();
	ADC12_start_conversion();

	// Put the processor in low-power mode
	while(1)
	{
		LPM3;
	}

	return 0;
}
