#include <io.h>
#include <stdio.h>

#include "leds.h"
#include "clock.h"
#include "uart0.h"
#include "tsl2550.h"

/* Define putchar for printf */
int putchar (int c)
{
    return uart0_putchar(c);
}

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

uint8_t adc0, adc1;

int main(void) 
{
    WDTCTL = WDTPW | WDTHOLD;
    set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
    uart0_init(UART0_CONFIG_1MHZ_38400);

    printf("TSL2550 light sensor test program\r\n");
    
    tsl2550_init();
    adc0 = tsl2550_powerup();
    
    uart0_init(UART0_CONFIG_1MHZ_38400);
    printf("powerup result = %x\r\n", adc0);
    
    while (1)
    {
        tsl2550_init();
        adc0 = tsl2550_read_adc0();
        adc1 = tsl2550_read_adc1();
        
        uart0_init(UART0_CONFIG_1MHZ_38400);
        printf("adc0=%u, adc1=%u\r\n", adc0&0x7F,adc1&0x7F);
        
        delay(DELAY<<2);
    }
}
