
# There are a few environment variables that should be set up:
# FREERTOS_PATH should point to the folder containing the OS
# WSN430_DRIVERS_PATH should point to the folder containing the driver files
#
#WSN430_LIBRARY_PATH = ../../../lib


SOURCE_PATH = $(FREERTOS_PATH)/Source
PORT_PATH = $(FREERTOS_PATH)/Source/portable/GCC/MSP430F449

NAME = fds_sink

CC=msp430-gcc
OBJCOPY=msp430-objcopy
DEBUG=-g
OPT=-Os

INCLUDES = -I. -Imac -I$(WSN430_DRIVERS_PATH) -I$(FREERTOS_PATH)/Source/include

WARNINGS=-Wall -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wsign-compare \
		-Waggregate-return -Wmissing-prototypes -Wmissing-declarations -Wunused

CFLAGS=-mmcu=msp430x1611 $(OPT) $(DEBUG)  $(INCLUDES) -DGCC_MSP430 $(WARNINGS) -o $(NAME).elf


#
# Source files that can be built to THUMB mode.
#
SRC = \
main.c \
mac/mac.c \
$(SOURCE_PATH)/tasks.c \
$(SOURCE_PATH)/list.c \
$(SOURCE_PATH)/queue.c \
$(SOURCE_PATH)/portable/MemMang/heap_1.c \
$(PORT_PATH)/port.c \
$(WSN430_DRIVERS_PATH)/uart0.c \
$(WSN430_DRIVERS_PATH)/spi1.c \
$(WSN430_DRIVERS_PATH)/cc1101.c \
$(WSN430_DRIVERS_PATH)/clock.c \
$(WSN430_DRIVERS_PATH)/timerB.c \
$(WSN430_DRIVERS_PATH)/ds2411.c \
$(WSN430_DRIVERS_PATH)/ds1722.c \
$(WSN430_DRIVERS_PATH)/i2c0.c \
$(WSN430_DRIVERS_PATH)/tsl2550.c

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
	
