/**
 *  \file   timerA.h
 *  \brief  msp430 TimerA driver
 *  \author Clément Burin des Roziers
 *  \date   2008
 **/

#ifndef _TIMERB_H
#define _TIMERB_H

/**
 * Number of CCR register, and thus the maximum
 * number of alarms that can be set.
 */
#define TIMERB_CCR_NUMBER 7

#define TIMERB_DIV_1 0
#define TIMERB_DIV_2 1
#define TIMERB_DIV_4 2
#define TIMERB_DIV_8 3

#define TIMERB_ALARM_CCR0 0
#define TIMERB_ALARM_CCR1 1
#define TIMERB_ALARM_CCR2 2
#define TIMERB_ALARM_CCR3 3
#define TIMERB_ALARM_CCR4 4
#define TIMERB_ALARM_CCR5 5
#define TIMERB_ALARM_CCR6 6
#define TIMERB_ALARM_OVER 7

/**
 * Timer B callback function prototype
 */
typedef void (*timerBcb)(void);

/**
 * Initialize the timerA component.
 * It needs to be called fist.
 */
void     timerB_init                (void);

/**
 * Start the timer with SMCLK as its clock source,
 * and with given divider.
 * \param s_div divider used
 * \return 1 if ok, 0 if divider incorrect
 */
uint16_t timerB_start_SMCLK_div     (uint16_t s_div);

/**
 * Register a callback function for a given alarm
 * that will be called when this alarm expires.
 * \param alarm CCR number corresponding to the callback
 * \param f callback function to be called
 * \return 1 if ok, O if alarm number incorrect
 */
uint16_t timerB_register_cb         (uint16_t alarm, timerBcb f);

/**
 * Read the timerB counter value.
 * \return the TBR value.
 */
uint16_t timerB_time                (void);

/**
 * Set an alarm having the call time as reference.
 * \param alarm the CCR number to use
 * \param ticks the number of timer tick before expiration
 * \param period set 0 if the alarm should ring once,
 * otherwise set the period between two consecutive triggers
 * \return 1 if alarm set, 0 if error in alarm number
 */

uint16_t timerB_set_alarm_from_now  (uint16_t alarm, uint16_t ticks, uint16_t period);

/**
 * Set an alarm having a giben time as reference.
 * \param alarm the CCR number to use
 * \param ticks the number of timer tick before expiration
 * \param period set 0 if the alarm should ring once,
 * otherwise set the period between two consecutive triggers
 * \return 1 if alarm set, 0 if error in alarm number
 */
uint16_t timerB_set_alarm_from_time (uint16_t alarm, uint16_t ticks, uint16_t period, uint16_t ref);

/**
 * Disable an alarm, given its CCR number.
 * \param alarm the CCR corresponding number
 * \return 1 if ok, 0 if wrong alarm number
 */
uint16_t timerB_unset_alarm         (uint16_t alarm);

/**
 * Stop the timer. This will save power.
 */
void     timerB_stop                (void);

/* ******************* */

#endif
