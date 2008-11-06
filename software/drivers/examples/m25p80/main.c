#include <io.h>
#include <stdio.h>

#include "leds.h"
#include "clock.h"
#include "uart0.h"
#include "m25p80.h"

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

uint8_t page[256];

int main(void) 
{
    WDTCTL = WDTPW | WDTHOLD;
    set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
    uart0_init(UART0_CONFIG_1MHZ_38400);

    printf("M25P80 external flash memory test program\r\n");

    m25p80_init();
    
    uint8_t sig;
    
    sig = m25p80_get_signature();
    
    printf("signature is %x\r\n", sig);
    
    printf("Reading page #0\r\n");
    m25p80_load_page(0, page);
    int i;
    for (i=0;i<8;i++)
    {
        printf("%x\t",page[i]);
        if ((1+i)%8==0) printf("\r\n");
    }
    
    printf("Erasing sector #0\r\n");
    m25p80_erase_sector(0);
    
    printf("Reading page #0\r\n");
    m25p80_load_page(0, page);
    
    for (i=0;i<8;i++)
    {
        printf("%x\t",page[i]);
        if ((1+i)%8==0) printf("\r\n");
    }
    
    printf("Writing 0->255 in page #0\r\n");
    for (i=0;i<256;i++)
    {
        page[i] = i;
    }
    
    m25p80_save_page(0, page);
    for (i=0;i<256;i++)
    {
        page[i] = 0xAA;
    }
    
    
    printf("Reading page #0\r\n");
    m25p80_load_page(0, page);
    
    for (i=0;i<8;i++)
    {
        printf("%x\t",page[i]);
        if ((1+i)%8==0) printf("\r\n");
    }
    
    while (1)
    {}
}
