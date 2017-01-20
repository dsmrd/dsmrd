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

#ifndef DATABASE_H
#define DATABASE_H

typedef struct database_struct_t* database_t;
typedef struct database_data_struct_t* database_data_t;

typedef enum {
	DATABASE_TYPE_HOURLY = 1,
	DATABASE_TYPE_DAILY = 2,
	DATABASE_TYPE_WEEKLY = 3,
} database_type_t;


database_t database_init(char* name);
int database_evaluate(database_t inst, database_data_t stats, time_t a, double b);
int database_select(database_t inst, database_data_t data);
int database_exit(database_t inst);
database_data_t database_data_init(char* name, database_type_t type);
void database_data_exit(database_data_t inst);

#endif // DATABASE_H

