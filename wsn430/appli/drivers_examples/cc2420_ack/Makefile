WSN430 = ../../..

NAMES  = cc2420rx cc2420tx

SRC_cc2420rx = main_rx.c
SRC_cc2420tx = main_tx.c


# common sources
SRC  = $(WSN430)/drivers/cc2420.c
SRC += $(WSN430)/drivers/uart0.c
SRC += $(WSN430)/drivers/spi1.c
SRC += $(WSN430)/drivers/clock.c


INCLUDES  = -I. -I$(WSN430)/drivers/


-include $(WSN430)/drivers/Makefile.common

