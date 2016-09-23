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

#ifndef HTTP_H
#define HTTP_H

#include <netinet/in.h>
#include "dispatch.h"
#include "dsmr.h"

typedef struct http_server_vars_struct_t* http_server_vars_t;

struct http_server_vars_struct_t {
    char request_method[32];
    char request_uri[256];
};

typedef struct struct_handler_t* handler_t;

handler_t handler_init(int newsockfd, struct sockaddr_in cli_addr, dsmr_t dsmr);
int handler_open(handler_t inst, dispatch_t dis);
int handler_read(void* data);
int handler_close(void* data);
int handler_get_fd(handler_t inst);
int http_write_response(handler_t inst, int status, char* content);
void handler_register_resource(handler_t inst, char* resource, char* method,
        int (*cb)(handler_t inst, http_server_vars_t server, void* data, dsmr_t dsmr), void* data);
void handler_register_default(handler_t inst,
		int (*cb)(handler_t inst, http_server_vars_t server, void* data), void* data);

#endif // HTTP_H

