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
 * \author Clément Burin des Roziers
 * \date 2009
 */
#include <io.h>
#include <signal.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Project includes */
#include "clock.h"
#include "leds.h"
#include "timerB.h"
#include "global.h"

#include "mainTask.h"
#include "pollingTask.h"
#include "radioTask.h"

/* Function Protypes */
static void prvSetupHardware(void);
void vApplicationIdleHook(void);
static uint16_t uTimerOverflow(void);

/**
 * The main function.
 */
int main(void) {
	/* Setup the hardware. */
	prvSetupHardware();

	xUSART0Mutex = xSemaphoreCreateMutex();
	xUSART1Mutex = xSemaphoreCreateMutex();

	// Ensure Mutexes are released
	xSemaphoreGive( xUSART0Mutex );
	xSemaphoreGive( xUSART1Mutex );

	/* Create all the tasks */
	mainTask_create(3);
	pollingTask_create(2);
	radioTask_create(1);

	/* Start the scheduler. */
	vTaskStartScheduler();

	/* As the scheduler has been started we should never get here! */
	return 0;
}

/**
 * Initialize the main hardware parameters.
 */
static void prvSetupHardware(void) {
	/* Stop the watchdog timer. */
	WDTCTL = WDTPW + WDTHOLD;

	/* Setup MCLK 8MHz and SMCLK 1MHz */
	set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
	// Set ACLK divider 1
	set_aclk_div(1);

	// Setup TimerB 4096Hz
	timerB_init();
	TBR = 0;
	timerB_start_ACLK_div(TIMERB_DIV_8);
	timerB_register_cb(TIMERB_ALARM_OVER, uTimerOverflow);
	myTime = 0;

	/* Initialize the LEDs */
	LEDS_INIT();
	LEDS_ON();

	/* Enable Interrupts */
	eint();
}

void vApplicationIdleHook(void) {
	LPM0;
}

static uint16_t uTimerOverflow(void) {
	myTime++;
	return 0;
}
