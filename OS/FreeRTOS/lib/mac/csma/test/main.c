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

	/* Initialize the MAC layer, channel 0 */
	mac_init(xSPIMutex, packet_received, 0);

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
static volatile uint16_t myfriend;

void packet_received(uint16_t from, uint8_t* pkt, uint16_t length, int8_t rssi) {
	printf("Frame received [%u] from %.4x (%d): %s\r\n", length, from, rssi,
			pkt);
	if (myfriend == 0x0) {
		myfriend = from;
		printf("My New Friend is %.4x\n", myfriend);
	}
}

static void vSendingTask(void* pvParameters) {
	myfriend = 0x0;
	while (1) {
		vTaskDelay(50);
		printf("\nSending hello broadcast\n");
		mac_send(MAC_BROADCAST_ADDR, (uint8_t*) "Test", sizeof("Test"), 0);

		if (myfriend) {
			vTaskDelay(50);
			printf("\nSending frame to my friend %.4x\n", myfriend);
			mac_send(myfriend, (uint8_t*) "Test", sizeof("Test"), 0);
		}
	}
}
