WSN430 = ../../../..
FREERTOS = $(WSN430)/OS/FreeRTOS

SOURCE_PATH = $(FREERTOS)/Source
PORT_PATH = $(FREERTOS)/Source/portable/GCC/MSP430F449

NAMES    = adc_sink_cc1101 adc_sink_cc2420


SRC_cc1101  = $(WSN430)/drivers/cc1101.c
SRC_cc1101 += $(FREERTOS)/lib/phy/phy_cc1101.c
SRC_cc2420  = $(WSN430)/drivers/cc2420.c
SRC_cc2420 += $(FREERTOS)/lib/phy/phy_cc2420.c


SRC_adc_sink_cc1101 = $(SRC_cc1101)
SRC_adc_sink_cc2420 = $(SRC_cc2420)


INCLUDES  = -I. -I$(WSN430)/drivers -I$(SOURCE_PATH)/include
INCLUDES += -I$(FREERTOS)/lib/mac/tdma
INCLUDES += -I$(FREERTOS)/lib/phy


SRC  = main.c printer.c
SRC += $(FREERTOS)/lib/mac/tdma/tdma_coord.c
SRC += $(FREERTOS)/lib/mac/tdma/tdma_table.c
SRC += $(SOURCE_PATH)/tasks.c
SRC += $(SOURCE_PATH)/list.c
SRC += $(SOURCE_PATH)/queue.c
SRC += $(SOURCE_PATH)/portable/MemMang/heap_1.c
SRC += $(PORT_PATH)/port.c
SRC += $(WSN430)/drivers/uart0.c
SRC += $(WSN430)/drivers/timerB.c
SRC += $(WSN430)/drivers/spi1.c
SRC += $(WSN430)/drivers/clock.c
SRC += $(WSN430)/drivers/ds2411.c



-include $(WSN430)/drivers/Makefile.common

