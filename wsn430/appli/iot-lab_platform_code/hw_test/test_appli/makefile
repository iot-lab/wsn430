NAME = hw_test

CC=msp430-gcc
OBJCOPY=msp430-objcopy
DEBUG=-g
OPT=-Os

USE_CC2420=0

INCLUDES = -I. -I$(WSN430_DRIVERS_PATH)

WARNINGS=-Wall -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wsign-compare \
		-Waggregate-return -Wmissing-prototypes -Wmissing-declarations -Wunused

CFLAGS=-mmcu=msp430x1611 $(OPT) $(DEBUG)  $(INCLUDES) -DGCC_MSP430 $(WARNINGS) -o $(NAME).elf


#
# Source files that can be built to THUMB mode.
#
SRC = \
main.c \
$(WSN430_DRIVERS_PATH)/clock.c \
$(WSN430_DRIVERS_PATH)/spi1.c \
$(WSN430_DRIVERS_PATH)/ds1722.c \
$(WSN430_DRIVERS_PATH)/ds2411.c \
$(WSN430_DRIVERS_PATH)/i2c0.c \
$(WSN430_DRIVERS_PATH)/m25p80.c \
$(WSN430_DRIVERS_PATH)/mcp73861.c \
$(WSN430_DRIVERS_PATH)/tsl2550.c \
$(WSN430_DRIVERS_PATH)/timerA.c \
$(WSN430_DRIVERS_PATH)/uart0.c

ifeq ($(USE_CC2420), 1)
    SRC += $(WSN430_DRIVERS_PATH)/cc2420.c
    CFLAGS += -DUSE_CC2420=1
else
    SRC += $(WSN430_DRIVERS_PATH)/cc1101.c
    CFLAGS += -DUSE_CC2420=0
endif

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

