WSN430 = ../../../../../..
FREERTOS = $(WSN430)/OS/FreeRTOS


ifeq ($(BUILD),MDS)
# for clean use 'make BUILD=MDS clean'
ifndef MDS_DRIVERS_PATH
$(error Please set environment variable MDS_DRIVERS_PATH to the mds drivers folder)
endif # MDS_DRIVERS_PATH
endif # BUILD

SOURCE_PATH = $(FREERTOS)/Source
PORT_PATH = $(FREERTOS)/Source/portable/GCC/MSP430F449


NAMES_wsn  = tdma_test_coord_cc1101 tdma_test_node_cc1101
NAMES_wsn += tdma_test_coord_cc2420 tdma_test_node_cc2420
NAMES_mds  = tdma_test_coord_cc2500 tdma_test_node_cc2500


INCLUDES  = -I. -I$(SOURCE_PATH)/include
INCLUDES += -I$(FREERTOS)/lib/phy
INCLUDES += -I$(FREERTOS)/lib/mac/tdma

ifneq ($(BUILD),MDS)
DRIVERS_PATH  = $(WSN430)/drivers
INCLUDES     += -I$(DRIVERS_PATH)
CPU           = -mmcu=msp430x1611
NAMES         = $(NAMES_wsn)
else
DRIVERS_PATH  = $(MDS_DRIVERS_PATH)
INCLUDES     += -I$(DRIVERS_PATH)
CPU           = -mmcu=msp430x2618
CFLAGS        = -D__MDS__ -DCLK_FREQ=8000000 -D__MAC__=0xBEEF
NAMES         = $(NAMES_mds)
endif



# Intermediate SRC variables

SRC_coord  = main_coord.c
SRC_coord += $(FREERTOS)/lib/mac/tdma/tdma_coord.c
SRC_coord += $(FREERTOS)/lib/mac/tdma/tdma_table.c

SRC_node  = main_node.c
SRC_node += $(FREERTOS)/lib/mac/tdma/tdma_node.c

SRC_wsn  = $(DRIVERS_PATH)/spi1.c
SRC_wsn += $(DRIVERS_PATH)/ds2411.c

SRC_cc1101  = $(DRIVERS_PATH)/cc1101.c
SRC_cc1101 += $(FREERTOS)/lib/phy/phy_cc1101.c

SRC_cc2420  = $(DRIVERS_PATH)/cc2420.c
SRC_cc2420 += $(FREERTOS)/lib/phy/phy_cc2420.c

SRC_mds  = $(DRIVERS_PATH)/spi0.c
SRC_cc2500  = $(DRIVERS_PATH)/cc2500.c
SRC_cc2500 += $(FREERTOS)/lib/phy/phy_cc2500.c


# target specific SRC variables
SRC_tdma_test_coord_cc1101 = $(SRC_coord) $(SRC_wsn) $(SRC_cc1101)
SRC_tdma_test_node_cc1101  = $(SRC_node)  $(SRC_wsn) $(SRC_cc1101)
SRC_tdma_test_coord_cc2420 = $(SRC_coord) $(SRC_wsn) $(SRC_cc2420)
SRC_tdma_test_node_cc2420  = $(SRC_node)  $(SRC_wsn) $(SRC_cc2420)
SRC_tdma_test_coord_cc2500 = $(SRC_coord) $(SRC_mds) $(SRC_cc2500)
SRC_tdma_test_node_cc2500  = $(SRC_node)  $(SRC_mds) $(SRC_cc2500)


SRC  = $(SOURCE_PATH)/tasks.c
SRC += $(SOURCE_PATH)/list.c
SRC += $(SOURCE_PATH)/queue.c
SRC += $(SOURCE_PATH)/portable/MemMang/heap_1.c
SRC += $(PORT_PATH)/port.c
SRC += $(DRIVERS_PATH)/uart0.c
SRC += $(DRIVERS_PATH)/clock.c
SRC += $(DRIVERS_PATH)/timerB.c


-include $(WSN430)/drivers/Makefile.common

