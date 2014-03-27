WSN430 = ../../..

NAMES  = idle


# common sources
SRC  = main.c

DEBUG  =

CFLAGS += -ffunction-sections
CFLAGS += -Wl,--gc-sections,--undefined=_reset_vector_,--undefined=InterruptVectors,--undefined=_copy_data_init__,--undefined=_clear_bss_init__,--undefined=_end_of_init_


INCLUDES  = -I. -I$(WSN430)/drivers/


-include $(WSN430)/drivers/Makefile.common

