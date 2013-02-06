
# Setup paths to source code
# FREERTOS_PATH should point to the FreeRTOS folder
# WSN430_DRIVERS_PATH should point to the drivers folder


SOURCE_PATH = $(FREERTOS_PATH)/Source
PORT_PATH = $(FREERTOS_PATH)/Source/portable/GCC/MSP430F449

NAME = tdma_sink

CC=msp430-gcc
OBJCOPY=msp430-objcopy
DEBUG=-g
OPT=-Os

INCLUDES = -I. -I../sensor -I$(WSN430_DRIVERS_PATH) -I$(SOURCE_PATH)/include -I$(FREERTOS_PATH)/lib/tdma

WARNINGS=-Wall -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wsign-compare \
		-Waggregate-return -Wmissing-prototypes -Wmissing-declarations -Wunused

CFLAGS=-mmcu=msp430x1611 $(OPT) $(DEBUG)  $(INCLUDES) -DGCC_MSP430 $(WARNINGS) -o $(NAME).elf


#
# Source files that can be built to THUMB mode.
#
SRC = \
main.c \
$(SOURCE_PATH)/tasks.c \
$(SOURCE_PATH)/list.c \
$(SOURCE_PATH)/queue.c \
$(SOURCE_PATH)/portable/MemMang/heap_1.c \
$(PORT_PATH)/port.c \
$(WSN430_DRIVERS_PATH)/uart0.c \
$(WSN430_DRIVERS_PATH)/timerB.c \
$(WSN430_DRIVERS_PATH)/spi1.c \
$(WSN430_DRIVERS_PATH)/cc1101.c \
$(WSN430_DRIVERS_PATH)/clock.c \
$(WSN430_DRIVERS_PATH)/ds2411.c \
$(FREERTOS_PATH)/lib/tdma/tdma_coord.c \
$(FREERTOS_PATH)/lib/tdma/tdma_table.c

#
# Define all object files.
#
OBJ = $(SRC:.c=.o)

all : clean $(OBJ)
	$(CC) $(OBJ) $(CFLAGS)
	$(OBJCOPY) -O ihex $(NAME).elf $(NAME).hex

$(OBJ) : %.o : %.c makefile
	$(CC) -c $(CFLAGS) $< -o $@	
	
clean :
	rm -f ${NAME}.elf ${NAME}.hex ${OBJ}
	
