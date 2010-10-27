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

/* MAC task include */
#include "tdma_node.h"

/* Hardware initialization */
static void prvSetupHardware(void);

static void beacon(uint8_t id, uint16_t time);
static void associated(void);
static void lost(void);
static void rx(uint8_t* data, uint16_t length);

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

	mac_set_event_handler(MAC_ASSOCIATED, associated);
	mac_set_data_received_handler(rx);
	mac_set_event_handler(MAC_LOST, lost);
	mac_set_beacon_handler(beacon);
	mac_send_command(MAC_ASSOCIATE);

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

	printf("FreeRTOS TDMA node test program\r\n");

	/* Enable Interrupts */
	eint();
}

static uint8_t data[4] = { 0 };
static void beacon(uint8_t id, uint16_t timestamp) {
	putchar('b');
	if (id % 4 == 0) {
		data[0]++;
		mac_send(data, 119);
		putchar('S');
	}

}

static void associated(void) {
	putchar('A');
}

static void lost(void) {
	putchar('L');
}

static void rx(uint8_t* data, uint16_t length) {
	putchar('r');
}

int putchar(int c) {
	return uart0_putchar(c);
}

void vApplicationIdleHook(void);
void vApplicationIdleHook(void) {
	_BIS_SR(LPM3_bits);
}
