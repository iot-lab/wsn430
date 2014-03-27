WSN430 = ../../../..

NAMES = tdma_node tdma_coord


SRC_tdma_node  = main_node.c
SRC_tdma_node += $(WSN430)/lib/mac/tdma/tdma_n.c

SRC_tdma_coord  = main_coord.c
SRC_tdma_coord += $(WSN430)/lib/mac/tdma/tdma_c.c
SRC_tdma_coord += $(WSN430)/lib/mac/tdma/tdma_mgt.c



SRC  = $(WSN430)/drivers/cc1101.c
SRC += $(WSN430)/drivers/ds2411.c
SRC += $(WSN430)/drivers/clock.c
SRC += $(WSN430)/drivers/uart0.c
SRC += $(WSN430)/drivers/spi1.c
SRC += $(WSN430)/drivers/timerB.c

INCLUDES  = -I$(WSN430)/drivers
INCLUDES += -I$(WSN430)/lib/mac/tdma/


include $(WSN430)/drivers/Makefile.common

