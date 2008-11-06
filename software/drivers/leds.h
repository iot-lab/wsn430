#ifndef _LEDS_H
#define _LEDS_H

/**********************
* Leds 
**********************/

#define LED_OUT   P5OUT

#define BIT_BLUE     (1 << 6)
#define BIT_GREEN    (1 << 5)
#define BIT_RED      (1 << 4)

/**
 * Turn the blue LED on.
 */
#define LED_BLUE_ON()      LED_OUT &= ~BIT_BLUE
/**
 * Turn the blue LED off.
 */
#define LED_BLUE_OFF()     LED_OUT |=  BIT_BLUE
/**
 * Toggle the blue LED.
 */
#define LED_BLUE_TOGGLE()  LED_OUT ^=  BIT_BLUE
/**
 * Turn the green LED on.
 */
#define LED_GREEN_ON()     LED_OUT &= ~BIT_GREEN
/**
 * Turn the green LED off.
 */
#define LED_GREEN_OFF()    LED_OUT |=  BIT_GREEN
/**
 * Toggle the green LED.
 */
#define LED_GREEN_TOGGLE() LED_OUT ^=  BIT_GREEN
/**
 * Turn the red LED on.
 */
#define LED_RED_ON()       LED_OUT &= ~BIT_RED
/**
 * Turn the red LED off.
 */
#define LED_RED_OFF()      LED_OUT |=  BIT_RED
/**
 * Toggle the red LED.
 */
#define LED_RED_TOGGLE()   LED_OUT ^=  BIT_RED

/**
 * Turn the three LEDs on.
 */
#define LEDS_ON()      LED_OUT &= ~(BIT_BLUE | BIT_GREEN | BIT_RED)
/**
 * Turn the three LEDs off.
 */
#define LEDS_OFF()     LED_OUT |=  (BIT_BLUE | BIT_GREEN | BIT_RED)

/**
 * Set the three LEDs according to a given value.
 * The value should be between 0 and 7,
 * bit0 for red LED
 * bit1 for green LED
 * bit2 for blue LED
 * \param x the value
 */
#define LEDS_SET(x) \
do {                \
  LEDS_OFF();       \
  LED_OUT &= ~((x&0x7)<<4);  \
} while(0) 

/**
 * Configure the IO pins.
 */
#define LEDS_INIT()                             \
do {                                            \
   P5OUT  &= ~(BIT_BLUE | BIT_GREEN | BIT_RED); \
   P5DIR  |=  (BIT_BLUE | BIT_GREEN | BIT_RED); \
   P5SEL  &= ~(BIT_BLUE | BIT_GREEN | BIT_RED); \
} while(0)

#endif
