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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "../config.h"
#include "logging.h"
#include "options.h"
#include "dsmr.h"
#include "serial.h"
#include "dnssd.h"
#include "dispatch.h"
#include "accept.h"
#include "daemon.h"
#include "mqtt.h"


static struct struct_dsmr_t dsmr;

static mqtt_t m;

/*
static void cb(void* key, void* value) {
	obis_object_t o = value;
	printf("%s = ", (char*)key);
	switch (o->type) {
		case INTEGER:
			printf("INTEGER %d", o->v.i);
			break;
		case DOUBLE:
			printf("DOUBLE %f %s", o->v.f.d, o->v.f.s);
			break;
		case TIME:
			printf("TIME %s", ctime(&(o->v.t)));
			break;
		case MIN5:
			printf("MIN5 %f %s", o->v.m.d, ctime(&(o->v.m.t)));
			break;
		case STRING:
			printf("STRING %s", o->v.s);
			break;
	}
	printf("\n");
}
*/

static int publish(dsmr_t dsmr_) {
	obis_object_t object;
	char buf[1024];

	object = rbtree_get(dsmr_->objects, OBIS_ELECTR_TO_CLIENT_TARIFF1);
	snprintf(buf, sizeof(buf), "%f", object->v.f.d);
	mqtt_publish(m, "/dsmrd/devices/0/tariffs/1/delivered", buf);

	object = rbtree_get(dsmr_->objects, OBIS_ELECTR_TO_CLIENT_TARIFF2);
	snprintf(buf, sizeof(buf), "%f", object->v.f.d);
	mqtt_publish(m, "/dsmrd/devices/0/tariffs/2/delivered", buf);

	object = rbtree_get(dsmr_->objects, OBIS_ELECTR_BY_CLIENT_TARIFF1);
	snprintf(buf, sizeof(buf), "%f", object->v.f.d);
	mqtt_publish(m, "/dsmrd/devices/0/tariffs/1/received", buf);

	object = rbtree_get(dsmr_->objects, OBIS_ELECTR_BY_CLIENT_TARIFF2);
	snprintf(buf, sizeof(buf), "%f", object->v.f.d);
	mqtt_publish(m, "/dsmrd/devices/0/tariffs/2/received", buf);

	return 0;
}

static int dsmr_handle(dsmr_t dsmr_) {
	dsmr = *dsmr_;
	//dsmr_print(dsmr_);
	//rbtree_foreach(dsmr_->objects, cb);

	publish(dsmr_);

	return 0;
}

static int decoder(char* buf, ssize_t sz) {
	return dsmr_decode(buf, sz);
}

int main(int argc, char* argv[]) {
	dispatch_t dis;
	serial_t ser;
	accept_t acc;
	avahi_t ava;

	printf("%s -- (C)2016 M.J. de Wit\n", PACKAGE_NAME);

	options_t options = options_init(argc, argv);

	if (options->daemonize) {
		daemonize();
	}

	(void) logging_init(options->daemonize, options->verbose, PACKAGE, LOG_DAEMON);
	ser = serial_init(options->tty, options->baud, options->is_tty, decoder);
	(void) dsmr_init(dsmr_handle, &dsmr);
	ava = avahi_init(options->dnssd_name);
	m = mqtt_init();

	dis = dispatch_init();

	mqtt_open(m, dis, options->mqtt_name, options->mqtt_host, options->mqtt_port);

	acc = accept_init(options->port, &dsmr);
	accept_open(acc, dis);

	(void) serial_open(ser, dis);

	avahi_open(ava, dis);

	dispatch_handle_events(dis);

	mqtt_exit(m);

	dispatch_close(dis);

	return EXIT_SUCCESS;
}

