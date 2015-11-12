# Utility to program wsn430 using bsl over USB dock
# Make sure that the USB of the docks is mapped to uart1 (i.e bsl Tx/RX)
# Note: the wsn430 USB dock uses same technique as telosb (ADG715BRU over ftdi)
#
# syntax with defaults:
# make -f wsn430.makefile erase
# make -f wsn430.makefile reset
#
# Syntax with custom
# makefile -f wsn430.makefile erase PORT='/dev/ttyUSB1'
# makefile -f wsn430.makefile upload PORT='/dev/ttyUSB1' FILE='myFile.hex'
#
# author : Roudy Dagher (roudy.dagher@inria.fr)

# default USB port of the dock
PORT ?= '/dev/ttyUSB0'

include $(WSN430)/drivers/Makefile.common

# BSL tool from contiki
ifndef CONTIKI
      ${error CONTIKI not defined! You must specify where Contiki resides}
endif
BSL := $(CONTIKI)/tools/sky/msp430-bsl-linux --telosb 


upload:
	@if test "$(FILE)" = "" ; then \
 		echo "FILE not set, call with param with hex file name ex. FILE=myapp.hex"; \
		exit 1; \
	fi
	@echo ">>Programming $(FILE) to $(PORT)"
	$(BSL) -r -e -c $(PORT) -I -p $(FILE)

reset:
	@echo ">>Resetting device on $(PORT)"
	$(BSL) -r -c $(PORT)

erase:
	@echo ">>Erasing device on $(PORT)"
	$(BSL) -e -c $(PORT)

.PHONY: erase upload reset

