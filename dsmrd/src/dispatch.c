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
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <regex.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "logging.h"
#include "dsmr.h"



typedef struct struct_dispatch_hook_t* dispatch_hook_t;
struct struct_dispatch_hook_t {
	dispatch_hook_t next;
	dispatch_hook_t prev;
	int fd;
	int (*cb_read)(void*);
	int (*cb_write)(void*);
	int (*cb_except)(void*);
	int (*cb_close)(void*);
	void* instance;
};


struct struct_dispatch_t {
	int nfds;
	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;
	struct timeval timeout;
	int done;
	dispatch_hook_t free;
	dispatch_hook_t used;
};

typedef struct struct_dispatch_t* dispatch_t;



static dispatch_t singleton;
void termination_handler(int signum) {
	//printf("SIGTERM!!!\n");
	singleton->done = 1;
}


dispatch_hook_t dispatch_hook_init() {
	dispatch_hook_t inst;
	inst = (dispatch_hook_t) calloc(sizeof(struct struct_dispatch_hook_t), 1);
	return inst;
}



char* fdset2string(fd_set* s, int nfds) {
	static char buf[256];
	int i;
	for (i=0; i<nfds; i++) {
		if (FD_ISSET(i, s)) {
			buf[i] = '1';
		} else {
			buf[i] = '0';
		}
	}
	buf[nfds+1]='\0';
	return buf;
}

dispatch_t dispatch_init() {
	dispatch_t inst;
	struct sigaction new_action, old_action;
	int i;
	int rval;

	inst = (dispatch_t) calloc(sizeof(struct struct_dispatch_t), 1);
	FD_ZERO(&(inst->readfds));
	FD_ZERO(&(inst->writefds));
	FD_ZERO(&(inst->exceptfds));
#define NOF_REQUEST_HOOKS 20
	for (i = 0; i < NOF_REQUEST_HOOKS; i++) {
		dispatch_hook_t hook = dispatch_hook_init();
		hook->next = inst->free;
		if (inst->free != NULL) {
			inst->free->prev = hook;
		}
		inst->free = hook;
	}
	inst->timeout.tv_sec = 10;
	inst->timeout.tv_usec = 0;

	singleton = inst;

	new_action.sa_handler = termination_handler;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = 0;
	rval = sigaction(SIGTERM, &new_action, &old_action);
	if (rval != 0) {
		error("Cannot register signal handler: %s", strerror(errno));
	}
	rval = sigaction(SIGINT, &new_action, NULL);
	if (rval != 0) {
		error("Cannot register signal handler: %s", strerror(errno));
	}

	return inst;
}

#define max(a, b) ((a>b)?(a):(b))
static void dispatch_nfds(dispatch_t dis) {
	dispatch_hook_t hook = dis->used;
	int nfds = 0;
	while (hook) {
		nfds = max(nfds, hook->fd);
		hook = hook->next;
	}
	dis->nfds = nfds+1;
	debug("Number of file-descriptors = %d", nfds+1);
	//debug("read:   %s", fdset2string(&(dis->readfds), nfds+1));
	//debug("write:  %s", fdset2string(&(dis->writefds), nfds+1));
	//debug("except: %s", fdset2string(&(dis->exceptfds), nfds+1));
}

int dispatch_register(dispatch_t dis, int fd, int (readcb)(void*), int (writecb)(void*), int (exceptcb)(void*), int (closecb)(void*), void* inst) {
	if (dis->free == NULL) {
		error("Out of request_uris");
	} else {
		// Pop
		dispatch_hook_t hook = dis->free;
		dis->free = dis->free->next;

		// Fill in
		hook->fd = fd;
		hook->cb_read = readcb;
		hook->cb_write = writecb;
		hook->cb_except = exceptcb;
		hook->cb_close = closecb;
		hook->instance = inst;

		// Set select bits
		if (readcb != NULL) { FD_SET(fd, &(dis->readfds)); }
		if (writecb != NULL) { FD_SET(fd, &(dis->writefds)); }
		if (exceptcb != NULL) { FD_SET(fd, &(dis->exceptfds)); }

		// Push
		hook->next = dis->used;
		hook->prev = 0;
		if (dis->used != NULL) {
			dis->used->prev = hook;
		}
		dis->used = hook;

		dispatch_nfds(dis);

		info("Registered handler %d", fd);
	}

	return 0;
}

int dispatch_unreg_hook(dispatch_t dis, dispatch_hook_t hook) {
	// Remove from chain
	if (hook->next != NULL) {
		hook->next->prev = hook->prev;
	}
	if (hook->prev != NULL) {
		hook->prev->next = hook->next;
	} else {
		dis->used = hook->next;
	}

	// Clear select bits
	if (hook->cb_read != NULL) { FD_CLR(hook->fd, &(dis->readfds)); }
	if (hook->cb_write != NULL) { FD_CLR(hook->fd, &(dis->writefds)); }
	if (hook->cb_except != NULL) { FD_CLR(hook->fd, &(dis->exceptfds)); }

	info("Unregistered handler %d", hook->fd);

	// Clear and put in free
	memset(hook, 0, sizeof(struct struct_dispatch_hook_t));
	hook->next = dis->free;
	dis->free = hook;

	// Recalc nfds
	dispatch_nfds(dis);

	return 0;
}

int dispatch_unregister(dispatch_t dis, void* inst) {
	dispatch_hook_t hook;

	hook = dis->used;
	while (hook) {
		if (hook->instance == inst) {
			break;
		}
		hook = hook->next;
	}
	if (hook != 0) {
		dispatch_unreg_hook(dis, hook);
	}

	return 0;
}

int dispatch_handle_events(dispatch_t dis) {
	info("Started event handler");
	int rval;
	struct timeval timeout;
	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;

	while (!dis->done) {
		timeout = dis->timeout;
		readfds = dis->readfds;
		writefds = dis->writefds;
		exceptfds = dis->exceptfds;

		rval = select(dis->nfds, &readfds, &writefds, &exceptfds, &timeout);
		if (rval < 0) {
			if (errno != EINTR) {
				error("Select failed: %s", strerror(errno));
				dis->done = 1;
			}
		} else {
			dispatch_hook_t hook = dis->used;
			while (hook) {
				dispatch_hook_t hooknext;
				int rval = 0;

				hooknext = hook->next;

				if (FD_ISSET(hook->fd, &readfds)) {
					rval = hook->cb_read(hook->instance);
//debug("Read %d", rval);
//if (rval != 0) {
//}
//rval = 0;
				}
				if ((rval >= 0) && FD_ISSET(hook->fd, &writefds)) {
					rval = hook->cb_write(hook->instance);
				}
				if ((rval >= 0) && FD_ISSET(hook->fd, &exceptfds)) {
					rval = hook->cb_except(hook->instance);
				}

				if (rval < 0) {
					if (hook->cb_close) {
						debug("Closing %d", hook->fd);
						hook->cb_close(hook->instance);
					} else {
						debug("Unregistering %d", hook->fd);
						dispatch_unreg_hook(dis, hook);
					}
				}

				hook = hooknext;
			}
		}
	}

	return 0;
}

int dispatch_close(dispatch_t dis) {
	dispatch_hook_t hook = dis->used;
	while (hook) {
		dispatch_hook_t hooknext;
		int rval;

		hooknext = hook->next;

		if (hook->cb_close != NULL) {
			rval = hook->cb_close(hook->instance);
			if (rval != 0) {
				error("Cannot close hook");
			}
		} else {
			dispatch_unreg_hook(dis, hook);
		}

		hook = hooknext;
	}

	info("Stopped event handler");

	return 0;
}

