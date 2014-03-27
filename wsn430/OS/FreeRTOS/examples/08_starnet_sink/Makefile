WSN430 = ../../../..
FREERTOS = $(WSN430)/OS/FreeRTOS

SOURCE_PATH = $(FREERTOS)/Source
PORT_PATH = $(FREERTOS)/Source/portable/GCC/MSP430F449
LIB_PATH = $(FREERTOS)/lib

NAMES = starnet_coord_cc1101


INCLUDES  = -I. -I$(WSN430)/drivers
INCLUDES += -I$(SOURCE_PATH)/include
INCLUDES += -I$(LIB_PATH)/mac/starnet/



SRC  = main.c
SRC += $(LIB_PATH)/mac/starnet/starnet_sink.c
SRC += $(WSN430)/drivers/cc1101.c

SRC += $(SOURCE_PATH)/tasks.c
SRC += $(SOURCE_PATH)/list.c
SRC += $(SOURCE_PATH)/queue.c
SRC += $(SOURCE_PATH)/portable/MemMang/heap_1.c
SRC += $(PORT_PATH)/port.c
SRC += $(WSN430)/drivers/uart0.c

SRC += $(WSN430)/drivers/spi1.c
SRC += $(WSN430)/drivers/clock.c
SRC += $(WSN430)/drivers/ds2411.c



-include $(WSN430)/drivers/Makefile.common

