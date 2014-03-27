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
#include "tdma_coord.h"
#include "dataSensor.h"

/* Hardware initialization */
static void prvSetupHardware(void);

static void new_node(uint16_t);
static void new_data(uint16_t);

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
	mac_create_task(xSPIMutex, configMAX_PRIORITIES - 1);

	mac_set_node_associated_handler(new_node);
	mac_set_data_received_handler(new_data);

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

	printf("FreeRTOS SENSOR TDMA coordinator\r\n");

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

static void new_node(uint16_t node) {
	printf("new node: %4x\n", node);
}


static uint32_t inline sensor_to_mv(uint16_t raw) {
	uint32_t temp;
	temp = raw;
	temp *= 2500;
	temp >>= 12;
	return temp;
}

static void new_data(uint16_t node) {
	sensor_data_t *data;
	uint16_t length;
	data = (sensor_data_t*) mac_read(node, &length);
	uint16_t i,j;


	for (i = 0; i < data->length; i++) {
	  printf("%.4x:%u", node, data->seq - data->length + 1 + i);
		for (j=0;j<SIZEDATA;j++)
		  printf(" %u", data->measures[i][j]);
		printf("\r\n");
	}
}
