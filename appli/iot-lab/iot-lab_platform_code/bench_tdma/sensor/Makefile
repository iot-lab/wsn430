# Setup paths to source code

WSN430 = ../../../../..
FREERTOS = $(WSN430)/OS/FreeRTOS

SOURCE_PATH = $(FREERTOS)/Source
PORT_PATH = $(FREERTOS)/Source/portable/GCC/MSP430F449

NAME = tdma_sensor

CC=msp430-gcc
OBJCOPY=msp430-objcopy
DEBUG=-g
OPT=-Os

INCLUDES = -I. -I$(WSN430)/drivers -I$(SOURCE_PATH)/include -I$(FREERTOS)/lib/mac/tdma

WARNINGS=-Wall -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wsign-compare \
		-Waggregate-return -Wmissing-prototypes -Wmissing-declarations -Wunused

CFLAGS=-mmcu=msp430x1611 $(OPT) $(DEBUG)  $(INCLUDES) -DGCC_MSP430 $(WARNINGS) -o $(NAME).elf


#
# Source files that can be built to THUMB mode.
#
SRC = \
main.c sensor.c \
$(SOURCE_PATH)/tasks.c \
$(SOURCE_PATH)/list.c \
$(SOURCE_PATH)/queue.c \
$(SOURCE_PATH)/portable/MemMang/heap_1.c \
$(PORT_PATH)/port.c \
$(WSN430)/drivers/uart0.c \
$(WSN430)/drivers/timerB.c \
$(WSN430)/drivers/spi1.c \
$(WSN430)/drivers/cc1101.c \
$(WSN430)/drivers/clock.c \
$(WSN430)/drivers/ds2411.c \
$(FREERTOS)/lib/mac/tdma/tdma_node.c

#
# Define all object files.
#
OBJ = $(SRC:.c=.o)

all : clean $(OBJ)
	$(CC) $(OBJ) $(CFLAGS)
	$(OBJCOPY) -O ihex $(NAME).elf $(NAME).hex

$(OBJ) : %.o : %.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	rm -f ${NAME}.elf ${NAME}.hex ${OBJ}
