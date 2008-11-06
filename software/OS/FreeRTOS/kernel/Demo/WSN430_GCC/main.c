/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"

/* LEDs config */
#define mainLED_TASK_PRIORITY (tskIDLE_PRIORITY + 1)
#define LED_OUT   P5OUT
#define BIT_BLUE   (1 << 6)
#define BIT_GREEN (1 << 5)
#define BIT_RED    (1 << 4)

/*
 * The LEDs flashing tasks
 */
static void vTaskLED0( void *pvParameters );
static void vTaskLED1( void *pvParameters );
static void vTaskLED2( void *pvParameters );
/*
 * Perform Hardware initialization.
 */
static void prvSetupHardware( void );
int main( void )
{
  /* Setup the hardware ready for the demo. */
  prvSetupHardware();
  /* Start the LEDs tasks */
  xTaskCreate( vTaskLED0, "LED0", configMINIMAL_STACK_SIZE, \
     NULL, mainLED_TASK_PRIORITY, NULL );
  xTaskCreate( vTaskLED1, "LED1", configMINIMAL_STACK_SIZE, \
     NULL, mainLED_TASK_PRIORITY, NULL );
  xTaskCreate( vTaskLED2, "LED2", configMINIMAL_STACK_SIZE, \
     NULL, mainLED_TASK_PRIORITY, NULL );
  /* Start the scheduler. */
  vTaskStartScheduler();
  /* As the scheduler has been started the demo application
  tasks will be executing and we should never get here! */
  return 0;
}
/* First LED flash task */
static void vTaskLED0( void *pvParameters )
{
  while (1)
  {
    /* Toggle blue LED and wait 500 ticks */
    LED_OUT ^= BIT_BLUE;
    vTaskDelay(500);
  }
}
/* Second LED flash task */
static void vTaskLED1( void *pvParameters )
{
  while (1)
  {
    /* Toggle green LED and wait 1000 ticks */
    LED_OUT ^= BIT_GREEN;
    vTaskDelay(1000);
  }
}
/* Third LED flash task */
static void vTaskLED2( void *pvParameters )
{
  while (1)
  {
    /* Toggle red LED and wait 2000 ticks */
    LED_OUT ^= BIT_RED;
    vTaskDelay(2000);
  }
}
static void prvSetupHardware( void )
{
  /* Stop the watchdog timer. */
  WDTCTL = WDTPW + WDTHOLD;
  /* Setup MCLK 8MHz and SMCLK 1MHz */
  DCOCTL = 0;
  BCSCTL1 = 0;
  BCSCTL2 = SELM_2 | (SELS | DIVS_3) ;
  /* Wait for cristal to stabilize */
  int i;
  do {
    /* Clear OSCFault flag */
   IFG1 &= ~OFIFG;
    /* Time for flag to set */
   for (i = 0xff; i > 0; i--) nop();
  } while ((IFG1 & OFIFG) != 0);
  /* Configure IO for LED use */
  P5SEL &= ~(BIT_BLUE | BIT_GREEN | BIT_RED);
  P5OUT &= ~(BIT_BLUE | BIT_GREEN | BIT_RED);
  P5DIR |= (BIT_BLUE | BIT_GREEN | BIT_RED);
}
