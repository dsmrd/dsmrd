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

#ifndef DISPATCH_H
#define DISPATCH_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <sys/time.h>

typedef struct struct_dispatch_t* dispatch_t;
typedef struct dispatch_hook_struct_t* dispatch_hook_t;
typedef struct dispatch_timer_struct_t* dispatch_timer_t;
typedef unsigned long dispatch_interval_t;

dispatch_t dispatch_init();
dispatch_hook_t dispatch_register(dispatch_t dis, int fd, /*@null@*/ int (readcb)(void*), /*@null@*/ int (writecb)(void*), /*@null@*/ int (exceptcb)(void*), /*@null@*/ int (closecb)(void*), /*@null@*/ void* inst);
int dispatch_unregister_for_data(dispatch_t dis, void* inst);
int dispatch_unregister(dispatch_t dis, dispatch_hook_t inst);
dispatch_timer_t dispatch_create_timer(dispatch_t inst, int usec, void (*cb)(void*), void* data);
dispatch_timer_t dispatch_create_one_shot(dispatch_t inst, const struct timeval* expire, void (*cb)(void*), void* data);
int dispatch_remove_timer(dispatch_t inst, dispatch_timer_t t);
void dispatch_interval2timeval(dispatch_interval_t ival, /*@out@*/ struct timeval* tv);
dispatch_interval_t dispatch_timeval2interval(struct timeval* tv);
int dispatch_handle_events(dispatch_t dis);
int dispatch_quit(dispatch_t dis);
int dispatch_close(dispatch_t dis);
int dispatch_exit(dispatch_t dis);

#endif // DISPATCH_H

