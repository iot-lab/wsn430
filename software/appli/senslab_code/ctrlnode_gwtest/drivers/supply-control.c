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
	P2OUT &= ~0x08;		// P2.3 at low level
	P2SEL &= ~0x08;		// P2.3 as GPIO
	P2DIR |= 0x08;		// P2.3 as output

	// set K2 command as output gpio
	P2OUT &= ~0x01;		// P2.0 at low level
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

