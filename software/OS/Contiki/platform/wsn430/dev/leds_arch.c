
#include "contiki.h"
#include "leds.h"


#define LED_OUT   P5OUT

#define BIT_BLUE     (1 << 6)
#define BIT_GREEN    (1 << 5)
#define BIT_RED      (1 << 4)

void leds_arch_init(void)
{
    P5OUT  &= ~(BIT_BLUE | BIT_GREEN | BIT_RED);
    P5DIR  |=  (BIT_BLUE | BIT_GREEN | BIT_RED);
    P5SEL  &= ~(BIT_BLUE | BIT_GREEN | BIT_RED);
    LED_OUT |=  (BIT_BLUE | BIT_GREEN | BIT_RED);
}

uint8_t leds_arch_get(void)
{
    return (~(LED_OUT>>4)) & 0x07;
}

void leds_arch_set(uint8_t leds)
{
    LED_OUT |=  (BIT_BLUE | BIT_GREEN | BIT_RED);
    LED_OUT &= ( (~(leds<<4)) & 0x70 );
}
