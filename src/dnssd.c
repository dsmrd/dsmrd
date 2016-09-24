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
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-common/alternative.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#include <avahi-common/timeval.h>
#include "logging.h"
#include "list.h"
#include "dispatch.h"

typedef struct ava_struct_t* avahi_t;

struct AvahiTimeout {
	struct timeval expiry;
	AvahiTimeoutCallback callback;
	void* userdata;
	int enable;
	avahi_t poll;
};

struct AvahiWatch {
	int fd;
	AvahiWatchEvent event;
	AvahiWatchCallback callback;
	int (*readcb)(void*);
	int (*writecb)(void*);
	int (*exceptcb)(void*);
	void* userdata;
	dispatch_t dispatch;
};

struct ava_struct_t {
	AvahiClient* client;
	AvahiEntryGroup* group;
	AvahiPoll poll;
	list_t timeouts;
	list_t watches;
	char* name;
	dispatch_t dispatch;
};


static void create_services(AvahiClient* c, avahi_t i);

static void entry_group_callback(AvahiEntryGroup* group, AvahiEntryGroupState state, void* userdata) {
	avahi_t inst = NULL;
	char* n;

	switch (state) {
		case AVAHI_ENTRY_GROUP_ESTABLISHED :
			debug("Service '%s' successfully established.", inst->name);
			break;
		case AVAHI_ENTRY_GROUP_COLLISION : 
			warning("Service name collision, renaming service to '%s'", inst->name);
			n = avahi_alternative_service_name(inst->name);
			avahi_free(inst->name);
			inst->name = n;
			create_services(avahi_entry_group_get_client(group), inst);
			break;
		case AVAHI_ENTRY_GROUP_FAILURE :
			error("Entry group failure: %s", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(group))));
			dispatch_quit(inst->dispatch);
			break;
		case AVAHI_ENTRY_GROUP_UNCOMMITED:
			debug("AVAHI_ENTRY_GROUP_UNCOMMITED");
			break;
		case AVAHI_ENTRY_GROUP_REGISTERING:
			debug("AVAHI_ENTRY_GROUP_REGISTERING");
			break;
	}
}

static void create_services(AvahiClient* c, avahi_t inst) {
	int ret;

	assert(avahi_entry_group_is_empty(inst->group));

	info("Adding DNS-SD service '%s'", inst->name);

	if ((ret = avahi_entry_group_add_service(inst->group, AVAHI_IF_UNSPEC, AVAHI_PROTO_INET, 0, inst->name, "_http._tcp", NULL, NULL, 8888, NULL)) < 0) {
		error("Failed to add _http._tcp service: %s", avahi_strerror(ret));
		dispatch_quit(inst->dispatch);
	} else {
		if ((ret = avahi_entry_group_commit(inst->group)) < 0) {
			error("Failed to commit entry group: %s", avahi_strerror(ret));
			dispatch_quit(inst->dispatch);
		}
	}
}

static void client_callback(AvahiClient* c, AvahiClientState state, void* userdata) {
	avahi_t inst = (avahi_t) userdata;

	if (!inst->group) {
		inst->group = avahi_entry_group_new(c, entry_group_callback, inst);
		if (!(inst->group)) {
			error("avahi_entry_group_new() failed: %s", avahi_strerror(avahi_client_errno(c)));
//dispatch_quit(I//);
	}
}

	switch (state) {
		case AVAHI_CLIENT_S_RUNNING:
			debug("Avahi running");
			create_services(c, inst);
			break;
		case AVAHI_CLIENT_FAILURE:
			error("Avahi client failure: %s", avahi_strerror(avahi_client_errno(c)));
			dispatch_quit(inst->dispatch);
			break;
		case AVAHI_CLIENT_S_COLLISION:
		case AVAHI_CLIENT_S_REGISTERING:
			debug("Avahi registering/collision");
			//if (inst->group) {
				avahi_entry_group_reset(inst->group);
			//}
			break;
		case AVAHI_CLIENT_CONNECTING:
			debug("Avahi connecting");
			break;
	}
}

/*
static void modify_callback(AvahiTimeout* e, void* userdata) {
	avahi_t inst = e->poll;
	AvahiClient* client = userdata;

	info("Doing some weird modification");
	avahi_free(inst->name);
	inst->name = avahi_strdup("Modified MegaPrinter");
	if (avahi_client_get_state(client) == AVAHI_CLIENT_S_RUNNING) {
		if (group) {
			avahi_entry_group_reset(group);
		}
		create_services(client, inst);
	}
}
*/

int avahi_watch_readcb(void* userdata) {
	AvahiWatch* inst = (AvahiWatch*) userdata;
	debug("avahi_watch_readcb");
	inst->callback(inst, inst->fd, AVAHI_WATCH_IN, inst->userdata);
	return 0;
}

int avahi_watch_writecb(void* userdata) {
	AvahiWatch* inst = (AvahiWatch*) userdata;
	debug("avahi_watch_writecb");
	inst->callback(inst, inst->fd, AVAHI_WATCH_OUT, inst->userdata);
	return 0;
}

int avahi_watch_exceptcb(void* userdata) {
	AvahiWatch* inst = (AvahiWatch*) userdata;
	debug("avahi_watch_exceptcb");
	inst->callback(inst, inst->fd, AVAHI_WATCH_ERR, inst->userdata);
	return 0;
}

int avahi_watch_closecb(void* userdata) {
	AvahiWatch* inst = (AvahiWatch*) userdata;
	debug("avahi_watch_closecb");
	inst->callback(inst, inst->fd, AVAHI_WATCH_HUP, inst->userdata);
	return 0;
}

static char* watchevent2str(AvahiWatchEvent event) {
	char* rval;
	switch (event) {
		case AVAHI_WATCH_IN:  rval = "AVAHI_WATCH_IN";  break;
		case AVAHI_WATCH_OUT: rval = "AVAHI_WATCH_OUT"; break;
		case AVAHI_WATCH_ERR: rval = "AVAHI_WATCH_ERR"; break;
		case AVAHI_WATCH_HUP: rval = "AVAHI_WATCH_HUP"; break;
		default: rval = "AVAHI_WATCH_???";
	}
	return rval;
}

static AvahiWatch* avahi_watch_new(const AvahiPoll* poll, int fd, AvahiWatchEvent event, AvahiWatchCallback callback, void* userdata) {
	AvahiWatch* inst;
	int rval;
	avahi_t ava = (avahi_t) poll->userdata;

	debug("avahi_watch_new %p %s %p %p", poll, watchevent2str(event), callback, userdata);
	inst = (AvahiWatch*) malloc(sizeof(AvahiWatch));
	inst->fd = fd;
	inst->event = event;
	inst->callback = callback;
	inst->userdata = userdata;
	inst->dispatch = ava->dispatch;
	switch (event) {
		case AVAHI_WATCH_IN:  inst->readcb = avahi_watch_readcb; break;
		case AVAHI_WATCH_OUT: inst->writecb = avahi_watch_writecb; break;
		case AVAHI_WATCH_ERR: inst->exceptcb = avahi_watch_exceptcb; break;
		case AVAHI_WATCH_HUP:
		default:
							  error("Wrong event?");
	}
	rval = dispatch_register(inst->dispatch, inst->fd, inst->readcb, inst->writecb, inst->exceptcb, avahi_watch_closecb, NULL, inst);
	assert(rval == 0);

	return inst;
}

static void avahi_watch_update(AvahiWatch* inst, AvahiWatchEvent event) {
	int rval;

	debug("avahi_watch_update %s", watchevent2str(event));
	rval = dispatch_unregister(inst->dispatch, inst);
	if (rval != 0) {
		error("Cannot unregister");
		dispatch_quit(inst->dispatch);
	} else {
		switch (event) {
			case AVAHI_WATCH_IN:  inst->readcb = avahi_watch_readcb; break;
			case AVAHI_WATCH_OUT: inst->writecb = avahi_watch_writecb; break;
			case AVAHI_WATCH_ERR: inst->exceptcb = avahi_watch_exceptcb; break;
			case AVAHI_WATCH_HUP:
			default:
								  error("Wrong event?");
		}
		rval = dispatch_register(inst->dispatch, inst->fd, inst->readcb, inst->writecb, inst->exceptcb, avahi_watch_closecb, NULL, inst);
		if (rval != 0) {
			error("Cannot unregister");
			dispatch_quit(inst->dispatch);
		}
	}
}

static AvahiWatchEvent avahi_watch_get_events(AvahiWatch* inst) {
	debug("avahi_watch_get_events");
	error("Unexpected call");
	dispatch_quit(inst->dispatch);
	return 0;
}

static void avahi_watch_free(AvahiWatch* w) {
	debug("avahi_watch_free");
}

static AvahiTimeout* avahi_timeout_new(const AvahiPoll* poll, const struct timeval* tv, AvahiTimeoutCallback callback, void* userdata) {
	AvahiTimeout* inst;
	avahi_t ava = (avahi_t) poll->userdata;

	inst = (AvahiTimeout*) malloc(sizeof(AvahiTimeout));
	if (inst == NULL) {
		error("Cannot allocate timeout");
		dispatch_quit(ava->dispatch);
	} else {
		inst->callback = callback;
		inst->userdata = userdata;
		if (tv) {
			inst->enable = 1;
			inst->expiry = *tv;
		} else {
			inst->enable = 0;
		}
		inst->poll = ava;

		list_add(ava->timeouts, inst);

		if (tv) {
			debug("avahi_timeout_new %p %p tv_sec=%d tv_usec=%d", inst, ava, tv->tv_sec, tv->tv_usec);
		} else {
			debug("avahi_timeout_new inst=%p ava=%p tv=%p", inst, ava, tv);
	}
}

	return inst;
}

static void avahi_timeout_update(AvahiTimeout* inst, const struct timeval* tv) {
	debug("avahi_timeout_update %p", inst);
	if (tv) {
		inst->enable = 1;
		inst->expiry = *tv;
	} else {
		inst->enable = 0;
	}
}

static void avahi_timeout_free(AvahiTimeout* inst) {
	debug("avahi_timeout_free %p", inst);
	free(inst);
}

static bool timeout_cmp(const void* t1, const void* t2) {
	return t1 == t2;
}

static void timeout_lree(void* t) {
}

avahi_t avahi_init(char* name) {
	avahi_t inst = (avahi_t) calloc(sizeof(struct ava_struct_t), 1);
	debug("avahi_init %p", inst);

	inst->poll.userdata = inst;

	inst->poll.watch_new = avahi_watch_new;
	inst->poll.watch_update = avahi_watch_update;
	inst->poll.watch_get_events = avahi_watch_get_events;
	inst->poll.watch_free = avahi_watch_free;

	inst->poll.timeout_new = avahi_timeout_new;
	inst->poll.timeout_update = avahi_timeout_update;
	inst->poll.timeout_free = avahi_timeout_free;

	inst->name = avahi_strdup(name);

	inst->timeouts = list_init(timeout_cmp, timeout_lree);

	return inst;
}

void avahi_open(avahi_t inst, dispatch_t dis) {
	int err;

	inst->dispatch = dis;

	inst->client = avahi_client_new(&inst->poll, 0, client_callback, inst, &err);
	if (!(inst->client)) {
		error("Failed to create client: %s", avahi_strerror(err));
	}
}

void ava_close(avahi_t inst) {
	if (inst->client) {
		avahi_client_free(inst->client);
	}
	avahi_free(inst->name);
}

