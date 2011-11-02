NAME     = route
CPU      = msp430x1611
CC       = msp430-gcc

SRC = main.c \
      $(WSN430_DRIVERS_PATH)/cc1100.c \
      $(WSN430_DRIVERS_PATH)/ds2411.c \
      $(WSN430_DRIVERS_PATH)/clock.c \
      $(WSN430_DRIVERS_PATH)/uart0.c \
      $(WSN430_DRIVERS_PATH)/spi1.c \
      $(WSN430_DRIVERS_PATH)/timerA.c \
      $(WSN430_DRIVERS_PATH)/timerB.c \
      $(SENSTOOLS_LIB_PATH)/mac/csma_cc1100.c \
      $(SENSTOOLS_LIB_PATH)/net/route.c

OBJECTS = $(SRC:.c=.o)

INCLUDES = -I$(WSN430_DRIVERS_PATH) \
           -I$(SENSTOOLS_LIB_PATH)/mac \
           -I$(SENSTOOLS_LIB_PATH)/net

CFLAGS   = -mmcu=${CPU} -Wall ${INCLUDES} -g -O2


all: ${NAME}.elf

${NAME}.elf: ${OBJECTS} main.o
	${CC} -mmcu=${CPU} -o $@ ${OBJECTS}

clean:
	rm -f ${NAME}.elf ${OBJECTS}

#project dependencies

$(OBJECTS): %.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@
