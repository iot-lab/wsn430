/**
 *  \file   timerA.c
 *  \brief  msp430 timerA driver
 *  \author Clément Burin des Roziers
 *  \date   2008
 **/

#include <io.h>
#include <signal.h>
#include "timerA.h"

static timerAcb timerA_callbacks[TIMERA_CCR_NUMBER+1];
static uint16_t timerA_periods[TIMERA_CCR_NUMBER];
static uint16_t *TACCTLx = (uint16_t*) 0x162;
static uint16_t *TACCRx  = (uint16_t*) 0x172;

void timerA_init()
{
    uint16_t i;
    
    // stop everything
    TACTL = 0;
    
    // clear the CCR and CCTL registers, and the associated callbacks
    for (i=0;i<TIMERA_CCR_NUMBER;i++)
    {
        TACCTLx[i] = 0;
        TACCRx[i] = 0;
        timerA_callbacks[i] = 0x0;
        timerA_periods[i] = 0;
    }
    // clear the overflow callback
    timerA_callbacks[TIMERA_CCR_NUMBER+1] = 0x0;
}

uint16_t timerA_start_ACLK_div (uint16_t s_div)
{
    // check if divider is correct
    if (s_div > 3)
    {
        return 0;
    }
    
    // update configuration register
    TACTL = (TASSEL_1) | MC_2 | (s_div<<6);
    
    return 1;
}

uint16_t timerA_register_cb (uint16_t alarm, timerAcb f)
{
    if (alarm >= TIMERA_CCR_NUMBER+1)
    {
        return 0;
    }
    
    timerA_callbacks[alarm] = f;
    
    if (alarm == TIMERA_ALARM_OVER)
    {
        // if callback is NULL, disable overflow interrupt
        if (f == 0x0)
        {
            TACTL &= ~(TAIE);
        }
        // if not NULL, enable OF interrupt
        else
        {
            TACTL |= TAIE;
        }
    }
    return 1;
}

uint16_t timerA_time()
{
    return TAR;
}

uint16_t timerA_set_alarm_from_now  (uint16_t alarm, uint16_t ticks, uint16_t period)
{
    uint16_t now;
    now = TAR;
    
    if (alarm > TIMERA_CCR_NUMBER)
    {
        return 0;
    }
    
    TACCRx[alarm] = now + ticks;
    TACCTLx[alarm] = CCIE;
    timerA_periods[alarm] = period;
    
    return 1;
}

uint16_t timerA_set_alarm_from_time (uint16_t alarm, uint16_t ticks, uint16_t period, uint16_t ref)
{    
    if (alarm > TIMERA_CCR_NUMBER)
    {
        return 0;
    }
    
    TACCRx[alarm] = ref + ticks;
    TACCTLx[alarm] = CCIE;
    timerA_periods[alarm] = period;
    
    return 1;
}

uint16_t timerA_unset_alarm(uint16_t alarm)
{
    if (alarm > TIMERA_CCR_NUMBER)
    {
        return 0;
    }
    
    TACCRx[alarm] = 0;
    TACCTLx[alarm] = 0;
    timerA_periods[alarm] = 0;
    timerA_callbacks[alarm] = 0;
    
    return 1;
}

void timerA_stop()
{
    // stop mode
    TACTL &= ~(MC0|MC1);
}

interrupt (TIMERA0_VECTOR) timerA0irq( void )
{
    if (timerA_periods[0])
    {
        TACCRx[0] += timerA_periods[0];
    }
    else
    {
        TACCRx[0] = 0;
        TACCTLx[0] = 0;
    }
    
    if (timerA_callbacks[0])
    {
        (*timerA_callbacks[0])();
    }
}

interrupt (TIMERA1_VECTOR) timerA1irq( void )
{
    uint16_t alarm;
    
    alarm = TAIV >> 1;
    
    // if overflow, just call the callback
    if (alarm == 0x05)
    {
        if (timerA_callbacks[TIMERA_ALARM_OVER])
        {
            (*timerA_callbacks[TIMERA_ALARM_OVER])();
        }
    }
    else
    {
        if (timerA_periods[alarm])
        {
            TACCRx[alarm] += timerA_periods[alarm];
        }
        else
        {
            TACCRx[alarm] = 0;
            TACCTLx[alarm] = 0;
        }
        
        if (timerA_callbacks[alarm])
        {
            (*timerA_callbacks[alarm])();
        }
    }
}
