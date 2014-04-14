# Setup paths to source code

WSN430 = ../../../..

NAME = stop

CC=msp430-gcc
OBJCOPY=msp430-objcopy
DEBUG=-g
OPT=-Os


INCLUDES = -I. -I$(WSN430)/drivers
WARNINGS=-Wall -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wsign-compare \
		-Waggregate-return -Wmissing-prototypes -Wmissing-declarations -Wunused

CFLAGS=-mmcu=msp430x1611 $(OPT) $(DEBUG) $(INCLUDES) $(WARNINGS) -o $(NAME).elf


#
# Source files that can be built to THUMB mode.
#
SRC = 	main.c \
	$(WSN430)/drivers/clock.c \
	$(WSN430)/drivers/ds1722.c \
	$(WSN430)/drivers/ds2411.c \
	$(WSN430)/drivers/tsl2550.c \
	$(WSN430)/drivers/m25p80.c \
	$(WSN430)/drivers/i2c0.c \
	$(WSN430)/drivers/spi1.c

#
# Define all object files.
#
OBJ = $(SRC:.c=.o)

a.out : $(OBJ) Makefile
	$(CC) $(OBJ) $(CFLAGS)
	$(OBJCOPY) -O ihex $(NAME).elf $(NAME).hex

$(OBJ) : %.o : %.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	rm -f ${NAME}.elf ${NAME}.hex ${OBJ}

