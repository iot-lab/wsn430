#ifndef _RELAYS_H_
#define _RELAYS_H_

#define RELAYS_INIT() do \
{ \
    // set K1 command as output gpio \
    P2SEL &= ~0x08; // P2.3 as GPIO \
    P2DIR |= 0x08; // P2.3 as output \
 \
    // set K2 command as output gpio \
    P2SEL &= ~0x01; // P2.0 as GPIO \
    P2DIR |= 0x01; // P2.0 as output \
} while (0)

// P2.3 at high level
#define RELAY_DC_ON()  P2OUT |= 0x08
// P2.3 at low level
#define RELAY_DC_OFF() P2OUT &= ~0x08

// P2.0 at high level
#define RELAY_BATT_ON()  P2OUT |= 0x01
// P2.0 at low level
#define RELAY_BATT_OFF() P2OUT &= ~0x01


#endif
