ifndef WSN430_DRIVERS_PATH
$(error Please set environment variable WSN430_DRIVERS_PATH to the drivers folder)
endif
ifndef WSN430_LIB_PATH
$(error Please set environment variable WSN430_LIB_PATH to the OS free library folder containing phy and mac folders)
endif

NAMES = tdma_node tdma_coord


SRC_tdma_node  = main_node.c
SRC_tdma_node += $(WSN430_LIB_PATH)/mac/tdma/tdma_n.c

SRC_tdma_coord  = main_coord.c
SRC_tdma_coord += $(WSN430_LIB_PATH)/mac/tdma/tdma_c.c
SRC_tdma_coord += $(WSN430_LIB_PATH)/mac/tdma/tdma_mgt.c



SRC  = $(WSN430_DRIVERS_PATH)/cc1101.c
SRC += $(WSN430_DRIVERS_PATH)/ds2411.c
SRC += $(WSN430_DRIVERS_PATH)/clock.c
SRC += $(WSN430_DRIVERS_PATH)/uart0.c
SRC += $(WSN430_DRIVERS_PATH)/spi1.c
SRC += $(WSN430_DRIVERS_PATH)/timerB.c

INCLUDES  = -I$(WSN430_DRIVERS_PATH)
INCLUDES += -I$(WSN430_LIB_PATH)/mac/tdma/


-include $(WSN430_DRIVERS_PATH)/Makefile.common

