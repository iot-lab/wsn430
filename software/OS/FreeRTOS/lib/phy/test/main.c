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

	#ifndef __MDS__
	/* Setup MCLK 8MHz and SMCLK 1MHz */
	set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
	#else
	set_smclk_src(SMCLK_SRC_DCO);
	set_mclk_src(MCLK_SRC_DCO);
	set_aclk_src(ACLK_SRC_XT1);

	#if(CLK_FREQ == 8000000)
	set_dco_speed(DCO_CLK_8MHZ);
	#elif(CLK_FREQ == 16000000)
	set_dco_speed(DCO_CLK_16MHZ);
	#else
	#error "CLK_FREQ not supported"
	#endif
	#endif

	LEDS_INIT();
	LEDS_OFF();

	#ifndef __MDS__
	uart0_init(UART0_CONFIG_1MHZ_115200);
	#else
	uart0_init(UART_SPEED_115200);
	#endif
	printf("FreeRTOS PHY MAC test program\r\n");
	printf("Type 's' to start/stop sending continuously\n");
	#ifndef __MDS__
	uart0_register_callback(char_rx);
	#else
	uart0_register_callbacks(char_rx, NULL);
	#endif

	/* Enable Interrupts */
	eint();
}

int putchar(int c) {
	#ifndef __MDS__
	return uart0_putchar(c);
	#else
	uart0_putchar(c);
	return c;
	#endif
}

void vApplicationIdleHook(void);
void vApplicationIdleHook(void) {
	_BIS_SR(LPM0_bits);
}

void packet_received(uint8_t * data, uint16_t length, int8_t rssi,
		uint16_t time) {
	printf("Frame received:  %s\n", data);
}
volatile uint16_t rx;


static uint16_t char_rx(uint8_t c) {
	if (c == 's') {
		rx = !rx;
	}
	return 1;
}

static void vSendingTask(void* pvParameters) {
	static char msg[32];
	static uint16_t len, i = 0;
	rx = 0;
	phy_rx();

	while (1) {
		printf("RX...\n");
		while (!rx) {
			vTaskDelay(20);
		}
		printf("TX...\n");

		while (rx) {
			len = sprintf(msg, "Hello %u\n", i++);
			if (phy_send((uint8_t*)msg, len, 0x0)) {
				printf("Frame sent: %s", msg);
			} else {
				printf("Frame send error\n");
			}
			LED_GREEN_TOGGLE();
			vTaskDelay(5);
		}
	}
}
