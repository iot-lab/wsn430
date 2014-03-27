# Setup paths to source code
# Support both development and tutorials configuration

WSN430 = ../../../..

NAMES = demo_cc1101 demo_cc2420


SRC_demo_cc1101  = $(WSN430)/drivers/cc1101.c
SRC_demo_cc1101 += $(WSN430)/lib/mac/csma_cc1101.c

SRC_demo_cc2420  = $(WSN430)/drivers/cc2420.c
SRC_demo_cc2420 += $(WSN430)/lib/mac/csma_cc2420.c



SRC  = main.c
SRC += $(WSN430)/drivers/clock.c
SRC += $(WSN430)/drivers/uart0.c
SRC += $(WSN430)/drivers/spi1.c
SRC += $(WSN430)/drivers/ds2411.c
SRC += $(WSN430)/drivers/timerB.c
SRC += $(WSN430)/drivers/timerA.c


INCLUDES = -I. -I$(WSN430)/drivers -I$(WSN430)/lib/mac


include $(WSN430)/drivers/Makefile.common

