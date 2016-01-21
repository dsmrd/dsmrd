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
#include <sys/types.h>
#include <sys/socket.h>
#include "logging.h"
#include "dsmr.h"
#include "dispatch.h"
#include "accept.h"
#include "http.h"
#include "options.h"



typedef enum {
	REQUEST,
	HEADER,
	DATA,
	DONE
} http_state_t;

typedef enum {
	COPY,
	DROP,
	PARSE_REQUEST,
	PARSE_HEADER,
	HANDLE_REQUEST
} http_action_t;

typedef struct struct_http_decoder_t* http_decoder_t;

struct struct_http_decoder_t {
    int len;
	int startline;
	http_state_t state;
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

http_decoder_t http_init(int (callback)(void*, http_decoder_t), void* data) {
	http_decoder_t inst;
	int rval;

	inst = (http_decoder_t) calloc(sizeof(struct struct_http_decoder_t), 1);

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

	return inst;
}

int http_exit(http_decoder_t inst) {
	regfree(&inst->regex_request);
	regfree(&inst->regex_header);
	free(inst);
	return 0;
}

static char* subnstr(char* dest, const char* string, int so, int eo, size_t n) {
	int len = eo - so;
	int min = (len < (n-1)) ? len : (n-1);
	strncpy(dest, string+so, min);
	dest[min] = '\0';
	return dest;
}

int http_decode(http_decoder_t inst, char* buf, ssize_t len, int (callback)(http_decoder_t)) {
	int i;
	for (i = 0; i < len; i++) {
		http_action_t action = COPY;
		switch (inst->state) {
			case REQUEST:
				switch (buf[i]) {
					case '\n':
						//printf("<LF>\n--HEADER--\n");
						inst->state = HEADER;
						action = PARSE_REQUEST;
						break;
					case '\r':
						//printf("<CR>");
						action = DROP;
						break;
					default:
						//printf("%c", buf[i]);
						break;
				}
				break;
			case HEADER:
				switch (buf[i]) {
					case '\n':
						//printf("<H-LF>\n");
						if (inst->startline) {
							if (inst->http_content_length != 0) {
								//printf("--DATA--\n");
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
						//printf("<H-CR>");
						action = DROP;
						break;
					default:
						//printf("%c", buf[i]);
						break;
				}
				break;
			case DATA:
				//printf("%c", buf[i]);
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
				//printf("REQUEST: %s\n", inst->buf);
				inst->len = 0;

				{
					int j;
					regmatch_t pmatch[5];

					j = regexec(&inst->regex_request, inst->buf, 5, pmatch, 0);
					if (j != 0) {
						error("regexec");
					} else {
						subnstr(inst->request_method, inst->buf, pmatch[1].rm_so, pmatch[1].rm_eo, sizeof(inst->request_method));
						subnstr(inst->request_uri, inst->buf, pmatch[2].rm_so, pmatch[2].rm_eo, sizeof(inst->request_uri));

						debug("METHOD='%s'", inst->request_method);
						debug("RESOURCE='%s'", inst->request_uri);
					}
				}


				break;
			case PARSE_HEADER:
				inst->buf[inst->len] = '\0';
				//printf("HEADER: %s\n", inst->buf);
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
						subnstr(tag, inst->buf, pmatch[1].rm_so, pmatch[1].rm_eo, sizeof(tag));
						subnstr(val, inst->buf, pmatch[2].rm_so, pmatch[2].rm_eo, sizeof(val));

						//debug("TAG='%s'", tag);
						//debug("VAL='%s'", val);
						if (strcmp(tag, "Content-Length") == 0) {
							inst->http_content_length = atoi(val);
							//debug("contlen=%d", inst->http_content_length);
						}
					}
				}
				break;
			case HANDLE_REQUEST:
				debug("REQUEST FINISHED");
				inst->callback(inst->data, inst);
				inst->len = 0;
				break;
		}
	}
	//printf("\n");

	return 0;
}









struct struct_handler_t {
	int fd;
	struct sockaddr_in addr;
	http_decoder_t decoder;
	dsmr_t dsmr;
	dispatch_t dis;
};

int handler_read(void*);

handler_t handler_init(int newsockfd, struct sockaddr_in cli_addr, dsmr_t dsmr) {
	handler_t inst;
	inst = (handler_t) calloc(sizeof(struct struct_handler_t), 1);
	inst->fd = newsockfd;
	inst->addr = cli_addr;
	inst->dsmr = dsmr;
	debug("Handler init");
	return inst;
}


static int do_index(handler_t inst) {
	char index[PATH_MAX];
	char buf[16*2014];
	char b2[16*2014];

	snprintf(index, sizeof(index), "%s/%s", options.wwwdir, "index.html");
	int fd = open(index, O_RDONLY);
	if (fd < 0) {
		error("Open error '%s'", index);
	}
	int rval = read(fd, b2, sizeof(b2));
	if (rval < 0) {
		error("Read error");
	}
	close(fd);

	snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\nContent-Length: %lu\r\n\r\n%s", strlen(b2), b2);

	rval = write(inst->fd, buf, strlen(buf));
	if (rval < 0) {
		error("Cannot write result");
	}

	return rval;
}

static int do_tariff1(handler_t inst) {
	char buf[16*2014];
	char b2[256];
	int rval;

	snprintf(b2, sizeof(b2), "{ \"value\": %.3f, \"unit\": \"%s\" }", inst->dsmr->electr_to_client_tariff1, "kWh");
	snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\nContent-Type: application/javascript\r\nContent-Length: %lu\r\n\r\n%s", strlen(b2), b2);

	rval = write(inst->fd, buf, strlen(buf));
	if (rval < 0) {
		error("Cannot write result");
	}

	return rval;
}

static int do_tariff2(handler_t inst) {
	char b2[256];
	char buf[256];
	int rval;

	snprintf(b2, sizeof(b2), "{ \"value\": %.3f, \"unit\": \"%s\" }", inst->dsmr->electr_to_client_tariff2, "kWh");
	snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\nContent-Type: application/javascript\r\nContent-Length: %lu\r\n\r\n%s", strlen(b2), b2);

	rval = write(inst->fd, buf, strlen(buf));
	if (rval < 0) {
		error("Cannot write result");
	}

	return rval;
}

struct {
	char* resource;
	struct {
		char* method;
		int (*f)(handler_t inst);
	} xx[5];
} rest[] = {
	{ "/",                        { { "GET", do_index } } },
	{ "/api/electricity/tariff1", { { "GET", do_tariff1 } } },
	{ "/api/electricity/tariff2", { { "GET", do_tariff2 } } },
};

int handler_callback(void* data, http_decoder_t decoder) {
	handler_t inst = (handler_t) data;
	char buf[16*2014];
	int rval = 0;

	info("Served %s %s", decoder->request_method, decoder->request_uri);

	int i, j=0;
	for (i=0; i<sizeof(rest)/sizeof(rest[0]); i++) {
		if (strcmp(rest[i].resource, decoder->request_uri) == 0) {
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
		snprintf(buf, sizeof(buf), "HTTP/1.1 404 Not found\r\nContent-Length: 0\r\n\r\n");
		rval = write(inst->fd, buf, strlen(buf));
		if (rval < 0) {
			error("Cannot write 404");
		}
	} else {
		if (j == sizeof(rest[i].xx)/sizeof(rest[0].xx[i])) {
			debug("405 Method Not Allowed");
			snprintf(buf, sizeof(buf), "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n");
			rval = write(inst->fd, buf, strlen(buf));
			if (rval < 0) {
				error("Cannot write 405");
			}
		} else {
			rval = rest[i].xx[j].f(inst);
			if (rval < 0) {
				error("Cannot handle request");
			}
		}
	}

	return rval;
}

int handler_open(handler_t inst, dispatch_t dis) {
	debug("Handler open");
	inst->decoder = http_init(handler_callback, inst);
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
	//buf[nread] = '\0';
	//printf("%s", buf);
	if (len < 1) {
		debug("Read %d", len);
		len = -1;
	} else {
		len = http_decode(inst->decoder, buf, len, NULL);
	}

	return len;
}

int handler_close(void* data) {
	handler_t hdlr = (handler_t) data;

    (void) dispatch_unregister(hdlr->dis, hdlr);

	http_exit(hdlr->decoder);

	close(hdlr->fd);

	free(hdlr);

	debug("Handler close");

	return 0;
}



