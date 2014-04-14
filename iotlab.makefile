all: tests
tests: compile_tests


CONTIKI_APPS = $(shell find OS/Contiki -name Makefile \
			   -exec dirname {} \;)

# Non contiki apps, no need of a RADIO option
APPS = $(shell find . -iname Makefile \
			   -not -path './OS/Contiki/*' -and \
			   -not -path './OS/TinyOS/*' -and \
			   -not -path './drivers/Makefile' \
			   -not -path './OS/FreeRTOS/doc/*' \
			   -exec dirname {} \;)

# remove non compiling examples
APPS := $(filter-out ./appli/iot-lab/iot-lab_platform_code/bench_tdma/sink , $(APPS))
APPS := $(filter-out ./appli/iot-lab/iot-lab_platform_code/bench_tdma/sensor , $(APPS))
APPS := $(filter-out ./appli/iot-lab/conferences_tutorials_demos/showroom/motion/sensor_accel , $(APPS))


ALL_APPS = $(CONTIKI_APPS) $(APPS)
clean-ALL_APPS = $(addprefix clean-, $(ALL_APPS))

$(CONTIKI_APPS): % :
	make -s -C $* RADIO=WITH_CC1101
	make -s -C $* RADIO=WITH_CC2420


# should clean before building because objects are note created per firmware
# so previous compilations interfere with current one
$(APPS): % :
	make -s -C $* clean
	make -s -C $*

$(clean-ALL_APPS): clean-% :
	make -s -C $* clean


compile_tests: $(ALL_APPS)
clean: $(clean-ALL_APPS)


.PHONY: all tests compile_tests clean $(ALL_APPS) $(clean-ALL_APPS)
