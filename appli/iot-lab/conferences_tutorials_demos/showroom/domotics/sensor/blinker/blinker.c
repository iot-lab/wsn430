#include <io.h>
#include <stdlib.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Project Includes */
#include "blinker.h"

/* Driver Includes */
#include "leds.h"

#define CMD_MASK       (0x3 << 6)
#define CMD_LED_ON     (0x0 << 6)
#define CMD_LED_OFF    (0x1 << 6)
#define CMD_LED_TOGGLE (0x2 << 6)

#define LED_MASK   (0x3 << 4)
#define LED_RED    (0x0 << 4)
#define LED_GREEN  (0x1 << 4)
#define LED_BLUE   (0x2 << 4)
#define LED_ALL    (0x3 << 4)

void vBlinkerRequest(uint8_t cmd)
{
    switch (cmd & CMD_MASK)
    {
        case CMD_LED_ON:
            switch (cmd & LED_MASK)
            {
                case LED_RED:
                    LED_RED_ON();
                    break;
                case LED_GREEN:
                    LED_GREEN_ON();
                    break;
                case LED_BLUE:
                    LED_BLUE_ON();
                    break;
                case LED_ALL:
                    LEDS_ON();
                    break;
            }
            break;

        case CMD_LED_OFF:
            switch (cmd & LED_MASK)
            {
                case LED_RED:
                    LED_RED_OFF();
                    break;
                case LED_GREEN:
                    LED_GREEN_OFF();
                    break;
                case LED_BLUE:
                    LED_BLUE_OFF();
                    break;
                case LED_ALL:
                    LEDS_OFF();
                    break;
            }
            break;
        case CMD_LED_TOGGLE:
            switch (cmd & LED_MASK)
            {
                case LED_RED:
                    LED_RED_TOGGLE();
                    break;
                case LED_GREEN:
                    LED_GREEN_TOGGLE();
                    break;
                case LED_BLUE:
                    LED_BLUE_TOGGLE();
                    break;
                case LED_ALL:
                    LED_RED_TOGGLE();
                    LED_GREEN_TOGGLE();
                    LED_BLUE_TOGGLE();
                    break;
            }
            break;
    }
}
