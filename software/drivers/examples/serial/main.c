#include <io.h>

#include "leds.h"
#include "clock.h"
#include "uart0.h"

/**********************
 * Delay function.
 **********************/

#define DELAY 0x800

void delay(unsigned int d) 
{
  int i,j;
  for(j=0; j < 0xff; j++)
    {
      for (i = 0; i<d; i++) 
	{
	  nop();
	  nop();
	}
    }
}

/**********************
 * Leds 
 **********************/

int led_state;

void led_change( void )
{
  LEDS_OFF();
  switch (led_state)
    {
    case 0: LED_RED_ON();   break;
    case 1: LED_GREEN_ON(); break;
    case 2: LED_BLUE_ON();  break;
    case 3: LEDS_ON();    break;
    }
  led_state = (led_state + 1) & 0x3;
}

/**********************
 *
 **********************/
int i = 0;

int main(void) 
{
  int i;

  char h[] = "Hello World\r\n";
  
  set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
  
  uart0_init(UART0_CONFIG_1MHZ_38400);
  
  i = 0;
  do {
    uart0_putchar(h[i]);
    i+=1;
  } while (h[i] != 0);

  LEDS_INIT();
  LEDS_ON();
  delay(DELAY);
  LEDS_OFF();

  led_state = 0;

  while (1) 
    {                         
      for (i=0; i<99; i++)
	{
	  led_change();
	  delay(DELAY >> 2);
	}
    }
}
