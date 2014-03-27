WSN430 = ../wsn430

NAMES  = tutorial_cc1101
NAMES += tutorial_cc2420


# common sources
SRC  = main.c
SRC += $(WSN430)/drivers/uart0.c
SRC += $(WSN430)/drivers/clock.c
SRC += $(WSN430)/drivers/ds1722.c
SRC += $(WSN430)/drivers/spi1.c
SRC += $(WSN430)/drivers/timerA.c
SRC += $(WSN430)/drivers/i2c0.c
SRC += $(WSN430)/drivers/tsl2550.c

#
# CSMA layers
#
SRC += $(WSN430)/drivers/ds2411.c
SRC += $(WSN430)/drivers/timerB.c

SRC_tutorial_cc1101  = $(WSN430)/drivers/cc1101.c
SRC_tutorial_cc1101 += $(WSN430)/lib/mac/csma_cc1101.c

SRC_tutorial_cc2420  = $(WSN430)/drivers/cc2420.c
SRC_tutorial_cc2420 += $(WSN430)/lib/mac/csma_cc2420.c


INCLUDES  = -I. -I$(WSN430)/drivers
INCLUDES += -I$(WSN430)/lib/mac


include $(WSN430)/drivers/Makefile.common

