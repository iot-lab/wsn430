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

WSN430 = ../../..

NAMES     = mobile anchor sink idle

SRC_mobile  = mobile.c
SRC_anchor  = anchor.c
SRC_sink    = sink.c
SRC_idle    = idle.c

SRC  = main.c
SRC += $(WSN430)/drivers/cc1101.c
SRC += $(WSN430)/drivers/ds2411.c
SRC += $(WSN430)/drivers/ds1722.c
SRC += $(WSN430)/drivers/tsl2550.c
SRC += $(WSN430)/drivers/m25p80.c
SRC += $(WSN430)/drivers/i2c0.c
SRC += $(WSN430)/drivers/clock.c
SRC += $(WSN430)/drivers/uart0.c
SRC += $(WSN430)/drivers/spi1.c
SRC += $(WSN430)/drivers/timerB.c
SRC += $(WSN430)/drivers/timerA.c
SRC += $(WSN430)/lib/mac/csma_cc1101.c


INCLUDES  = -I$(WSN430)/drivers
INCLUDES += -I$(WSN430)/lib/mac


-include $(WSN430)/drivers/Makefile.common



