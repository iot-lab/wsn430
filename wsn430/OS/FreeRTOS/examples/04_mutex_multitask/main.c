/**
 * \file main.c
 */
#include <stdio.h>
#include <io.h>
#include <signal.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Project includes */
#include "radioTask.h"

/* Driver includes */
#include "clock.h"
#include "leds.h"
#include "uart0.h"
#include "ds1722.h"
#include "m25p80.h"

/* Function Prototypes */
static void prvSetupHardware( void );
static void vFlashTask(void* pvParameters);
static void vTempTask(void* pvParameters);


int putchar(int c)
{
	return uart0_putchar(c);
}

/* Global Variables */
xQueueHandle xFlashQueue, xRadioQueue;
xSemaphoreHandle xSPIMutex;

/**
 * The main function.
 */
int main( void )
{
	/* Setup the hardware. */
	prvSetupHardware();

	/* Create the Queues */
	xRadioQueue = xQueueCreate( 5, sizeof(uint8_t) );
	xFlashQueue = xQueueCreate( 5, sizeof(uint8_t) );

	/* Create the Mutex */
	xSPIMutex = xSemaphoreCreateMutex();

	/* Add the tasks to the scheduler */
	xTaskCreate(vTempTask, (const signed char*) "Temperature", configMINIMAL_STACK_SIZE, NULL, 1, NULL );
	xTaskCreate(vFlashTask, (const signed char*) "Flash",      configMINIMAL_STACK_SIZE, NULL, 1, NULL );

	/* Create the Radio Task */
	vCreateRadioTask(xRadioQueue, xSPIMutex);

	/* Start the scheduler. */
	vTaskStartScheduler();

	/* As the scheduler takes been started we should never get here! */
	return 0;
}

/**
 * Initialize the main hardware parameters.
 */
static void prvSetupHardware( void )
{
	/* Stop the watchdog timer. */
	WDTCTL = WDTPW + WDTHOLD;

	/* Setup MCLK 8MHz and SMCLK 1MHz */
	set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();

	/* Init the LEDs */
	LEDS_INIT();
	LEDS_OFF();

	/* Init the UART module */
	uart0_init(UART0_CONFIG_1MHZ_115200);
	printf("Mutex Example Program Start\r\n");

	/* Enable Interrupts */
	eint();
}

/**
 * The Flash task function.
 * It waits for incoming data. It stores data from the queue
 * in a 256byte local buffer, and when it's full, writes it to the flash.
 * \param pvParameter NULL is passed as parameter.
 */
static void vFlashTask(void* pvParameters)
{
	uint8_t  rx_item;
	uint8_t  buffer[256];
	uint16_t buffer_index = 0;
	uint16_t flash_page = 0;

	/* Wait for the mutex to be free and take it */
	xSemaphoreTake(xSPIMutex, portMAX_DELAY);

	/* Erase the flash */
	LED_RED_ON();
	printf("Flash takes mutex\r\n");
	m25p80_init();
	m25p80_erase_bulk();
	m25p80_load_page(0, buffer);

	printf("Flash gives mutex\r\n");
	LED_RED_OFF();

	/* Give the mutex back */
	xSemaphoreGive(xSPIMutex);

	while (1)
	{
		/* Wait for an item to be put in the queue */
		if ( xQueueReceive(xFlashQueue, &rx_item, portMAX_DELAY) )
		{
			/* Store this item in the buffer */
			buffer[buffer_index] = rx_item;
			buffer_index++;

			if (buffer_index == 256)
			{
				buffer_index = 0;

				xSemaphoreTake(xSPIMutex, portMAX_DELAY);
				LED_RED_ON();
				printf("Flash takes mutex\r\n");

				m25p80_save_page(flash_page, buffer);

				printf("Flash gives mutex\r\n");
				LED_RED_OFF();
				xSemaphoreGive(xSPIMutex);

				flash_page ++;
			}
		}
	}
}

/**
 * The temperature measurement task function.
 * It reads the temperature from the sensor and puts it on the radio
 * and flash queues for sending and storing.
 * \param pvParameters NULL is passed, unused here.
 */
static void vTempTask(void* pvParameters)
{
	uint8_t msb;
	uint16_t xLastWakeTime = xTaskGetTickCount();
	uint16_t sampleCount = 0, sampleAvg = 0;

	/* The sample period is 1000 ticks, about 1s */
	const uint16_t xWakePeriod    = 200;

	/* The maximum time waiting for the mutex is 100 ticks */
	const uint16_t xMaxMutexBlockTime = 100;

	/* Initialize the temperature sensor */
	xSemaphoreTake(xSPIMutex, portMAX_DELAY);
	LED_GREEN_ON();
	ds1722_init();
	ds1722_set_res(8);
	ds1722_sample_cont();
	LED_GREEN_OFF();
	xSemaphoreGive(xSPIMutex);

	/* Infinite loop */
	while(1)
	{
		/* Block until xWakePeriod ticks since previous call */
		vTaskDelayUntil(&xLastWakeTime, xWakePeriod);

		/* Try to take the mutex for xMaxMutexBlockTime ticks max */
		if ( xSemaphoreTake(xSPIMutex, xMaxMutexBlockTime) )
		{
			LED_GREEN_ON();
			/* If got the mutex : */
			/* Read the sensor */
			msb = ds1722_read_MSB();

			/* Give the mutex back */
			xSemaphoreGive(xSPIMutex);
			LED_GREEN_OFF();

			/* Do calculation */
			sampleAvg += msb;
			sampleCount += 1;

			if (sampleCount == 4)
			{
				sampleAvg >>= 2;
				/* Put the read value on the flash and radio queues
				 * Don't block */
				xQueueSendToBack(xFlashQueue, &sampleAvg, 0);
				xQueueSendToBack(xRadioQueue, &sampleAvg, 0);

				/* Reset the variables */
				sampleCount = 0;
				sampleAvg = 0;
			}
		} /* if the mutex was not obtained, skip the sampling for now */
	}
}
