/* Standard includes. */
#include <io.h>
#include <signal.h>
#include <iomacros.h>
#include <stdio.h>
#include <string.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Drivers includes. */
#include "leds.h"
#include "clock.h"
#include "timer.h"

/* Task priorities. */
#define mainLED_TASK_PRIORITY			( tskIDLE_PRIORITY + 1 )

/* Constants needed */
#define SYNC_FREQ_HZ				500

/* ********************************
 * Task and functions declarations.
 * ********************************/
void vTask_LED       ( void * pvParameters );
void prvSetupHardware( void );
void led_change      ( void );


/* *********************
 * Global variables.
 * *********************/
xSemaphoreHandle xSemaphore;		/* Will be used for synchronisation */
int              led_state;		/* Used to memorize the leds state */
int              task_suspended;	/* Flag used to detect over-run */
int              timer_count;		/* Count the timer interruptions */


/* *************************
 * timerB callback
 * Used for synchronisation
 * *************************/
void timer_cb(void)
{
	/* We want to toggle the LED each second */
	if (++timer_count == SYNC_FREQ_HZ)
	{
		/* Is the task suspended ? */
		if (task_suspended == 1) /* Yes */
		{
			/* Variables reinitilisation */
			timer_count    = 0;
			task_suspended = 0;

			/* Resume the suspended task. */
			xSemaphoreGiveFromISR( xSemaphore, NULL );
			
		}
		else /* No */
		{
			/* Error : Over run*/
			LEDS_OFF();
			LED_RED_ON;
			while(1);
		}
	}
}

/**********************
 * Leds 
 **********************/
void led_change( void )
{
	LEDS_OFF();
	switch (led_state)
	{
		case 0: LED_RED_ON;   break;
		case 1: LED_GREEN_ON; break;
		case 2: LED_BLUE_ON;  break;
		case 3: LEDS_ON();    break;
	}
	led_state = (led_state + 1) & 3;
}


/* *********************
 * main
 * *********************/
int main (void)
{
	/* Variables initialisation */
	led_state      = 0;
	task_suspended = 0;
	timer_count    = 0;

	/*Create the binary semaphore used for synchronisation */
	vSemaphoreCreateBinary( xSemaphore );

	/* Setup the microcontroller hardware for the demo. */
	prvSetupHardware();

	/* Task Creations */
	xTaskCreate(	vTask_LED,
			"TaskLED",
			configMINIMAL_STACK_SIZE,
			NULL,
			mainLED_TASK_PRIORITY,
			NULL);

	/* Start the scheduler. */
	vTaskStartScheduler();

	return 0;
}


/* *********************
 * Toggle led
 * *********************/
void vTask_LED( void * pvParameters )
{
	/* Timer B start */
	timerB7_SMCLK_start_Hz(SYNC_FREQ_HZ);

	for( ;; )
	{
		/* Wait for synchro */
		task_suspended = 1;
		xSemaphoreTake(xSemaphore, portMAX_DELAY);

		led_change();
	}
}

/* *********************
 * Setup Hardware
 * *********************/
void prvSetupHardware( void )
{
	/* Setup the IO as per the SoftBaugh demo for the same target hardware. */
	P1SEL  = 0x00;        // Selector         = 0:GPIO     1:peripheral
	P1DIR  = 0xff;        // Direction        = 0:input    1:output
	P1IE   = 0x00;        // Interrupt enable = 0:disable  1:enable
	P1IES  = 0x00;        // Edge select      = 0:L to H   1:H to L
	P1IFG  = 0x00;        // Clear flags

	P2SEL  = 0x00;
	P2DIR  = 0xff;
	P2IE   = 0x00;
	P2IES  = 0x00;
	P2IFG  = 0x00;

	/* Stop the watchdog. */
	WDTCTL = WDTPW + WDTHOLD;

	/* Set MCLK on XT2 at 8MHz */
	set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz ();

	/* LEDS initilization */
	LEDS_INIT();
	LEDS_OFF();

	/* Enable Interrupts */
	eint();

	/* Timer B Initialisation (define the handler) */
	timerB7_register_callback(timer_cb);

	/*********************************************************************/
	/* Note : We don't use the timer A because he is already used by     */
	/*        FreeRTOS for the scheduling.                               */
	/*********************************************************************/
}
/*-----------------------------------------------------------*/


