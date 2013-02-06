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
 *  \file   ADC.h
 *  \brief  MSP430 ADC12 driver
 *  \author Christophe Braillon
 *  \date   May 09
 **/

#ifndef _ADC_H
#define _ADC_H


#define ADC(i) (1<<i)

typedef enum
{
	ADC12_CHANNEL0    = INCH_0,
	ADC12_CHANNEL1    = INCH_1,
	ADC12_CHANNEL2    = INCH_2,
	ADC12_CHANNEL3    = INCH_3,
	ADC12_CHANNEL4    = INCH_4,
	ADC12_CHANNEL5    = INCH_5,
	ADC12_CHANNEL6    = INCH_6,
	ADC12_CHANNEL7    = INCH_7,
	ADC12_VEREFP      = INCH_8,
	ADC12_VEREFM      = INCH_9,
	ADC12_TEMP        = INCH_10,
	ADC12_AVCC_AVSS_2 = INCH_11
} adc_channel_t;

typedef enum
{
	ADC12_AVCC_AVSS    = SREF_0,
	ADC12_VREFP_AVSS   = SREF_1,
	ADC12_VEREFP_AVSS  = SREF_2,
	ADC12_AVCC_VREFM   = SREF_3,
	ADC12_VREFP_VREFM  = SREF_4,
	ADC12_VEREFP_VREFM = SREF_5
} adc_ref_t;

typedef enum
{
	SINGLE          = CONSEQ_0,
	SEQUENCE        = CONSEQ_1,
	REPEAT_SINGLE   = CONSEQ_2,
	REPEAT_SEQUENCE = CONSEQ_3
} adc_conseq_t;

typedef enum
{
	GEN_1_5V,
	GEN_2_5V,
	GEN_OFF
} adc_ref_gen_t;

typedef enum
{
	ADC12_OSC = ADC12SSEL_0,
	ADC12_ACLK = ADC12SSEL_1,
	ADC12_MCLK = ADC12SSEL_2,
	ADC12_SMCLK = ADC12SSEL_3
} adc_clock_t;

typedef enum
{
	DIV_1 = ADC12DIV_0,
	DIV_2 = ADC12DIV_1,
	DIV_3 = ADC12DIV_2,
	DIV_4 = ADC12DIV_3,
	DIV_5 = ADC12DIV_4,
	DIV_6 = ADC12DIV_5,
	DIV_7 = ADC12DIV_6,
	DIV_8 = ADC12DIV_7
} adc_divisor_t;

typedef enum
{
	SAMPLE_4 = SHT0_0 | SHT1_0,
	SAMPLE_8 = SHT0_1 | SHT1_1,
	SAMPLE_16 = SHT0_2 | SHT1_2,
	SAMPLE_32 = SHT0_3 | SHT1_3,
	SAMPLE_64 = SHT0_4 | SHT1_4,
	SAMPLE_96 = SHT0_5 | SHT1_5,
	SAMPLE_128 = SHT0_6 | SHT1_6,
	SAMPLE_192 = SHT0_7 | SHT1_7,
	SAMPLE_256 = SHT0_8 | SHT1_8,
	SAMPLE_384 = SHT0_9 | SHT1_9,
	SAMPLE_512 = SHT0_10 | SHT1_10,
	SAMPLE_768 = SHT0_11 | SHT1_11,
	SAMPLE_1024 = SHT0_12 | SHT1_12
} adc_sampling_t;

typedef uint16_t (*ADC12cb)(uint8_t index, uint16_t value);

void ADC12_init(void);

void ADC12_on(void);
void ADC12_off(void);

void ADC12_enable(uint8_t channels);

void ADC12_set_clock(adc_clock_t c);
void ADC12_set_clock_divisor(adc_divisor_t d);
void ADC12_set_sampling(adc_sampling_t s);

void ADC12_enable_index(uint8_t index);
void ADC12_disable_index(uint8_t index);

void ADC12_start_conversion(void);
void ADC12_stop_conversion(void);

void ADC12_set_start_index(uint8_t index);
void ADC12_set_stop_index(uint8_t index);

void ADC12_configure_index(uint8_t index, uint16_t channel, adc_ref_t ref);

void ADC12_set_sequence_mode(adc_conseq_t mode);

void ADC12_set_reference_generator(adc_ref_gen_t ref);

void ADC12_register_cb(uint8_t index, ADC12cb f);

 #endif
