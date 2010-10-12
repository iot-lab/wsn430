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
#include "mac.h"

// Hardware initialization
static void prvSetupHardware(void);

// Task that sends packets
static void vSendingTask(void* pvParameters);

// function for handling received packets
void packet_received(uint16_t from, uint8_t* pkt, uint16_t pktLength,
		int8_t rssi);

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

	/* Initialize the MAC layer */
	mac_init(xSPIMutex, packet_received);

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

	printf("FreeRTOS CSMA MAC test program\r\n");

	/* Enable Interrupts */
	eint();
}

int putchar(int c) {
	return uart0_putchar(c);
}

void vApplicationIdleHook(void);
void vApplicationIdleHook(void) {
	_BIS_SR(LPM0_bits);
}

void packet_received(uint16_t from, uint8_t* pkt, uint16_t pktLength,
		int8_t rssi) {
	printf("Frame received [%u] from %.4x (%d): %s\r\n", pktLength, from, rssi, pkt);
}

static void vSendingTask(void* pvParameters) {
	while (1) {
		vTaskDelay(20);
//		if (mac_send(MAC_BROADCAST_ADDR, (uint8_t*) "Test", sizeof("Test"))) {
//			printf("Frame sent [%u]\r\n", sizeof("Test"));
//		} else {
//			printf("Frame send error\n");
//		}
	}
}
