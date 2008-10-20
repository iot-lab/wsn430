/**
 *  \file   timer.c
 *  \brief  msp430 timerA and TimerB driver
 *  \author Antoine Fraboulet
 *  \date   2005
 **/

#include <io.h>
#include <signal.h>
#include "timerB.h"

static timerBcb timerB_callbacks[TIMERB_CCR_NUMBER+1];
static uint16_t timerB_periods[TIMERB_CCR_NUMBER];
static uint16_t *TBCCTLx = (uint16_t*) 0x182;
static uint16_t *TBCCRx  = (uint16_t*) 0x192;

void timerB_init()
{
    uint16_t i;
    
    // stop everything
    TBCTL = 0;
    
    // clear the CCR and CCTL registers, and the associated callbacks
    for (i=0;i<TIMERB_CCR_NUMBER;i++)
    {
        TBCCTLx[i] = 0;
        TBCCRx[i] = 0;
        timerB_callbacks[i] = 0x0;
        timerB_periods[i] = 0;
    }
    // clear the overflow callback
    timerB_callbacks[TIMERB_CCR_NUMBER+1] = 0x0;
}

uint16_t timerB_start_SMCLK_div (uint16_t s_div)
{
    // check if divider is correct
    if (s_div > 3)
    {
        return 0;
    }
    
    // update configuration register
    TBCTL = (TBSSEL_2) | MC_2 | (s_div<<6);
    
    return 1;
}

uint16_t timerB_register_cb (uint16_t alarm, timerBcb f)
{
    if (alarm >= TIMERB_CCR_NUMBER+1)
    {
        return 0;
    }
    
    timerB_callbacks[alarm] = f;
    
    if (alarm == TIMERB_ALARM_OVER)
    {
        // if callback is NULL, disable overflow interrupt
        if (f == 0x0)
        {
            TBCTL &= ~(TBIE);
        }
        // if not NULL, enable OF interrupt
        else
        {
            TBCTL |= TBIE;
        }
    }
    return 1;
}

uint16_t timerB_time()
{
    return TBR;
}

uint16_t timerB_set_alarm_from_now  (uint16_t alarm, uint16_t ticks, uint16_t period)
{
    uint16_t now;
    now = TBR;
    
    if (alarm > TIMERB_CCR_NUMBER)
    {
        return 0;
    }
    
    TBCCRx[alarm] = now + ticks;
    TBCCTLx[alarm] = CCIE;
    timerB_periods[alarm] = period;
    
    return 1;
}

uint16_t timerB_set_alarm_from_time (uint16_t alarm, uint16_t ticks, uint16_t period, uint16_t ref)
{    
    if (alarm > TIMERB_CCR_NUMBER)
    {
        return 0;
    }
    
    TBCCRx[alarm] = ref + ticks;
    TBCCTLx[alarm] = CCIE;
    timerB_periods[alarm] = period;
    
    return 1;
}

uint16_t timerB_unset_alarm(uint16_t alarm)
{
    if (alarm > TIMERB_CCR_NUMBER)
    {
        return 0;
    }
    
    TBCCRx[alarm] = 0;
    TBCCTLx[alarm] = 0;
    timerB_periods[alarm] = 0;
    timerB_callbacks[alarm] = 0;
    
    return 1;
}

void timerB_stop()
{
    // stop mode
    TBCTL &= ~(MC0|MC1);
}

interrupt (TIMERB0_VECTOR) timerB0irq( void )
{
    if (timerB_periods[0])
    {
        TBCCRx[0] += timerB_periods[0];
    }
    else
    {
        TBCCRx[0] = 0;
        TBCCTLx[0] = 0;
    }
    
    if (timerB_callbacks[0])
    {
        (*timerB_callbacks[0])();
    }
}

interrupt (TIMERB1_VECTOR) timerB1irq( void )
{
    uint16_t alarm;
    
    alarm = TBIV >> 1;
    
    // if overflow, just call the callback
    if (alarm == 0x7)
    {
        if (timerB_callbacks[0x7])
        {
            (*timerB_callbacks[0x7])();
        }
    }
    else
    {
        if (timerB_periods[alarm])
        {
            TBCCRx[alarm] += timerB_periods[alarm];
        }
        else
        {
            TBCCRx[alarm] = 0;
            TBCCTLx[alarm] = 0;
        }
        
        if (timerB_callbacks[alarm])
        {
            (*timerB_callbacks[alarm])();
        }
    }
}
