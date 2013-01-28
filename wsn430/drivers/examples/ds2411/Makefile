WSN430_DRIVERS_PATH = ../..
ifndef WSN430_DRIVERS_PATH
$(error Please set environment variable WSN430_DRIVERS_PATH to the drivers folder)
endif

NAMES  = ds2411-example


# common sources
SRC  = main.c
SRC += $(WSN430_DRIVERS_PATH)/uart0.c
SRC += $(WSN430_DRIVERS_PATH)/clock.c
SRC += $(WSN430_DRIVERS_PATH)/ds2411.c


INCLUDES  = -I. -I$(WSN430_DRIVERS_PATH)


-include $(WSN430_DRIVERS_PATH)/Makefile.common

