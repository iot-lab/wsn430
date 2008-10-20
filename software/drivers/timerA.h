/**
 *  \file   timerA.h
 *  \brief  msp430 TimerA driver
 *  \author Clément Burin des Roziers
 *  \date   2008
 **/

#ifndef _TIMERA_H
#define _TIMERA_H

/**
 * Number of CCR register, and thus the maximum
 * number of alarms that can be set.
 */
#define TIMERA_CCR_NUMBER 3

#define TIMERA_DIV_1 0
#define TIMERA_DIV_2 1
#define TIMERA_DIV_4 2
#define TIMERA_DIV_8 3

#define TIMERA_ALARM_CCR0 0
#define TIMERA_ALARM_CCR1 1
#define TIMERA_ALARM_CCR2 2
#define TIMERA_ALARM_OVER 3

/**
 * Timer A callback function prototype
 */
typedef void (*timerAcb)(void);

/**
 * Initialize the timerA component.
 * It needs to be called fist.
 */
void     timerA_init                (void);

/**
 * Start the timer with ACLK as its clock source,
 * and with given divider.
 * \param s_div divider used
 * \return 1 if ok, 0 if divider incorrect
 */
uint16_t timerA_start_ACLK_div      (uint16_t s_div);

/**
 * Register a callback function for a given alarm
 * that will be called when this alarm expires.
 * \param alarm CCR number corresponding to the callback
 * \param f callback function to be called
 * \return 1 if ok, O if alarm number incorrect
 */
uint16_t timerA_register_cb         (uint16_t alarm, timerAcb f);

/**
 * Read the timerA counter value.
 * \return the TAR value.
 */
uint16_t timerA_time                (void);

/**
 * Set an alarm having the call time as reference.
 * \param alarm the CCR number to use
 * \param ticks the number of timer tick before expiration
 * \param period set 0 if the alarm should ring once,
 * otherwise set the period between two consecutive triggers
 * \return 1 if alarm set, 0 if error in alarm number
 */
uint16_t timerA_set_alarm_from_now  (uint16_t alarm, uint16_t ticks, uint16_t period);

/**
 * Set an alarm having a giben time as reference.
 * \param alarm the CCR number to use
 * \param ticks the number of timer tick before expiration
 * \param period set 0 if the alarm should ring once,
 * otherwise set the period between two consecutive triggers
 * \return 1 if alarm set, 0 if error in alarm number
 */
uint16_t timerA_set_alarm_from_time (uint16_t alarm, uint16_t ticks, uint16_t period, uint16_t ref);

/**
 * Disable an alarm, given its CCR number.
 * \param alarm the CCR corresponding number
 * \return 1 if ok, 0 if wrong alarm number
 */
uint16_t timerA_unset_alarm         (uint16_t alarm);

/**
 * Stop the timer. This will save power.
 */
void     timerA_stop                (void);

/* ******************* */

#endif
