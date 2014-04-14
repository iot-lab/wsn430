# Setup paths to source code

WSN430 = ../../../..
FREERTOS = $(WSN430)/OS/FreeRTOS

SOURCE_PATH = $(FREERTOS)/Source
PORT_PATH = $(FREERTOS)/Source/portable/GCC/MSP430F449

NAME = ctrlnode

ifeq ($(TARGET),)
	TARGET=13
endif

ifeq ($(TARGET),13)
	VERSION_14 = 0
else
	VERSION_14 = 1
endif

CC=msp430-gcc
OBJCOPY=msp430-objcopy
DEBUG=-g
OPT=-Os

INCLUDES = -I. -Itasks -Idrivers -I$(WSN430)/drivers -I$(SOURCE_PATH)/include

WARNINGS=-Wall -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wsign-compare \
		-Waggregate-return -Wmissing-prototypes -Wmissing-declarations -Wunused

CFLAGS=-mmcu=msp430x1611 $(OPT) $(DEBUG)  $(INCLUDES) -DGCC_MSP430 $(WARNINGS) -o $(NAME).elf


#
# Source files that can be built to THUMB mode.
#
SRC = \
main.c \
global.c \
drivers/ina209.c \
drivers/supply-control.c \
drivers/dac.c \
tasks/mainTask.c \
tasks/pollingTask.c \
$(SOURCE_PATH)/tasks.c \
$(SOURCE_PATH)/list.c \
$(SOURCE_PATH)/queue.c \
$(SOURCE_PATH)/portable/MemMang/heap_1.c \
$(PORT_PATH)/port.c \
$(WSN430)/drivers/uart1.c \
$(WSN430)/drivers/timerB.c \
$(WSN430)/drivers/spi1.c \
$(WSN430)/drivers/tsl2550.c \
$(WSN430)/drivers/ds1722.c \
$(WSN430)/drivers/i2c0.c \
$(WSN430)/drivers/clock.c

ifeq ($(VERSION_14), 0)
	SRC += tasks/radioTask13.c $(WSN430)/drivers/cc1101.c
else
	SRC += tasks/radioTask14.c $(WSN430)/drivers/cc2420.c
endif
#
# Define all object files.
#
OBJ = $(SRC:.c=.o)

all: a.out

a.out : $(OBJ) Makefile
	$(CC) $(OBJ) $(CFLAGS)
	$(OBJCOPY) -O ihex $(NAME).elf $(NAME).hex

$(OBJ) : %.o : %.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	rm -f ${NAME}.elf ${NAME}.hex ${OBJ}

