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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <regex.h>
#include "dsmr.h"
#include "logging.h"


static char* subnstr(char* dest, const char* string, int so, int eo, size_t n) {
    int len = eo - so;
    int min = (len < (n-1)) ? len : (n-1);
    strncpy(dest, string+so, min);
    dest[min] = '\0';
    return dest;
}

typedef enum {
	UNSET = 0,
	START = 1,
	SEP = 2,
	DATA = 3,
	EOR = 4,
	TRAIL = 5,
} dsmr_state_t;

typedef enum {
	COPY = 0,
	DROP = 1,
	TERMINATE = 2,
	END_OF_REC = 3,
} dsmr_action_t;

struct struct_dsmr_decoder_t {
	int location;
	int row;
	int startline;
	dsmr_state_t state;
	char buffer[256][4096];
	dsmr_t pkt;
	int (*callback)(dsmr_t);
	regex_t preg;
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


struct obis_map_t {
	char* reference;
	unsigned long key;
} obis_map[] = {
	{ "1-3:0.2.8",   OBIS_VERSION },
	{ "0-0:1.0.0",   OBIS_DATETIME_STAMP },
	{ "0-0:96.1.1",  OBIS_EQUIPMENT_IDENTIFIER },
	{ "1-0:1.8.1",   OBIS_ELECTR_TO_CLIENT_TARIFF1 },
	{ "1-0:1.8.2",   OBIS_ELECTR_TO_CLIENT_TARIFF2 },
	{ "1-0:2.8.1",   OBIS_ELECTR_BY_CLIENT_TARIFF1 },
	{ "1-0:2.8.2",   OBIS_ELECTR_BY_CLIENT_TARIFF2 },
    { "0-0:96.14.0", OBIS_ELECTR_TO_CLIENT_TARIFF_INDICATOR },
    { "1-0:1.7.0",   OBIS_ELECTR_POWER_DELIVERED },
    { "1-0:2.7.0",   OBIS_ELECTR_POWER_RECEIVED },
    { "0-0:96.7.21", OBIS_ELECTR_NOF_POWER_FAILURES },
	{ "0-0:96.7.9",  OBIS_ELECTR_NOF_LONG_POWER_FAILURES },
	{ "1-0:99.97.0", OBIS_ELECTR_POWER_FAILURE_EVENT_LOG },
	{ "1-0:32.32.0", OBIS_ELECTR_NOF_VOLTAGE_SAGE_L1 },
	{ "1-0:52.32.0", OBIS_ELECTR_NOF_VOLTAGE_SAGE_L2 },
	{ "1-0:72.32.0", OBIS_ELECTR_NOF_VOLTAGE_SAGE_L3 },
	{ "1-0:32.36.0", OBIS_ELECTR_NOF_VOLTAGE_SWELLS_L1 },
	{ "1-0:52.36.0", OBIS_ELECTR_NOF_VOLTAGE_SWELLS_L2 },
	{ "1-0:72.36.0", OBIS_ELECTR_NOF_VOLTAGE_SWELLS_L3 },
	{ "0-0:96.13.0", OBIS_ELECTR_TEXT_MESSAGE },
	{ "1-0:32.7.0",  OBIS_ELECTR_INST_VOLTAGE_L1 },
	{ "1-0:52.7.0",  OBIS_ELECTR_INST_VOLTAGE_L2 },
	{ "1-0:72.7.0",  OBIS_ELECTR_INST_VOLTAGE_L3 },
	{ "1-0:31.7.0",  OBIS_ELECTR_INST_CURRENT_L1 },
	{ "1-0:51.7.0",  OBIS_ELECTR_INST_CURRENT_L2 },
	{ "1-0:71.7.0",  OBIS_ELECTR_INST_CURRENT_L3 },
	{ "1-0:21.7.0",  OBIS_ELECTR_INST_ACTIVE_POWER_DELV_L1 },
	{ "1-0:41.7.0",  OBIS_ELECTR_INST_ACTIVE_POWER_DELV_L2 },
	{ "1-0:61.7.0",  OBIS_ELECTR_INST_ACTIVE_POWER_DELV_L3 },
	{ "1-0:22.7.0",  OBIS_ELECTR_INST_ACTIVE_POWER_RECV_L1 },
	{ "1-0:42.7.0",  OBIS_ELECTR_INST_ACTIVE_POWER_RECV_L2 },
	{ "1-0:62.7.0",  OBIS_ELECTR_INST_ACTIVE_POWER_RECV_L3 },
	{ "0-1:24.1.0",  OBIS_DEVICE1_TYPE },
	{ "0-1:96.1.0",  OBIS_DEVICE1_EQUIPMENT_IDENTIFIER },
	{ "0-1:24.2.1",  OBIS_DEVICE1_LAST_5MIN_VALUE },
	{ "0-2:24.1.0",  OBIS_DEVICE2_TYPE },
	{ "0-2:96.1.0",  OBIS_DEVICE2_EQUIPMENT_IDENTIFIER },
	{ "0-2:24.2.1",  OBIS_DEVICE2_LAST_5MIN_VALUE },
	{ "0-3:24.1.0",  OBIS_DEVICE3_TYPE },
	{ "0-3:96.1.0",  OBIS_DEVICE3_EQUIPMENT_IDENTIFIER },
	{ "0-3:24.2.1",  OBIS_DEVICE3_LAST_5MIN_VALUE },
	{ "0-4:24.1.0",  OBIS_DEVICE4_TYPE },
	{ "0-4:96.1.0",  OBIS_DEVICE4_EQUIPMENT_IDENTIFIER },
	{ "0-4:24.2.1",  OBIS_DEVICE4_LAST_5MIN_VALUE },
};

int obis_dec(double* out, char* in, char* unit);
int obis_decint(int* out, char* in);
int obis_hexdec(char* out, char* in);

static int dsmr_process() {
	int i;
	for (i = 1; i < dsmr_decoder.row; i++) {
		regmatch_t pmatch[15];
		int rval;
		//debug("%d: %s", i, dsmr_decoder.buffer[i]);

		rval = regexec(&dsmr_decoder.preg, dsmr_decoder.buffer[i], 15, pmatch, 0);
		if (rval != 0) {
			warning("Ill record received");
		} else {
			char obis_reference[256];
			char obis_value[256];
			int j;

			subnstr(obis_reference, dsmr_decoder.buffer[i], pmatch[1].rm_so, pmatch[1].rm_eo, sizeof(obis_reference));
			subnstr(obis_value, dsmr_decoder.buffer[i], pmatch[3].rm_so, pmatch[3].rm_eo, sizeof(obis_value));
			for (j=0; j<(sizeof(obis_map)/sizeof(struct obis_map_t)); j++) {
				if (!strcmp(obis_map[j].reference, obis_reference)) {
					break;
				}
			}

			if (j == (sizeof(obis_map)/sizeof(struct obis_map_t))) {
				error("Unknown OBIS code: '%s'", dsmr_decoder.buffer[i]);
			} else {
				switch (obis_map[j].key) {
					case OBIS_VERSION:
						rval = obis_hexdec(dsmr_decoder.pkt->version, obis_value);
						break;
					case OBIS_EQUIPMENT_IDENTIFIER:
						rval = obis_hexdec(dsmr_decoder.pkt->equipment_identifier, obis_value);
						break;
					case OBIS_ELECTR_TO_CLIENT_TARIFF1:
						rval = obis_dec(&dsmr_decoder.pkt->electr_to_client_tariff1, obis_value, "kWh");
						break;
					case OBIS_ELECTR_TO_CLIENT_TARIFF2:
						rval = obis_dec(&dsmr_decoder.pkt->electr_to_client_tariff2, obis_value, "kWh");
						break;
					case OBIS_ELECTR_BY_CLIENT_TARIFF1:
						rval = obis_dec(&dsmr_decoder.pkt->electr_by_client_tariff1, obis_value, "kWh");
						break;
					case OBIS_ELECTR_BY_CLIENT_TARIFF2:
						rval = obis_dec(&dsmr_decoder.pkt->electr_by_client_tariff2, obis_value, "kWh");
						break;
					case OBIS_ELECTR_POWER_DELIVERED:
						rval = obis_dec(&dsmr_decoder.pkt->electr_power_delivered, obis_value, "kW");
						break;
					case OBIS_ELECTR_POWER_RECEIVED:
						rval = obis_dec(&dsmr_decoder.pkt->electr_power_received, obis_value, "kW");
						break;
					case OBIS_ELECTR_INST_CURRENT_L1:
						rval = obis_dec(&dsmr_decoder.pkt->electr_inst_current_l1, obis_value, "A");
						break;
					case OBIS_ELECTR_INST_CURRENT_L2:
						rval = obis_dec(&dsmr_decoder.pkt->electr_inst_current_l2, obis_value, "A");
						break;
					case OBIS_ELECTR_INST_CURRENT_L3:
						rval = obis_dec(&dsmr_decoder.pkt->electr_inst_current_l3, obis_value, "A");
						break;
					case OBIS_ELECTR_INST_VOLTAGE_L1:
						rval = obis_dec(&dsmr_decoder.pkt->electr_inst_voltage_l1, obis_value, "V");
						break;
					case OBIS_ELECTR_INST_VOLTAGE_L2:
						rval = obis_dec(&dsmr_decoder.pkt->electr_inst_voltage_l2, obis_value, "V");
						break;
					case OBIS_ELECTR_INST_VOLTAGE_L3:
						rval = obis_dec(&dsmr_decoder.pkt->electr_inst_voltage_l3, obis_value, "V");
						break;
					case OBIS_ELECTR_INST_ACTIVE_POWER_DELV_L1:
						rval = obis_dec(&dsmr_decoder.pkt->electr_inst_active_power_delv_l1, obis_value, "kW");
						break;
					case OBIS_ELECTR_INST_ACTIVE_POWER_DELV_L2:
						rval = obis_dec(&dsmr_decoder.pkt->electr_inst_active_power_delv_l2, obis_value, "kW");
						break;
					case OBIS_ELECTR_INST_ACTIVE_POWER_DELV_L3:
						rval = obis_dec(&dsmr_decoder.pkt->electr_inst_active_power_delv_l3, obis_value, "kW");
						break;
					case OBIS_ELECTR_INST_ACTIVE_POWER_RECV_L1:
						rval = obis_dec(&dsmr_decoder.pkt->electr_inst_active_power_recv_l1, obis_value, "kW");
						break;
					case OBIS_ELECTR_INST_ACTIVE_POWER_RECV_L2:
						rval = obis_dec(&dsmr_decoder.pkt->electr_inst_active_power_recv_l2, obis_value, "kW");
						break;
					case OBIS_ELECTR_INST_ACTIVE_POWER_RECV_L3:
						rval = obis_dec(&dsmr_decoder.pkt->electr_inst_active_power_recv_l3, obis_value, "kW");
						break;
					case OBIS_ELECTR_NOF_POWER_FAILURES:
						rval = obis_decint(&dsmr_decoder.pkt->electr_nof_power_failures, obis_value);
						break;
					case OBIS_ELECTR_NOF_LONG_POWER_FAILURES:
						rval = obis_decint(&dsmr_decoder.pkt->electr_nof_long_power_failures, obis_value);
						break;
					case OBIS_ELECTR_NOF_VOLTAGE_SAGE_L1:
						rval = obis_decint(&dsmr_decoder.pkt->electr_nof_voltage_sage_l1, obis_value);
						break;
					case OBIS_ELECTR_NOF_VOLTAGE_SAGE_L2:
						rval = obis_decint(&dsmr_decoder.pkt->electr_nof_voltage_sage_l2, obis_value);
						break;
					case OBIS_ELECTR_NOF_VOLTAGE_SAGE_L3:
						rval = obis_decint(&dsmr_decoder.pkt->electr_nof_voltage_sage_l3, obis_value);
						break;
					case OBIS_ELECTR_NOF_VOLTAGE_SWELLS_L1:
						rval = obis_decint(&dsmr_decoder.pkt->electr_nof_voltage_swells_l1, obis_value);
						break;
					case OBIS_ELECTR_NOF_VOLTAGE_SWELLS_L2:
						rval = obis_decint(&dsmr_decoder.pkt->electr_nof_voltage_swells_l2, obis_value);
						break;
					case OBIS_ELECTR_NOF_VOLTAGE_SWELLS_L3:
						rval = obis_decint(&dsmr_decoder.pkt->electr_nof_voltage_swells_l3, obis_value);
						break;
					case OBIS_DEVICE1_TYPE:
						rval = obis_decint(&dsmr_decoder.pkt->device1_type, obis_value);
						break;
					case OBIS_DEVICE2_TYPE:
						rval = obis_decint(&dsmr_decoder.pkt->device2_type, obis_value);
						break;
					case OBIS_DEVICE3_TYPE:
						rval = obis_decint(&dsmr_decoder.pkt->device3_type, obis_value);
						break;
					case OBIS_DEVICE4_TYPE:
						rval = obis_decint(&dsmr_decoder.pkt->device4_type, obis_value);
						break;
					case OBIS_ELECTR_TO_CLIENT_TARIFF_INDICATOR:
						rval = obis_hexdec(dsmr_decoder.pkt->electr_tariff_indicator, obis_value);
						break;
					case OBIS_ELECTR_TEXT_MESSAGE:
						rval = obis_hexdec(dsmr_decoder.pkt->electr_text_message, obis_value);
						break;
					case OBIS_DEVICE1_EQUIPMENT_IDENTIFIER:
						rval = obis_hexdec(dsmr_decoder.pkt->device1_equipment_identifier, obis_value);
						break;
					case OBIS_DEVICE2_EQUIPMENT_IDENTIFIER:
						rval = obis_hexdec(dsmr_decoder.pkt->device2_equipment_identifier, obis_value);
						break;
					case OBIS_DEVICE3_EQUIPMENT_IDENTIFIER:
						rval = obis_hexdec(dsmr_decoder.pkt->device3_equipment_identifier, obis_value);
						break;
					case OBIS_DEVICE4_EQUIPMENT_IDENTIFIER:
						rval = obis_hexdec(dsmr_decoder.pkt->device4_equipment_identifier, obis_value);
						break;
					case OBIS_ELECTR_POWER_FAILURE_EVENT_LOG:
					case OBIS_DEVICE1_LAST_5MIN_VALUE:
					case OBIS_DEVICE2_LAST_5MIN_VALUE:
					case OBIS_DEVICE3_LAST_5MIN_VALUE:
					case OBIS_DEVICE4_LAST_5MIN_VALUE:
					case OBIS_DATETIME_STAMP:
						// Silently drop...
						break;
					default:
						error("Implementation error");
				}
				if (rval != 0) {
					error("Failed to decode");
				} else {
					dsmr_decoder.pkt->obis |= obis_map[j].key;
				}
			}
		}
	}

	dsmr_decoder.callback(dsmr_decoder.pkt);
	return 0;
}

int obis_dec(double* out, char* in, char* unit) {
	char arg1[256];
	char arg2[256];
	regex_t prg;
	regmatch_t pmtch[3];
	int rval;

	regcomp(&prg, "^\\(([0-9.]*)\\*([^)]*)\\)$", REG_EXTENDED);
	rval = regexec(&prg, in, sizeof(pmtch)/sizeof(pmtch[0]), pmtch, 0);
	if (rval != 0) {
		error("Ill var");
	} else {
		subnstr(arg2, in, pmtch[2].rm_so, pmtch[2].rm_eo, sizeof(arg2));
		if (strcmp(arg2, unit) != 0) {
			error("Ill unit");
		} else {
			subnstr(arg1, in, pmtch[1].rm_so, pmtch[1].rm_eo, sizeof(arg1));
			sscanf(arg1, "%lf", out);
		}
	}
	regfree(&prg);

	return rval;
}

int obis_decint(int* out, char* in) {
	char arg1[256];
	regex_t prg;
	regmatch_t pmtch[2];
	int rval;

	regcomp(&prg, "^\\(([0-9.]*)\\)$", REG_EXTENDED);
	rval = regexec(&prg, in, sizeof(pmtch)/sizeof(pmtch[0]), pmtch, 0);
	if (rval != 0) {
		error("Ill var");
	} else {
		subnstr(arg1, in, pmtch[1].rm_so, pmtch[1].rm_eo, sizeof(arg1));
		sscanf(arg1, "%d", out);
	}
	regfree(&prg);

	return rval;
}

int obis_hexdec(char* out, char* in) {
	char arg1[256];
	regex_t prg;
	regmatch_t pmtch[2];
	int rval;

	regcomp(&prg, "^\\(([^)]*)\\)$", REG_EXTENDED);
	rval = regexec(&prg, in, sizeof(pmtch)/sizeof(pmtch[0]), pmtch, 0);
	if (rval != 0) {
		error("Ill var");
	} else {
		subnstr(arg1, in, pmtch[1].rm_so, pmtch[1].rm_eo, sizeof(arg1));
		hexdec(out, arg1);
		out[strlen(arg1)/2] = '\0';
	}
	regfree(&prg);

	return rval;
}

int dsmr_init(int (callback)(dsmr_t), dsmr_t pkt) {
	int j;
	dsmr_decoder.startline = 1;
	dsmr_decoder.callback = callback;
	dsmr_decoder.pkt = pkt;
	j = regcomp(&dsmr_decoder.preg, "^([0-9]*-([0-9])*:[0-9\\.]*)(.*)$", REG_EXTENDED);
	if (j != 0) {
		error("Ncompile");
	}

	//regfree(&dsmr_decoder.preg);
	return 0;
}

int dsmr_decode(char* buf, ssize_t len) {
	int i = 0;
	dsmr_action_t action;
	for (i = i; i < len; i++) {
		action = COPY;
		switch (buf[i]) {
			case '/':
				if (dsmr_decoder.startline) {
					dsmr_decoder.state = START;
				}
				break;
			case '\n':
				switch (dsmr_decoder.state) {
					case UNSET:
					case START:
						dsmr_decoder.state = SEP;
						action = TERMINATE;
						break;
					case SEP:
						dsmr_decoder.state = DATA;
						action = DROP;
						break;
					case DATA:
						action = TERMINATE;
						break;
					case EOR:
						dsmr_decoder.state = TRAIL;
						action = END_OF_REC;
						break;
					default:
						printf("\nUnexpected CR in state %d\n", dsmr_decoder.state);
						action = DROP;
				}
				break;
			case '\r':
				action = DROP;
				break;
			case '!':
				switch (dsmr_decoder.state) {
					case DATA:
						if (dsmr_decoder.startline) {
							dsmr_decoder.state = EOR;
						}
						break;
					default:
						printf("\nUnexpected '!'\n");
				}
				break;
			default:
				switch (dsmr_decoder.state) {
					case START:
					case DATA:
						break;
					case UNSET:
					case EOR:
						action = DROP;
						break;
					default:
						printf("\nUnexpected character '%c'\n", buf[i]);
				}
				break;
		}
		if ((buf[i] == '\r') || (buf[i] == '\n')) {
			dsmr_decoder.startline = 1;
		} else {
			dsmr_decoder.startline = 0;
		}
		switch (action) {
			case COPY:
				dsmr_decoder.buffer[dsmr_decoder.row][dsmr_decoder.location] = buf[i];
				dsmr_decoder.location++;
				break;
			case TERMINATE:
				dsmr_decoder.buffer[dsmr_decoder.row][dsmr_decoder.location] = '\0';
				dsmr_decoder.row++;
				dsmr_decoder.location = 0;
				break;
			case END_OF_REC:
				dsmr_decoder.buffer[dsmr_decoder.row][dsmr_decoder.location] = '\0';
				dsmr_process();
dsmr_print(dsmr_decoder.pkt);
				dsmr_decoder.location = 0;
				dsmr_decoder.row = 0;
				break;
			case DROP:
				break;
		}
	}
	return 0;
}

int dsmr_print(dsmr_t dsmr) {
	printf("%32s: 0x%09lx\n", "bitmask", dsmr->obis);
	printf("%32s: '%s'\n", "version", dsmr->version);
	//time_t datetime_stamp;
	printf("%32s: '%s'\n", "equipment_identifier", dsmr->equipment_identifier);
	printf("%32s: %09.3f [kWh]\n", "electr_to_client_tariff1", dsmr->electr_to_client_tariff1);
	printf("%32s: %09.3f [kWh]\n", "electr_to_client_tariff2", dsmr->electr_to_client_tariff2);
	printf("%32s: %09.3f [kWh]\n", "electr_by_client_tariff1", dsmr->electr_by_client_tariff1);
	printf("%32s: %09.3f [kWh]\n", "electr_by_client_tariff2", dsmr->electr_by_client_tariff2);
	printf("%32s: '%s'\n", "electr_tariff_indicator", dsmr->electr_tariff_indicator);
	printf("%32s: %05.3f [kW]\n", "electr_power_delivered", dsmr->electr_power_delivered);
	printf("%32s: %05.3f [kW]\n", "electr_power_received", dsmr->electr_power_received);
	printf("%32s: %d\n", "electr_nof_power_failures", dsmr->electr_nof_power_failures);
	printf("%32s: %d\n", "electr_nof_long_power_failures", dsmr->electr_nof_long_power_failures);
	//time_t electr_power_failure_event_log;
	printf("%32s: %d\n", "electr_nof_voltage_sage_l1", dsmr->electr_nof_voltage_sage_l1);
	printf("%32s: %d\n", "electr_nof_voltage_sage_l2", dsmr->electr_nof_voltage_sage_l2);
	printf("%32s: %d\n", "electr_nof_voltage_sage_l3", dsmr->electr_nof_voltage_sage_l3);
	printf("%32s: %d\n", "electr_nof_voltage_swells_l1", dsmr->electr_nof_voltage_swells_l1);
	printf("%32s: %d\n", "electr_nof_voltage_swells_l2", dsmr->electr_nof_voltage_swells_l2);
	printf("%32s: %d\n", "electr_nof_voltage_swells_l3", dsmr->electr_nof_voltage_swells_l3);
	printf("%32s: '%s'\n", "electr_text_message", dsmr->electr_text_message);
	printf("%32s: %04.1f [V]\n", "electr_inst_voltage_l1", dsmr->electr_inst_voltage_l1);
	printf("%32s: %04.1f [V]\n", "electr_inst_voltage_l2", dsmr->electr_inst_voltage_l2);
	printf("%32s: %04.1f [V]\n", "electr_inst_voltage_l3", dsmr->electr_inst_voltage_l3);
	printf("%32s: %03.0f [A]\n", "electr_inst_current_l1", dsmr->electr_inst_current_l1);
	printf("%32s: %03.0f [A]\n", "electr_inst_current_l2", dsmr->electr_inst_current_l2);
	printf("%32s: %03.0f [A]\n", "electr_inst_current_l3", dsmr->electr_inst_current_l3);
	printf("%32s: %05.3f [kW]\n", "electr_inst_active_power_delv_l1", dsmr->electr_inst_active_power_delv_l1);
	printf("%32s: %05.3f [kW]\n", "electr_inst_active_power_delv_l2", dsmr->electr_inst_active_power_delv_l2);
	printf("%32s: %05.3f [kW]\n", "electr_inst_active_power_delv_l3", dsmr->electr_inst_active_power_delv_l3);
	printf("%32s: %05.3f [kW]\n", "electr_inst_active_power_recv_l1", dsmr->electr_inst_active_power_recv_l1);
	printf("%32s: %05.3f [kW]\n", "electr_inst_active_power_recv_l2", dsmr->electr_inst_active_power_recv_l2);
	printf("%32s: %05.3f [kW]\n", "electr_inst_active_power_recv_l3", dsmr->electr_inst_active_power_recv_l3);
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
