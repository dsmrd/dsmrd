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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED
#define __USE_XOPEN
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <regex.h>
#include <time.h>
#include "dsmr.h"
#include "logging.h"
#include "options.h"
#include "util.h"
#include "crc16.h"


#define DEVICE_TYPE_GAS		0x03
#define DEVICE_TYPE_WATER	0x07
#define DEVICE_TYPE_THERMAL	0x0D

typedef enum {
	STATE_UNDEF = 0,
	STATE_START = 1,
	STATE_SEPARATOR = 2,
	STATE_DATA = 3,
	STATE_END_TRANS = 4,
	STATE_END_TRANS_CRC = 5,
	STATE_TRAILER = 6,
} dsmr_state_t;

typedef enum {
	ACTION_UNDEF = 0,
	ACTION_BUFFER_FIELD = 1,
	ACTION_NONE = 2,
	ACTION_TERMINATE_FIELD = 3,
	ACTION_PROCESS_RECORD = 4,
	ACTION_BUFFER_CRC = 5,
} dsmr_action_t;

struct struct_dsmr_decoder_t {
	int location;
	int row;
	int startline;
	dsmr_state_t state;
	char buffer[256][4096];
	char crcbuffer[256];
	dsmr_t pkt;
	int (*callback)(dsmr_t);
	regex_t preg;
	unsigned short crc;
};

static struct struct_dsmr_decoder_t dsmr_decoder;


static const char hextable[] = {
	[0 ... 255] = -1,
	['0'] = 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
	['A'] = 10, 11, 12, 13, 14, 15,
	['a'] = 10, 11, 12, 13, 14, 15
};
void hexdec(char* buf, const char *hex) {
	char ret = 0; 
	int i = 0;
	while (*hex) {
		ret = hextable[(int)(*hex++)];
		ret = (ret << 4) | hextable[(int)(*hex++)];
		buf[i++] = ret;
	}
}

/*
   static int octet_string(char* out, char* buf, int len) {
   static v[] = 
   int i;
   for (i=0; i<len; i++) {
		char byte;
byte[i*2]
out[i] = (buf[i*2] << 4) || buf[i*2+1];
	}
}
*/


typedef enum {
	OBIS_TYPE_NULL,
	OBIS_TYPE_DOUBLE,
	OBIS_TYPE_INTEGER,
	OBIS_TYPE_TIME,
	OBIS_TYPE_5MIN,
	OBIS_TYPE_STRING,
	OBIS_TYPE_LSTRING,
} obis_type_t;

struct obis_map_t {
	char* reference;
	obis_type_t type;
	char* unit;
} obis_map[] = {
	{ OBIS_VERSION, OBIS_TYPE_LSTRING, NULL },
	{ OBIS_DATETIME_STAMP, OBIS_TYPE_TIME, NULL },
	{ OBIS_EQUIPMENT_IDENTIFIER, OBIS_TYPE_STRING, NULL },
	{ OBIS_ELECTR_TO_CLIENT_TARIFF1, OBIS_TYPE_DOUBLE, "kWh" },
	{ OBIS_ELECTR_TO_CLIENT_TARIFF2, OBIS_TYPE_DOUBLE, "kWh" },
	{ OBIS_ELECTR_BY_CLIENT_TARIFF1, OBIS_TYPE_DOUBLE, "kWh" },
	{ OBIS_ELECTR_BY_CLIENT_TARIFF2, OBIS_TYPE_DOUBLE, "kWh" },
	{ OBIS_ELECTR_TO_CLIENT_TARIFF_INDICATOR, OBIS_TYPE_INTEGER, NULL },
	{ OBIS_ELECTR_POWER_DELIVERED, OBIS_TYPE_DOUBLE, "kW" },
	{ OBIS_ELECTR_POWER_RECEIVED, OBIS_TYPE_DOUBLE, "kW" },
	{ OBIS_ELECTR_NOF_POWER_FAILURES, OBIS_TYPE_INTEGER, NULL },
	{ OBIS_ELECTR_NOF_LONG_POWER_FAILURES, OBIS_TYPE_INTEGER, NULL },
	{ OBIS_ELECTR_POWER_FAILURE_EVENT_LOG, OBIS_TYPE_NULL, NULL },
	{ OBIS_ELECTR_NOF_VOLTAGE_SAGE_L1, OBIS_TYPE_INTEGER, NULL },
	{ OBIS_ELECTR_NOF_VOLTAGE_SAGE_L2, OBIS_TYPE_INTEGER, NULL },
	{ OBIS_ELECTR_NOF_VOLTAGE_SAGE_L3, OBIS_TYPE_INTEGER, NULL },
	{ OBIS_ELECTR_NOF_VOLTAGE_SWELLS_L1, OBIS_TYPE_INTEGER, NULL },
	{ OBIS_ELECTR_NOF_VOLTAGE_SWELLS_L2, OBIS_TYPE_INTEGER, NULL },
	{ OBIS_ELECTR_NOF_VOLTAGE_SWELLS_L3, OBIS_TYPE_INTEGER, NULL },
	{ OBIS_ELECTR_TEXT_MESSAGE0, OBIS_TYPE_STRING, NULL },
	{ OBIS_ELECTR_TEXT_MESSAGE1, OBIS_TYPE_STRING, NULL }, // Not in standard!!
	{ OBIS_ELECTR_INST_VOLTAGE_L1, OBIS_TYPE_DOUBLE, "V" },
	{ OBIS_ELECTR_INST_VOLTAGE_L2, OBIS_TYPE_DOUBLE, "V" },
	{ OBIS_ELECTR_INST_VOLTAGE_L3, OBIS_TYPE_DOUBLE, "V" },
	{ OBIS_ELECTR_INST_CURRENT_L1, OBIS_TYPE_DOUBLE, "A" },
	{ OBIS_ELECTR_INST_CURRENT_L2, OBIS_TYPE_DOUBLE, "A" },
	{ OBIS_ELECTR_INST_CURRENT_L3, OBIS_TYPE_DOUBLE, "A" },
	{ OBIS_ELECTR_INST_ACTIVE_POWER_DELV_L1, OBIS_TYPE_DOUBLE, "kW" },
	{ OBIS_ELECTR_INST_ACTIVE_POWER_DELV_L2, OBIS_TYPE_DOUBLE, "kW" },
	{ OBIS_ELECTR_INST_ACTIVE_POWER_DELV_L3, OBIS_TYPE_DOUBLE, "kW" },
	{ OBIS_ELECTR_INST_ACTIVE_POWER_RECV_L1, OBIS_TYPE_DOUBLE, "kW" },
	{ OBIS_ELECTR_INST_ACTIVE_POWER_RECV_L2, OBIS_TYPE_DOUBLE, "kW" },
	{ OBIS_ELECTR_INST_ACTIVE_POWER_RECV_L3, OBIS_TYPE_DOUBLE, "kW" },
	{ OBIS_DEVICE1_TYPE, OBIS_TYPE_INTEGER, NULL },
	{ OBIS_DEVICE1_EQUIPMENT_IDENTIFIER, OBIS_TYPE_STRING, NULL },
	{ OBIS_DEVICE1_LAST_5MIN_VALUE, OBIS_TYPE_5MIN, NULL },
	{ OBIS_DEVICE2_TYPE, OBIS_TYPE_INTEGER, NULL },
	{ OBIS_DEVICE2_EQUIPMENT_IDENTIFIER, OBIS_TYPE_STRING, NULL },
	{ OBIS_DEVICE2_LAST_5MIN_VALUE, OBIS_TYPE_5MIN, NULL },
	{ OBIS_DEVICE3_TYPE, OBIS_TYPE_INTEGER, NULL },
	{ OBIS_DEVICE3_EQUIPMENT_IDENTIFIER, OBIS_TYPE_STRING, NULL },
	{ OBIS_DEVICE3_LAST_5MIN_VALUE, OBIS_TYPE_5MIN, NULL },
	{ OBIS_DEVICE4_TYPE, OBIS_TYPE_INTEGER, NULL },
	{ OBIS_DEVICE4_EQUIPMENT_IDENTIFIER, OBIS_TYPE_STRING, NULL },
	{ OBIS_DEVICE4_LAST_5MIN_VALUE, OBIS_TYPE_5MIN, NULL },
};

int obis_decode_double(obis_object_t, char* in, char* unit);
int obis_decode_integer(obis_object_t, char* in);
int obis_decode_string(obis_object_t, char* in);
int obis_decode_5min(obis_object_t, char* in, char* unit);
int obis_decode_time(obis_object_t, char* in);

static int dsmr_process() {
	int i;
	int crc;

	sscanf(dsmr_decoder.crcbuffer, "%04x", &crc);
	if ((!options.ignore_crc) && (strlen(dsmr_decoder.crcbuffer) > 0) && (dsmr_decoder.crc != crc)) {
		error("Dropped record, CRC mismatch (calc:%04x != msg:%04x)", dsmr_decoder.crc, crc);
	} else {
		for (i = 0; i < dsmr_decoder.row; i++) {
			regmatch_t pmatch[15];
			int rval;
			//debug("%d: %s", i, dsmr_decoder.buffer[i]);

			rval = regexec(&dsmr_decoder.preg, dsmr_decoder.buffer[i], 15, pmatch, 0);
			if (rval != 0) {
				warning("Ill record received: '%s'", dsmr_decoder.buffer[i]);
			} else {
				char obis_reference[256];
				char obis_value[256];
				int j;

				regsubstr(obis_reference, sizeof(obis_reference), dsmr_decoder.buffer[i], pmatch, 1);
				regsubstr(obis_value, sizeof(obis_value), dsmr_decoder.buffer[i], pmatch, 3);
				for (j=0; j<(sizeof(obis_map)/sizeof(struct obis_map_t)); j++) {
					if (!strcmp(obis_map[j].reference, obis_reference)) {
						break;
					}
				}

				if (j == (sizeof(obis_map)/sizeof(struct obis_map_t))) {
					error("Unknown OBIS code: '%s'", dsmr_decoder.buffer[i]);
				} else {
					obis_object_t de;
					if ((de = rbtree_get(dsmr_decoder.pkt->objects, obis_reference)) == NULL) {
						de = (obis_object_t) calloc(sizeof(struct obis_object_struct_t), 1);
						rbtree_put(dsmr_decoder.pkt->objects, strdup(obis_reference), de);
					}

					switch (obis_map[j].type) {
						case OBIS_TYPE_DOUBLE:
							rval = obis_decode_double(de, obis_value, obis_map[j].unit);
							break;
						case OBIS_TYPE_INTEGER:
							rval = obis_decode_integer(de, obis_value);
							break;
						case OBIS_TYPE_TIME:
							rval = obis_decode_time(de, obis_value);
							break;
						case OBIS_TYPE_5MIN:
							//if (dsmr_decoder.pkt->device1_type == 3) {
							rval = obis_decode_5min(de, obis_value, obis_map[j].unit);
							//} else {
							//info("Unknown device MBUS 1");
							//rval = -1;
							//}
							break;
						case OBIS_TYPE_STRING:
							rval = obis_decode_string(de, obis_value);
							break;
						case OBIS_TYPE_LSTRING:
							strncpy(de->v.s, obis_value, sizeof(de->v.s));
							de->type = STRING;
							rval = 0;
							break;
						case OBIS_TYPE_NULL:
							break;
					}

/*
					switch (obis_map[j].key) {
						case OBIS_DEVICE1_TYPE:
							dsmr_decoder.pkt->device1_type = de->v.i;
							//info("MBUS device 1 is %d", dsmr_decoder.pkt->device1_type);
							break;
						case OBIS_DEVICE2_TYPE:
							dsmr_decoder.pkt->device2_type = de->v.i;
							break;
						case OBIS_DEVICE3_TYPE:
							dsmr_decoder.pkt->device3_type = de->v.i;
							break;
						case OBIS_DEVICE4_TYPE:
							dsmr_decoder.pkt->device4_type = de->v.i;
							break;
					}
*/
					if (rval != 0) {
						error("Failed to decode %s", obis_map[j].reference);
					}
				}
			}
		}

		dsmr_decoder.callback(dsmr_decoder.pkt);
	}

	return 0;
}

int obis_decode_time(obis_object_t de, char* in) {
	struct tm lt;
	time_t t;
	strptime(in, "(%y%m%d%H%M%SW)", &lt);
	t = mktime(&lt);
	de->type = TIME;
	de->v.t = t;
	return 0;
}

int obis_decode_double(obis_object_t de, char* in, char* unit) {
	char arg1[256];
	char arg2[256];
	regex_t prg;
	regmatch_t pmtch[3];
	int rval;
	double out;

	regcomp(&prg, "^\\(([0-9.]*)\\*([^)]*)\\)$", REG_EXTENDED);
	rval = regexec(&prg, in, sizeof(pmtch)/sizeof(pmtch[0]), pmtch, 0);
	if (rval != 0) {
		error("Ill var: '%s'", in);
	} else {
		regsubstr(arg2, sizeof(arg2), in, pmtch, 2);
		if (strcmp(arg2, unit) != 0) {
			error("Ill unit %s != %s", arg2, unit);
			rval = -1;
		} else {
			regsubstr(arg1, sizeof(arg1), in, pmtch, 1);
			sscanf(arg1, "%lf", &out);
			de->type = DOUBLE;
			de->v.f.d = out;
			strcpy(de->v.f.s, arg2);
		}
	}
	regfree(&prg);

	return rval;
}

int obis_decode_integer(obis_object_t de, char* in) {
	char arg1[256];
	regex_t prg;
	regmatch_t pmtch[2];
	int rval;
	int out;

	regcomp(&prg, "^\\(([0-9.]*)\\)$", REG_EXTENDED);
	rval = regexec(&prg, in, sizeof(pmtch)/sizeof(pmtch[0]), pmtch, 0);
	if (rval != 0) {
		error("Ill var");
	} else {
		regsubstr(arg1, sizeof(arg1), in, pmtch, 1);
		sscanf(arg1, "%d", &out);
		de->type = INTEGER;
		de->v.i = out;
	}
	regfree(&prg);

	return rval;
}

int obis_decode_string(obis_object_t de, char* in) {
	char arg1[256];
	regex_t prg;
	regmatch_t pmtch[2];
	int rval;
	char out[256];

	regcomp(&prg, "^\\(([^)]*)\\)$", REG_EXTENDED);
	rval = regexec(&prg, in, sizeof(pmtch)/sizeof(pmtch[0]), pmtch, 0);
	if (rval != 0) {
		error("Ill var");
	} else {
		regsubstr(arg1, sizeof(arg1), in, pmtch, 1);
		hexdec(out, arg1);
		out[strlen(arg1)/2] = '\0';
		de->type = STRING;
		strcpy(de->v.s, out);
	}
	regfree(&prg);

	return rval;
}

int obis_decode_5min(obis_object_t de, char* in, char* unit) {
    char arg1[256];
    char arg2[256];
    regex_t prg;
    regmatch_t pmtch[5];
    int rval;
	time_t out1;
	double out2;

    regcomp(&prg, "^\\(([0-9]*)([A-Z]?)\\)\\(([^)]*)\\*([^)]*)\\)$", REG_EXTENDED);
    rval = regexec(&prg, in, sizeof(pmtch)/sizeof(pmtch[0]), pmtch, 0);
    if (rval != 0) {
        error("Ill var");
    } else {
		struct tm lt;
		regsubstr(arg1, sizeof(arg1), in, pmtch, 1);
		regsubstr(arg2, sizeof(arg2), in, pmtch, 3);
		regsubstr(de->v.m.s, sizeof(de->v.m.s), in, pmtch, 4);
		strptime(arg1, "%y%m%d%H%M%S", &lt);
		out1 = mktime(&lt);
		sscanf(arg2, "%lf", &out2);
		de->type = MIN5;
		de->v.m.t = out1;
		de->v.m.d = out2;
	}
	regfree(&prg);

	return rval;
}

int less_then(void* a, void* b) {
	return strcmp(a, b) < 0;
}

int equals(void* a, void* b) {
	return strcmp(a, b) == 0;
}

void free_key(void* a) {
	free(a);
}

void free_value(void* a) { }

int dsmr_init(int (callback)(dsmr_t), dsmr_t pkt) {
	int j;
	dsmr_decoder.startline = 1;
	dsmr_decoder.callback = callback;
	dsmr_decoder.pkt = pkt;
	j = regcomp(&dsmr_decoder.preg, "^([0-9]*-([0-9])*:[0-9\\.]*)(.*)$", REG_EXTENDED);
	if (j != 0) {
		error("Ncompile");
	}

	dsmr_decoder.pkt->objects = rbtree_init(less_then, equals, free_key, free_value);

	//regfree(&dsmr_decoder.preg);
	return 0;
}

int dsmr_decode(char* buf, ssize_t len) {
	int i = 0;
	dsmr_action_t action;
	for (i = i; i < len; i++) {
		action = ACTION_UNDEF;
		switch (dsmr_decoder.state) {
			case STATE_UNDEF:
				switch (buf[i]) {
					case '/':
						if (dsmr_decoder.startline) {
							dsmr_decoder.crc = crc16(dsmr_decoder.crc, buf[i]);
							//printf("<STATE_START>\n");
							dsmr_decoder.state = STATE_START;
						}
						break;
					default:
						// Silently drop everything is state STATE_UNDEF
						break;
				}
				action = ACTION_NONE;
				break;
			case STATE_START:
				dsmr_decoder.crc = crc16(dsmr_decoder.crc, buf[i]);
				switch (buf[i]) {
					case '\n':
						//printf("<STATE_SEPARATOR>\n");
						dsmr_decoder.state = STATE_SEPARATOR;
						break;
					default:
						// Silently drop header
						break;
				}
				action = ACTION_NONE;
				break;
			case STATE_SEPARATOR:
				dsmr_decoder.crc = crc16(dsmr_decoder.crc, buf[i]);
				switch (buf[i]) {
					case '\n':
						//printf("<STATE_DATA>\n");
						dsmr_decoder.state = STATE_DATA;
						break;
					case '\r':
						break;
					default:
						error("Ill char in state STATE_SEPARATOR");
						break;
				}
				action = ACTION_NONE;
				break;
			case STATE_DATA:
				dsmr_decoder.crc = crc16(dsmr_decoder.crc, buf[i]);
				switch (buf[i]) {
					case '!':
						if (dsmr_decoder.startline) {
							//printf("<STATE_END_TRANS>\n");
							dsmr_decoder.state = STATE_END_TRANS;
							action = ACTION_NONE;
						} else {
							action = ACTION_BUFFER_FIELD;
						}
						break;
					case '\n':
						action = ACTION_TERMINATE_FIELD;
						break;
					case '\r':
						action = ACTION_NONE;
						break;
					default:
						action = ACTION_BUFFER_FIELD;
						break;
				}
				break;
			case STATE_END_TRANS:
				switch (buf[i]) {
					case '\n':
						//printf("<STATE_TRAILER>\n");
						dsmr_decoder.state = STATE_TRAILER;
						action = ACTION_PROCESS_RECORD;
						break;
					case '\r':
						action = ACTION_NONE;
						break;
					default:
						//printf("<STATE_END_TRANS_CRC>\n");
						dsmr_decoder.state = STATE_END_TRANS_CRC;
						action = ACTION_BUFFER_CRC;
						break;
				}
				break;
			case STATE_END_TRANS_CRC:
				switch (buf[i]) {
					case '\n':
						//printf("<STATE_TRAILER>\n");
						dsmr_decoder.state = STATE_TRAILER;
						action = ACTION_PROCESS_RECORD;
						break;
					case '\r':
						action = ACTION_NONE;
						break;
					default:
						action = ACTION_BUFFER_CRC;
						break;
				}
				break;
			case STATE_TRAILER:
				switch (buf[i]) {
					case '/':
						if (dsmr_decoder.startline) {
							dsmr_decoder.crc = crc16(dsmr_decoder.crc, buf[i]);
							//printf("<STATE_START>\n");
							dsmr_decoder.state = STATE_START;
						}
						break;
					default:
						error("Ill char in state STATE_TRAILER");
						break;
				}
				action = ACTION_NONE;
				break;
			default:
				break;
		}
		if ((buf[i] == '\r') || (buf[i] == '\n')) {
			dsmr_decoder.startline = 1;
		} else {
			dsmr_decoder.startline = 0;
		}
		switch (action) {
			case ACTION_BUFFER_FIELD:
				dsmr_decoder.buffer[dsmr_decoder.row][dsmr_decoder.location] = buf[i];
				dsmr_decoder.location++;
				break;
			case ACTION_TERMINATE_FIELD:
				dsmr_decoder.buffer[dsmr_decoder.row][dsmr_decoder.location] = '\0';
				dsmr_decoder.row++;
				dsmr_decoder.location = 0;
				break;
			case ACTION_PROCESS_RECORD:
				dsmr_decoder.crcbuffer[dsmr_decoder.location] = '\0';
				dsmr_process();
				//dsmr_print(dsmr_decoder.pkt);
				dsmr_decoder.location = 0;
				dsmr_decoder.row = 0;
				dsmr_decoder.crc = 0;
				break;
			case ACTION_BUFFER_CRC:
				dsmr_decoder.crcbuffer[dsmr_decoder.location] = buf[i];
				dsmr_decoder.location++;
				break;
			case ACTION_NONE:
				break;
			case ACTION_UNDEF:
				error("Illegal action");
				break;
		}
	}
	return 0;
}

/*
int dsmr_print(dsmr_t dsmr) {
	printf("-----------------------------\n");
	printf("%32s: 0x%09llx\n", "bitmask", dsmr->obis);
	printf("%32s: '%s'\n", "version", dsmr->version);
{
	char buf[256];
	struct tm* lt = localtime(&dsmr->datetime_stamp);
	strftime(buf, sizeof(buf), "%c", lt);
	printf("%32s: %s\n", "datetime_stamp", buf);
}
	printf("%32s: '%s'\n", "equipment_identifier", dsmr->equipment_identifier);
	printf("%32s: %010.3f [kWh]\n", "electr_to_client_tariff1", dsmr->electr_to_client_tariff1);
	printf("%32s: %010.3f [kWh]\n", "electr_to_client_tariff2", dsmr->electr_to_client_tariff2);
	printf("%32s: %010.3f [kWh]\n", "electr_by_client_tariff1", dsmr->electr_by_client_tariff1);
	printf("%32s: %010.3f [kWh]\n", "electr_by_client_tariff2", dsmr->electr_by_client_tariff2);
	//printf("%32s: '%s'\n", "electr_tariff_indicator", dsmr->electr_tariff_indicator);
	printf("%32s: %d\n", "electr_tariff_indicator", dsmr->electr_tariff_indicator);
	printf("%32s: %06.3f [kW]\n", "electr_power_delivered", dsmr->electr_power_delivered);
	printf("%32s: %06.3f [kW]\n", "electr_power_received", dsmr->electr_power_received);
	printf("%32s: %d\n", "electr_nof_power_failures", dsmr->electr_nof_power_failures);
	printf("%32s: %d\n", "electr_nof_long_power_failures", dsmr->electr_nof_long_power_failures);
{
	char buf[256];
	struct tm* lt = localtime(&dsmr->electr_power_failure_event_log);
	strftime(buf, sizeof(buf), "%c", lt);
	printf("%32s: %s\n", "electr_power_failure_event_log", buf);
}
	printf("%32s: %d\n", "electr_nof_voltage_sage_l1", dsmr->electr_nof_voltage_sage_l1);
	printf("%32s: %d\n", "electr_nof_voltage_sage_l2", dsmr->electr_nof_voltage_sage_l2);
	printf("%32s: %d\n", "electr_nof_voltage_sage_l3", dsmr->electr_nof_voltage_sage_l3);
	printf("%32s: %d\n", "electr_nof_voltage_swells_l1", dsmr->electr_nof_voltage_swells_l1);
	printf("%32s: %d\n", "electr_nof_voltage_swells_l2", dsmr->electr_nof_voltage_swells_l2);
	printf("%32s: %d\n", "electr_nof_voltage_swells_l3", dsmr->electr_nof_voltage_swells_l3);
	printf("%32s: '%s'\n", "electr_text_message", dsmr->electr_text_message);
	printf("%32s: %05.1f [V]\n", "electr_inst_voltage_l1", dsmr->electr_inst_voltage_l1);
	printf("%32s: %05.1f [V]\n", "electr_inst_voltage_l2", dsmr->electr_inst_voltage_l2);
	printf("%32s: %05.1f [V]\n", "electr_inst_voltage_l3", dsmr->electr_inst_voltage_l3);
	printf("%32s: %03.0f [A]\n", "electr_inst_current_l1", dsmr->electr_inst_current_l1);
	printf("%32s: %03.0f [A]\n", "electr_inst_current_l2", dsmr->electr_inst_current_l2);
	printf("%32s: %03.0f [A]\n", "electr_inst_current_l3", dsmr->electr_inst_current_l3);
	printf("%32s: %06.3f [kW]\n", "electr_inst_active_power_delv_l1", dsmr->electr_inst_active_power_delv_l1);
	printf("%32s: %06.3f [kW]\n", "electr_inst_active_power_delv_l2", dsmr->electr_inst_active_power_delv_l2);
	printf("%32s: %06.3f [kW]\n", "electr_inst_active_power_delv_l3", dsmr->electr_inst_active_power_delv_l3);
	printf("%32s: %06.3f [kW]\n", "electr_inst_active_power_recv_l1", dsmr->electr_inst_active_power_recv_l1);
	printf("%32s: %06.3f [kW]\n", "electr_inst_active_power_recv_l2", dsmr->electr_inst_active_power_recv_l2);
	printf("%32s: %06.3f [kW]\n", "electr_inst_active_power_recv_l3", dsmr->electr_inst_active_power_recv_l3);
	printf("%32s: %d\n", "device1_type", dsmr->device1_type);
	printf("%32s: '%s'\n", "device1_equipment_identifier", dsmr->device1_equipment_identifier);
	printf("%32s: %f\n", "device1_last_5min_value", dsmr->device1_last_5min_value);
	printf("%32s: %d\n", "device2_type", dsmr->device2_type);
	printf("%32s: '%s'\n", "device2_equipment_identifier", dsmr->device2_equipment_identifier);
	printf("%32s: %f\n", "device2_last_5min_value", dsmr->device2_last_5min_value);
	printf("%32s: %d\n", "device3_type", dsmr->device3_type);
	printf("%32s: '%s'\n", "device3_equipment_identifier", dsmr->device3_equipment_identifier);
	printf("%32s: %f\n", "device3_last_5min_value", dsmr->device3_last_5min_value);
	printf("%32s: %d\n", "device4_type", dsmr->device4_type);
	printf("%32s: '%s'\n", "device4_equipment_identifier", dsmr->device4_equipment_identifier);
	printf("%32s: %f\n", "device4_last_5min_value", dsmr->device4_last_5min_value);

	return 0;
}
*/

