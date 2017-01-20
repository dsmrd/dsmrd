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

#ifndef STATS_H
#define STATS_H

typedef struct stats_struct_t* stats_t;
typedef struct stats_data_struct_t* stats_data_t;

typedef enum {
	STATS_TYPE_HOURLY = 1,
	STATS_TYPE_DAILY = 2,
	STATS_TYPE_WEEKLY = 3,
} stats_type_t;


stats_t stats_init(char* name);
int stats_evaluate(stats_t inst, stats_data_t stats, time_t a, double b);
int stats_select(stats_t inst, stats_data_t data);
int stats_exit(stats_t inst);
stats_data_t stats_data_init(char* name, stats_type_t type);
void stats_data_exit(stats_data_t inst);

#endif // STATS_H

