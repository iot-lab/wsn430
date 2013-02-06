ifndef FREERTOS_PATH
$(error Please set environment variable FREERTOS_PATH to the FreeRTOS folder)
endif
ifndef WSN430_DRIVERS_PATH
$(error Please set environment variable WSN430_DRIVERS_PATH to the drivers folder)
endif
SOURCE_PATH = $(FREERTOS_PATH)/Source
PORT_PATH = $(FREERTOS_PATH)/Source/portable/GCC/MSP430F449

NAMES    = mutex_multitask_cc1101

# Implemented only for cc1101
SRC_mutex_multitask_cc1101 = $(WSN430_DRIVERS_PATH)/cc1101.c


INCLUDES  = -I. -I$(WSN430_DRIVERS_PATH) -I$(SOURCE_PATH)/include

SRC  = main.c radioTask.c
SRC += $(SOURCE_PATH)/tasks.c
SRC += $(SOURCE_PATH)/list.c
SRC += $(SOURCE_PATH)/queue.c
SRC += $(SOURCE_PATH)/portable/MemMang/heap_1.c
SRC += $(PORT_PATH)/port.c
SRC += $(WSN430_DRIVERS_PATH)/clock.c

SRC += $(WSN430_DRIVERS_PATH)/uart0.c
SRC += $(WSN430_DRIVERS_PATH)/spi1.c
SRC += $(WSN430_DRIVERS_PATH)/ds1722.c
SRC += $(WSN430_DRIVERS_PATH)/m25p80.c



-include $(WSN430_DRIVERS_PATH)/Makefile.common

