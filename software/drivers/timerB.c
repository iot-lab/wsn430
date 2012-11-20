/*
 * Copyright  2008-2009 INRIA/SensTools
 * 
 * <dev-team@sentools.info>
 * 
 * This software is a set of libraries designed to develop applications
 * for the WSN430 embedded hardware platform.
 * 
 * This software is governed by the CeCILL license under French law and
 * abiding by the rules of distribution of free software.  You can  use, 
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info". 
 * 
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability. 
 * 
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or 
 * data to be ensured and,  more generally, to use and operate it in the 
 * same conditions as regards security. 
 * 
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 */

/**
 * \addtogroup timerB
 * @{
 */

/**
 * \file
 * \brief  MSP430 timerB driver
 * \author Antoine Fraboulet <antoine.fraboulet@insa-lyon.fr>
 * \author Colin Chaballier
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
 * \date   November 08
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
    timerB_callbacks[TIMERB_CCR_NUMBER] = 0x0;
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

uint16_t timerB_start_ACLK_div(uint16_t s_div)
{ 
	// check if divider is correct
    if (s_div > 3)
    {
        return 0;
    }
	
	  // update configuration register
    TBCTL = (TBSSEL_1) | MC_2 | (s_div<<6);
	return 1;
	
}

uint16_t timerB_register_cb (uint16_t alarm, timerBcb f)
{
    if (alarm > TIMERB_CCR_NUMBER)
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

uint16_t timerB_capture_start (uint16_t s_div)
{
	//timerB_start_ACLK_div(s_div);
	timerB_start_SMCLK_div(s_div);
	TBCCTL0 = CAP |SCS | CCIS1 | CM_3;
	return TBCCTL0;
}

uint16_t timerB_capture_stop (void)
{
	TBCCTL0 ^=  CCIS0;
	timerB_stop();
	return TBCCTL0;
}

uint16_t timerB_time_capture()
{
    return TBCCR0;
}

uint16_t timerB_ctl_status(void)
{
	return TBCTL;
}
uint16_t timerB_time()
{
    return TBR;
}

uint16_t timerB_set_alarm_from_now  (uint16_t alarm, uint16_t ticks, uint16_t period)
{
    uint16_t now;
    now = TBR;
    
    if (alarm >= TIMERB_CCR_NUMBER)
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
    if (alarm >= TIMERB_CCR_NUMBER)
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
    if (alarm >= TIMERB_CCR_NUMBER)
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
void timerB0irq( void );
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
        if ( timerB_callbacks[0]() )
        {
            LPM4_EXIT;
        }
    }
}

void timerB1irq( void );
interrupt (TIMERB1_VECTOR) timerB1irq( void )
{
    uint16_t alarm;
    
    alarm = TBIV >> 1;
    
    // if overflow, just call the callback
    if (alarm == 0x7)
    {
        if (timerB_callbacks[0x7])
        {
            if ( timerB_callbacks[0x7]() )
            {
                LPM4_EXIT;
            }
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
            if ( timerB_callbacks[alarm]() )
            {
                LPM4_EXIT;
            }
        }
    }
}

/**
 * @}
 */
