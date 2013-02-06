/*
 * Copyright  2008-2009 INRIA/SensLab
 * 
 * <dev-team@senslab.info>
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
 * \file
 * \brief Global data and methods header
 * \author Clément Burin des Roziers
 * \date 2009
 */
#ifndef _GLOBAL_H
#define _GLOBAL_H

extern xSemaphoreHandle xUSART0Mutex, xUSART1Mutex;
extern void* uart1_rx_cb;

#define UART_CONFIG UART1_CONFIG_1MHZ_115200

#define TIMERB_SECOND 4096
#define SYSTEM_SECOND configTICK_RATE_HZ

#define RADIO_ALARM TIMERB_ALARM_CCR0
#define SENSORPOLL_ALARM TIMERB_ALARM_CCR1
#define POWERPOLL_ALARM TIMERB_ALARM_CCR2
#define RADIOPOLL_ALARM TIMERB_ALARM_CCR3

void vGetUART(void);
void vReleaseUART(void);
void vGetTSL2550(void);
void vReleaseTSL2550(void);
void vGetINA209(void);
void vReleaseINA209(void);
void vGetDS1722(void);
void vReleaseDS1722(void);
void vGetCC1101(void);
void vReleaseCC1101(void);
void vGetCC2420(void);
void vReleaseCC2420(void);

enum led_policy {
	DEFAULT_LED_USE = 0, MANUAL_LED_USE = 1
};
extern uint16_t leds_policy;

extern uint16_t myTime;

#endif
