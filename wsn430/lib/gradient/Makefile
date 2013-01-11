# Copyright  2008-2009 INRIA/SensTools
#
# <dev-team@sentools.info>
#
# This software is a set of libraries designed to develop applications
# for the WSN430 embedded hardware platform.
#
# This software is governed by the CeCILL license under French law and
# abiding by the rules of distribution of free software.  You can  use,
# modify and/ or redistribute the software under the terms of the CeCILL
# license as circulated by CEA, CNRS and INRIA at the following URL
# "http://www.cecill.info".
#
# As a counterpart to the access to the source code and  rights to copy,
# modify and redistribute granted by the license, users are provided only
# with a limited warranty  and the software's author,  the holder of the
# economic rights,  and the successive licensors  have only  limited
# liability.
#
# In this respect, the user's attention is drawn to the risks associated
# with loading,  using,  modifying and/or developing or reproducing the
# software by the user in light of its specific status of free software,
# that may mean  that it is complicated to manipulate,  and  that  also
# therefore means  that it is reserved for developers  and  experienced
# professionals having in-depth computer knowledge. Users are therefore
# encouraged to load and test the software's suitability as regards their
# requirements in conditions enabling the security of their systems and/or
# data to be ensured and,  more generally, to use and operate it in the
# same conditions as regards security.
#
# The fact that you are presently reading this means that you have had
# knowledge of the CeCILL license and that you accept its terms.

ifndef FREERTOS_PATH
$(error Please set environment variable FREERTOS_PATH to the FreeRTOS folder)
endif
ifndef WSN430_DRIVERS_PATH
$(error Please set environment variable WSN430_DRIVERS_PATH to the drivers folder)
endif

SOURCE_PATH = $(FREERTOS_PATH)/Source
PORT_PATH = $(FREERTOS_PATH)/Source/portable/GCC/MSP430F449

NAMES       = mobile anchor sink dist

SRC_mobile  = mobile.c
SRC_anchor  = anchor.c
SRC_sink    = sink.c
SRC_dist    = dist.c


INCLUDES  = -I. -I$(WSN430_DRIVERS_PATH)
INCLUDES += -I$(SOURCE_PATH)/include
INCLUDES += -I$(FREERTOS_PATH)/lib/phy
INCLUDES += -I$(FREERTOS_PATH)/lib/mac/csma


SRC  = crc8.c
SRC += $(WSN430_DRIVERS_PATH)/cc2420.c
SRC += $(FREERTOS_PATH)/lib/mac/csma/csma.c
SRC += $(FREERTOS_PATH)/lib/phy/phy_cc2420.c

SRC += $(SOURCE_PATH)/tasks.c
SRC += $(SOURCE_PATH)/list.c
SRC += $(SOURCE_PATH)/queue.c
SRC += $(SOURCE_PATH)/portable/MemMang/heap_1.c
SRC += $(PORT_PATH)/port.c
SRC += $(WSN430_DRIVERS_PATH)/uart0.c

SRC += $(WSN430_DRIVERS_PATH)/spi1.c
SRC += $(WSN430_DRIVERS_PATH)/clock.c
SRC += $(WSN430_DRIVERS_PATH)/ds2411.c
SRC += $(WSN430_DRIVERS_PATH)/timerB.c


-include $(WSN430_DRIVERS_PATH)/Makefile.common


