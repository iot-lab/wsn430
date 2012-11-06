# Setup paths to source code
# Support both development and tutorials configuration

ifndef WSN430_DRIVERS_PATH
# $(error Please set environment variable WSN430_DRIVERS_PATH to the drivers folder)
WSN430_DRIVERS_PATH=../drivers
endif

NAMES = ex2


SRC  = main.c
SRC += $(WSN430_DRIVERS_PATH)/clock.c
SRC += $(WSN430_DRIVERS_PATH)/uart0.c
SRC += $(WSN430_DRIVERS_PATH)/spi1.c
SRC += $(WSN430_DRIVERS_PATH)/ds1722.c
SRC += $(WSN430_DRIVERS_PATH)/i2c0.c
SRC += $(WSN430_DRIVERS_PATH)/tsl2550.c


INCLUDES  = -I. -I$(WSN430_DRIVERS_PATH)

-include $(WSN430_DRIVERS_PATH)/Makefile.common

