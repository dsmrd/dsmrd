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
#include "http.h"
#include "daemon.h"
#include "mqtt.h"
#include "stats.h"


static struct struct_dsmr_t dsmr;

static mqtt_t m;

static stats_t stats;
static stats_data_t stats_electr_to_client_tariff1_hourly;
static stats_data_t stats_electr_to_client_tariff1_daily;
static stats_data_t stats_electr_to_client_tariff1_weekly;
static stats_data_t stats_electr_to_client_tariff2_hourly;
static stats_data_t stats_electr_to_client_tariff2_daily;
static stats_data_t stats_electr_to_client_tariff2_weekly;
static stats_data_t stats_electr_by_client_tariff1_hourly;
static stats_data_t stats_electr_by_client_tariff1_daily;
static stats_data_t stats_electr_by_client_tariff1_weekly;
static stats_data_t stats_electr_by_client_tariff2_hourly;
static stats_data_t stats_electr_by_client_tariff2_daily;
static stats_data_t stats_electr_by_client_tariff2_weekly;
static stats_data_t stats_device1_last_5min_value_hourly;
static stats_data_t stats_device1_last_5min_value_daily;
static stats_data_t stats_device1_last_5min_value_weekly;
static stats_data_t stats_device2_last_5min_value_hourly;
static stats_data_t stats_device2_last_5min_value_daily;
static stats_data_t stats_device2_last_5min_value_weekly;
static stats_data_t stats_device3_last_5min_value_hourly;
static stats_data_t stats_device3_last_5min_value_daily;
static stats_data_t stats_device3_last_5min_value_weekly;
static stats_data_t stats_device4_last_5min_value_hourly;
static stats_data_t stats_device4_last_5min_value_daily;
static stats_data_t stats_device4_last_5min_value_weekly;


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

static int do_stats(dsmr_t dsmr_) {
	obis_object_t object;
	obis_object_t time_object;

	time_object = rbtree_get(dsmr_->objects, OBIS_DATETIME_STAMP);
	if (time_object != NULL) {

		object = rbtree_get(dsmr_->objects, OBIS_ELECTR_TO_CLIENT_TARIFF1);
		if (object != NULL) {
			stats_evaluate(stats, stats_electr_to_client_tariff1_hourly, time_object->v.t, object->v.f.d);
			stats_evaluate(stats, stats_electr_to_client_tariff1_daily, time_object->v.t, object->v.f.d);
			stats_evaluate(stats, stats_electr_to_client_tariff1_weekly, time_object->v.t, object->v.f.d);
		}

		object = rbtree_get(dsmr_->objects, OBIS_ELECTR_TO_CLIENT_TARIFF2);
		if (object != NULL) {
			stats_evaluate(stats, stats_electr_to_client_tariff2_hourly, time_object->v.t, object->v.f.d);
			stats_evaluate(stats, stats_electr_to_client_tariff2_daily, time_object->v.t, object->v.f.d);
			stats_evaluate(stats, stats_electr_to_client_tariff2_weekly, time_object->v.t, object->v.f.d);
		}

		object = rbtree_get(dsmr_->objects, OBIS_ELECTR_BY_CLIENT_TARIFF1);
		if (object != NULL) {
			stats_evaluate(stats, stats_electr_by_client_tariff1_hourly, time_object->v.t, object->v.f.d);
			stats_evaluate(stats, stats_electr_by_client_tariff1_daily, time_object->v.t, object->v.f.d);
			stats_evaluate(stats, stats_electr_by_client_tariff1_weekly, time_object->v.t, object->v.f.d);
		}

		object = rbtree_get(dsmr_->objects, OBIS_ELECTR_BY_CLIENT_TARIFF2);
		if (object != NULL) {
			stats_evaluate(stats, stats_electr_by_client_tariff2_hourly, time_object->v.t, object->v.f.d);
			stats_evaluate(stats, stats_electr_by_client_tariff2_daily, time_object->v.t, object->v.f.d);
			stats_evaluate(stats, stats_electr_by_client_tariff2_weekly, time_object->v.t, object->v.f.d);
		}

		object = rbtree_get(dsmr_->objects, OBIS_DEVICE1_LAST_5MIN_VALUE);
		if (object != NULL) {
			stats_evaluate(stats, stats_device1_last_5min_value_hourly, object->v.m.t, object->v.m.d);
			stats_evaluate(stats, stats_device1_last_5min_value_daily, object->v.m.t, object->v.m.d);
			stats_evaluate(stats, stats_device1_last_5min_value_weekly, object->v.m.t, object->v.m.d);
		}

		object = rbtree_get(dsmr_->objects, OBIS_DEVICE2_LAST_5MIN_VALUE);
		if (object != NULL) {
			stats_evaluate(stats, stats_device2_last_5min_value_hourly, object->v.m.t, object->v.m.d);
			stats_evaluate(stats, stats_device2_last_5min_value_daily, object->v.m.t, object->v.m.d);
			stats_evaluate(stats, stats_device2_last_5min_value_weekly, object->v.m.t, object->v.m.d);
		}

		object = rbtree_get(dsmr_->objects, OBIS_DEVICE3_LAST_5MIN_VALUE);
		if (object != NULL) {
			stats_evaluate(stats, stats_device3_last_5min_value_hourly, object->v.m.t, object->v.m.d);
			stats_evaluate(stats, stats_device3_last_5min_value_daily, object->v.m.t, object->v.m.d);
			stats_evaluate(stats, stats_device3_last_5min_value_weekly, object->v.m.t, object->v.m.d);
		}

		object = rbtree_get(dsmr_->objects, OBIS_DEVICE4_LAST_5MIN_VALUE);
		if (object != NULL) {
			stats_evaluate(stats, stats_device4_last_5min_value_hourly, object->v.m.t, object->v.m.d);
			stats_evaluate(stats, stats_device4_last_5min_value_daily, object->v.m.t, object->v.m.d);
			stats_evaluate(stats, stats_device4_last_5min_value_weekly, object->v.m.t, object->v.m.d);
		}
	}

	return 0;
}

static int dsmr_handle(dsmr_t dsmr_) {
	dsmr = *dsmr_;
	//dsmr_print(dsmr_);
	//rbtree_foreach(dsmr_->objects, cb);

	publish(dsmr_);

	do_stats(dsmr_);

	return 0;
}

static int decoder(char* buf, ssize_t sz) {
	return dsmr_decode(buf, sz);
}

static int acc_cb(dispatch_t dis, int newsockfd, struct sockaddr_in cli_addr, void* data) {
	handler_t hdlr = handler_init(newsockfd, cli_addr, (dsmr_t) data);
	handler_open(hdlr, dis);
	return 0;
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
	stats = stats_init(options->stats_database);
	stats_electr_to_client_tariff1_hourly = stats_data_init(OBIS_ELECTR_TO_CLIENT_TARIFF1, STATS_TYPE_HOURLY);
	stats_electr_to_client_tariff1_daily = stats_data_init(OBIS_ELECTR_TO_CLIENT_TARIFF1, STATS_TYPE_DAILY);
	stats_electr_to_client_tariff1_weekly = stats_data_init(OBIS_ELECTR_TO_CLIENT_TARIFF1, STATS_TYPE_WEEKLY);
	stats_electr_to_client_tariff2_hourly = stats_data_init(OBIS_ELECTR_TO_CLIENT_TARIFF2, STATS_TYPE_HOURLY);
	stats_electr_to_client_tariff2_daily = stats_data_init(OBIS_ELECTR_TO_CLIENT_TARIFF2, STATS_TYPE_DAILY);
	stats_electr_to_client_tariff2_weekly = stats_data_init(OBIS_ELECTR_TO_CLIENT_TARIFF2, STATS_TYPE_WEEKLY);
	stats_electr_by_client_tariff1_hourly = stats_data_init(OBIS_ELECTR_BY_CLIENT_TARIFF1, STATS_TYPE_HOURLY);
	stats_electr_by_client_tariff1_daily = stats_data_init(OBIS_ELECTR_BY_CLIENT_TARIFF1, STATS_TYPE_DAILY);
	stats_electr_by_client_tariff1_weekly = stats_data_init(OBIS_ELECTR_BY_CLIENT_TARIFF1, STATS_TYPE_WEEKLY);
	stats_electr_by_client_tariff2_hourly = stats_data_init(OBIS_ELECTR_BY_CLIENT_TARIFF2, STATS_TYPE_HOURLY);
	stats_electr_by_client_tariff2_daily = stats_data_init(OBIS_ELECTR_BY_CLIENT_TARIFF2, STATS_TYPE_DAILY);
	stats_electr_by_client_tariff2_weekly = stats_data_init(OBIS_ELECTR_BY_CLIENT_TARIFF2, STATS_TYPE_WEEKLY);
	stats_device1_last_5min_value_hourly = stats_data_init(OBIS_DEVICE1_LAST_5MIN_VALUE, STATS_TYPE_HOURLY);
	stats_device1_last_5min_value_daily = stats_data_init(OBIS_DEVICE1_LAST_5MIN_VALUE, STATS_TYPE_DAILY);
	stats_device1_last_5min_value_weekly = stats_data_init(OBIS_DEVICE1_LAST_5MIN_VALUE, STATS_TYPE_WEEKLY);
	stats_device2_last_5min_value_hourly = stats_data_init(OBIS_DEVICE2_LAST_5MIN_VALUE, STATS_TYPE_HOURLY);
	stats_device2_last_5min_value_daily = stats_data_init(OBIS_DEVICE2_LAST_5MIN_VALUE, STATS_TYPE_DAILY);
	stats_device2_last_5min_value_weekly = stats_data_init(OBIS_DEVICE2_LAST_5MIN_VALUE, STATS_TYPE_WEEKLY);
	stats_device3_last_5min_value_hourly = stats_data_init(OBIS_DEVICE3_LAST_5MIN_VALUE, STATS_TYPE_HOURLY);
	stats_device3_last_5min_value_daily = stats_data_init(OBIS_DEVICE3_LAST_5MIN_VALUE, STATS_TYPE_DAILY);
	stats_device3_last_5min_value_weekly = stats_data_init(OBIS_DEVICE3_LAST_5MIN_VALUE, STATS_TYPE_WEEKLY);
	stats_device4_last_5min_value_hourly = stats_data_init(OBIS_DEVICE4_LAST_5MIN_VALUE, STATS_TYPE_HOURLY);
	stats_device4_last_5min_value_daily = stats_data_init(OBIS_DEVICE4_LAST_5MIN_VALUE, STATS_TYPE_DAILY);
	stats_device4_last_5min_value_weekly = stats_data_init(OBIS_DEVICE4_LAST_5MIN_VALUE, STATS_TYPE_WEEKLY);

	ser = serial_init(options->tty, options->baud, options->is_tty, decoder);
	(void) dsmr_init(dsmr_handle, &dsmr);
	ava = avahi_init(options->dnssd_name);
	m = mqtt_init();

	dis = dispatch_init();

	mqtt_open(m, dis, options->mqtt_name, options->mqtt_host, options->mqtt_port, 10);

	acc = accept_init(options->port, acc_cb, &dsmr);
	accept_open(acc, dis);

	(void) serial_open(ser, dis);

	avahi_open(ava, dis);

	dispatch_handle_events(dis);

	mqtt_exit(m);

	dispatch_close(dis);

	stats_data_exit(stats_electr_to_client_tariff1_hourly);
	stats_data_exit(stats_electr_to_client_tariff1_daily);
	stats_data_exit(stats_electr_to_client_tariff1_weekly);
	stats_data_exit(stats_electr_to_client_tariff2_hourly);
	stats_data_exit(stats_electr_to_client_tariff2_daily);
	stats_data_exit(stats_electr_to_client_tariff2_weekly);
	stats_data_exit(stats_electr_by_client_tariff1_hourly);
	stats_data_exit(stats_electr_by_client_tariff1_daily);
	stats_data_exit(stats_electr_by_client_tariff1_weekly);
	stats_data_exit(stats_electr_by_client_tariff2_hourly);
	stats_data_exit(stats_electr_by_client_tariff2_daily);
	stats_data_exit(stats_electr_by_client_tariff2_weekly);
	stats_data_exit(stats_device1_last_5min_value_hourly);
	stats_data_exit(stats_device1_last_5min_value_daily);
	stats_data_exit(stats_device1_last_5min_value_weekly);
	stats_data_exit(stats_device2_last_5min_value_hourly);
	stats_data_exit(stats_device2_last_5min_value_daily);
	stats_data_exit(stats_device2_last_5min_value_weekly);
	stats_data_exit(stats_device3_last_5min_value_hourly);
	stats_data_exit(stats_device3_last_5min_value_daily);
	stats_data_exit(stats_device3_last_5min_value_weekly);
	stats_data_exit(stats_device4_last_5min_value_hourly);
	stats_data_exit(stats_device4_last_5min_value_daily);
	stats_data_exit(stats_device4_last_5min_value_weekly);
	stats_exit(stats);

	return EXIT_SUCCESS;
}

