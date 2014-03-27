WSN430 = ../../../../../..
FREERTOS = $(WSN430)/OS/FreeRTOS

SOURCE_PATH = $(FREERTOS)/Source
PORT_PATH = $(FREERTOS)/Source/portable/GCC/MSP430F449

NAMES = csma_test_cc1101 csma_test_cc2420


SRC_csma_test_cc1101  = $(WSN430)/drivers/cc1101.c
SRC_csma_test_cc1101 += $(FREERTOS)/lib/phy/phy_cc1101.c

SRC_csma_test_cc2420  = $(WSN430)/drivers/cc2420.c
SRC_csma_test_cc2420 += $(FREERTOS)/lib/phy/phy_cc2420.c

SRC  = main.c
SRC += $(FREERTOS)/lib/mac/csma/csma.c

SRC += $(SOURCE_PATH)/tasks.c
SRC += $(SOURCE_PATH)/list.c
SRC += $(SOURCE_PATH)/queue.c
SRC += $(SOURCE_PATH)/portable/MemMang/heap_1.c
SRC += $(PORT_PATH)/port.c
SRC += $(WSN430)/drivers/uart0.c

SRC += $(WSN430)/drivers/spi1.c
SRC += $(WSN430)/drivers/clock.c
SRC += $(WSN430)/drivers/ds2411.c
SRC += $(WSN430)/drivers/timerB.c


INCLUDES  = -I. -I$(WSN430)/drivers -I$(SOURCE_PATH)/include
INCLUDES += -I$(FREERTOS)/lib/mac/csma
INCLUDES += -I$(FREERTOS)/lib/phy


-include $(WSN430)/drivers/Makefile.common

