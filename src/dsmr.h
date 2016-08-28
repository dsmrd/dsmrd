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

#define OBIS_VERSION							0x0000000000000001ULL
#define OBIS_DATETIME_STAMP						0x0000000000000002ULL
#define OBIS_EQUIPMENT_IDENTIFIER				0x0000000000000004ULL
#define OBIS_ELECTR_TO_CLIENT_TARIFF1			0x0000000000000008ULL
#define OBIS_ELECTR_TO_CLIENT_TARIFF2			0x0000000000000010ULL
#define OBIS_ELECTR_BY_CLIENT_TARIFF1			0x0000000000000020ULL
#define OBIS_ELECTR_BY_CLIENT_TARIFF2			0x0000000000000040ULL
#define OBIS_ELECTR_TO_CLIENT_TARIFF_INDICATOR	0x0000000000000080ULL
#define OBIS_ELECTR_POWER_DELIVERED				0x0000000000000100ULL
#define OBIS_ELECTR_POWER_RECEIVED				0x0000000000000200ULL
#define OBIS_ELECTR_NOF_POWER_FAILURES			0x0000000000000400ULL
#define OBIS_ELECTR_NOF_LONG_POWER_FAILURES		0x0000000000000800ULL
#define OBIS_ELECTR_POWER_FAILURE_EVENT_LOG		0x0000000000001000ULL
#define OBIS_ELECTR_NOF_VOLTAGE_SAGE_L1			0x0000000000002000ULL
#define OBIS_ELECTR_NOF_VOLTAGE_SAGE_L2			0x0000000000004000ULL
#define OBIS_ELECTR_NOF_VOLTAGE_SAGE_L3			0x0000000000008000ULL
#define OBIS_ELECTR_NOF_VOLTAGE_SWELLS_L1		0x0000000000010000ULL
#define OBIS_ELECTR_NOF_VOLTAGE_SWELLS_L2		0x0000000000020000ULL
#define OBIS_ELECTR_NOF_VOLTAGE_SWELLS_L3		0x0000000000040000ULL
#define OBIS_ELECTR_TEXT_MESSAGE0				0x0000000000080000ULL
#define OBIS_ELECTR_INST_VOLTAGE_L1				0x0000000000100000ULL
#define OBIS_ELECTR_INST_VOLTAGE_L2				0x0000000000200000ULL
#define OBIS_ELECTR_INST_VOLTAGE_L3				0x0000000000400000ULL
#define OBIS_ELECTR_INST_CURRENT_L1				0x0000000000800000ULL
#define OBIS_ELECTR_INST_CURRENT_L2				0x0000000001000000ULL
#define OBIS_ELECTR_INST_CURRENT_L3				0x0000000002000000ULL
#define OBIS_ELECTR_INST_ACTIVE_POWER_DELV_L1	0x0000000000000000ULL
#define OBIS_ELECTR_INST_ACTIVE_POWER_DELV_L2	0x0000000004000000ULL
#define OBIS_ELECTR_INST_ACTIVE_POWER_DELV_L3	0x0000000008000000ULL
#define OBIS_ELECTR_INST_ACTIVE_POWER_RECV_L1	0x0000000010000000ULL
#define OBIS_ELECTR_INST_ACTIVE_POWER_RECV_L2	0x0000000020000000ULL
#define OBIS_ELECTR_INST_ACTIVE_POWER_RECV_L3	0x0000000040000000ULL
#define OBIS_DEVICE1_TYPE						0x0000000080000000ULL
#define OBIS_DEVICE1_EQUIPMENT_IDENTIFIER		0x0000000100000000ULL
#define OBIS_DEVICE1_LAST_5MIN_VALUE			0x0000000200000000ULL
#define OBIS_DEVICE2_TYPE						0x0000000400000000ULL
#define OBIS_DEVICE2_EQUIPMENT_IDENTIFIER		0x0000000800000000ULL
#define OBIS_DEVICE2_LAST_5MIN_VALUE			0x0000001000000000ULL
#define OBIS_DEVICE3_TYPE						0x0000002000000000ULL
#define OBIS_DEVICE3_EQUIPMENT_IDENTIFIER		0x0000004000000000ULL
#define OBIS_DEVICE3_LAST_5MIN_VALUE			0x0000008000000000ULL
#define OBIS_DEVICE4_TYPE						0x0000010000000000ULL
#define OBIS_DEVICE4_EQUIPMENT_IDENTIFIER		0x0000020000000000ULL
#define OBIS_DEVICE4_LAST_5MIN_VALUE			0x0000040000000000ULL
#define OBIS_ELECTR_TEXT_MESSAGE1				0x8000000000000000ULL

#define OBIS_VERSION_LENGTH			((2/2)+1)
#define OBIS_TARIFF_IND_LENGTH		((4/2)+1)
#define OBIS_EQUIP_ID_LENGTH		((96/2)+1)
#define OBIS_TEXT_MSG_LENGTH		((2048/2)+1)

struct struct_dsmr_t {
	unsigned long long obis;
	char   version[OBIS_VERSION_LENGTH];
	time_t datetime_stamp;
	char   equipment_identifier[OBIS_EQUIP_ID_LENGTH];
	double electr_to_client_tariff1;
	double electr_to_client_tariff2;
	double electr_by_client_tariff1;
	double electr_by_client_tariff2;
	//char   electr_tariff_indicator[OBIS_TARIFF_IND_LENGTH];
	int    electr_tariff_indicator;
	double electr_power_delivered;
	double electr_power_received;
	int    electr_nof_power_failures;
	int    electr_nof_long_power_failures;
	time_t electr_power_failure_event_log;
	int    electr_nof_voltage_sage_l1;
	int    electr_nof_voltage_sage_l2;
	int    electr_nof_voltage_sage_l3;
	int    electr_nof_voltage_swells_l1;
	int    electr_nof_voltage_swells_l2;
	int    electr_nof_voltage_swells_l3;
	char   electr_text_message[OBIS_TEXT_MSG_LENGTH];
	double electr_inst_voltage_l1;
	double electr_inst_voltage_l2;
	double electr_inst_voltage_l3;
	double electr_inst_current_l1;
	double electr_inst_current_l2;
	double electr_inst_current_l3;
	double electr_inst_active_power_delv_l1;
	double electr_inst_active_power_delv_l2;
	double electr_inst_active_power_delv_l3;
	double electr_inst_active_power_recv_l1;
	double electr_inst_active_power_recv_l2;
	double electr_inst_active_power_recv_l3;
	int    device1_type;
	char   device1_equipment_identifier[OBIS_EQUIP_ID_LENGTH];
	time_t device1_capture_time;
	double device1_last_5min_value;
	int    device2_type;
	char   device2_equipment_identifier[OBIS_EQUIP_ID_LENGTH];
	double device2_last_5min_value;
	time_t device2_capture_time;
	int    device3_type;
	char   device3_equipment_identifier[OBIS_EQUIP_ID_LENGTH];
	double device3_last_5min_value;
	time_t device3_capture_time;
	int    device4_type;
	char   device4_equipment_identifier[OBIS_EQUIP_ID_LENGTH];
	double device4_last_5min_value;
	time_t device4_capture_time;
};

typedef struct struct_dsmr_t* dsmr_t;

int dsmr_init(int (callback)(dsmr_t), dsmr_t);
int dsmr_decode(char* buf, ssize_t len);
int dsmr_print(dsmr_t dsmr);

#endif // _DSMR_H

