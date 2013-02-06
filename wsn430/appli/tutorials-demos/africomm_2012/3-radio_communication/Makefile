ifndef WSN430_DRIVERS_PATH
$(error Please set environment variable WSN430_DRIVERS_PATH to the drivers folder)
endif

# define one name for each executable to be built
NAMES = tp


SRC  = main.c
SRC += $(WSN430_DRIVERS_PATH)/uart0.c
SRC += $(WSN430_DRIVERS_PATH)/spi1.c
SRC += $(WSN430_DRIVERS_PATH)/cc1101.c
SRC += $(WSN430_DRIVERS_PATH)/clock.c

INCLUDES  = -I. -I$(WSN430_DRIVERS_PATH)

# the makefile common will generate NAMES.hex and NAMES.elf files, and define 'all' and 'clean' directives
-include $(WSN430_DRIVERS_PATH)/Makefile.common


