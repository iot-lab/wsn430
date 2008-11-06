# makfile configuration
NAME            = leds-example
OBJECTS         = main.o
CPU             = msp430x1611
INCLUDES        = -I../..
CFLAGS          = -mmcu=${CPU} -O2 -Wall -g $(INCLUDES)

#switch the compiler (for the internal make rules)
CC              = msp430-gcc

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
main.o: main.c

