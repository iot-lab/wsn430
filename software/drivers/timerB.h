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
 * \defgroup timerB Timer B driver
 * \ingroup wsn430
 * @{
 * The timerB driver offers some abstraction for the corresponding MSP430 block.
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
 * The timerB can be sourced either by ACLK or SMCLK
 * and can register 7 simultaneous timer alarms plus an overflow alarm.
 * 
 */

/**
 * \file
 * \brief  MSP430 timerB driver header
 * \author Antoine Fraboulet <antoine.fraboulet@insa-lyon.fr>
 * \author Colin Chaballier
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
 * \author Christophe Braillon <christophe.braillon@inria.fr>
 * \date   November 08
 **/

#ifndef _TIMERB_H
#define _TIMERB_H

/**
 * Number of CCR register, and thus the maximum
 * number of alarms that can be set.
 */
#define TIMERB_CCR_NUMBER 7

/**
 * \name alarm available clock divisions
 * @{
 */
#define TIMERB_DIV_1 0
#define TIMERB_DIV_2 1
#define TIMERB_DIV_4 2
#define TIMERB_DIV_8 3
/**
 * @}
 */

/**
 * \name alarm constants
 * @{
 */

#define TIMERB_ALARM_CCR0 0
#define TIMERB_ALARM_CCR1 1
#define TIMERB_ALARM_CCR2 2
#define TIMERB_ALARM_CCR3 3
#define TIMERB_ALARM_CCR4 4
#define TIMERB_ALARM_CCR5 5
#define TIMERB_ALARM_CCR6 6
#define TIMERB_ALARM_OVER 7
/**
 * @}
 */

/**
 * \brief Timer B callback function prototype.
 * \return a non-zero in order to wake the CPU after the IRQ.
 */
typedef uint16_t (*timerBcb)(void);

/**
 * \brief Initialize the timerB component.
 * 
 * It must be called before any other module function.
 */
void timerB_init(void);

/**
 * \brief Start the timer with SMCLK as its clock source,
 * and with given divider.
 * \param s_div divider used
 * \return 1 if ok, 0 if divider incorrect
 */
uint16_t timerB_start_SMCLK_div(uint16_t s_div);

uint16_t timerB_start_ACLK_div(uint16_t s_div);

/**
 * \brief Register a callback function for a given alarm
 * that will be called when this alarm expires.
 * \param alarm CCR number corresponding to the callback
 * \param f callback function to be called
 * \return 1 if ok, O if alarm number incorrect
 */
uint16_t timerB_register_cb(uint16_t alarm, timerBcb f);
/**
 * Configures the timerB to perform capture
 * and measure elapsed time. The capture is 
 * triggered by software.
 */
uint16_t timerB_capture_start(uint16_t s_div);
/**
 * Triggers a capture by toggling bit CCIS0
 * to switch the capture signal between Vcc and GND.
 * \return TBR timer value
 */
uint16_t timerB_capture_stop (void);

uint16_t timerB_time_capture(void);

uint16_t timerB_ctl_status(void);

/**
 * \brief Read the timerB counter value.
 * \return the TBR value.
 */
uint16_t timerB_time(void);

/**
 * \brief Set an alarm having the call time as reference.
 * \param alarm the CCR number to use
 * \param ticks the number of timer tick before expiration
 * \param period set 0 if the alarm should ring once,
 * otherwise set the period between two consecutive triggers
 * \return 1 if alarm set, 0 if error in alarm number
 */

uint16_t timerB_set_alarm_from_now(uint16_t alarm, uint16_t ticks, uint16_t period);

/**
 * \brief Set an alarm having a given time as reference.
 * \param alarm the CCR number to use
 * \param ticks the number of timer tick before expiration
 * \param period set 0 if the alarm should fire once,
 * otherwise set the period between two consecutive triggers
 * \param ref the time reference from which to set the alarm (for example use timerA_time()).
 * \return 1 if alarm set, 0 if error in alarm number
 */
uint16_t timerB_set_alarm_from_time(uint16_t alarm, uint16_t ticks, uint16_t period, uint16_t ref);

/**
 * \brief Disable an alarm, given its CCR number.
 * \param alarm the CCR corresponding number
 * \return 1 if ok, 0 if wrong alarm number
 */
uint16_t timerB_unset_alarm(uint16_t alarm);

/**
 * \brief Stop the timer.
 * 
 * This will save power.
 */
void timerB_stop(void);

#endif

/**
 * @}
 */
