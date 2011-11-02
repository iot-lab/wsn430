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
#

ifndef WSN430_DRIVERS_PATH
$(error Please set environment variable WSN430_DRIVERS_PATH to the drivers folder)
endif
ifndef SENSTOOLS_LIB_PATH
$(error Please set environment variable SENSTOOLS_LIB_PATH to the OS free library folder containing phy and mac folders)
endif


NAMES     = mobile anchor sink idle

SRC_mobile  = mobile.c
SRC_anchor  = anchor.c
SRC_sink    = sink.c
SRC_idle    = idle.c

SRC  = main.c
SRC += $(WSN430_DRIVERS_PATH)/cc1100.c
SRC += $(WSN430_DRIVERS_PATH)/ds2411.c
SRC += $(WSN430_DRIVERS_PATH)/ds1722.c
SRC += $(WSN430_DRIVERS_PATH)/tsl2550.c
SRC += $(WSN430_DRIVERS_PATH)/m25p80.c
SRC += $(WSN430_DRIVERS_PATH)/i2c0.c
SRC += $(WSN430_DRIVERS_PATH)/clock.c
SRC += $(WSN430_DRIVERS_PATH)/uart0.c
SRC += $(WSN430_DRIVERS_PATH)/spi1.c
SRC += $(WSN430_DRIVERS_PATH)/timerB.c
SRC += $(WSN430_DRIVERS_PATH)/timerA.c
SRC += $(SENSTOOLS_LIB_PATH)/mac/csma_cc1100.c


INCLUDES  = -I$(WSN430_DRIVERS_PATH)
INCLUDES += -I$(SENSTOOLS_LIB_PATH)/mac


-include $(WSN430_DRIVERS_PATH)/Makefile.common



