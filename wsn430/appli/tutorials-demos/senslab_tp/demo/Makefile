# Setup paths to source code
# Support both development and tutorials configuration

ifndef WSN430_DRIVERS_PATH
# $(error Please set environment variable WSN430_DRIVERS_PATH to the drivers folder)
WSN430_DRIVERS_PATH = ../drivers
endif
ifndef WSN430_LIB_PATH
# $(error Please set environment variable WSN430_LIB_PATH to the OS free library folder containing phy and mac folders)
WSN430_LIB_PATH = ..
endif


NAMES = demo_cc1100 demo_cc2420


SRC_demo_cc1100  = $(WSN430_DRIVERS_PATH)/cc1100.c
SRC_demo_cc1100 += $(WSN430_LIB_PATH)/mac/csma_cc1100.c

SRC_demo_cc2420  = $(WSN430_DRIVERS_PATH)/cc2420.c
SRC_demo_cc2420 += $(WSN430_LIB_PATH)/mac/csma_cc2420.c



SRC  = main.c
SRC += $(WSN430_DRIVERS_PATH)/clock.c
SRC += $(WSN430_DRIVERS_PATH)/uart0.c
SRC += $(WSN430_DRIVERS_PATH)/spi1.c
SRC += $(WSN430_DRIVERS_PATH)/ds2411.c
SRC += $(WSN430_DRIVERS_PATH)/timerB.c
SRC += $(WSN430_DRIVERS_PATH)/timerA.c


INCLUDES = -I. -I$(WSN430_DRIVERS_PATH) -I$(WSN430_LIB_PATH)/mac


-include $(WSN430_DRIVERS_PATH)/Makefile.common

