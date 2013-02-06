# Setup paths to source code

NAME = opennode_gwtest

CC=msp430-gcc
OBJCOPY=msp430-objcopy
DEBUG=-g
OPT=-Os

INCLUDES = -I. -Idrivers -I$(WSN430_DRIVERS_PATH)

WARNINGS=-Wall -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wsign-compare \
		-Waggregate-return -Wmissing-prototypes -Wmissing-declarations -Wunused

CFLAGS=-mmcu=msp430x1611 $(OPT) $(DEBUG) $(INCLUDES) $(WARNINGS) -o $(NAME).elf


#
# Source files that can be built to THUMB mode.
#
SRC = \
main.c \
$(WSN430_DRIVERS_PATH)/uart0.c \
$(WSN430_DRIVERS_PATH)/clock.c

#
# Define all object files.
#
OBJ = $(SRC:.c=.o)

a.out : $(OBJ) makefile
	$(CC) $(OBJ) $(CFLAGS)
	msp430-objcopy -O ihex $(NAME).elf $(NAME).hex

$(OBJ) : %.o : %.c makefile
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	rm -f ${NAME}.elf ${NAME}.hex ${OBJ}

