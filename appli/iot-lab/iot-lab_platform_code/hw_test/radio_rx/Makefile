
WSN430 = ../../../../..

NAME = radio_rx

CC=msp430-gcc
OBJCOPY=msp430-objcopy
DEBUG=-g
OPT=-Os

USE_CC2420=0

INCLUDES = -I. -I$(WSN430)/drivers

WARNINGS=-Wall -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wsign-compare \
		-Waggregate-return -Wmissing-prototypes -Wmissing-declarations -Wunused

CFLAGS=-mmcu=msp430x1611 $(OPT) $(DEBUG)  $(INCLUDES) -DGCC_MSP430 $(WARNINGS) -o $(NAME).elf


#
# Source files that can be built to THUMB mode.
#
SRC = \
main.c \
$(WSN430)/drivers/clock.c \
$(WSN430)/drivers/uart0.c \
$(WSN430)/drivers/spi1.c

ifeq ($(USE_CC2420), 1)
    SRC += $(WSN430)/drivers/cc2420.c
    CFLAGS += -DUSE_CC2420=1
else
    SRC += $(WSN430)/drivers/cc1101.c
    CFLAGS += -DUSE_CC2420=0
endif
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

