WSN430 = ../../../..
NAMES     = phy_test_node  phy_test_sink


INCLUDES  = -I$(WSN430)/drivers
INCLUDES += -I..

SRC_phy_test_node = main_node.c
SRC_phy_test_sink = main_sink.c

SRC  = ../simplephy.c
SRC += $(WSN430)/drivers/cc1101.c

SRC += $(WSN430)/drivers/clock.c
SRC += $(WSN430)/drivers/uart0.c
SRC += $(WSN430)/drivers/spi1.c


-include $(WSN430)/drivers/Makefile.common

