/*
 * DSMRd - Dutch Smart Meter Requirements Daemon
 * Copyright (C)2016  M.J. de Wit
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _DSMR_H
#define _DSMR_H

#include <stdlib.h>
#include <time.h>
#include "rbtree.h"

typedef struct obis_object_struct_t* obis_object_t;

typedef enum { DOUBLE, INTEGER, TIME, MIN5, STRING } obis_object_type_t;

struct obis_object_struct_t {
    obis_object_type_t type;
    union {
        struct {
			double d;
			char s[5];
		} f;
		int i;
		time_t t;
		char s[1024];
		struct {
			double d;
			time_t t;
		} m;
	} v;
};


#define OBIS_VERSION                            "1-3:0.2.8"
#define OBIS_DATETIME_STAMP                     "0-0:1.0.0"
#define OBIS_EQUIPMENT_IDENTIFIER               "0-0:96.1.1"
#define OBIS_ELECTR_TO_CLIENT_TARIFF1           "1-0:1.8.1"
#define OBIS_ELECTR_TO_CLIENT_TARIFF2           "1-0:1.8.2"
#define OBIS_ELECTR_BY_CLIENT_TARIFF1           "1-0:2.8.1"
#define OBIS_ELECTR_BY_CLIENT_TARIFF2           "1-0:2.8.2"
#define OBIS_ELECTR_TO_CLIENT_TARIFF_INDICATOR  "0-0:96.14.0"
#define OBIS_ELECTR_POWER_DELIVERED             "1-0:1.7.0"
#define OBIS_ELECTR_POWER_RECEIVED              "1-0:2.7.0"
#define OBIS_ELECTR_NOF_POWER_FAILURES          "0-0:96.7.21"
#define OBIS_ELECTR_NOF_LONG_POWER_FAILURES     "0-0:96.7.9"
#define OBIS_ELECTR_POWER_FAILURE_EVENT_LOG     "1-0:99.97.0"
#define OBIS_ELECTR_NOF_VOLTAGE_SAGE_L1         "1-0:32.32.0"
#define OBIS_ELECTR_NOF_VOLTAGE_SAGE_L2         "1-0:52.32.0"
#define OBIS_ELECTR_NOF_VOLTAGE_SAGE_L3         "1-0:72.32.0"
#define OBIS_ELECTR_NOF_VOLTAGE_SWELLS_L1       "1-0:32.36.0"
#define OBIS_ELECTR_NOF_VOLTAGE_SWELLS_L2       "1-0:52.36.0"
#define OBIS_ELECTR_NOF_VOLTAGE_SWELLS_L3       "1-0:72.36.0"
#define OBIS_ELECTR_TEXT_MESSAGE0               "0-0:96.13.0"
#define OBIS_ELECTR_TEXT_MESSAGE1               "0-0:96.13.1"
#define OBIS_ELECTR_INST_VOLTAGE_L1             "1-0:32.7.0"
#define OBIS_ELECTR_INST_VOLTAGE_L2             "1-0:52.7.0"
#define OBIS_ELECTR_INST_VOLTAGE_L3             "1-0:72.7.0"
#define OBIS_ELECTR_INST_CURRENT_L1             "1-0:31.7.0"
#define OBIS_ELECTR_INST_CURRENT_L2             "1-0:51.7.0"
#define OBIS_ELECTR_INST_CURRENT_L3             "1-0:71.7.0"
#define OBIS_ELECTR_INST_ACTIVE_POWER_DELV_L1   "1-0:21.7.0"
#define OBIS_ELECTR_INST_ACTIVE_POWER_DELV_L2   "1-0:41.7.0"
#define OBIS_ELECTR_INST_ACTIVE_POWER_DELV_L3   "1-0:61.7.0"
#define OBIS_ELECTR_INST_ACTIVE_POWER_RECV_L1   "1-0:22.7.0"
#define OBIS_ELECTR_INST_ACTIVE_POWER_RECV_L2   "1-0:42.7.0"
#define OBIS_ELECTR_INST_ACTIVE_POWER_RECV_L3   "1-0:62.7.0"
#define OBIS_DEVICE1_TYPE                       "0-1:24.1.0"
#define OBIS_DEVICE1_EQUIPMENT_IDENTIFIER       "0-1:96.1.0"
#define OBIS_DEVICE1_LAST_5MIN_VALUE            "0-1:24.2.1"
#define OBIS_DEVICE2_TYPE                       "0-2:24.1.0"
#define OBIS_DEVICE2_EQUIPMENT_IDENTIFIER       "0-2:96.1.0"
#define OBIS_DEVICE2_LAST_5MIN_VALUE            "0-2:24.2.1"
#define OBIS_DEVICE3_TYPE                       "0-3:24.1.0"
#define OBIS_DEVICE3_EQUIPMENT_IDENTIFIER       "0-3:96.1.0"
#define OBIS_DEVICE3_LAST_5MIN_VALUE            "0-3:24.2.1"
#define OBIS_DEVICE4_TYPE                       "0-4:24.1.0"
#define OBIS_DEVICE4_EQUIPMENT_IDENTIFIER       "0-4:96.1.0"
#define OBIS_DEVICE4_LAST_5MIN_VALUE            "0-4:24.2.1"

#define OBIS_VERSION_LENGTH			((2/2)+1)
#define OBIS_TARIFF_IND_LENGTH		((4/2)+1)
#define OBIS_EQUIP_ID_LENGTH		((96/2)+1)
#define OBIS_TEXT_MSG_LENGTH		((2048/2)+1)

struct struct_dsmr_t {
	int    device1_type;
	int    device2_type;
	int    device3_type;
	int    device4_type;

	rbtree_t objects;
};

typedef struct struct_dsmr_t* dsmr_t;

int dsmr_init(int (callback)(dsmr_t), dsmr_t);
int dsmr_decode(char* buf, ssize_t len);
int dsmr_print(dsmr_t dsmr);

#endif // _DSMR_H

