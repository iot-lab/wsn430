ifndef FREERTOS_PATH
$(error Please set environment variable FREERTOS_PATH to the FreeRTOS folder)
endif
ifndef WSN430_DRIVERS_PATH
$(error Please set environment variable WSN430_DRIVERS_PATH to the drivers folder)
endif
SOURCE_PATH = $(FREERTOS_PATH)/Source
PORT_PATH = $(FREERTOS_PATH)/Source/portable/GCC/MSP430F449

NAMES    = adc_node_cc1100 adc_node_cc2420


SRC_cc1100  = $(WSN430_DRIVERS_PATH)/cc1100.c
SRC_cc1100 += $(FREERTOS_PATH)/lib/phy/phy_cc1100.c
SRC_cc2420  = $(WSN430_DRIVERS_PATH)/cc2420.c
SRC_cc2420 += $(FREERTOS_PATH)/lib/phy/phy_cc2420.c



SRC_adc_node_cc1100 = $(SRC_cc1100)
SRC_adc_node_cc2420 = $(SRC_cc2420)

INCLUDES  = -I. -I$(WSN430_DRIVERS_PATH) -I$(SOURCE_PATH)/include
INCLUDES += -I$(FREERTOS_PATH)/lib/mac/tdma
INCLUDES += -I$(FREERTOS_PATH)/lib/phy

SRC  = main.c adc.c
SRC += $(FREERTOS_PATH)/lib/mac/tdma/tdma_node.c
SRC += $(SOURCE_PATH)/tasks.c
SRC += $(SOURCE_PATH)/list.c
SRC += $(SOURCE_PATH)/queue.c
SRC += $(SOURCE_PATH)/portable/MemMang/heap_1.c
SRC += $(PORT_PATH)/port.c
SRC += $(WSN430_DRIVERS_PATH)/uart0.c

SRC += $(WSN430_DRIVERS_PATH)/timerB.c
SRC += $(WSN430_DRIVERS_PATH)/spi1.c
SRC += $(WSN430_DRIVERS_PATH)/clock.c
SRC += $(WSN430_DRIVERS_PATH)/ds2411.c



-include $(WSN430_DRIVERS_PATH)/Makefile.common

