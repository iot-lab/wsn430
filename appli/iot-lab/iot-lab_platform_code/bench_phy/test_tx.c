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

/* PHY task include */
#include "phy.h"

// Hardware initialization
static void prvSetupHardware(void);

// Task that sends packets
static void vSendingTask(void* pvParameters);

// function for handling received packets
void packet_received(uint8_t * data, uint16_t length, int8_t rssi,
		uint16_t time);
static uint16_t char_rx(uint8_t c);

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

	/* Initialize the MAC layer, channel 4 */
	phy_init(xSPIMutex, packet_received, 4, PHY_TX_0dBm);

	/* Add the local task */
	xTaskCreate( vSendingTask, (const signed char*) "sender", configMINIMAL_STACK_SIZE, NULL, 1, NULL );

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
	printf("FreeRTOS PHY TX test program\n");

	/* Enable Interrupts */
	eint();
}

int putchar(int c) {
	uart0_putchar(c);
	return c;
}

void vApplicationIdleHook(void) {
	_BIS_SR(LPM0_bits);
}

void packet_received(uint8_t * data, uint16_t length, int8_t rssi,
		uint16_t time) {
	LED_RED_TOGGLE();
	printf("Frame received, len = %u\n", length);
}

static void vSendingTask(void* pvParameters) {
	uint8_t msg[128];
	uint16_t count = 0;
	while (1) {
		// Copy counter to frame
		msg[0] = count >> 8;
		msg[1] = count & 0xFF;
		// Send frame
		if (!phy_send(msg, 64, 0x0)) {
			printf("Frame send error\n");
		}
		count++;
		LED_GREEN_TOGGLE();
		
		vTaskDelay(1);
	}
}
