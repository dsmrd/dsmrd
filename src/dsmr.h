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

struct struct_dsmr_t {
	//double afgenomen_stroom_dal;
	//double afgenomen_stroom_piek;
	//double teruggeleverde_stroom_dal;
	//double teruggeleverde_stroom_piek;
	//int    actueel_tarief;
	//double huidig_verbruik;
	//double huidige_terruglevering;
	//int    maximum_stroom_fase;
	//int    stand_van_schakelaar;

	char   version[2];
	time_t datetime_stamp;
	char*  equipment_identifier;
	double electr_to_client_tariff1;
	double electr_to_client_tariff2;
	double electr_by_client_tariff1;
	double electr_by_client_tariff2;
	char*  electr_tariff_indicator;
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
	char*  electr_text_message;
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
	char*  device1_equipment_identifier;
	double device1_last_5min_value;
	int    device2_type;
	char*  device2_equipment_identifier;
	double device2_last_5min_value;
	int    device3_type;
	char*  device3_equipment_identifier;
	double device3_last_5min_value;
	int    device4_type;
	char*  device4_equipment_identifier;
	double device4_last_5min_value;
};

typedef struct struct_dsmr_t* dsmr_t;

int dsmr_init(int (callback)(dsmr_t), dsmr_t);
int dsmr_decode(char* buf, ssize_t len);
int dsmr_print(dsmr_t dsmr);

#endif // _DSMR_H

