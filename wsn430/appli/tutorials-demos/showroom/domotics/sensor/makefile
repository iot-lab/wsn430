# Setup paths to source code
SOURCE_PATH = $(FREERTOS_PATH)/Source
PORT_PATH = $(FREERTOS_PATH)/Source/portable/GCC/MSP430F449

NAME = node_monitor

CC=msp430-gcc
OBJCOPY=msp430-objcopy
DEBUG=-g
OPT=-Os

INCLUDES = -I. -Inet -Isensor -Iblinker -I$(WSN430_DRIVERS_PATH) -I$(SOURCE_PATH)/include -I$(FREERTOS_PATH)/lib/starnet/end_device

WARNINGS=-Wall -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wsign-compare \
		-Waggregate-return -Wmissing-prototypes -Wmissing-declarations -Wunused

CFLAGS=-mmcu=msp430x1611 $(OPT) $(DEBUG)  $(INCLUDES) -DGCC_MSP430 $(WARNINGS) -o $(NAME).elf


#
# Source files that can be built to THUMB mode.
#
SRC = \
main.c \
$(FREERTOS_PATH)/lib/starnet/end_device/mac.c \
net/net.c \
sensor/sensor.c \
blinker/blinker.c \
$(SOURCE_PATH)/tasks.c \
$(SOURCE_PATH)/list.c \
$(SOURCE_PATH)/queue.c \
$(SOURCE_PATH)/portable/MemMang/heap_1.c \
$(PORT_PATH)/port.c \
$(WSN430_DRIVERS_PATH)/cc1100.c \
$(WSN430_DRIVERS_PATH)/spi1.c \
$(WSN430_DRIVERS_PATH)/clock.c \
$(WSN430_DRIVERS_PATH)/timerB.c \
$(WSN430_DRIVERS_PATH)/ds2411.c \
$(WSN430_DRIVERS_PATH)/ds1722.c \
$(WSN430_DRIVERS_PATH)/tsl2550.c \
$(WSN430_DRIVERS_PATH)/i2c0.c

#
# Define all object files.
#
OBJ = $(SRC:.c=.o)

a.out : $(OBJ) makefile
	$(CC) $(OBJ) $(CFLAGS)

$(OBJ) : %.o : %.c makefile
	$(CC) -c $(CFLAGS) $< -o $@	
	
clean :
	rm -f ${NAME}.elf ${OBJ}
	
