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
#include <time.h>
#include <sys/time.h>
#include <regex.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "logging.h"
#include "list.h"
#include "dispatch.h"

#define min(a,b) (((a)<(b))?(a):(b))


struct dispatch_hook_struct_t {
	int fd;
	int (*cb_read)(void*);
	int (*cb_write)(void*);
	int (*cb_except)(void*);
	int (*cb_close)(void*);
	/*@null@*/ /*@shared@*/ void* data;
};

struct struct_dispatch_t {
	int nfds;
	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;
	int done;
	int sigterm;
	int sigint;
	list_t hooks;
	list_t timers;
};

struct dispatch_timer_struct_t {
	struct timeval expire;
	struct timeval interval;
	int reload;
	void (*cb)(void*);
	/*@null@*/ /*@shared@*/ void* data;
};


static dispatch_t singleton;
void termination_handler(int signum) {
	switch (signum) {
		case SIGTERM:
			debug("SIGINT");
			singleton->sigterm = 1;
			break;
		case SIGINT:
			debug("SIGINT");
			singleton->sigint = 1;
			break;
		default:
			debug("SIGXXX %d", signum);
			break;
	}
}

static bool ptrcmp(const void* a, const void* b) {
	bool rval;
	rval = !(a == b);
	return rval;
}

void dispatch_interval2timeval(dispatch_interval_t ival, /*@out@*/ struct timeval* tv) {
	tv->tv_sec = (long) ival / 1000000;
	tv->tv_usec = (long) ival % 1000000;
}

dispatch_interval_t dispatch_timeval2interval(struct timeval* tv) {
	return tv->tv_sec * 1000000 + tv->tv_usec;
}

/*@null@*/ static dispatch_timer_t dispatch_timer_init(const struct timeval* expire, const struct timeval* interval, void (*cb)(void*), /*@null@*/ void* data) {
	dispatch_timer_t inst;

	inst = (dispatch_timer_t) calloc(sizeof(*inst), 1);
	if (inst == NULL) {
		error("Cannot allocate timer");
	} else {
		inst->expire = *expire;
		if (interval != NULL) {
			inst->interval = *interval;
			inst->reload = 1;
		}
		inst->cb = cb;
		inst->data = data;
		//debug("(%p) Timer interval = %d (us)", inst, inst->interval);
	}

	return inst;
}

static void dispatch_timer_exit(void* data) {
	dispatch_timer_t inst = (void*) data;
	debug("(%p) Freeing timer", inst);
	free(inst);
}

int dispatch_quit(dispatch_t inst) {
	debug("(%p) Quitting dispatcher", inst);
	inst->done = 1;
	return 0;
}

/*@null@*/ dispatch_hook_t dispatch_hook_init(int fd, int (*readcb)(void*), int (*writecb)(void*), int (*exceptcb)(void*), int (*closecb)(void*), /*@null@*/ void* data) {
	dispatch_hook_t inst;

	inst = (dispatch_hook_t) calloc(sizeof(struct dispatch_hook_struct_t), 1);
	if (inst != NULL) {
		inst->fd = fd;
		inst->cb_read = readcb;
		inst->cb_write = writecb;
		inst->cb_except = exceptcb;
		inst->cb_close = closecb;
		inst->data = data;
	}

	return inst;
}

void dispatch_hook_exit(void* data) {
	dispatch_hook_t inst = (dispatch_hook_t) data;
	free(inst);
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
	int rval;

	inst = (dispatch_t) calloc(sizeof(struct struct_dispatch_t), 1);
	if (inst != NULL) {
		FD_ZERO(&(inst->readfds));
		FD_ZERO(&(inst->writefds));
		FD_ZERO(&(inst->exceptfds));

		inst->hooks = list_init(ptrcmp, dispatch_hook_exit);
		inst->timers = list_init(ptrcmp, dispatch_timer_exit);

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
	}

	return inst;
}

int dispatch_exit(dispatch_t inst) {
	list_exit(inst->hooks);
	list_exit(inst->timers);
	free(inst);

	return 0;
}

#define max(a, b) ((a>b)?(a):(b))
static void dispatch_nfds(dispatch_t inst) {
	dispatch_hook_t hook;
	int nfds = -1;

	iter_t ihook = list_head(inst->hooks);
	if (iter_get(ihook) != NULL) {
		do {
			hook = (dispatch_hook_t) iter_get(ihook);
			nfds = max(nfds, hook->fd);
		} while (iter_next(ihook));
		inst->nfds = nfds+1;
	}

	debug("(%p) Number of file-descriptors = %d", inst, inst->nfds);
}

dispatch_hook_t dispatch_register(dispatch_t inst, int fd, int (readcb)(void*), int (writecb)(void*), int (exceptcb)(void*), int (closecb)(void*), void* data) {
	dispatch_hook_t hook;

	hook = dispatch_hook_init(fd, readcb, writecb, exceptcb, closecb, data);
	if (hook != NULL) {
		list_add(inst->hooks, hook);

		if (readcb != NULL) { FD_SET(fd, &(inst->readfds)); }
		if (writecb != NULL) { FD_SET(fd, &(inst->writefds)); }
		if (exceptcb != NULL) { FD_SET(fd, &(inst->exceptfds)); }

		dispatch_nfds(inst);

		info("Registered handler %d", fd);
	}

	return hook;
}

int dispatch_unregister(dispatch_t inst, dispatch_hook_t hook) {
	debug("(%p) Unregistering hook %p", inst, hook);

	if (hook->cb_read != NULL) { FD_CLR(hook->fd, &(inst->readfds)); }
	if (hook->cb_write != NULL) { FD_CLR(hook->fd, &(inst->writefds)); }
	if (hook->cb_except != NULL) { FD_CLR(hook->fd, &(inst->exceptfds)); }

	(void) list_remove_by_value(inst->hooks, hook);

	info("Unregistered handler %d", hook->fd);

	dispatch_nfds(inst);

	return 0;
}

int dispatch_unregister_for_data(dispatch_t inst, void* data) {
	dispatch_hook_t hook = NULL;

	iter_t ihook = list_head(inst->hooks);
	if (iter_get(ihook) != NULL) {
		do {
			hook = iter_get(ihook);
			if (hook->data == data) {
				break;
			}
		} while (iter_next(ihook));
	}
	if (iter_get(ihook) == 0) {
		error("Cannot unregister hook (no hook)");
	} else {
		if (((dispatch_hook_t)iter_get(ihook))->data != data) {
			error("Cannot unregister hook (no match found)");
		} else {
			dispatch_unregister(inst, hook);
		}
	}

	return 0;
}

static dispatch_interval_t dispatch_timer_evaluate(dispatch_t dis, dispatch_timer_t inst) {
	struct timeval now;
	struct timeval diff;

	(void) gettimeofday(&now, NULL);

	if (timercmp(&inst->expire, &now, <)) {
		inst->cb(inst->data);
		if (inst->reload) {
			timeradd(&inst->expire, &inst->interval, &inst->expire);
		} else {
			(void) list_remove_by_value(dis->timers, inst);
		}
	}

	timersub(&inst->expire, &now, &diff);
	return dispatch_timeval2interval(&diff);
}

dispatch_timer_t dispatch_create_timer(dispatch_t inst, int usec, void (*cb)(void*), void* data) {
	dispatch_timer_t rval;
	struct timeval interval;
	struct timeval now;
	struct timeval expire;

	(void) gettimeofday(&now, NULL);
	dispatch_interval2timeval(usec, &interval);
	timeradd(&now, &interval, &expire);

	rval = dispatch_timer_init(&expire, &interval, cb, data);
	list_add(inst->timers, rval);

	return rval;
}

dispatch_timer_t dispatch_create_one_shot(dispatch_t inst, const struct timeval* expire, void (*cb)(void*), void* data) {
	dispatch_timer_t rval;

	rval = dispatch_timer_init(expire, NULL, cb, data);
	list_add(inst->timers, rval);

	return rval;
}

int dispatch_remove_timer(dispatch_t inst, dispatch_timer_t t) {
	(void) list_remove_by_value(inst->timers, t);
	return 0;
}

static list_t _dispatch_list_copy(list_t l) {
	list_t cpy;
	iter_t ihook;

	cpy = list_init(ptrcmp, NULL);
	ihook = list_head(l);
	while (!iter_eof(ihook)) {
		list_add(cpy, iter_get(ihook));
		iter_next(ihook);
	}
	iter_exit(ihook);

	return cpy;
}

static dispatch_interval_t dispatch_handle_timers(dispatch_t inst) {
	int to;
	dispatch_interval_t mto = -1;
	list_t timer_list;
	iter_t timer_iter;

	timer_list = _dispatch_list_copy(inst->timers);
	timer_iter = list_head(timer_list);
	while (!iter_eof(timer_iter)) {
		to = dispatch_timer_evaluate(inst, iter_get(timer_iter));
		mto = min(mto, to);
		iter_next(timer_iter);
	}
	iter_exit(timer_iter);
	list_exit(timer_list);

	return mto;
}

int dispatch_handle_events(dispatch_t inst) {
	info("Started event handler");
	int rval;
	struct timeval timeout;
	struct timeval* ptimeout;
	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;
	dispatch_interval_t to = 100;

	while (!(inst->done && (list_size(inst->hooks) == 0))) {
		readfds = inst->readfds;
		writefds = inst->writefds;
		exceptfds = inst->exceptfds;

		if (to != -1) {
			dispatch_interval2timeval(to, &timeout);
			ptimeout = &timeout;
		} else {
			ptimeout = NULL;
		}

		rval = select(inst->nfds, &readfds, &writefds, &exceptfds, ptimeout);
		if (rval < 0) {
			if (errno != EINTR) {
				error("Select failed: %s", strerror(errno));
				inst->done = 1;
				break;
			}
		} else {
			iter_t ihook;
			list_t cpy;

			cpy = list_init(ptrcmp, NULL);
			ihook = list_head(inst->hooks);
			if (iter_get(ihook) != NULL) {
				do {
					list_add(cpy, iter_get(ihook));
				} while (iter_next(ihook));
			}
			iter_exit(ihook);

			ihook = list_head(cpy);
			if (iter_get(ihook) != NULL) {
				do {
					dispatch_hook_t hook = (dispatch_hook_t) iter_get(ihook);
					int rval = 0;

					if (FD_ISSET(hook->fd, &readfds)) {
						rval = hook->cb_read(hook->data);
					}
					if ((rval >= 0) && FD_ISSET(hook->fd, &writefds)) {
						rval = hook->cb_write(hook->data);
					}
					if ((rval >= 0) && FD_ISSET(hook->fd, &exceptfds)) {
						rval = hook->cb_except(hook->data);
					}

					if (rval < 0) {
						if (hook->cb_close) {
							debug("(%p) Closing %d", hook, hook->fd);
							hook->cb_close(hook->data);
						} else {
							debug("(%p) Unregistering %d", hook, hook->fd);
							dispatch_unregister(inst, hook);
						}
					}
				} while (iter_next(ihook));
			}

			iter_exit(ihook);
			list_exit(cpy);

			to = dispatch_handle_timers(inst);

			if ((inst->sigint) || (inst->sigterm)) {
				iter_t ihook;
				list_t cpy;

				cpy = list_init(ptrcmp, NULL);
				ihook = list_head(inst->hooks);
				if (iter_get(ihook) != NULL) {
					do {
						list_add(cpy, iter_get(ihook));
					} while (iter_next(ihook));
				}

				ihook = list_head(cpy);
				if (iter_get(ihook) != NULL) {
					do {
						dispatch_hook_t hook;
						hook = (dispatch_hook_t) iter_get(ihook);
						if (hook->cb_close != NULL) {
							hook->cb_close(hook->data);
						}
					} while (iter_next(ihook));
				}
				//printf("Done %d\n", list_size(inst->hooks));

				list_exit(cpy);

				inst->done = 1;
			}
		}
	}

	debug("hooks: %d", list_size(inst->hooks));
	debug("timers: %d", list_size(inst->timers));

	return 0;
}

int dispatch_close(dispatch_t inst) {
	dispatch_hook_t hook;
	iter_t ihook;

	ihook = list_head(inst->hooks);
	if (iter_get(ihook) != NULL) {
		do {
			int rval;

			hook = (dispatch_hook_t) iter_get(ihook);

			if (hook->cb_close != NULL) {
				rval = hook->cb_close(hook->data);
				if (rval != 0) {
					error("Cannot close hook");
				}
			} else {
				dispatch_unregister(inst, hook);
			}
		} while (iter_next(ihook));
	} else {
		info("No processes to stop");
	}

	info("Stopped event handler");

	return 0;
}

