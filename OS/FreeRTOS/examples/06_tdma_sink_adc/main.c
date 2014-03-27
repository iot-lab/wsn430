#include <io.h>
#include <signal.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Project includes */
#include "clock.h"
#include "uart0.h"
#include "leds.h"

/* Other Tasks */
#include "printer.h"

/* MAC task include */
#include "tdma_coord.h"

/* Hardware initialization */
static void prvSetupHardware(void);

/* Global Variables */
static xSemaphoreHandle xSPIMutex;

/**
 * The main function.
 */
int main(void) {
	/* Setup the hardware. */
	prvSetupHardware();

	/* Create the SPI mutex */
	xSPIMutex = xSemaphoreCreateMutex();

	/* Create the task of the application */
	mac_create_task(xSPIMutex);
	vCreatePrinterTask(1);

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

	LEDS_INIT();
	LEDS_OFF();

	uart0_init(UART0_CONFIG_1MHZ_115200);

	printf("FreeRTOS ADC TDMA coordinator\r\n");

	/* Enable Interrupts */
	eint();
}

int putchar(int c) {
	return uart0_putchar(c);
}

void vApplicationIdleHook(void);
void vApplicationIdleHook(void) {
	_BIS_SR(LPM3_bits);
}

