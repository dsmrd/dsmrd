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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "logging.h"
#include "dsmr.h"
#include "http.h"
#include "rest.h"
#include "options.h"


static dsmr_t dsmr;


static int http_get_file(handler_t handler, http_server_vars_t server, /*@unused@*/ void* data) {
	char index[PATH_MAX];
	char buf[1024];
	int fd;
	int rval = 0;
	long content_length;
	ssize_t sz;
	struct stat st;

	if (strcmp(server->request_uri, "/") == 0) {
		(void) snprintf(index, sizeof(index), "%s/%s", options.wwwdir, "index.html");
	} else {
		(void) snprintf(index, sizeof(index), "%s/%s", options.wwwdir, server->request_uri);
	}

	fd = open(index, O_RDONLY);
	if (fd < 0) {
		error("Open error '%s'", index);

		(void) snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n");
		sz = write(handler_get_fd(handler), buf, strlen(buf));
		if (sz < 0) {
			error("Cannot write header");
		}
	} else {
		rval = fstat(fd, &st);
		if (rval != 0) {
			error("Cannot stat file");
		} else {
			content_length = st.st_size;

			(void) snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", content_length);
			sz = write(handler_get_fd(handler), buf, strlen(buf));
			if (sz < 0) {
				error("Cannot write header");
			}

			while (sz > 0) {
				sz = read(fd, buf, sizeof(buf));
				if (sz < 0) {
					error("Read error");
				} else {
					sz = write(handler_get_fd(handler), buf, (size_t) sz);
					if (sz < 0) {
						error("Cannot write result");
					}
				}
			}
		}
	}
	(void) close(fd);

	return rval;
}

static int http_get_obis1(handler_t handler, /*@unused@*/ http_server_vars_t method, void* data) {
	static char buf[1024] = "x";
	obis_object_t object;

	object = rbtree_get(dsmr->objects, data);
	if (object == NULL) {
		(void) snprintf(buf, sizeof(buf), "N/A '%s'", (char*)data);
	} else {
		(void) snprintf(buf, sizeof(buf), "%lu", (unsigned long)(object->v.m.t));
	}

	return http_write_response(handler, 200, buf);
}

static int http_get_obis2(handler_t handler, /*@unused@*/ http_server_vars_t method, void* data) {
	static char buf[1024] = "x";
	obis_object_t object;

	object = rbtree_get(dsmr->objects, data);
	if (object == NULL) {
		(void) snprintf(buf, sizeof(buf), "N/A '%s'", (char*)data);
	} else {
		(void) snprintf(buf, sizeof(buf), "%f", object->v.m.d);
	}

	return http_write_response(handler, 200, buf);
}

static int http_get_obis(handler_t handler, /*@unused@*/ http_server_vars_t method, void* data) {
	static char buf[1024] = "x";
	obis_object_t object;

	object = rbtree_get(dsmr->objects, data);
	if (object == NULL) {
		(void) snprintf(buf, sizeof(buf), "N/A '%s'", (char*)data);
	} else {
		switch (object->type) {
			case DOUBLE:
				(void) snprintf(buf, sizeof(buf), "%.3f", object->v.f.d);
				break;
			case INTEGER:
				(void) snprintf(buf, sizeof(buf), "%i", object->v.i);
				break;
			case TIME:
				//(void) snprintf(buf, sizeof(buf), "\"%s\"", ctime(&(object->v.t)));
				//buf[strlen(buf)-2] = '\"';
				//buf[strlen(buf)-1] = '\0';
				(void) snprintf(buf, sizeof(buf), "%lu", (unsigned long)(object->v.t));
				break;
			case MIN5:
				(void) snprintf(buf, sizeof(buf), "\"%f, %s\"", object->v.m.d, ctime(&(object->v.m.t)));
				buf[strlen(buf)-2] = '\"';
				buf[strlen(buf)-1] = '\0';
				break;
			case STRING:
				(void) snprintf(buf, sizeof(buf), "\"%s\"", object->v.s);
				break;
			default:
				break;
		}
	}

	return http_write_response(handler, 200, buf);
}

static int http_get_api(handler_t handler, /*@unused@*/ http_server_vars_t method, /*@unused@*/ void* data) {
	char* buf = "[{\"version\":3}]";
	return http_write_response(handler, 200, buf);
}

static int http_get_devices(handler_t handler, /*@unused@*/ http_server_vars_t method, /*@unused@*/ void* data) {
	char* buf = "[ { \"0\": 0 }, { \"1\": 1 }, { \"2\": 2 }, { \"3\": 3 }, { \"4\": 4 } ]";
	return http_write_response(handler, 200, buf);
}

struct {
	char* resource;
	char* method;
	handler_callback_t callback;
	char* data;
} resource_table[] = {
	{ "/api",                                       "GET", http_get_api,     OBIS_VERSION },
	{ "/api/version",                               "GET", http_get_obis,    OBIS_VERSION },
	{ "/api/devices",                               "GET", http_get_devices, NULL },
	{ "/api/devices/0/timestamp",                   "GET", http_get_obis,    OBIS_DATETIME_STAMP },
	{ "/api/devices/0/tariffs/indicator",           "GET", http_get_obis,    OBIS_ELECTR_TO_CLIENT_TARIFF_INDICATOR },
	{ "/api/devices/0/tariffs/1/delivered",         "GET", http_get_obis,    OBIS_ELECTR_TO_CLIENT_TARIFF1 },
	{ "/api/devices/0/tariffs/2/delivered",         "GET", http_get_obis,    OBIS_ELECTR_TO_CLIENT_TARIFF2 },
	{ "/api/devices/0/tariffs/1/received",          "GET", http_get_obis,    OBIS_ELECTR_BY_CLIENT_TARIFF1 },
	{ "/api/devices/0/tariffs/2/received",          "GET", http_get_obis,    OBIS_ELECTR_BY_CLIENT_TARIFF2 },
	{ "/api/devices/0/phases/1/power_delivered",    "GET", http_get_obis,    OBIS_ELECTR_INST_ACTIVE_POWER_DELV_L1 },
	{ "/api/devices/0/phases/2/power_delivered",    "GET", http_get_obis,    OBIS_ELECTR_INST_ACTIVE_POWER_DELV_L2 },
	{ "/api/devices/0/phases/3/power_delivered",    "GET", http_get_obis,    OBIS_ELECTR_INST_ACTIVE_POWER_DELV_L3 },
	{ "/api/devices/0/phases/1/power_received",     "GET", http_get_obis,    OBIS_ELECTR_INST_ACTIVE_POWER_RECV_L1 },
	{ "/api/devices/0/phases/2/power_received",     "GET", http_get_obis,    OBIS_ELECTR_INST_ACTIVE_POWER_RECV_L2 },
	{ "/api/devices/0/phases/3/power_received",     "GET", http_get_obis,    OBIS_ELECTR_INST_ACTIVE_POWER_RECV_L3 },
	{ "/api/devices/0/phases/1/current",            "GET", http_get_obis,    OBIS_ELECTR_INST_CURRENT_L1 },
	{ "/api/devices/0/phases/2/current",            "GET", http_get_obis,    OBIS_ELECTR_INST_CURRENT_L2 },
	{ "/api/devices/0/phases/3/current",            "GET", http_get_obis,    OBIS_ELECTR_INST_CURRENT_L3 },
	{ "/api/devices/0/phases/1/voltage",            "GET", http_get_obis,    OBIS_ELECTR_INST_VOLTAGE_L1 },
	{ "/api/devices/0/phases/2/voltage",            "GET", http_get_obis,    OBIS_ELECTR_INST_VOLTAGE_L2 },
	{ "/api/devices/0/phases/3/voltage",            "GET", http_get_obis,    OBIS_ELECTR_INST_VOLTAGE_L3 },
	{ "/api/devices/0/power/delivered",             "GET", http_get_obis,    OBIS_ELECTR_POWER_DELIVERED },
	{ "/api/devices/0/power/received",              "GET", http_get_obis,    OBIS_ELECTR_POWER_RECEIVED },
	{ "/api/devices/0/equipment",                   "GET", http_get_obis,    OBIS_EQUIPMENT_IDENTIFIER },
	{ "/api/devices/0/nof_power_failures",          "GET", http_get_obis,    OBIS_ELECTR_NOF_POWER_FAILURES },
	{ "/api/devices/0/nof_long_power_failures",     "GET", http_get_obis,    OBIS_ELECTR_NOF_LONG_POWER_FAILURES },
	{ "/api/devices/0/power_fail_event_log",        "GET", http_get_obis,    OBIS_ELECTR_POWER_FAILURE_EVENT_LOG },
	{ "/api/devices/0/phases/1/nof_voltage_sage",   "GET", http_get_obis,    OBIS_ELECTR_NOF_VOLTAGE_SAGE_L1 },
	{ "/api/devices/0/phases/2/nof_voltage_sage",   "GET", http_get_obis,    OBIS_ELECTR_NOF_VOLTAGE_SAGE_L2 },
	{ "/api/devices/0/phases/3/nof_voltage_sage",   "GET", http_get_obis,    OBIS_ELECTR_NOF_VOLTAGE_SAGE_L3 },
	{ "/api/devices/0/phases/1/nof_voltage_swells", "GET", http_get_obis,    OBIS_ELECTR_NOF_VOLTAGE_SWELLS_L1 },
	{ "/api/devices/0/phases/2/nof_voltage_swells", "GET", http_get_obis,    OBIS_ELECTR_NOF_VOLTAGE_SWELLS_L2 },
	{ "/api/devices/0/phases/3/nof_voltage_swells", "GET", http_get_obis,    OBIS_ELECTR_NOF_VOLTAGE_SWELLS_L3 },
	{ "/api/devices/0/message/0",                   "GET", http_get_obis,    OBIS_ELECTR_TEXT_MESSAGE0 },
	{ "/api/devices/0/message/1",                   "GET", http_get_obis,    OBIS_ELECTR_TEXT_MESSAGE1 },
	{ "/api/devices/1/type",                        "GET", http_get_obis,    OBIS_DEVICE1_TYPE },
	{ "/api/devices/1/equipment",                   "GET", http_get_obis,    OBIS_DEVICE1_EQUIPMENT_IDENTIFIER },
	{ "/api/devices/1/timestamp",                   "GET", http_get_obis1,   OBIS_DEVICE1_LAST_5MIN_VALUE },
	{ "/api/devices/1/phases/0/delivered",          "GET", http_get_obis2,   OBIS_DEVICE1_LAST_5MIN_VALUE },
	{ "/api/devices/2/type",                        "GET", http_get_obis,    OBIS_DEVICE2_TYPE },
	{ "/api/devices/2/equipment",                   "GET", http_get_obis,    OBIS_DEVICE2_EQUIPMENT_IDENTIFIER },
	{ "/api/devices/2/timestamp",                   "GET", http_get_obis1,   OBIS_DEVICE2_LAST_5MIN_VALUE },
	{ "/api/devices/2/phases/0/delivered",          "GET", http_get_obis2,   OBIS_DEVICE2_LAST_5MIN_VALUE },
	{ "/api/devices/3/type",                        "GET", http_get_obis,    OBIS_DEVICE3_TYPE },
	{ "/api/devices/3/equipment",                   "GET", http_get_obis,    OBIS_DEVICE3_EQUIPMENT_IDENTIFIER },
	{ "/api/devices/3/timestamp",                   "GET", http_get_obis1,   OBIS_DEVICE3_LAST_5MIN_VALUE },
	{ "/api/devices/3/phases/0/delivered",          "GET", http_get_obis2,   OBIS_DEVICE3_LAST_5MIN_VALUE },
	{ "/api/devices/4/type",                        "GET", http_get_obis,    OBIS_DEVICE4_TYPE },
	{ "/api/devices/4/equipment",                   "GET", http_get_obis,    OBIS_DEVICE4_EQUIPMENT_IDENTIFIER },
	{ "/api/devices/4/timestamp",                   "GET", http_get_obis1,   OBIS_DEVICE4_LAST_5MIN_VALUE },
	{ "/api/devices/4/phases/0/delivered",          "GET", http_get_obis2,   OBIS_DEVICE4_LAST_5MIN_VALUE },
};

int rest_init(dsmr_t d) {
	dsmr = d;
	return 0;
}

int rest_register(handler_t handler) {
	int i;

	handler_register_default(handler, http_get_file, NULL);
	for (i=0; i<(sizeof(resource_table)/sizeof(resource_table[0])); i++) {
		handler_register_resource(handler, resource_table[i].resource, resource_table[i].method, resource_table[i].callback, resource_table[i].data);
	}

	return 0;
}

