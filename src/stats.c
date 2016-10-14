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
#include "stats.h"


struct stats_struct_t {
	sqlite3* db;
	sqlite3_stmt* stmt;
	sqlite3_stmt* select;
};

struct stats_data_struct_t {
	stats_type_t type;
	time_t prevtime;
	double prevvalue;
	char name[32];
};


/*
static const char* type2str(stats_type_t t) {
	char* map[] = { NULL,  "HOURLY", "DAYLY", "WEEKLY" };
	return map[t];
}
*/

#define STATS_TABLE_NAME "dsmrd"
#define STATS_TYPE_NAME "type"
#define STATS_METER_NAME "meter"
#define STATS_TIMESTAMP_NAME "timestamp"
#define STATS_VALUE_NAME "value"
#define STATS_TYPE_PARAM ":t"
#define STATS_METER_PARAM ":name"
#define STATS_TIMESTAMP_PARAM ":a"
#define STATS_VALUE_PARAM ":b"

stats_t stats_init(char* name) {
	stats_t inst;
	char buf[256];
	int rval;

	inst = malloc(sizeof(*inst));

	rval = sqlite3_open(name, &inst->db);
	if (rval != SQLITE_OK){
		error("Can't open database %s: %s", name, sqlite3_errmsg(inst->db));
		sqlite3_close(inst->db);
	}

	rval = sqlite3_exec(inst->db, "CREATE TABLE IF NOT EXISTS " STATS_TABLE_NAME "("
			STATS_TYPE_NAME " INT, " STATS_METER_NAME " TEXT, "
			STATS_TIMESTAMP_NAME " INT, " STATS_VALUE_NAME " DOUBLE)", NULL, NULL, NULL);
	if (rval != SQLITE_OK) {
		error("Can't prepare statement: %s", sqlite3_errmsg(inst->db));
	}

	rval = sqlite3_prepare_v2(inst->db, "INSERT INTO " STATS_TABLE_NAME " (" STATS_TYPE_NAME ", " STATS_METER_NAME ", " STATS_TIMESTAMP_NAME ", " STATS_VALUE_NAME ") VALUES (" STATS_TYPE_PARAM ", " STATS_METER_PARAM ", " STATS_TIMESTAMP_PARAM ", " STATS_VALUE_PARAM ");", -1, &inst->stmt, NULL);
	if (rval != SQLITE_OK) {
		error("Can't prepare statement: %s", sqlite3_errmsg(inst->db));
	}

	snprintf(buf, sizeof(buf), "SELECT strftime(\"%%w\", %s, \"unixepoch\", \"localtime\") AS day, strftime(\"%%H\", %s, \"unixepoch\", \"localtime\") AS hour, min(%s), max(%s), avg(%s) FROM %s WHERE %s=%s GROUP BY day, hour",
			STATS_TIMESTAMP_NAME, STATS_TIMESTAMP_NAME,
			STATS_VALUE_NAME, STATS_VALUE_NAME, STATS_VALUE_NAME,
			STATS_TABLE_NAME, STATS_TYPE_NAME, STATS_TYPE_PARAM);

	rval = sqlite3_prepare_v2(inst->db, buf, -1, &inst->select, NULL);
	if (rval != SQLITE_OK) {
		error("Can't prepare statement: %s", sqlite3_errmsg(inst->db));
	}

	return inst;
}

int stats_update(stats_t inst, int type, char* m, int i, double d) {
	int rval;

	rval = sqlite3_bind_int(inst->stmt, sqlite3_bind_parameter_index(inst->stmt, STATS_TYPE_PARAM), type);
	if (rval != SQLITE_OK) {
		error("Can't bind " STATS_TYPE_NAME ": %s", sqlite3_errmsg(inst->db));
	}

	rval = sqlite3_bind_text(inst->stmt, sqlite3_bind_parameter_index(inst->stmt, STATS_METER_PARAM), m, -1, NULL);
	if (rval != SQLITE_OK) {
		error("Can't bind " STATS_METER_NAME ": %s", sqlite3_errmsg(inst->db));
	}

	rval = sqlite3_bind_int(inst->stmt, sqlite3_bind_parameter_index(inst->stmt, STATS_TIMESTAMP_PARAM), i);
	if (rval != SQLITE_OK) {
		error("Can't bind " STATS_TIMESTAMP_NAME ": %s", sqlite3_errmsg(inst->db));
	}

	rval = sqlite3_bind_double(inst->stmt, sqlite3_bind_parameter_index(inst->stmt, STATS_VALUE_PARAM), d);
	if (rval != SQLITE_OK) {
		error("Can't bind " STATS_VALUE_NAME ": %s", sqlite3_errmsg(inst->db));
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

int stats_select(stats_t inst, stats_data_t data) {
	int rval;

	rval = sqlite3_bind_int(inst->select, sqlite3_bind_parameter_index(inst->select, STATS_TYPE_PARAM), data->type);
	if (rval != SQLITE_OK) {
		error("Can't bind name: %s", sqlite3_errmsg(inst->db));
	}

	rval = SQLITE_ROW;
	while (rval == SQLITE_ROW) {
		rval = sqlite3_step(inst->select);

		if (rval == SQLITE_ROW) {
			printf("row %d %02d %0.15f %0.15f %0.15f\n",
					sqlite3_column_int(inst->select, 0),
					sqlite3_column_int(inst->select, 1),
					sqlite3_column_double(inst->select, 2),
					sqlite3_column_double(inst->select, 3),
					sqlite3_column_double(inst->select, 4)
				  );
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

int stats_exit(stats_t inst) {
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



int stats_evaluate(stats_t inst, stats_data_t stats, time_t a, double b) {
	int rval = 0;
	time_t newtime = -1;
	struct tm* t;

	switch (stats->type) {
		case STATS_TYPE_HOURLY:
			t = localtime(&a);
			t->tm_sec = 0;
			t->tm_min = 0;
			newtime = mktime(t);
			break;
		case STATS_TYPE_DAYLY:
			t = localtime(&a);
			t->tm_sec = 0;
			t->tm_min = 0;
			t->tm_hour = 0;
			newtime = mktime(t);
			break;
		case STATS_TYPE_WEEKLY:
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
				printf("%s - %f\n", buf, b);
				rval = stats_update(inst, stats->type, stats->name, stats->prevtime, b - stats->prevvalue);
			}
			stats->prevvalue = b;
		}
	}
	stats->prevtime = newtime;

	return rval;
}

stats_data_t stats_data_init(char* name, stats_type_t type) {
	stats_data_t inst;
	inst = (stats_data_t) calloc(sizeof(*inst), 1);
	inst->type = type;
	strncpy(inst->name, name, sizeof(inst->type));
	return inst;
}

void stats_data_exit(stats_data_t inst) {
	free(inst);
}

