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
 * \defgroup timerA Timer A driver
 * \ingroup wsn430
 * @{
 * The timerA driver offers some abstraction for the corresponding MSP430 block.
 * This driver allows the generation of alarms. Once a timer is configured
 * for a given clock tick, it is possible to set several alarms
 * that will call some registered callback functions
 * once they have expired.
 * 
 * An alarm is mainly defined by a time reference and a duration.
 * The alarm expires (or fires) when the timer counter reaches
 * the time reference plus the duration. It is also possible
 * to declare periodic alarms that will call the callback
 * function once every duration.
 * 
 * The timerA is sourced from ACLK (limitation due to driver design),
 * and can register 3 simultaneous timer alarms plus an overflow alarm.
 * 
 */

/**
 * \file
 * \brief  MSP430 timerA driver header
 * \author Antoine Fraboulet <antoine.fraboulet@insa-lyon.fr>
 * \author Colin Chaballier
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
 * \date   November 08
 **/


#ifndef _TIMERA_H
#define _TIMERA_H

/**
 * Number of CCR register, and thus the maximum
 * number of alarms that can be set.
 */
#define TIMERA_CCR_NUMBER 3


/**
 * \name alarm available clock divisions
 * @{
 */
#define TIMERA_DIV_1 0
#define TIMERA_DIV_2 1
#define TIMERA_DIV_4 2
#define TIMERA_DIV_8 3
/**
 * @}
 */

/**
 * \name alarm constants
 * @{
 */
#define TIMERA_ALARM_CCR0 0
#define TIMERA_ALARM_CCR1 1
#define TIMERA_ALARM_CCR2 2
#define TIMERA_ALARM_OVER 3
/**
 * @}
 */

/**
 * \brief Timer A callback function prototype.
 * \return a non-zero in order to wake the CPU after the IRQ.
 */
typedef uint16_t(*timerAcb)(void);

/**
 * \brief Initialize the timerA component.
 * 
 * It must be called before any other module function.
 */
void timerA_init(void);

/**
 * \brief Start the timer with ACLK as its clock source,
 * and with given divider.
 * \param s_div divider used
 * \return 1 if ok, 0 if divider incorrect
 */
uint16_t timerA_start_ACLK_div(uint16_t s_div);

/**
 * \brief Register a callback function for a given alarm
 * that will be called when this alarm expires.
 * \param alarm CCR number corresponding to the callback
 * \param f callback function to be called
 * \return 1 if ok, O if alarm number incorrect
 */
uint16_t timerA_register_cb(uint16_t alarm, timerAcb f);

/**
 * \brief Read the timerA counter value.
 * \return the TAR value.
 */
uint16_t timerA_time(void);

/**
 * \brief Set an alarm having the call time as reference.
 * \param alarm the CCR number to use
 * \param ticks the number of timer tick before expiration
 * \param period set 0 if the alarm should ring once,
 * otherwise set the period between two consecutive triggers
 * \return 1 if alarm set, 0 if error in alarm number
 */
uint16_t timerA_set_alarm_from_now(uint16_t alarm, uint16_t ticks, uint16_t period);

/**
 * \brief Set an alarm having a given time as reference.
 * \param alarm the CCR number to use
 * \param ticks the number of timer ticks before expiration
 * \param period set 0 if the alarm should fire once,
 * otherwise set the period between two consecutive triggers
 * \param ref the time reference from which to set the alarm (for example use timerA_time()).
 * \return 1 if alarm set, 0 if error in alarm number
 */
uint16_t timerA_set_alarm_from_time(uint16_t alarm, uint16_t ticks, uint16_t period, uint16_t ref);

/**
 * \brief Update the period of an alarm.
 * \param alarm the CCR alarm number
 * \param period the new period to set
 * \return 1 if alarm updated, 0 if error
 */
uint16_t timerA_update_alarm_period(uint16_t alarm, uint16_t period);

/**
 * \brief Disable an alarm, given its CCR number.
 * \param alarm the CCR corresponding number
 * \return 1 if ok, 0 if wrong alarm number
 */
uint16_t timerA_unset_alarm(uint16_t alarm);

/**
 * \brief Stop the timer.
 * 
 * This will save power.
 */
void timerA_stop(void);

/* ******************* */

#endif

/**
 * @}
 */
