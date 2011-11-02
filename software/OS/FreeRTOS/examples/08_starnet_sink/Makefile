ifndef FREERTOS_PATH
$(error Please set environment variable FREERTOS_PATH to the FreeRTOS folder)
endif
ifndef WSN430_DRIVERS_PATH
$(error Please set environment variable WSN430_DRIVERS_PATH to the drivers folder)
endif
SOURCE_PATH = $(FREERTOS_PATH)/Source
PORT_PATH = $(FREERTOS_PATH)/Source/portable/GCC/MSP430F449
LIB_PATH = $(FREERTOS_PATH)/lib

NAMES = starnet_coord_cc1100


INCLUDES  = -I. -I$(WSN430_DRIVERS_PATH)
INCLUDES += -I$(SOURCE_PATH)/include
INCLUDES += -I$(LIB_PATH)/mac/starnet/



SRC  = main.c
SRC += $(LIB_PATH)/mac/starnet/starnet_sink.c
SRC += $(WSN430_DRIVERS_PATH)/cc1100.c

SRC += $(SOURCE_PATH)/tasks.c
SRC += $(SOURCE_PATH)/list.c
SRC += $(SOURCE_PATH)/queue.c
SRC += $(SOURCE_PATH)/portable/MemMang/heap_1.c
SRC += $(PORT_PATH)/port.c
SRC += $(WSN430_DRIVERS_PATH)/uart0.c

SRC += $(WSN430_DRIVERS_PATH)/spi1.c
SRC += $(WSN430_DRIVERS_PATH)/clock.c
SRC += $(WSN430_DRIVERS_PATH)/ds2411.c



-include $(WSN430_DRIVERS_PATH)/Makefile.common

