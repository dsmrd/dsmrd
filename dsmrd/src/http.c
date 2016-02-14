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

#include <sys/time.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <regex.h>
#include <limits.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "logging.h"
#include "dsmr.h"
#include "dispatch.h"
#include "accept.h"
#include "http.h"
#include "options.h"
#include "util.h"


typedef enum {
	REQUEST,
	HEADER,
	DATA,
	DONE
} http_decoder_state_t;

typedef enum {
	COPY,
	DROP,
	PARSE_REQUEST,
	PARSE_HEADER,
	HANDLE_REQUEST
} http_decoder_action_t;

typedef struct struct_http_decoder_t* http_decoder_t;

struct struct_http_decoder_t {
	int len;
	int startline;
	http_decoder_state_t state;
	char request_method[32];
	char request_uri[256];
	int http_content_length;
	int dataread;
	char buf[4096];
	regex_t regex_request;
	regex_t regex_header;
	int (*callback)(void*, http_decoder_t);
	void* data;
};

struct struct_handler_t {
	int fd;
	struct sockaddr_in addr;
	http_decoder_t decoder;
	dsmr_t dsmr;
	dispatch_t dis;
};

int handler_read(void*);


static /*@null@*/ http_decoder_t http_decoder_init(int (callback)(void*, http_decoder_t), void* data) {
	http_decoder_t inst;
	int rval;

	inst = (http_decoder_t) calloc(sizeof(struct struct_http_decoder_t), 1);
	if (inst != 0) {
		rval = regcomp(&inst->regex_request, "^\\([^ ]*\\) \\([^ ]*\\) \\(.*\\)$", 0);
		if (rval != 0) {
			error("regcomp");
		}

		rval = regcomp(&inst->regex_header, "^\\([^:]*\\): \\(.*\\)$", 0);
		if (rval != 0) {
			error("regcomp");
		}

		inst->callback = callback;
		inst->data = data;
	}

	return inst;
}

static int http_decoder_exit(http_decoder_t inst) {
	regfree(&inst->regex_request);
	regfree(&inst->regex_header);
	free(inst);
	return 0;
}

static int http_decoder_read(http_decoder_t inst, char* buf, ssize_t len) {
	ssize_t i;
	for (i = 0; i < len; i++) {
		http_decoder_action_t action = COPY;
		switch (inst->state) {
			case REQUEST:
				switch (buf[i]) {
					case '\n':
						inst->state = HEADER;
						action = PARSE_REQUEST;
						break;
					case '\r':
						action = DROP;
						break;
					default:
						break;
				}
				break;
			case HEADER:
				switch (buf[i]) {
					case '\n':
						if (inst->startline != 0) {
							if (inst->http_content_length != 0) {
								inst->state = DATA;
								action = DROP;
							} else {
								inst->state = REQUEST;
								action = HANDLE_REQUEST;
							}
						} else {
							action = PARSE_HEADER;
						}
						break;
					case '\r':
						action = DROP;
						break;
					default:
						break;
				}
				break;
			case DATA:
				inst->dataread++;
				if (inst->dataread == inst->http_content_length) {
					debug("data received %d bytes", inst->dataread);
					inst->state = REQUEST;
					action = HANDLE_REQUEST;
				}
				break;
			case DONE:
				error("Err data received");
				break;
		}

		if ((buf[i] == '\n')) {
			inst->startline = 1;
		} else {
			if (buf[i] != '\r') {
				inst->startline = 0;
			}
		}

		switch (action) {
			case COPY:
				inst->buf[inst->len++] = buf[i];
				break;
			case DROP:
				break;
			case PARSE_REQUEST:
				inst->buf[inst->len] = '\0';
				inst->len = 0;

				{
					int j;
					regmatch_t pmatch[5];

					j = regexec(&inst->regex_request, inst->buf, 5, pmatch, 0);
					if (j != 0) {
						error("regexec");
					} else {
						(void) regsubstr(inst->request_method, sizeof(inst->request_method), inst->buf, pmatch, 1);
						(void) regsubstr(inst->request_uri, sizeof(inst->request_uri), inst->buf, pmatch, 2);

						debug("METHOD='%s'", inst->request_method);
						debug("RESOURCE='%s'", inst->request_uri);
					}
				}
				break;
			case PARSE_HEADER:
				inst->buf[inst->len] = '\0';
				inst->len = 0;
				{
					int j;
					regmatch_t pmatch[5];

					j = regexec(&inst->regex_header, inst->buf, 5, pmatch, 0);
					if (j != 0) {
						error("regexec");
					} else {
						char tag[256];
						char val[256];
						(void) regsubstr(tag, sizeof(tag), inst->buf, pmatch, 1);
						(void) regsubstr(val, sizeof(val), inst->buf, pmatch, 2);

						if (strcmp(tag, "Content-Length") == 0) {
							inst->http_content_length = atoi(val);
						}
					}
				}
				break;
			case HANDLE_REQUEST:
				debug("REQUEST FINISHED");
				(void) inst->callback(inst->data, inst);
				inst->len = 0;
				break;
		}
	}

	return 0;
}

static int http_response_index(handler_t inst, /*@unused@*/ char* method, /*@unused@*/ char* uri) {
	char index[PATH_MAX];
	char buf[32*2014];
	char b2[32*2014];
	int fd;
	ssize_t rval;

	(void) snprintf(index, sizeof(index), "%s/%s", options.wwwdir, "index.html");
	fd = open(index, O_RDONLY);
	if (fd < 0) {
		error("Open error '%s'", index);
	}
	rval = read(fd, b2, sizeof(b2));
	if (rval < 0) {
		error("Read error");
	}
	(void) close(fd);

	(void) snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\nContent-Length: %lu\r\n\r\n%s", strlen(b2), b2);

	rval = write(inst->fd, buf, strlen(buf));
	if (rval < 0) {
		error("Cannot write result");
	}

	return (int) rval;
}

static char* http_response_tariff1(handler_t inst, char* method, ...) {
	va_list ap;
	static char buf[16*2014];
	double t;
	char* arg0;

	va_start(ap, method);

	arg0 = va_arg(ap, char*);

	if (strcmp("/api/electricity/tariff1", arg0) == 0) {
		t = inst->dsmr->electr_to_client_tariff1;
	} else {
		t = inst->dsmr->electr_to_client_tariff2;
	}
	(void) snprintf(buf, sizeof(buf), "{ \"value\": %.3f, \"unit\": \"%s\" }", t, "kWh");

	va_end(ap);

	return buf;
}

static char* http_response_tariff_received(handler_t inst, char* method, ...) {
	va_list ap;
	static char buf[256];
	double t;
	char* arg0;

	va_start(ap, method);

	arg0 = va_arg(ap, char*);
	//int arg1 = atoi(va_arg(ap, char*));

	if (strcmp("/api/electricity/tariffs/1/received", arg0) == 0) {
		//if (arg1 == 1) {
		t = inst->dsmr->electr_by_client_tariff1;
	} else {
		t = inst->dsmr->electr_by_client_tariff2;
	}
	(void) snprintf(buf, sizeof(buf), "%.3f", t);

	va_end(ap);

	return buf;
}

static char* http_response_tariff_delivered(handler_t inst, char* method, ...) {
	va_list ap;
	static char buf[256];
	double t;
	char* arg0;

	va_start(ap, method);

	arg0 = va_arg(ap, char*);
	//int arg1 = atoi(va_arg(ap, char*));

	if (strcmp("/api/electricity/tariffs/1/delivered", arg0) == 0) {
		//if (arg1 == 1) {
		t = inst->dsmr->electr_to_client_tariff1;
	} else {
		t = inst->dsmr->electr_to_client_tariff2;
	}
	(void) snprintf(buf, sizeof(buf), "%.3f", t);

	va_end(ap);

	return buf;
}

static char* http_response_total_power_delivered(handler_t inst, char* method, ...) {
	va_list ap;
	static char buf[256];
	double t;

	va_start(ap, method);

	t = inst->dsmr->electr_power_delivered;
	(void) snprintf(buf, sizeof(buf), "%.3f", t);

	va_end(ap);

	return buf;
}

static char* http_response_total_power_received(handler_t inst, char* method, ...) {
	va_list ap;
	static char buf[256];
	double t;

	va_start(ap, method);

	t = inst->dsmr->electr_power_received;
	(void) snprintf(buf, sizeof(buf), "%.3f", t);

	va_end(ap);

	return buf;
}

static char* http_response_phase_power_received(handler_t inst, char* method, ...) {
	va_list ap;
	static char buf[256];
	double t;
	int arg1;

	va_start(ap, method);

	(void) va_arg(ap, char*);
	arg1 = atoi(va_arg(ap, char*));

	if (arg1 == 1) {
		t = inst->dsmr->electr_inst_active_power_recv_l1;
	} else if (arg1 == 2) {
		t = inst->dsmr->electr_inst_active_power_recv_l2;
	} else {
		t = inst->dsmr->electr_inst_active_power_recv_l3;
	}
	(void) snprintf(buf, sizeof(buf), "%.3f", t);

	va_end(ap);

	return buf;
}

static char* http_response_phase_power_delivered(handler_t inst, char* method, ...) {
	va_list ap;
	static char buf[256];
	double t;
	int arg1;

	va_start(ap, method);

	(void) va_arg(ap, char*);
	arg1 = atoi(va_arg(ap, char*));

	if (arg1 == 1) {
		t = inst->dsmr->electr_inst_active_power_delv_l1;
	} else if (arg1 == 2) {
		t = inst->dsmr->electr_inst_active_power_delv_l2;
	} else {
		t = inst->dsmr->electr_inst_active_power_delv_l3;
	}
	(void) snprintf(buf, sizeof(buf), "%.3f", t);

	va_end(ap);

	return buf;
}

static char* http_response_phase_current(handler_t inst, char* method, ...) {
	va_list ap;
	static char buf[256];
	double t;
	int arg1;

	va_start(ap, method);

	(void) va_arg(ap, char*);
	arg1 = atoi(va_arg(ap, char*));

	if (arg1 == 1) {
		t = inst->dsmr->electr_inst_current_l1;
	} else if (arg1 == 2) {
		t = inst->dsmr->electr_inst_current_l2;
	} else {
		t = inst->dsmr->electr_inst_current_l3;
	}
	(void) snprintf(buf, sizeof(buf), "%.3f", t);

	va_end(ap);

	return buf;
}

static char* http_response_phase_voltage(handler_t inst, char* method, ...) {
	va_list ap;
	static char buf[256];
	double t;
	int arg1;

	va_start(ap, method);

	(void) va_arg(ap, char*);
	arg1 = atoi(va_arg(ap, char*));

	if (arg1 == 1) {
		t = inst->dsmr->electr_inst_voltage_l1;
	} else if (arg1 == 2) {
		t = inst->dsmr->electr_inst_voltage_l2;
	} else {
		t = inst->dsmr->electr_inst_voltage_l3;
	}
	(void) snprintf(buf, sizeof(buf), "%.3f", t);

	va_end(ap);

	return buf;
}

static char* http_response_indicator(handler_t inst, char* method, ...) {
	va_list ap;
	static char buf[256];

	va_start(ap, method);

	//snprintf(buf, sizeof(buf), "%s", inst->dsmr->electr_tariff_indicator);
	(void) snprintf(buf, sizeof(buf), "%d", inst->dsmr->electr_tariff_indicator);

	va_end(ap);

	return buf;
}

static char* http_response_equipment(handler_t inst, char* method, ...) {
	va_list ap;
	static char buf[256];

	va_start(ap, method);

	(void) snprintf(buf, sizeof(buf), "\"%s\"", inst->dsmr->equipment_identifier);

	va_end(ap);

	return buf;
}

static char* http_response_gas_equipment(handler_t inst, char* method, ...) {
	va_list ap;
	static char buf[256];

	va_start(ap, method);

	(void) snprintf(buf, sizeof(buf), "\"%s\"", inst->dsmr->device1_equipment_identifier);

	va_end(ap);

	return buf;
}

static char* http_response_gas_delivered(handler_t inst, char* method, ...) {
	va_list ap;
	static char buf[256];

	va_start(ap, method);
	(void) snprintf(buf, sizeof(buf), "%.3f", inst->dsmr->device1_last_5min_value);
	va_end(ap);

	return buf;
}

static int rest_inited = 0;
struct {
	char* resource;
	regex_t regex;
	struct {
		char* method;
		char* (*f)(handler_t inst, char* method, ...);
	} xx[5];
} rest[] = {
	{ "/api/electricity/tariff[12]",           {}, { { "GET", http_response_tariff1 } } },
	{ "/api/electricity/tariffs/indicator", {}, { { "GET", http_response_indicator } } },
	{ "/api/electricity/tariffs/[12]/delivered", {}, { { "GET", http_response_tariff_delivered } } },
	{ "/api/electricity/tariffs/[12]/received", {}, { { "GET", http_response_tariff_received } } },
	{ "/api/electricity/phases/([123])/power_delivered", {}, { { "GET", http_response_phase_power_delivered } } },
	{ "/api/electricity/phases/([123])/power_received", {}, { { "GET", http_response_phase_power_received } } },
	{ "/api/electricity/phases/([123])/current", {}, { { "GET", http_response_phase_current } } },
	{ "/api/electricity/phases/([123])/voltage", {}, { { "GET", http_response_phase_voltage } } },
	{ "/api/electricity/power/delivered", {}, { { "GET", http_response_total_power_delivered } } },
	{ "/api/electricity/power/received", {}, { { "GET", http_response_total_power_received } } },
	{ "/api/electricity/equipment", {}, { { "GET", http_response_equipment } } },
	{ "/api/gas/equipment", {}, { { "GET", http_response_gas_equipment } } },
	{ "/api/gas/delivered", {}, { { "GET", http_response_gas_delivered } } },
};

static int handler_callback(void* data, http_decoder_t decoder) {
	handler_t inst = (handler_t) data;
	char buf[16*2014];
	int rval = 0;

	info("Served %s %s", decoder->request_method, decoder->request_uri);

	if (strncmp("/api", decoder->request_uri, 4) != 0) {
		http_response_index(inst, decoder->request_method, decoder->request_uri);
	} else {
		regmatch_t pmatch[5];
		int i, j=0;

		for (i=0; i<sizeof(rest)/sizeof(rest[0]); i++) {
			if (0 == regexec(&rest[i].regex, decoder->request_uri, 5, pmatch, 0)) {
				for (j=0; j<sizeof(rest[i].xx)/sizeof(rest[0].xx[i]); j++) {
					if (strcmp(rest[i].xx[j].method, decoder->request_method) == 0) {
						break;
					}
				}
				break;
			}
		}

		if (i == sizeof(rest)/sizeof(rest[0])) {
			debug("404 Not found");
			(void) snprintf(buf, sizeof(buf), "HTTP/1.1 404 Not found\r\nContent-Length: 0\r\n\r\n");
			rval = write(inst->fd, buf, strlen(buf));
			if (rval < 0) {
				error("Cannot write 404");
			}
		} else {
			if (j == sizeof(rest[i].xx)/sizeof(rest[0].xx[i])) {
				debug("405 Method Not Allowed");
				(void) snprintf(buf, sizeof(buf), "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n");
				rval = write(inst->fd, buf, strlen(buf));
				if (rval < 0) {
					error("Cannot write 405");
				}
			} else {
				char buf[256];
				char* b2;
				char arg0[256];
				char arg1[256];
				char arg2[256];

				(void) regsubstr(arg0, sizeof(arg0), decoder->request_uri, pmatch, 0);
				(void) regsubstr(arg1, sizeof(arg1), decoder->request_uri, pmatch, 1);
				(void) regsubstr(arg2, sizeof(arg2), decoder->request_uri, pmatch, 2);

				b2 = rest[i].xx[j].f(inst, NULL /*decoder->request_uri*/, arg0, arg1, arg2);

				(void) snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\nContent-Type: application/javascript\r\nContent-Length: %lu\r\n\r\n%s", strlen(b2), b2);
				rval = write(inst->fd, buf, strlen(buf));
				if (rval < 0) {
					error("Cannot write result");
				}
			}
		}
	}

	return rval;
}

handler_t handler_init(int newsockfd, struct sockaddr_in cli_addr, dsmr_t dsmr) {
	handler_t inst;
	int i;

	inst = (handler_t) calloc(sizeof(struct struct_handler_t), 1);
	inst->fd = newsockfd;
	inst->addr = cli_addr;
	inst->dsmr = dsmr;

	if (!rest_inited) {
		for (i=0; i<sizeof(rest)/sizeof(rest[0]); i++) {
			(void) regcomp(&rest[i].regex, rest[i].resource, REG_EXTENDED);
		}
		rest_inited = 1;
	}

	debug("Handler init");

	return inst;
}

int handler_open(handler_t inst, dispatch_t dis) {
	debug("Handler open");
	inst->decoder = http_decoder_init(handler_callback, inst);
	dispatch_register(dis, inst->fd, handler_read, NULL, NULL, handler_close, inst);
	inst->dis = dis;
	return 0;
}

int handler_read(void* data) {
	handler_t inst = (handler_t) data;
	ssize_t len;
	char buf[4096];

	debug("Handler read");

	len = read(inst->fd, buf, sizeof(buf)-1);
	if (len < 1) {
		debug("Read %d", len);
		len = -1;
	} else {
		len = http_decoder_read(inst->decoder, buf, len);
	}

	return len;
}

int handler_close(void* data) {
	handler_t hdlr = (handler_t) data;

	(void) dispatch_unregister(hdlr->dis, hdlr);

	http_decoder_exit(hdlr->decoder);

	close(hdlr->fd);

	free(hdlr);

	debug("Handler close");

	return 0;
}

