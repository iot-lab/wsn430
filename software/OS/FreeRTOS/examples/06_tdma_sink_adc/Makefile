ifndef FREERTOS_PATH
	$(error Please set environment variable FREERTOS_PATH to the FreeRTOS folder)
endif
ifndef WSN430_DRIVERS_PATH
	$(error Please set environment variable WSN430_DRIVERS_PATH to the drivers folder)
endif
SOURCE_PATH = $(FREERTOS_PATH)/Source
PORT_PATH = $(FREERTOS_PATH)/Source/portable/GCC/MSP430F449

NAME    = adc_sink

# Set Radio driver
DRIVER = cc1100
# DRIVER = cc2420

TARGETS = $(NAME)_$(DRIVER).elf $(NAME)_$(DRIVER).hex


INCLUDES  = -I. -I$(WSN430_DRIVERS_PATH) -I$(SOURCE_PATH)/include
INCLUDES += -I$(FREERTOS_PATH)/lib/mac/tdma
INCLUDES += -I$(FREERTOS_PATH)/lib/phy


SRC  = main.c printer.c
SRC += $(FREERTOS_PATH)/lib/mac/tdma/tdma_coord.c
SRC += $(FREERTOS_PATH)/lib/mac/tdma/tdma_table.c
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
ifeq ($(DRIVER), cc1100)
SRC += $(WSN430_DRIVERS_PATH)/cc1100.c
SRC += $(FREERTOS_PATH)/lib/phy/phy_cc1100.c
else
SRC += $(WSN430_DRIVERS_PATH)/cc2420.c
SRC += $(FREERTOS_PATH)/lib/phy/phy_cc2420.c
endif

-include ../Makefile.common

