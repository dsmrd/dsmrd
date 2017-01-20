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


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sqlite3.h>
#include "logging.h"
#include "database.h"


struct database_struct_t {
	sqlite3* db;
	sqlite3_stmt* stmt;
	sqlite3_stmt* select;
};

struct database_data_struct_t {
	database_type_t type;
	time_t prevtime;
	double prevvalue;
	char name[32];
};


/*
static const char* type2str(database_type_t t) {
	char* map[] = { NULL,  "HOURLY", "DAILY", "WEEKLY" };
	return map[t];
}
*/

#define DATABASE_TABLE_NAME "dsmrd"
#define DATABASE_TYPE_NAME "type"
#define DATABASE_METER_NAME "meter"
#define DATABASE_TIMESTAMP_NAME "timestamp"
#define DATABASE_VALUE_NAME "value"
#define DATABASE_TYPE_PARAM ":t"
#define DATABASE_METER_PARAM ":name"
#define DATABASE_TIMESTAMP_PARAM ":a"
#define DATABASE_VALUE_PARAM ":b"

database_t database_init(char* name) {
	database_t inst;
	char buf[256];
	int rval;

	inst = calloc(sizeof(*inst), 1);

	rval = sqlite3_open(name, &inst->db);
	if (rval != SQLITE_OK){
		error("Can't open database %s: %s", name, sqlite3_errmsg(inst->db));
		sqlite3_close(inst->db);
	}

	rval = sqlite3_exec(inst->db, "CREATE TABLE IF NOT EXISTS " DATABASE_TABLE_NAME "("
			DATABASE_TYPE_NAME " INT, " DATABASE_METER_NAME " TEXT, "
			DATABASE_TIMESTAMP_NAME " INT, " DATABASE_VALUE_NAME " DOUBLE)", NULL, NULL, NULL);
	if (rval != SQLITE_OK) {
		error("Can't prepare statement: %s", sqlite3_errmsg(inst->db));
	}

	rval = sqlite3_prepare_v2(inst->db, "INSERT INTO " DATABASE_TABLE_NAME " (" DATABASE_TYPE_NAME ", " DATABASE_METER_NAME ", " DATABASE_TIMESTAMP_NAME ", " DATABASE_VALUE_NAME ") VALUES (" DATABASE_TYPE_PARAM ", " DATABASE_METER_PARAM ", " DATABASE_TIMESTAMP_PARAM ", " DATABASE_VALUE_PARAM ");", -1, &inst->stmt, NULL);
	if (rval != SQLITE_OK) {
		error("Can't prepare statement: %s", sqlite3_errmsg(inst->db));
	}

	snprintf(buf, sizeof(buf), "SELECT strftime(\"%%w\", %s, \"unixepoch\", \"localtime\") AS day, strftime(\"%%H\", %s, \"unixepoch\", \"localtime\") AS hour, min(%s), max(%s), avg(%s) FROM %s WHERE %s=%s GROUP BY day, hour",
			DATABASE_TIMESTAMP_NAME, DATABASE_TIMESTAMP_NAME,
			DATABASE_VALUE_NAME, DATABASE_VALUE_NAME, DATABASE_VALUE_NAME,
			DATABASE_TABLE_NAME, DATABASE_TYPE_NAME, DATABASE_TYPE_PARAM);

	rval = sqlite3_prepare_v2(inst->db, buf, -1, &inst->select, NULL);
	if (rval != SQLITE_OK) {
		error("Can't prepare statement: %s", sqlite3_errmsg(inst->db));
	}

	return inst;
}

int database_update(database_t inst, int type, char* m, int i, double d) {
	int rval;

	rval = sqlite3_bind_int(inst->stmt, sqlite3_bind_parameter_index(inst->stmt, DATABASE_TYPE_PARAM), type);
	if (rval != SQLITE_OK) {
		error("Can't bind " DATABASE_TYPE_NAME ": %s", sqlite3_errmsg(inst->db));
	}

	rval = sqlite3_bind_text(inst->stmt, sqlite3_bind_parameter_index(inst->stmt, DATABASE_METER_PARAM), m, -1, NULL);
	if (rval != SQLITE_OK) {
		error("Can't bind " DATABASE_METER_NAME ": %s", sqlite3_errmsg(inst->db));
	}

	rval = sqlite3_bind_int(inst->stmt, sqlite3_bind_parameter_index(inst->stmt, DATABASE_TIMESTAMP_PARAM), i);
	if (rval != SQLITE_OK) {
		error("Can't bind " DATABASE_TIMESTAMP_NAME ": %s", sqlite3_errmsg(inst->db));
	}

	rval = sqlite3_bind_double(inst->stmt, sqlite3_bind_parameter_index(inst->stmt, DATABASE_VALUE_PARAM), d);
	if (rval != SQLITE_OK) {
		error("Can't bind " DATABASE_VALUE_NAME ": %s", sqlite3_errmsg(inst->db));
	}

	rval = SQLITE_ROW;
	while (rval == SQLITE_ROW) {
		rval = sqlite3_step(inst->stmt);
	}
	if (rval != SQLITE_DONE) {
		error("Can't step: %s", sqlite3_errmsg(inst->db));
	}

	rval = sqlite3_reset(inst->stmt);
	if (rval != SQLITE_OK) {
		error("Can't reset: %s", sqlite3_errmsg(inst->db));
	}

	return 0;
}

int database_select(database_t inst, database_data_t data) {
	int rval;

	rval = sqlite3_bind_int(inst->select, sqlite3_bind_parameter_index(inst->select, DATABASE_TYPE_PARAM), data->type);
	if (rval != SQLITE_OK) {
		error("Can't bind name: %s", sqlite3_errmsg(inst->db));
	}

	rval = SQLITE_ROW;
	while (rval == SQLITE_ROW) {
		rval = sqlite3_step(inst->select);

		if (rval == SQLITE_ROW) {
/*
			printf("row %d %02d %0.15f %0.15f %0.15f\n",
					sqlite3_column_int(inst->select, 0),
					sqlite3_column_int(inst->select, 1),
					sqlite3_column_double(inst->select, 2),
					sqlite3_column_double(inst->select, 3),
					sqlite3_column_double(inst->select, 4)
				  );
*/
		}

	}
	if (rval != SQLITE_DONE) {
		error("Can't step: %s", sqlite3_errmsg(inst->db));
	}

	rval = sqlite3_reset(inst->select);
	if (rval != SQLITE_OK) {
		error("Can't reset: %s", sqlite3_errmsg(inst->db));
	}

	return 0;
}

int database_exit(database_t inst) {
	int rval;

	rval = sqlite3_finalize(inst->stmt);
	if (rval != SQLITE_OK) {
		error("Can't finalize: %s", sqlite3_errmsg(inst->db));
	}

	rval = sqlite3_finalize(inst->select);
	if (rval != SQLITE_OK) {
		error("Can't finalize: %s", sqlite3_errmsg(inst->db));
	}

	rval = sqlite3_close(inst->db);
	if (rval != SQLITE_OK) {
		error("Can't close: %s", sqlite3_errmsg(inst->db));
	}

	free(inst);

	return rval;
}



int database_evaluate(database_t inst, database_data_t stats, time_t a, double b) {
	int rval = 0;
	time_t newtime = -1;
	struct tm* t;

	switch (stats->type) {
		case DATABASE_TYPE_HOURLY:
			t = localtime(&a);
			t->tm_sec = 0;
			t->tm_min = 0;
			newtime = mktime(t);
			break;
		case DATABASE_TYPE_DAILY:
			t = localtime(&a);
			t->tm_sec = 0;
			t->tm_min = 0;
			t->tm_hour = 0;
			newtime = mktime(t);
			break;
		case DATABASE_TYPE_WEEKLY:
			t = localtime(&a);
			t->tm_sec = 0;
			t->tm_min = 0;
			t->tm_hour = 0;
			t->tm_isdst = -1;
			a = mktime(t);
			while (t->tm_wday > 0) {
				a--;
				t = localtime(&a);
				t->tm_sec = 0;
				t->tm_min = 0;
				t->tm_hour = 0;
				t->tm_isdst = -1;
				a = mktime(t);
			}
			newtime = a;
			break;
	}

	assert(newtime != -1);
	if (stats->prevtime != 0) {
		if (stats->prevtime != newtime) {
			if (stats->prevvalue != 0) {
				char buf[256];
				t = localtime(&newtime);
				strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t);
				//printf("%s - %f\n", buf, b);
				rval = database_update(inst, stats->type, stats->name, stats->prevtime, b - stats->prevvalue);
			}
			stats->prevvalue = b;
		}
	}
	stats->prevtime = newtime;

	return rval;
}

database_data_t database_data_init(char* name, database_type_t type) {
	database_data_t inst;
	inst = (database_data_t) calloc(sizeof(*inst), 1);
	inst->type = type;
	strncpy(inst->name, name, sizeof(inst->name));
	return inst;
}

void database_data_exit(database_data_t inst) {
	free(inst);
}

