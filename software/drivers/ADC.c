/*
 * Copyright  2008-2009 INRIA/SensTools
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
 *  \file   ADC.c
 *  \brief  MSP430 ADC12 driver
 *  \author Christophe Braillon
 *  \date   May 09
 **/

#include <io.h>
#include <signal.h>
#include "ADC.h"
#include "leds.h"

static volatile uint8_t *ADC12MCTLx = (uint8_t*) ADC12MCTL;
static volatile uint16_t *ADC12MEMx = (uint16_t*) ADC12MEM;
static ADC12cb ADC12_callbacks[16];

void adc12irq(void);

interrupt (ADC_VECTOR) adc12irq()
{
	uint16_t memory;
	uint16_t value;

	memory = ADC12IV >> 1;

	switch(memory)
	{
		case 0: break;
		case 1: /* overflow */ break;
		case 2: /* conversion time overflow */ break;

		default:
			memory -= 3;
				value = ADC12MEMx[memory];

			if(ADC12_callbacks[memory])
			{
				if(ADC12_callbacks[memory](memory, value))
				{
					LPM4_EXIT;
				}
			}
	}
}

void ADC12_init();

void ADC12_init()
{
	uint8_t i;

	for(i = 0; i < 16; i++)
	{
		ADC12_callbacks[i] = 0;
		ADC12MCTLx[i] = INCH_11 | SREF_0;
	}

	// Default values:
	// - Overflow and time overflow interrupts disabled
	// - Sample-and-hold time are 512 clock cycles
	ADC12CTL0 = SHT0_10 | SHT1_10;
	// Default values:
	// - ACLK is used as a source clock
	// - Clock divisor is set to 1
	// - Sample input signal not inverted
	// - SAMPCON is sourced from the sampling timer
	// - Sample-and-hold source is ADC12SC
	ADC12CTL1 = SHP | ADC12SSEL_1 | ADC12DIV_0;
}

void ADC12_on()
{
	ADC12CTL0 |= ADC12ON;
}

void ADC12_off()
{
	ADC12CTL0 &= ~(ADC12ON);
}

void ADC12_enable(uint8_t channels)
{
	P6SEL |= channels;
	P6SEL &= ~channels;

	P6DIR &= ~channels;
}

void ADC12_set_clock(adc_clock_t c)
{
	ADC12CTL1 &= ~ADC12SSEL_3;
	ADC12CTL1 |= c;
}
void ADC12_set_clock_divisor(adc_divisor_t d)
{
	ADC12CTL1 &= 0xFF1F;
	ADC12CTL1 |= d;
}

void ADC12_set_sampling(adc_sampling_t s)
{
	ADC12CTL0 &= 0x00FF;
	ADC12CTL0 |= s;
}

void ADC12_set_start_index(uint8_t index)
{
	ADC12CTL1 &= ~(CSTARTADD3 | CSTARTADD2 | CSTARTADD1 | CSTARTADD0);
	ADC12CTL1 |= index << 12;
}

void ADC12_set_stop_index(uint8_t index)
{
	uint8_t i;

	for(i = 0; i < 16; i++)
	{
		ADC12MCTLx[i] &= ~(EOS);
	}

	ADC12MCTLx[index] |= EOS;
}

void ADC12_start_conversion()
{
	ADC12CTL0 |= ENC | ADC12SC;
}

void ADC12_stop_conversion()
{
	ADC12CTL0 &= ~(ENC);
}

void ADC12_set_sequence_mode(adc_conseq_t mode)
{
	ADC12CTL1 &= ~(CONSEQ1 | CONSEQ0);
	ADC12CTL1 |= mode;

	if(mode == SINGLE)
	{
		ADC12CTL0 &= ~(MSC);
	}
	else
	{
		ADC12CTL0 |= MSC;
	}
}

void ADC12_set_reference_generator(adc_ref_gen_t ref)
{
	if(ref == GEN_OFF)
	{
		ADC12CTL0 &= ~(REFON);
	}
	else
	{
		ADC12CTL0 |= REFON;

		if(ref == GEN_1_5V)
		{
			ADC12CTL0 &= ~(REF2_5V);
		}
		else
		{
			ADC12CTL0 |= REF2_5V;
		}
	}
}

void ADC12_configure_index(uint8_t index, uint16_t channel, adc_ref_t ref)
{
	ADC12MCTLx[index] &= EOS;
	ADC12MCTLx[index] |= channel | ref;
}

void ADC12_register_cb(uint8_t index, ADC12cb f)
{
	if(f)
	{
		ADC12IE |= (1 << index);
	}
	else
	{
		ADC12IE &= ~(1 << index);
	}

	ADC12_callbacks[index]= f;
}
