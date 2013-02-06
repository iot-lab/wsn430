ifndef WSN430_DRIVERS_PATH
$(error Please set environment variable WSN430_DRIVERS_PATH to the drivers folder)
endif


NAMES     = phy_test_node  phy_test_sink


INCLUDES  = -I$(WSN430_DRIVERS_PATH)
INCLUDES += -I..

SRC_phy_test_node = main_node.c
SRC_phy_test_sink = main_sink.c

SRC  = ../simplephy.c
SRC += $(WSN430_DRIVERS_PATH)/cc1101.c

SRC += $(WSN430_DRIVERS_PATH)/clock.c
SRC += $(WSN430_DRIVERS_PATH)/uart0.c
SRC += $(WSN430_DRIVERS_PATH)/spi1.c


-include $(WSN430_DRIVERS_PATH)/Makefile.common

