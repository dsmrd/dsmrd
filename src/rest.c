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


int http_get_file(handler_t handler, http_server_vars_t server, void* data) {
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

			(void) snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\nContent-Length: %lu\r\n\r\n", content_length);
			sz = write(handler_get_fd(handler), buf, strlen(buf));
			if (sz < 0) {
				error("Cannot write header");
			}

			while (sz > 0) {
				sz = read(fd, buf, sizeof(buf));
				if (sz < 0) {
					error("Read error");
				} else {
					sz = write(handler_get_fd(handler), buf, sz);
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

static int http_get_obis(handler_t handler, http_server_vars_t method, void* data, dsmr_t dsmr) {
	static char buf[1024] = "x";
	obis_object_t object;

	object = rbtree_get(dsmr->objects, data);
	if (object == NULL) {
		snprintf(buf, sizeof(buf), "N/A '%s'", (char*)data);
	} else {
		switch (object->type) {
			case DOUBLE:
				(void) snprintf(buf, sizeof(buf), "%.3f", object->v.f.d);
				break;
			case INTEGER:
				(void) snprintf(buf, sizeof(buf), "%i", object->v.i);
				break;
			case TIME:
				(void) snprintf(buf, sizeof(buf), "%s", ctime(&(object->v.t)));
				buf[strlen(buf)] = '\0';
				break;
			case MIN5:
				(void) snprintf(buf, sizeof(buf), "{ %f, %s }", object->v.m.d, ctime(&(object->v.m.t)));
				buf[strlen(buf)] = '\0';
				break;
			case STRING:
				(void) snprintf(buf, sizeof(buf), "%s", object->v.s);
				break;
			default:
				break;
		}
	}

	return http_write_response(handler, 200, buf);
}

void rest_init(handler_t handler) {
	handler_register_default(handler, http_get_file, NULL);
	handler_register_resource(handler, "/api/electricity/tariff1",                  "GET", http_get_obis, OBIS_ELECTR_TO_CLIENT_TARIFF1);
	handler_register_resource(handler, "/api/electricity/tariff2",                  "GET", http_get_obis, OBIS_ELECTR_TO_CLIENT_TARIFF2);
	handler_register_resource(handler, "/api/electricity/tariffs/indicator",        "GET", http_get_obis, OBIS_ELECTR_TO_CLIENT_TARIFF_INDICATOR);
	handler_register_resource(handler, "/api/electricity/tariffs/1/delivered",      "GET", http_get_obis, OBIS_ELECTR_TO_CLIENT_TARIFF1);
	handler_register_resource(handler, "/api/electricity/tariffs/2/delivered",      "GET", http_get_obis, OBIS_ELECTR_TO_CLIENT_TARIFF2);
	handler_register_resource(handler, "/api/electricity/tariffs/1/received",       "GET", http_get_obis, OBIS_ELECTR_BY_CLIENT_TARIFF1);
	handler_register_resource(handler, "/api/electricity/tariffs/2/received",       "GET", http_get_obis, OBIS_ELECTR_BY_CLIENT_TARIFF2);
	handler_register_resource(handler, "/api/electricity/phases/1/power_delivered", "GET", http_get_obis, OBIS_ELECTR_INST_ACTIVE_POWER_DELV_L1);
	handler_register_resource(handler, "/api/electricity/phases/2/power_delivered", "GET", http_get_obis, OBIS_ELECTR_INST_ACTIVE_POWER_DELV_L2);
	handler_register_resource(handler, "/api/electricity/phases/3/power_delivered", "GET", http_get_obis, OBIS_ELECTR_INST_ACTIVE_POWER_DELV_L3);
	handler_register_resource(handler, "/api/electricity/phases/1/power_received",  "GET", http_get_obis, OBIS_ELECTR_INST_ACTIVE_POWER_RECV_L1);
	handler_register_resource(handler, "/api/electricity/phases/2/power_received",  "GET", http_get_obis, OBIS_ELECTR_INST_ACTIVE_POWER_RECV_L2);
	handler_register_resource(handler, "/api/electricity/phases/3/power_received",  "GET", http_get_obis, OBIS_ELECTR_INST_ACTIVE_POWER_RECV_L3);
	handler_register_resource(handler, "/api/electricity/phases/1/current",         "GET", http_get_obis, OBIS_ELECTR_INST_CURRENT_L1);
	handler_register_resource(handler, "/api/electricity/phases/2/current",         "GET", http_get_obis, OBIS_ELECTR_INST_CURRENT_L2);
	handler_register_resource(handler, "/api/electricity/phases/3/current",         "GET", http_get_obis, OBIS_ELECTR_INST_CURRENT_L3);
	handler_register_resource(handler, "/api/electricity/phases/1/voltage",         "GET", http_get_obis, OBIS_ELECTR_INST_VOLTAGE_L1);
	handler_register_resource(handler, "/api/electricity/phases/2/voltage",         "GET", http_get_obis, OBIS_ELECTR_INST_VOLTAGE_L2);
	handler_register_resource(handler, "/api/electricity/phases/3/voltage",         "GET", http_get_obis, OBIS_ELECTR_INST_VOLTAGE_L3);
	handler_register_resource(handler, "/api/electricity/power/delivered",          "GET", http_get_obis, OBIS_ELECTR_POWER_DELIVERED);
	handler_register_resource(handler, "/api/electricity/power/received",           "GET", http_get_obis, OBIS_ELECTR_POWER_RECEIVED);
	handler_register_resource(handler, "/api/electricity/equipment",                "GET", http_get_obis, OBIS_EQUIPMENT_IDENTIFIER);
	handler_register_resource(handler, "/api/gas/equipment",                        "GET", http_get_obis, OBIS_DEVICE1_EQUIPMENT_IDENTIFIER);
	handler_register_resource(handler, "/api/gas/delivered",                        "GET", http_get_obis, OBIS_DEVICE1_LAST_5MIN_VALUE);
    handler_register_resource(handler, "/api/version", "GET", http_get_obis, OBIS_VERSION);
    handler_register_resource(handler, "/api/datetimestamp", "GET", http_get_obis, OBIS_DATETIME_STAMP);
    handler_register_resource(handler, "/api/nof_power_failures", "GET", http_get_obis, OBIS_ELECTR_NOF_POWER_FAILURES);
    handler_register_resource(handler, "/api/nof_long_power_failures", "GET", http_get_obis, OBIS_ELECTR_NOF_LONG_POWER_FAILURES);
    handler_register_resource(handler, "/api/power_fail_event_log", "GET", http_get_obis, OBIS_ELECTR_POWER_FAILURE_EVENT_LOG);
    handler_register_resource(handler, "/api/nof_voltage_sage_l1", "GET", http_get_obis, OBIS_ELECTR_NOF_VOLTAGE_SAGE_L1);
    handler_register_resource(handler, "/api/nof_voltage_sage_l2", "GET", http_get_obis, OBIS_ELECTR_NOF_VOLTAGE_SAGE_L2);
    handler_register_resource(handler, "/api/nof_voltage_sage_l3", "GET", http_get_obis, OBIS_ELECTR_NOF_VOLTAGE_SAGE_L3);
    handler_register_resource(handler, "/api/nof_voltage_swells_l1", "GET", http_get_obis, OBIS_ELECTR_NOF_VOLTAGE_SWELLS_L1);
    handler_register_resource(handler, "/api/nof_voltage_swells_l2", "GET", http_get_obis, OBIS_ELECTR_NOF_VOLTAGE_SWELLS_L2);
    handler_register_resource(handler, "/api/nof_voltage_swells_l3", "GET", http_get_obis, OBIS_ELECTR_NOF_VOLTAGE_SWELLS_L3);
    handler_register_resource(handler, "/api/text_message0", "GET", http_get_obis, OBIS_ELECTR_TEXT_MESSAGE0);
    handler_register_resource(handler, "/api/text_message1", "GET", http_get_obis, OBIS_ELECTR_TEXT_MESSAGE1);
}

//#define OBIS_DEVICE1_TYPE                       "0-1:24.1.0"
//#define OBIS_DEVICE1_EQUIPMENT_IDENTIFIER       "0-1:96.1.0"
//#define OBIS_DEVICE1_LAST_5MIN_VALUE            "0-1:24.2.1"
