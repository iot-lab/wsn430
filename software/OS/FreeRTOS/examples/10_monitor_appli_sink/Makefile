ifndef FREERTOS_PATH
$(error Please set environment variable FREERTOS_PATH to the FreeRTOS folder)
endif
ifndef WSN430_DRIVERS_PATH
$(error Please set environment variable WSN430_DRIVERS_PATH to the drivers folder)
endif
ifndef SENSTOOLS_LIB_PATH
$(error Please set environment variable SENSTOOLS_LIB_PATH to the OS free library folder containing phy and mac folders)
endif
SOURCE_PATH = $(FREERTOS_PATH)/Source
PORT_PATH = $(FREERTOS_PATH)/Source/portable/GCC/MSP430F449

NAMES = sink_example_cc1100


INCLUDES  = -I. -Imac -Iinterface
INCLUDES += -I$(WSN430_DRIVERS_PATH)
INCLUDES += -I$(SENSTOOLS_LIB_PATH)/phy/simplephy
INCLUDES += -I$(FREERTOS_PATH)/Source/include

SRC  = main.c
SRC += mac/mac.c
SRC += interface/interface.c

SRC += $(SENSTOOLS_LIB_PATH)/phy/simplephy/simplephy.c
SRC += $(WSN430_DRIVERS_PATH)/cc1100.c

SRC += $(SOURCE_PATH)/tasks.c
SRC += $(SOURCE_PATH)/list.c
SRC += $(SOURCE_PATH)/queue.c
SRC += $(SOURCE_PATH)/portable/MemMang/heap_1.c
SRC += $(PORT_PATH)/port.c
SRC += $(WSN430_DRIVERS_PATH)/spi1.c
SRC += $(WSN430_DRIVERS_PATH)/clock.c
SRC += $(WSN430_DRIVERS_PATH)/timerB.c
SRC += $(WSN430_DRIVERS_PATH)/ds2411.c
SRC += $(WSN430_DRIVERS_PATH)/uart0.c

-include $(WSN430_DRIVERS_PATH)/Makefile.common

