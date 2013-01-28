WSN430_DRIVERS_PATH = ../..
ifndef WSN430_DRIVERS_PATH
$(error Please set environment variable WSN430_DRIVERS_PATH to the drivers folder)
endif

NAMES  = tsl2550-example


# common sources
SRC  = main.c
SRC += $(WSN430_DRIVERS_PATH)/tsl2550.c
SRC += $(WSN430_DRIVERS_PATH)/i2c0.c
SRC += $(WSN430_DRIVERS_PATH)/uart0.c
SRC += $(WSN430_DRIVERS_PATH)/clock.c


INCLUDES  = -I. -I$(WSN430_DRIVERS_PATH)


-include $(WSN430_DRIVERS_PATH)/Makefile.common

