SENSLAB_INCLUDES = clock leds timer

NAME            = timer_sync
CC              = msp430-gcc
OBJCOPY         = msp430-objcopy
DEBUG           = -g
OPT             = -Os
WARNINGS        = -Wall -Wpointer-arith -Wbad-function-cast -Wcast-align -Wsign-compare \
                  -Waggregate-return -Wunused
CFLAGS          = $(WARNINGS) -DGCC_MSP430 -DFREQ_BASE_27MHz -mmcu=msp430x1611 $(OPT) $(DEBUG) -I. -I$(FREERTOS_PATH)/Source/include \
                  $(addprefix "-I$(SENSLAB_DRIVERS_PATH)/wsn430-",$(SENSLAB_INCLUDES))

# Setup paths to source code
SOURCE_PATH  = $(FREERTOS_PATH)/Source
PORT_PATH    = $(FREERTOS_PATH)/Source/portable/GCC/MSP430F449


SRC1 = main_timer_sync.c
OBJ1 = $(SRC1:.c=.o)

.SILENT:

.PHONY: clean download

#
# Define all object files.
#
SRC = \
$(SOURCE_PATH)/tasks.c \
$(SOURCE_PATH)/queue.c \
$(SOURCE_PATH)/list.c \
$(SOURCE_PATH)/portable/MemMang/heap_1.c \
$(PORT_PATH)/port.c \
$(SENSLAB_DRIVERS_PATH)/wsn430-timer/timer.c \
$(SENSLAB_DRIVERS_PATH)/wsn430-clock/clock.c \

OBJ = $(SRC:.c=.o)

all: ${NAME}.elf

$(NAME).elf : $(OBJ1) $(OBJ)
	echo "== Linking $@ =="
	$(CC) -o $@ $(OBJ1) $(OBJ) $(CFLAGS) 

$(OBJ)  : %.o : %.c
	echo "== Compiling $@ =="
	$(CC) -c $(CFLAGS) $< -o $@	

$(OBJ1) : %.o : %.c
	echo "== Compiling $@ =="
	$(CC) -c $(CFLAGS) $< -o $@	

clean :
	echo "== Cleaning =="
	rm -f *~ *.o *.elf
	touch makefile
	
download:
	echo "== Downloading =="
	msp430-jtag -e $(NAME).elf
