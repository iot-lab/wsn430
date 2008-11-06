# makfile configuration
NAME            = m25p80-example
CPU             = msp430x1611
INCLUDES        = -I../..
CFLAGS          = -mmcu=${CPU} -O2 -Wall -g $(INCLUDES)

#switch the compiler (for the internal make rules)
CC              = msp430-gcc

SRC = ../../clock.c ../../uart0.c ../../m25p80.c main.c

OBJECTS = $(SRC:.c=.o)

#all should be the first target. it's built when make is run without args
all: ${NAME}.elf

#additional rules for files
${NAME}.elf: ${OBJECTS}
	${CC} -mmcu=${CPU} -o $@ ${OBJECTS}

download: all
	msp430-jtag -e ${NAME}.elf

clean:
	rm -f ${NAME}.elf ${OBJECTS}

#project dependencies
$(OBJECTS): %.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@	

