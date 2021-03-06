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

#ifndef ACCEPT_H
#define ACCEPT_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <netinet/in.h>
#include "dispatch.h"

typedef struct struct_accept_t* accept_t;
typedef int (*accept_callback_t)(dispatch_t, int, struct sockaddr_in, /*@null@*/ /*@shared@*/ void*);

/*@null@*/ accept_t accept_init(int port, accept_callback_t cb, /*@null@*/ /*@shared@*/ void* data);
int accept_open(accept_t acc, /*@shared@*/ dispatch_t dis);

#endif // ACCEPT_H

