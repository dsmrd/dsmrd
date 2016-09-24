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
#include "dsmr.h"
#include "dispatch.h"
#include "http.h"
#include "accept.h"



struct struct_accept_t {
	int fd;
	int port;
	dispatch_t dis;
	dsmr_t dsmr;
};

accept_t accept_init(int port, dsmr_t dsmr) {
	accept_t acc;

	info("Initializing webserver");

	acc = (accept_t) calloc(sizeof(struct struct_accept_t), 1);
	if (acc == NULL) {
		error("Failed to allocate acceptor");
	} else {
		acc->dsmr = dsmr;
		acc->port = port;
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
		rval = setsockopt(acc->fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
		if (rval != 0) {
			error("Cannot set socket options: %s", strerror(errno));
		} else {
			serv_addr.sin_family      = AF_INET;
			serv_addr.sin_addr.s_addr = INADDR_ANY;
			serv_addr.sin_port        = htons(acc->port);
			rval = bind(acc->fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
			if (rval < 0) {
				error("Cannot bind port: %s", strerror(errno));
			} else {
				rval = listen(acc->fd, 5);
				if (rval < 0) {
					error("Cannot listen: %s", strerror(errno));
				} else {
					dispatch_register(dis, acc->fd, accept_read, NULL, NULL, accept_close, NULL, acc);
					acc->dis = dis;
				}
			}
		}
	}

	return rval;
}

int accept_read(void* inst) {
	accept_t acc = (accept_t) inst;
	struct sockaddr_in cli_addr;
	socklen_t clilen;
	int newsockfd;

	clilen = sizeof(cli_addr);
    newsockfd = accept(acc->fd, (struct sockaddr *) &cli_addr, &clilen);
	if (newsockfd < 0) {
		error("Cannot accept incoming connection: %s", strerror(errno));
	} else {
		debug("Incoming connection");
		handler_t hdlr = handler_init(newsockfd, cli_addr, acc->dsmr);
		handler_open(hdlr, acc->dis);
	}

	return 0;
}

int accept_close(void* inst) {
	accept_t acc = (accept_t) inst;

	(void) dispatch_unregister(acc->dis, inst);

	close(acc->fd);

	free(acc);

	info("Stopped webserver");

	return 0;
}

