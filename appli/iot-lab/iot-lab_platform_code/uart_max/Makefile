# Setup paths to source code

WSN430 = ../../../..
NAME = uartmax

CC=msp430-gcc
OBJCOPY=msp430-objcopy
JTAG=msp430-jtag
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
	$(WSN430)/drivers/uart0.c

#
# Define all object files.
#
OBJ = $(SRC:.c=.o)

a.out : $(OBJ) Makefile
	$(CC) $(OBJ) $(CFLAGS)
	$(OBJCOPY) -O ihex $(NAME).elf $(NAME).hex
	$(JTAG) -e $(NAME).elf

$(OBJ) : %.o : %.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	rm -f ${NAME}.elf ${NAME}.hex ${OBJ}

