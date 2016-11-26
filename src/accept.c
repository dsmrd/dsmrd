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
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <regex.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "logging.h"
#include "dispatch.h"
#include "accept.h"


struct struct_accept_t {
	int fd;
	int port;
	/*@shared@*/ dispatch_t dis;
	accept_callback_t cb;
	/*@null@*/ /*@shared@*/ void* data;
};


static int accept_read(void* inst);
static int accept_close(/*@only@*/ void* inst);


accept_t accept_init(int port, accept_callback_t cb, void* data) {
	accept_t acc;

	info("Initializing webserver");

	acc = (accept_t) calloc(sizeof(struct struct_accept_t), 1);
	if (acc == NULL) {
		error("Failed to allocate acceptor");
	} else {
		acc->port = port;
		acc->cb = cb;
		acc->data = data;
	}

	return acc;
}

int accept_open(accept_t acc, dispatch_t dis) {
	struct sockaddr_in serv_addr;
	int rval = -1;

	info("Binding to port %d", acc->port);

	acc->fd = socket(AF_INET, SOCK_STREAM, 0);
	if (acc->fd < 0) { 
		error("Cannot create socket: %s", strerror(errno));
		rval = -1;
	} else {
		int optval = 1;
		rval = setsockopt(acc->fd, SOL_SOCKET, SO_REUSEADDR, &optval, (socklen_t) sizeof(optval));
		if (rval != 0) {
			error("Cannot set socket options: %s", strerror(errno));
		} else {
			serv_addr.sin_family      = AF_INET;
			serv_addr.sin_addr.s_addr = INADDR_ANY;
			serv_addr.sin_port        = htons((uint16_t) acc->port);
			rval = bind(acc->fd, (struct sockaddr *) &serv_addr, (socklen_t) sizeof(serv_addr));
			if (rval < 0) {
				error("Cannot bind port: %s", strerror(errno));
			} else {
				rval = listen(acc->fd, 5);
				if (rval < 0) {
					error("Cannot listen: %s", strerror(errno));
				} else {
					(void) dispatch_register(dis, acc->fd, accept_read, NULL, NULL, accept_close, acc);
					acc->dis = dis;
				}
			}
		}
	}

	return rval;
}

static int accept_read(void* inst) {
	accept_t acc = (accept_t) inst;
	struct sockaddr_in cli_addr;
	socklen_t clilen;
	int newsockfd;

	clilen = (socklen_t) sizeof(cli_addr);
	memset(&cli_addr, 0, sizeof(cli_addr));
	newsockfd = accept(acc->fd, (struct sockaddr *) &cli_addr, &clilen);
	if (newsockfd < 0) {
		error("Cannot accept incoming connection: %s", strerror(errno));
	} else {
		debug("Incoming connection");
		(void) acc->cb(acc->dis, newsockfd, cli_addr, acc->data);
	}

	return 0;
}

static int accept_close(void* inst) {
	accept_t acc = (accept_t) inst;

	(void) dispatch_unregister_for_data(acc->dis, inst);

	(void) close(acc->fd);

	free(acc);

	info("Stopped webserver");

	return 0;
}

