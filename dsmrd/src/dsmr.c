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
#include <time.h>
#include "dsmr.h"
#include "logging.h"
#include "options.h"


static char* subnstr(char* dest, const char* string, int so, int eo, size_t n) {
    int len = eo - so;
    int min = (len < (n-1)) ? len : (n-1);
    strncpy(dest, string+so, min);
    dest[min] = '\0';
    return dest;
}

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


static unsigned short crc16_table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};



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
	{ "0-0:96.13.0", OBIS_ELECTR_TEXT_MESSAGE0 },
	{ "0-0:96.13.1", OBIS_ELECTR_TEXT_MESSAGE1 }, // Not in standard!!
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

static unsigned short crc16(unsigned short crc, unsigned char buf) {
	return (crc >> 8) ^ crc16_table[(crc & 0xff) ^ buf];
}

int obis_dec(double* out, char* in, char* unit);
int obis_decint(int* out, char* in);
int obis_hexdec(char* out, char* in);

static int dsmr_process() {
	int i;
	int crc;

	sscanf(dsmr_decoder.crcbuffer, "%04x", &crc);
	if ((!options.ignore_crc) && (strlen(dsmr_decoder.crcbuffer) > 0) && (dsmr_decoder.crc != crc)) {
		error("Dropped record, CRC mismatch (calc:%04x != msg:%04x)", dsmr_decoder.crc, crc);
	} else {
		for (i = 1; i < dsmr_decoder.row; i++) {
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
						case OBIS_DATETIME_STAMP:
							{
								struct tm lt;
								strptime(obis_value, "(%y%m%d%H%M%SW)", &lt);
								dsmr_decoder.pkt->datetime_stamp = mktime(&lt);
							}
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
							//rval = obis_hexdec(dsmr_decoder.pkt->electr_tariff_indicator, obis_value);
							rval = obis_decint(&dsmr_decoder.pkt->electr_tariff_indicator, obis_value);
							break;
						case OBIS_ELECTR_TEXT_MESSAGE0:
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
						case OBIS_ELECTR_TEXT_MESSAGE1: // Not in standard!!
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
	}

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
		error("Ill var: '%s'", in);
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

int dsmr_print(dsmr_t dsmr) {
	printf("-----------------------------\n");
	printf("%32s: 0x%09lx\n", "bitmask", dsmr->obis);
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

