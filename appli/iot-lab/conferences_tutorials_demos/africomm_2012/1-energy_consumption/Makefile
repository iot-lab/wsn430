WSN430 = ../../../../..

# define one name for each executable to be built
NAMES = tp

SRC  = main.c
SRC += $(WSN430)/drivers/clock.c
SRC += $(WSN430)/drivers/uart0.c
SRC += $(WSN430)/drivers/spi1.c
SRC += $(WSN430)/drivers/ds1722.c
SRC += $(WSN430)/drivers/i2c0.c
SRC += $(WSN430)/drivers/timerA.c
SRC += $(WSN430)/drivers/tsl2550.c

INCLUDES  = -I. -I$(WSN430)/drivers

# the Makefile common will generate NAMES.hex and NAMES.elf files, and define 'all' and 'clean' directives
include $(WSN430)/drivers/Makefile.common

