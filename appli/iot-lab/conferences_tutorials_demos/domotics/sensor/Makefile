# Setup paths to source code

WSN430 = ../../../../..
FREERTOS = $(WSN430)/OS/FreeRTOS
SOURCE_PATH = $(FREERTOS)/Source
PORT_PATH = $(FREERTOS)/Source/portable/GCC/MSP430F449

NAME = node_monitor

CC=msp430-gcc
OBJCOPY=msp430-objcopy
DEBUG=-g
OPT=-Os

INCLUDES = -I. -Inet -Isensor -Iblinker -I$(WSN430)/drivers -I$(SOURCE_PATH)/include -I$(FREERTOS)/lib/mac/starnet

WARNINGS=-Wall -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wsign-compare \
		-Waggregate-return -Wmissing-prototypes -Wmissing-declarations -Wunused

CFLAGS=-mmcu=msp430x1611 $(OPT) $(DEBUG)  $(INCLUDES) -DGCC_MSP430 $(WARNINGS) -o $(NAME).elf


#
# Source files that can be built to THUMB mode.
#
SRC = \
main.c \
$(FREERTOS)/lib/mac/starnet/starnet_node.c \
net/net.c \
sensor/sensor.c \
blinker/blinker.c \
$(SOURCE_PATH)/tasks.c \
$(SOURCE_PATH)/list.c \
$(SOURCE_PATH)/queue.c \
$(SOURCE_PATH)/portable/MemMang/heap_1.c \
$(PORT_PATH)/port.c \
$(WSN430)/drivers/cc1101.c \
$(WSN430)/drivers/spi1.c \
$(WSN430)/drivers/clock.c \
$(WSN430)/drivers/timerB.c \
$(WSN430)/drivers/ds2411.c \
$(WSN430)/drivers/ds1722.c \
$(WSN430)/drivers/tsl2550.c \
$(WSN430)/drivers/i2c0.c

#
# Define all object files.
#
OBJ = $(SRC:.c=.o)

a.out : $(OBJ) Makefile
	$(CC) $(OBJ) $(CFLAGS)

$(OBJ) : %.o : %.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	rm -f ${NAME}.elf ${OBJ}

