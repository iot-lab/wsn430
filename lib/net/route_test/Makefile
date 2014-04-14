WSN430 = ../../..

NAME     = route
CPU      = msp430x1611
CC       = msp430-gcc

SRC = main.c \
      $(WSN430)/drivers/cc1101.c \
      $(WSN430)/drivers/ds2411.c \
      $(WSN430)/drivers/clock.c \
      $(WSN430)/drivers/uart0.c \
      $(WSN430)/drivers/spi1.c \
      $(WSN430)/drivers/timerA.c \
      $(WSN430)/drivers/timerB.c \
      $(WSN430)/lib/mac/csma_cc1101.c \
      $(WSN430)/lib/net/route.c

OBJECTS = $(SRC:.c=.o)

INCLUDES = -I$(WSN430)/drivers \
           -I$(WSN430)/lib/mac \
           -I$(WSN430)/lib/net

CFLAGS   = -mmcu=${CPU} -Wall ${INCLUDES} -g -O2


all: ${NAME}.elf

${NAME}.elf: ${OBJECTS} main.o
	${CC} -mmcu=${CPU} -o $@ ${OBJECTS}

clean:
	rm -f ${NAME}.elf ${OBJECTS}

#project dependencies

$(OBJECTS): %.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@
