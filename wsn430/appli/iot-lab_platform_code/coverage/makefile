# Setup paths to source code

NAME = coverage

CC=msp430-gcc
OBJCOPY=msp430-objcopy
JTAG=msp430-jtag
DEBUG=-g
OPT=-Os


INCLUDES = -I. -I$(WSN430_DRIVERS_PATH)
WARNINGS=-Wall -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wsign-compare \
		-Waggregate-return -Wmissing-prototypes -Wmissing-declarations -Wunused

CFLAGS=-mmcu=msp430x1611 $(OPT) $(DEBUG) $(INCLUDES) $(WARNINGS) -o $(NAME).elf


#
# Source files that can be built to THUMB mode.
#
SRC = 	main.c radio.c crc8.c \
	$(WSN430_DRIVERS_PATH)/clock.c \
	$(WSN430_DRIVERS_PATH)/cc1100.c \
	$(WSN430_DRIVERS_PATH)/spi1.c \
	$(WSN430_DRIVERS_PATH)/uart0.c \
	$(WSN430_DRIVERS_PATH)/timerA.c

#
# Define all object files.
#
OBJ = $(SRC:.c=.o)

a.out : $(OBJ) makefile
	$(CC) $(OBJ) $(CFLAGS)
	$(OBJCOPY) -O ihex $(NAME).elf $(NAME).hex
	#$(JTAG) -e $(NAME).elf

$(OBJ) : %.o : %.c makefile
	$(CC) -c $(CFLAGS) $< -o $@	
	
clean :
	rm -f ${NAME}.elf ${NAME}.hex ${OBJ}
	
