# Set this variable to MDS to build for the MDS board, or WSN for the WSN board
BUILD = "WSN"

# Setup paths to source code
# MDS_DRIVERS_PATH should point to the mds drivers folder

WSN430 = ../../..
FREERTOS = $(WSN430)/OS/FreeRTOS


SOURCE_PATH = $(FREERTOS)/Source
PORT_PATH = $(FREERTOS)/Source/portable/GCC/MSP430F449


NAME_RX = test_rx
NAME_TX = test_tx

CC=msp430-gcc
OBJCOPY=msp430-objcopy
DEBUG=-g
OPT=-Os

INCLUDES = -I. -I$(SOURCE_PATH)/include -I$(FREERTOS)/lib/phy -I$(FREERTOS)/lib/mac/csma

#WARNINGS=-Wall -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wsign-compare \
#		-Waggregate-return -Wmissing-prototypes -Wmissing-declarations -Wunused

WARNINGS=-Wall -Wshadow

ifneq ($(BUILD),"MDS")
DRIVERS_PATH=$(WSN430)/drivers
INCLUDES += -I$(DRIVERS_PATH)
CFLAGS=-mmcu=msp430x1611 $(OPT) $(DEBUG) $(INCLUDES) -DGCC_MSP430 $(WARNINGS)
else
DRIVERS_PATH=$(MDS_DRIVERS_PATH)
INCLUDES += -I$(DRIVERS_PATH)
CFLAGS=-mmcu=msp430x2618 $(OPT) $(DEBUG) $(INCLUDES) -D__MDS__ -D__MAC__=0xBEEF -DCLK_FREQ=8000000 -DGCC_MSP430 $(WARNINGS)
endif

#
# Source files that can be built to THUMB mode.
#
SRC = \
$(SOURCE_PATH)/tasks.c \
$(SOURCE_PATH)/list.c \
$(SOURCE_PATH)/queue.c \
$(SOURCE_PATH)/portable/MemMang/heap_1.c \
$(PORT_PATH)/port.c \
$(DRIVERS_PATH)/uart0.c \
$(DRIVERS_PATH)/clock.c \
$(DRIVERS_PATH)/timerB.c

ifneq ($(BUILD),"MDS")
SRC += \
$(DRIVERS_PATH)/spi1.c \
$(DRIVERS_PATH)/ds2411.c

SRC_1101 =  \
$(DRIVERS_PATH)/cc1101.c \
$(FREERTOS)/lib/phy/phy_cc1101.c

SRC_2420 =  \
$(DRIVERS_PATH)/cc2420.c \
$(FREERTOS)/lib/phy/phy_cc2420.c

OBJ_1101 = $(SRC_1101:.c=.o)
OBJ_2420 = $(SRC_2420:.c=.o)
else
SRC += \
$(DRIVERS_PATH)/spi0.c

SRC_2500 = \
$(DRIVERS_PATH)/cc2500.c \
$(FREERTOS)/lib/phy/phy_cc2500.c

OBJ_2500 = $(SRC_2500:.c=.o)
endif

#
# Define all object files.
#
OBJ = $(SRC:.c=.o)

OBJ_ALL = ${OBJ} ${OBJ_1101} ${OBJ_2420} ${OBJ_2500} ${NAME_RX}.o ${NAME_TX}.o

ifneq ($(BUILD),"MDS")
all : ${NAME}_1101 ${NAME}_2420
else
all: ${NAME}_2500
endif

${NAME}_1101 : ${OBJ} ${OBJ_1101} ${NAME_RX}.o ${NAME_TX}.o
	$(CC) ${OBJ} ${OBJ_1101} ${NAME_RX}.o $(CFLAGS) -o $(NAME_RX)_1101.elf
	$(CC) ${OBJ} ${OBJ_1101} ${NAME_TX}.o $(CFLAGS) -o $(NAME_TX)_1101.elf

${NAME}_2420 : ${OBJ} ${OBJ_2420}
	$(CC) ${OBJ} ${OBJ_2420} ${NAME_RX}.o $(CFLAGS) -o $(NAME_RX)_2420.elf
	$(CC) ${OBJ} ${OBJ_2420} ${NAME_TX}.o $(CFLAGS) -o $(NAME_TX)_2420.elf

${NAME}_2500 : ${OBJ} ${OBJ_2500}
	$(CC) ${OBJ} ${OBJ_2500} ${NAME_RX}.o $(CFLAGS) -o $(NAME)_2500.elf
	$(CC) ${OBJ} ${OBJ_2420} ${NAME_TX}.o $(CFLAGS) -o $(NAME)_2420.elf

$(OBJ_ALL): %.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	rm -f ${OBJ_ALL} *.elf *.hex

