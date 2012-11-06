/*
 * Copyright  2008-2009 INRIA/SensLab
 *
 * <dev-team@senslab.info>
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

/*
 * dac.h
 *
 *  Created on: Apr 29, 2010
 *      Author: burindes
 */
#include <io.h>
#include "dac.h"

void dac_init(void) {
	// Configure the ADC12 to provide the voltage reference
	ADC12CTL0 = REF2_5V | REFON;

	// Configure the DAC module, 12bit, full scale, fastest mode
	DAC12_0CTL = 0;
	DAC12_0CTL = DAC12SREF_0 | DAC12RES | DAC12LSEL_0 | DAC12AMP_7;
	DAC12_1CTL = 0;
	DAC12_1CTL = DAC12SREF_0 | DAC12RES | DAC12LSEL_0 | DAC12AMP_7;

	// Start calibration
	DAC12_0CTL |= DAC12CALON;
	DAC12_1CTL |= DAC12CALON;

	// wait until calibration done
	while (DAC12_0CTL & DAC12CALON || DAC12_1CTL & DAC12CALON)
		;

	// Set both channels to 0
	dac0_set(0);
	dac1_set(0);
}

void dac0_set(uint16_t value) {
	DAC12_0DAT = value;
}
void dac1_set(uint16_t value) {
	DAC12_1DAT = value;

}
