#include <io.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "clock.h"
#include "uart0.h"
#include "leds.h"
#include "mac.h"

int putchar(int c)
{
    return uart0_putchar(c);
}

uint16_t frame_received(uint8_t packet[], uint16_t length, uint16_t src_addr, int16_t rssi);

void vADCInit(void);
uint16_t* ADC12MEMx = 0x140;

volatile int16_t send;

struct {
    uint16_t lt, bt, heel;
} msg;

int main (void)
{
    WDTCTL = WDTPW+WDTHOLD;                   // Stop watchdog timer

    set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
    set_aclk_div(1);

    LEDS_INIT();
    LEDS_OFF();
    LED_BLUE_ON();

    // uart
    uart0_init(UART0_CONFIG_1MHZ_115200);
    printf("Leg FSR\n");
    eint();

    // mac initialization
    mac_init(10);

    vADCInit();

    send=0;
    while(1)
    {
        LPM0;
        if (send == 1) {

            mac_send((uint8_t*)&msg.lt, 6, MAC_BROADCAST);
            LED_RED_TOGGLE();
        }
    }

    return 0;
}


static void vADCInit(void)
{
    /* Configure ADC pins for analog input */
    P6SEL |=  ( (0x1) | (0x1<<1) | (0x1<<3) | (0x1<<4) | (0x1<<6) | (0x1<<7) );
    P6DIR &= ~( (0x1) | (0x1<<1) | (0x1<<3) | (0x1<<4) | (0x1<<6) | (0x1<<7) );

    /* 38us sampling, internal reference 1.5V */
    ADC12CTL0 = SHT0_7 | REFON | ADC12ON;

    /* Start@ADD0, TimerA OUT1 source, Repeat seq. of chan. */
    ADC12CTL1 = SHS_1 | SHP | CONSEQ_3;

    /* Channel A1, vREF */
    ADC12MCTL0 = INCH_1 | SREF_1;

    /* Channel A4, vREF */
    ADC12MCTL1 = INCH_4 | SREF_1;

    /* Channel A3, vREF */
    ADC12MCTL2 = INCH_3 | SREF_1 | EOS;

    /* Interrupt on end of sequence */
    ADC12IE = (0x1<<2);

    /* Timer A clear*/
    TACTL = TACLR;

    /* Source from ACLK (32kHz), div 8, UP mode */
    TACTL |= TASSEL_1 | MC_3;

    /* Up mode */
    TACCR0 = 401;
    TACCR1 = 400;

    /* CCR1 set/reset output mode */
    TACCTL1 = OUTMOD_3;

    /* Start Timer */
    TACTL &= ~(TACLR);

    /* Start ADC conversions */
    ADC12CTL0 |= ENC;
}


/**
 * ADC12 Interrupt service routine
 * Execute once the 6 ADCs have been sampled
 */
interrupt(ADC12_VECTOR) adc12irq(void)
{
    LED_GREEN_TOGGLE();

    msg.lt = ADC12MEMx[0];
    msg.bt = ADC12MEMx[1];
    msg.heel = ADC12MEMx[2];

    send = 1;
    LPM4_EXIT;
}
