NAME     = sensor
CPU      = msp430x1611
CC       = msp430-gcc

SRC = main.c \
      $(WSN430_DRIVERS_PATH)/cc1101.c \
      $(WSN430_DRIVERS_PATH)/ds2411.c \
      $(WSN430_DRIVERS_PATH)/clock.c \
      $(WSN430_DRIVERS_PATH)/uart0.c \
      $(WSN430_DRIVERS_PATH)/spi1.c \
      $(WSN430_DRIVERS_PATH)/timerB.c \
      $(WSN430_DRIVERS_PATH)/timerA.c \
      $(WSN430_DRIVERS_PATH)/i2c0.c \
      $(WSN430_DRIVERS_PATH)/../motion_capture/lis3lv02dq.c \
      $(WSN430_LIB_PATH)/mac/csma.c

OBJ = $(SRC:.c=.o)

INCLUDES = -I$(WSN430_DRIVERS_PATH) \
           -I$(WSN430_DRIVERS_PATH)/../motion_capture/ \
           -I$(WSN430_LIB_PATH)/mac

CFLAGS   = -mmcu=${CPU} -Wall ${INCLUDES} -g -O2


all: ${NAME}

${NAME}: ${OBJ}
	${CC} -mmcu=${CPU} -o ${NAME}.elf ${OBJ}

clean:
	rm -f ${NAME}.elf ${OBJ}

#project dependencies

$(OBJECTS): %.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@
