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

/**
 *  \file
 *  \brief  Open Node supply control routines for the Control node
 *  \author Colin Chaballier
 *  \date   2008
 **/

#include <io.h>
#include <iomacros.h>

# include "supply-control.h"

// P2.1 is used to read SMus alert signals from INA209 chips
// P2.0 is used to control K2, ie the relay for the open node battery supply
// P2.3 is used to control K1, ie the relay for the open node main supply


void setup_senslabgw_ios(void){

// MANDATORY - Set P2.1 as input for SMBus-Alert signal from INA209 chips
	P2SEL &= ~0x02;		// P2.1 as GPIO
	P2DIR &= ~0x02;		// P2.1 as input


	// set K1 command as output gpio
	P2SEL &= ~0x08;		// P2.3 as GPIO
	P2DIR |= 0x08;		// P2.3 as output

	// set K2 command as output gpio
	P2SEL &= ~0x01;		// P2.0 as GPIO
	P2DIR |= 0x01;		// P2.0 as output

}

void set_opennode_main_supply_on(void){
	P2OUT |= 0x08;		// P2.3 at high level
}

void set_opennode_main_supply_off(void){
	P2OUT &= ~0x08;		// P2.3 at low level
}

void set_opennode_battery_supply_on(void){
	P2OUT |= 0x01;		// P2.0 at high level
}

void set_opennode_battery_supply_off(void){
	P2OUT &= ~0x01;		// P2.0 at low level
}

