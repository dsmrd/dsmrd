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
#include <time.h>
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

typedef struct http_server_vars_struct_t* http_server_vars_t;

struct http_server_vars_struct_t {
	char request_method[32];
	char request_uri[256];
};

typedef struct struct_http_decoder_t* http_decoder_t;

struct struct_http_decoder_t {
	int len;
	int startline;
	http_decoder_state_t state;
	struct http_server_vars_struct_t server;
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
	rbtree_t resources;
	dsmr_t dsmr;
	dispatch_t dis;
};

struct resource_info {
	int (*callback)(handler_t inst, http_server_vars_t server, void* data);
	void* data;
};

typedef enum {
	HTTP_RESPONSE_CODE_OK                    = 200,
	HTTP_RESPONSE_CODE_CREATED               = 201,
	HTTP_RESPONSE_CODE_BAD_REQUEST           = 400,
	HTTP_RESPONSE_CODE_UNAUTHORIZED          = 401,
	HTTP_RESPONSE_CODE_NOT_FOUND             = 404,
	HTTP_RESPONSE_CODE_METHOD_NOT_ALLOWED    = 405,
	HTTP_RESPONSE_CODE_CONFLICT              = 409,
	HTTP_RESPONSE_CODE_INTERNAL_SERVER_ERROR = 500,
} http_response_code_t;

struct {
	http_response_code_t code;
	char* name;
} http_status_codes[] = {
	{ HTTP_RESPONSE_CODE_OK,                    "OK"                    },
	{ HTTP_RESPONSE_CODE_CREATED,               "Created"               },
	{ HTTP_RESPONSE_CODE_BAD_REQUEST,           "Bad Request"           },
	{ HTTP_RESPONSE_CODE_UNAUTHORIZED,          "Unauthorized"          },
	{ HTTP_RESPONSE_CODE_NOT_FOUND,             "Not Found"             },
	{ HTTP_RESPONSE_CODE_METHOD_NOT_ALLOWED,    "Method Not Allowed"    },
	{ HTTP_RESPONSE_CODE_CONFLICT,              "Conflict"              },
	{ HTTP_RESPONSE_CODE_INTERNAL_SERVER_ERROR, "Internal Server Error" },
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
						(void) regsubstr(inst->server.request_method, sizeof(inst->server.request_method), inst->buf, pmatch, 1);
						(void) regsubstr(inst->server.request_uri, sizeof(inst->server.request_uri), inst->buf, pmatch, 2);

						debug("METHOD='%s'", inst->server.request_method);
						debug("RESOURCE='%s'", inst->server.request_uri);
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

static int http_write_response(handler_t inst, int status, char* content) {
	char buf[256];
	int rval;
	unsigned int content_length = 0;
	char* cont = "";
	char* ename = "Unknown";
	int i = 0;

	while (i < sizeof(http_status_codes)/sizeof(http_status_codes[0])) {
		if (http_status_codes[i].code == status) {
			ename = http_status_codes[i].name;
			break;
		}
		i++;
	}

	if (content != NULL) {
		content_length = strlen(content);
		cont = content;
	}

	(void) snprintf(buf, sizeof(buf),
			"HTTP/1.1 %d %s\r\n"
			"Content-Type: application/javascript\r\n"
			"Content-Length: %u\r\n\r\n%s",
			status, ename,
			content_length, cont);
	rval = write(inst->fd, buf, strlen(buf));
	if (rval < 0) {
		error("Cannot write result");
	}

	return rval;
}

static int http_get_file(handler_t inst, http_server_vars_t server) {
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
		sz = write(inst->fd, buf, strlen(buf));
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
			sz = write(inst->fd, buf, strlen(buf));
			if (sz < 0) {
				error("Cannot write header");
			}

			while (sz > 0) {
				sz = read(fd, buf, sizeof(buf));
				if (sz < 0) {
					error("Read error");
				} else {
					sz = write(inst->fd, buf, sz);
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

static int http_get_obis(handler_t inst, http_server_vars_t method, void* data) {
	static char buf[1024] = "x";
	obis_object_t object;

	object = rbtree_get(inst->dsmr->objects, data);
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

	return http_write_response(inst, 200, buf);
}


static int handler_callback(void* data, http_decoder_t decoder) {
	handler_t inst = (handler_t) data;
	int rval = 0;
	rbtree_t resource;
	struct resource_info* rinfo;

	debug("Served %s %s", decoder->server.request_method, decoder->server.request_uri);

	resource = rbtree_get(inst->resources, decoder->server.request_uri);
	if (resource == NULL) {
		rval = http_get_file(inst, &decoder->server);
	} else {
		rinfo = (struct resource_info*) rbtree_get(resource, decoder->server.request_method);
		if (rinfo == NULL) {
			rval = http_write_response(inst, HTTP_RESPONSE_CODE_METHOD_NOT_ALLOWED, "Method Not Allowed");
		} else {
			rval = rinfo->callback(inst, &decoder->server, rinfo->data);
		}
	}

	return rval;
}

int string_less_then(void* a, void* b) { return strcmp(a, b) < 0; }
int string_equals(void* a, void* b) { return strcmp(a,b) == 0; }
void string_free(void* a) { free(a); }
void rbtree_free(void* a) { rbtree_exit(a); }
void resource_free(void* a) { free(a); }

void handler_register_resource(handler_t inst, char* resource, char* method,
		int (*cb)(handler_t inst, http_server_vars_t server, void* data), void* data) {
	rbtree_t m;
	struct resource_info* info;

	m = rbtree_get(inst->resources, resource);
	if (m == NULL) {
		m = rbtree_init(string_less_then, string_equals, string_free, resource_free);
		rbtree_put(inst->resources, resource, m);
	}
	info = malloc(sizeof(struct resource_info));
	info->callback = cb;
	info->data = data;
	rbtree_put(m, method, info);
}

void rest_init(handler_t inst) {
	handler_register_resource(inst, "/api/electricity/tariff1",                  "GET", http_get_obis, "1-0:1.8.1");
	handler_register_resource(inst, "/api/electricity/tariff2",                  "GET", http_get_obis, "1-0:1.8.2");
	handler_register_resource(inst, "/api/electricity/tariffs/indicator",        "GET", http_get_obis, "0-0:96.14.0");
	handler_register_resource(inst, "/api/electricity/tariffs/1/delivered",      "GET", http_get_obis, "1-0:1.8.1");
	handler_register_resource(inst, "/api/electricity/tariffs/2/delivered",      "GET", http_get_obis, "1-0:1.8.2");
	handler_register_resource(inst, "/api/electricity/tariffs/1/received",       "GET", http_get_obis, "1-0:2.8.1");
	handler_register_resource(inst, "/api/electricity/tariffs/2/received",       "GET", http_get_obis, "1-0:2.8.2");
	handler_register_resource(inst, "/api/electricity/phases/1/power_delivered", "GET", http_get_obis, "1-0:21.7.0");
	handler_register_resource(inst, "/api/electricity/phases/2/power_delivered", "GET", http_get_obis, "1-0:41.7.0");
	handler_register_resource(inst, "/api/electricity/phases/3/power_delivered", "GET", http_get_obis, "1-0:61.7.0");
	handler_register_resource(inst, "/api/electricity/phases/1/power_received",  "GET", http_get_obis, "1-0:22.7.0");
	handler_register_resource(inst, "/api/electricity/phases/2/power_received",  "GET", http_get_obis, "1-0:42.7.0");
	handler_register_resource(inst, "/api/electricity/phases/3/power_received",  "GET", http_get_obis, "1-0:62.7.0");
	handler_register_resource(inst, "/api/electricity/phases/1/current",         "GET", http_get_obis, "1-0:31.7.0");
	handler_register_resource(inst, "/api/electricity/phases/2/current",         "GET", http_get_obis, "1-0:51.7.0");
	handler_register_resource(inst, "/api/electricity/phases/3/current",         "GET", http_get_obis, "1-0:71.7.0");
	handler_register_resource(inst, "/api/electricity/phases/1/voltage",         "GET", http_get_obis, "1-0:32.7.0");
	handler_register_resource(inst, "/api/electricity/phases/2/voltage",         "GET", http_get_obis, "1-0:52.7.0");
	handler_register_resource(inst, "/api/electricity/phases/3/voltage",         "GET", http_get_obis, "1-0:72.7.0");
	handler_register_resource(inst, "/api/electricity/power/delivered",          "GET", http_get_obis, "1-0:1.7.0");
	handler_register_resource(inst, "/api/electricity/power/received",           "GET", http_get_obis, "1-0:2.7.0");
	handler_register_resource(inst, "/api/electricity/equipment",                "GET", http_get_obis, "0-0:96.1.1");
	handler_register_resource(inst, "/api/gas/equipment",                        "GET", http_get_obis, "");
	handler_register_resource(inst, "/api/gas/delivered",                        "GET", http_get_obis, "");
    //{ "/api/version", "1-3:0.2.8" },
    //{ "/api/datetimestamp", "0-0:1.0.0" },
    //{ "/api/nof_power_failures", "0-0:96.7.21" },
    //{ "/api/nof_long_power_failures", "0-0:96.7.9" },
    //{ "/api/power_fail_event_log", "1-0:99.97.0" },
    //{ "/api/nof_voltage_sage_l1", "1-0:32.32.0" },
    //{ "/api/nof_voltage_sage_l2", "1-0:52.32.0" },
    //{ "/api/nof_voltage_sage_l3", "1-0:72.32.0" },
    //{ "/api/nof_voltage_swells_l1", "1-0:32.36.0" },
    //{ "/api/nof_voltage_swells_l2", "1-0:52.36.0" },
    //{ "/api/nof_voltage_swells_l3", "1-0:72.36.0" },
    //{ "/api/text_message0", "0-0:96.13.0" },
    //{ "/api/text_message1", "0-0:96.13.1" },
}

handler_t handler_init(int newsockfd, struct sockaddr_in cli_addr, dsmr_t dsmr) {
	handler_t inst;

	inst = (handler_t) calloc(sizeof(struct struct_handler_t), 1);
	inst->fd = newsockfd;
	inst->addr = cli_addr;
	inst->dsmr = dsmr;

	inst->resources = rbtree_init(string_less_then, string_equals, string_free, rbtree_free);

	rest_init(inst);

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

	rbtree_exit(hdlr->resources);
	free(hdlr);

	debug("Handler close");

	return 0;
}

