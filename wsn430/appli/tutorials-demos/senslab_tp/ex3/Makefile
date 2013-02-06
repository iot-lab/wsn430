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


NAMES = ex3_cc1101 ex3_cc2420


SRC_ex3_cc1101  = log_rssi_cc1101.c
SRC_ex3_cc1101 += $(WSN430_DRIVERS_PATH)/cc1101.c
SRC_ex3_cc1101 += $(WSN430_LIB_PATH)/mac/csma_cc1101.c

SRC_ex3_cc2420  = log_rssi_cc2420.c
SRC_ex3_cc2420 += $(WSN430_DRIVERS_PATH)/cc2420.c
SRC_ex3_cc2420 += $(WSN430_LIB_PATH)/mac/csma_cc2420.c


SRC  = main.c
SRC += $(WSN430_DRIVERS_PATH)/clock.c
SRC += $(WSN430_DRIVERS_PATH)/uart0.c
SRC += $(WSN430_DRIVERS_PATH)/spi1.c
SRC += $(WSN430_DRIVERS_PATH)/ds2411.c
SRC += $(WSN430_DRIVERS_PATH)/timerB.c


INCLUDES = -I. -I$(WSN430_DRIVERS_PATH) -I$(WSN430_LIB_PATH)/mac

-include $(WSN430_DRIVERS_PATH)/Makefile.common

