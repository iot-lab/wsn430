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
#include "timerB.h"

/* MAC task include */
#include "tdma_node.h"

#define TX_PERIOD 32768/20
#define TX_LENGTH 32
#define TX_BURST 1

/* Hardware initialization */
static void prvSetupHardware(void);

static void beacon(uint8_t id, uint16_t time);
static void associated(void);
static void lost(void);
static void rx(uint8_t* data, uint16_t length);

static void vSensorTask(void* pvParameters);

static uint16_t send_time(void);

/* Global Variables */
static xSemaphoreHandle xSPIMutex;
static xSemaphoreHandle tx_sem;
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

	vSemaphoreCreateBinary(tx_sem);

	/* Create the task */
	xTaskCreate(vSensorTask, (const signed char * const ) "sensor",
			configMINIMAL_STACK_SIZE, NULL, 1, NULL );

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

static void vSensorTask(void* pvParameters) {
	static uint8_t frame[120];

	// Start MAC layer
	mac_send_command(MAC_ASSOCIATE);
	mac_set_event_handler(MAC_ASSOCIATED, associated);
	mac_set_event_handler(MAC_LOST, lost);
	mac_set_beacon_handler(beacon);
	mac_set_data_received_handler(rx);

	printf("Node Address: %.4X\n", mac_addr);
	frame[0] = 0;

	timerB_set_alarm_from_now(TIMERB_ALARM_CCR6, 32768, TX_PERIOD);
	timerB_register_cb(TIMERB_ALARM_CCR6, send_time);

	xSemaphoreTake(tx_sem, 0);

	for (;;) {
		if (xSemaphoreTake(tx_sem,portMAX_DELAY) == pdTRUE) {
			int i;
			LED_GREEN_TOGGLE();
			for (i = 0; i < TX_BURST; i++) {
				// Send
				(*frame)++;
				mac_send(frame, TX_LENGTH);
				printf("s%u", *frame);
			}
			putchar('\n');
		}
	}
}

static uint16_t send_time(void) {
	portBASE_TYPE yield;
	if (xSemaphoreGiveFromISR(tx_sem, &yield) && yield) {
		portYIELD();
	}
	return 1;
}

static void beacon(uint8_t id, uint16_t timestamp) {
	LED_RED_TOGGLE();
	putchar('b');
}

static void associated(void) {
	printf("asso\n");
}

static void lost(void) {
	printf("#lost\n");
}

static void rx(uint8_t* data, uint16_t length) {
}

int putchar(int c) {
	return uart0_putchar(c);
}

void vApplicationIdleHook(void);
void vApplicationIdleHook(void) {
	_BIS_SR(LPM3_bits);
}
