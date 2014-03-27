WSN430 = ../../../../..

NAME     = sensor
CPU      = msp430x1611
CC       = msp430-gcc

SRC = main.c \
      $(WSN430)/drivers/cc1101.c \
      $(WSN430)/drivers/ds2411.c \
      $(WSN430)/drivers/clock.c \
      $(WSN430)/drivers/uart0.c \
      $(WSN430)/drivers/spi1.c \
      $(WSN430)/drivers/timerB.c \
      $(WSN430)/lib/mac/csma_cc1101.c

OBJ = $(SRC:.c=.o)

INCLUDES = -I$(WSN430)/drivers \
           -I$(WSN430)/drivers/../motion_capture/ \
           -I$(WSN430)/lib/mac

CFLAGS   = -mmcu=${CPU} -Wall ${INCLUDES} -g -O2


all: ${NAME}

${NAME}: ${OBJ}
	${CC} -mmcu=${CPU} -o ${NAME}.elf ${OBJ}

clean:
	rm -f ${NAME}.elf ${OBJ}

#project dependencies

$(OBJECTS): %.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@
