/*
 * Copyright  2008-2009 INRIA/SensTools
 *
 * <dev-team@sentools.info>
 *
 * This software is a set of libraries designed to develop applications
 * for the WSN430 embedded hardware platform.
 *
 * This software is governed by the CeCILL license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 */


#include <io.h>
#include "tdma_common.h"
#include "tdma_table.h"

static uint16_t table[SLOT_COUNT];

void tdma_table_clear(void) {
	int16_t i;
	for (i = 0; i < SLOT_COUNT; i++) {
		table[i] = 0x0;
	}
}

uint8_t tdma_table_add(uint16_t node) {
	int16_t i;
	for (i = 0; i < SLOT_COUNT; i++) {
		if ((table[i] == 0x0) || (table[i] == node)) {
			table[i] = node;
			return i+1;
		}
	}
	return 0;
}

uint8_t tdma_table_del(uint16_t node) {
	int16_t i;
	for (i = 0; i < SLOT_COUNT; i++) {
		if (table[i] == node) {
			table[i] = 0x0;
			return i+1;
		}
	}
	return 0;
}

uint16_t tdma_table_pos(uint16_t node) {
	int16_t i;
	for (i = 0; i < SLOT_COUNT; i++) {
		if (table[i] == node) {
			return i+1;
		}
	}
	return 0;
}
