# Setup paths to source code
# Support both development and tutorials configuration

ifndef WSN430_DRIVERS_PATH
# $(error Please set environment variable WSN430_DRIVERS_PATH to the drivers folder)
WSN430_DRIVERS_PATH=../drivers
endif

# define one name for each executable to be built
NAMES = ex1


SRC  = main.c
SRC += $(WSN430_DRIVERS_PATH)/clock.c
SRC += $(WSN430_DRIVERS_PATH)/timerA.c


INCLUDES  = -I. -I$(WSN430_DRIVERS_PATH)

-include $(WSN430_DRIVERS_PATH)/Makefile.common
