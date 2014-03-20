ifndef WSN430_DRIVERS_PATH
$(error Please set environment variable WSN430_DRIVERS_PATH to the drivers folder)
endif
ifndef WSN430_LIB_PATH
	$(error Please set environment variable WSN430_LIB_PATH to the OS free library folder containing phy and mac folders)
endif


NAMES  = tutorial_cc1101
NAMES += tutorial_cc2420


# common sources
SRC  = main.c
SRC += $(WSN430_DRIVERS_PATH)/uart0.c
SRC += $(WSN430_DRIVERS_PATH)/clock.c
SRC += $(WSN430_DRIVERS_PATH)/ds1722.c
SRC += $(WSN430_DRIVERS_PATH)/spi1.c
SRC += $(WSN430_DRIVERS_PATH)/timerA.c
SRC += $(WSN430_DRIVERS_PATH)/i2c0.c
SRC += $(WSN430_DRIVERS_PATH)/tsl2550.c

#
# CSMA layers
#
SRC += $(WSN430_DRIVERS_PATH)/ds2411.c
SRC += $(WSN430_DRIVERS_PATH)/timerB.c

SRC_tutorial_cc1101  = $(WSN430_DRIVERS_PATH)/cc1101.c
SRC_tutorial_cc1101 += $(WSN430_LIB_PATH)/mac/csma_cc1101.c

SRC_tutorial_cc2420  = $(WSN430_DRIVERS_PATH)/cc2420.c
SRC_tutorial_cc2420 += $(WSN430_LIB_PATH)/mac/csma_cc2420.c


INCLUDES  = -I. -I$(WSN430_DRIVERS_PATH)
INCLUDES += -I$(WSN430_LIB_PATH)/mac


-include $(WSN430_DRIVERS_PATH)/Makefile.common

