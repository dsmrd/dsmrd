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
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>
//#include <stdio.h>
//#include <unistd.h>
//#include <stdlib.h>

//#include "../config.h"
//#include "logging.h"
//#include "options.h"
#include "dsmr.h"
//#include "serial.h"
//#ifdef FEATURE_DNS_SD
//#include "dnssd.h"
//#endif // FEATURE_DNS_SD
//#include "dispatch.h"
//#include "accept.h"
//#include "http.h"
//#include "daemon.h"
//#ifdef FEATURE_MQTT
//#include "mqtt.h"
//#endif // FEATURE_MQTT
#include "database.h"
//#include "rest.h"
//#include "publish.h"


static database_data_t database_electr_to_client_tariff1_hourly;
static database_data_t database_electr_to_client_tariff1_daily;
static database_data_t database_electr_to_client_tariff1_weekly;
static database_data_t database_electr_to_client_tariff2_hourly;
static database_data_t database_electr_to_client_tariff2_daily;
static database_data_t database_electr_to_client_tariff2_weekly;
static database_data_t database_electr_by_client_tariff1_hourly;
static database_data_t database_electr_by_client_tariff1_daily;
static database_data_t database_electr_by_client_tariff1_weekly;
static database_data_t database_electr_by_client_tariff2_hourly;
static database_data_t database_electr_by_client_tariff2_daily;
static database_data_t database_electr_by_client_tariff2_weekly;
static database_data_t database_device1_last_5min_value_hourly;
static database_data_t database_device1_last_5min_value_daily;
static database_data_t database_device1_last_5min_value_weekly;
static database_data_t database_device2_last_5min_value_hourly;
static database_data_t database_device2_last_5min_value_daily;
static database_data_t database_device2_last_5min_value_weekly;
static database_data_t database_device3_last_5min_value_hourly;
static database_data_t database_device3_last_5min_value_daily;
static database_data_t database_device3_last_5min_value_weekly;
static database_data_t database_device4_last_5min_value_hourly;
static database_data_t database_device4_last_5min_value_daily;
static database_data_t database_device4_last_5min_value_weekly;


int statistics_init() {
	database_electr_to_client_tariff1_hourly = database_data_init(OBIS_ELECTR_TO_CLIENT_TARIFF1, DATABASE_TYPE_HOURLY);
	database_electr_to_client_tariff1_daily = database_data_init(OBIS_ELECTR_TO_CLIENT_TARIFF1, DATABASE_TYPE_DAILY);
	database_electr_to_client_tariff1_weekly = database_data_init(OBIS_ELECTR_TO_CLIENT_TARIFF1, DATABASE_TYPE_WEEKLY);
	database_electr_to_client_tariff2_hourly = database_data_init(OBIS_ELECTR_TO_CLIENT_TARIFF2, DATABASE_TYPE_HOURLY);
	database_electr_to_client_tariff2_daily = database_data_init(OBIS_ELECTR_TO_CLIENT_TARIFF2, DATABASE_TYPE_DAILY);
	database_electr_to_client_tariff2_weekly = database_data_init(OBIS_ELECTR_TO_CLIENT_TARIFF2, DATABASE_TYPE_WEEKLY);
	database_electr_by_client_tariff1_hourly = database_data_init(OBIS_ELECTR_BY_CLIENT_TARIFF1, DATABASE_TYPE_HOURLY);
	database_electr_by_client_tariff1_daily = database_data_init(OBIS_ELECTR_BY_CLIENT_TARIFF1, DATABASE_TYPE_DAILY);
	database_electr_by_client_tariff1_weekly = database_data_init(OBIS_ELECTR_BY_CLIENT_TARIFF1, DATABASE_TYPE_WEEKLY);
	database_electr_by_client_tariff2_hourly = database_data_init(OBIS_ELECTR_BY_CLIENT_TARIFF2, DATABASE_TYPE_HOURLY);
	database_electr_by_client_tariff2_daily = database_data_init(OBIS_ELECTR_BY_CLIENT_TARIFF2, DATABASE_TYPE_DAILY);
	database_electr_by_client_tariff2_weekly = database_data_init(OBIS_ELECTR_BY_CLIENT_TARIFF2, DATABASE_TYPE_WEEKLY);
	database_device1_last_5min_value_hourly = database_data_init(OBIS_DEVICE1_LAST_5MIN_VALUE, DATABASE_TYPE_HOURLY);
	database_device1_last_5min_value_daily = database_data_init(OBIS_DEVICE1_LAST_5MIN_VALUE, DATABASE_TYPE_DAILY);
	database_device1_last_5min_value_weekly = database_data_init(OBIS_DEVICE1_LAST_5MIN_VALUE, DATABASE_TYPE_WEEKLY);
	database_device2_last_5min_value_hourly = database_data_init(OBIS_DEVICE2_LAST_5MIN_VALUE, DATABASE_TYPE_HOURLY);
	database_device2_last_5min_value_daily = database_data_init(OBIS_DEVICE2_LAST_5MIN_VALUE, DATABASE_TYPE_DAILY);
	database_device2_last_5min_value_weekly = database_data_init(OBIS_DEVICE2_LAST_5MIN_VALUE, DATABASE_TYPE_WEEKLY);
	database_device3_last_5min_value_hourly = database_data_init(OBIS_DEVICE3_LAST_5MIN_VALUE, DATABASE_TYPE_HOURLY);
	database_device3_last_5min_value_daily = database_data_init(OBIS_DEVICE3_LAST_5MIN_VALUE, DATABASE_TYPE_DAILY);
	database_device3_last_5min_value_weekly = database_data_init(OBIS_DEVICE3_LAST_5MIN_VALUE, DATABASE_TYPE_WEEKLY);
	database_device4_last_5min_value_hourly = database_data_init(OBIS_DEVICE4_LAST_5MIN_VALUE, DATABASE_TYPE_HOURLY);
	database_device4_last_5min_value_daily = database_data_init(OBIS_DEVICE4_LAST_5MIN_VALUE, DATABASE_TYPE_DAILY);
	database_device4_last_5min_value_weekly = database_data_init(OBIS_DEVICE4_LAST_5MIN_VALUE, DATABASE_TYPE_WEEKLY);
	return 0;
}

int statistics_exit() {
	database_data_exit(database_electr_to_client_tariff1_hourly);
	database_data_exit(database_electr_to_client_tariff1_daily);
	database_data_exit(database_electr_to_client_tariff1_weekly);
	database_data_exit(database_electr_to_client_tariff2_hourly);
	database_data_exit(database_electr_to_client_tariff2_daily);
	database_data_exit(database_electr_to_client_tariff2_weekly);
	database_data_exit(database_electr_by_client_tariff1_hourly);
	database_data_exit(database_electr_by_client_tariff1_daily);
	database_data_exit(database_electr_by_client_tariff1_weekly);
	database_data_exit(database_electr_by_client_tariff2_hourly);
	database_data_exit(database_electr_by_client_tariff2_daily);
	database_data_exit(database_electr_by_client_tariff2_weekly);
	database_data_exit(database_device1_last_5min_value_hourly);
	database_data_exit(database_device1_last_5min_value_daily);
	database_data_exit(database_device1_last_5min_value_weekly);
	database_data_exit(database_device2_last_5min_value_hourly);
	database_data_exit(database_device2_last_5min_value_daily);
	database_data_exit(database_device2_last_5min_value_weekly);
	database_data_exit(database_device3_last_5min_value_hourly);
	database_data_exit(database_device3_last_5min_value_daily);
	database_data_exit(database_device3_last_5min_value_weekly);
	database_data_exit(database_device4_last_5min_value_hourly);
	database_data_exit(database_device4_last_5min_value_daily);
	database_data_exit(database_device4_last_5min_value_weekly);
	return 0;
}

int do_stats(database_t stats, dsmr_t dsmr_) {
    obis_object_t object;
    obis_object_t time_object;

    time_object = rbtree_get(dsmr_->objects, OBIS_DATETIME_STAMP);
    if (time_object != NULL) {

        object = rbtree_get(dsmr_->objects, OBIS_ELECTR_TO_CLIENT_TARIFF1);
        if (object != NULL) {
            database_evaluate(stats, database_electr_to_client_tariff1_hourly, time_object->v.t, object->v.f.d);
            database_evaluate(stats, database_electr_to_client_tariff1_daily, time_object->v.t, object->v.f.d);
            database_evaluate(stats, database_electr_to_client_tariff1_weekly, time_object->v.t, object->v.f.d);
        }

        object = rbtree_get(dsmr_->objects, OBIS_ELECTR_TO_CLIENT_TARIFF2);
        if (object != NULL) {
            database_evaluate(stats, database_electr_to_client_tariff2_hourly, time_object->v.t, object->v.f.d);
            database_evaluate(stats, database_electr_to_client_tariff2_daily, time_object->v.t, object->v.f.d);
            database_evaluate(stats, database_electr_to_client_tariff2_weekly, time_object->v.t, object->v.f.d);
        }

        object = rbtree_get(dsmr_->objects, OBIS_ELECTR_BY_CLIENT_TARIFF1);
        if (object != NULL) {
            database_evaluate(stats, database_electr_by_client_tariff1_hourly, time_object->v.t, object->v.f.d);
            database_evaluate(stats, database_electr_by_client_tariff1_daily, time_object->v.t, object->v.f.d);
            database_evaluate(stats, database_electr_by_client_tariff1_weekly, time_object->v.t, object->v.f.d);
        }

        object = rbtree_get(dsmr_->objects, OBIS_ELECTR_BY_CLIENT_TARIFF2);
        if (object != NULL) {
            database_evaluate(stats, database_electr_by_client_tariff2_hourly, time_object->v.t, object->v.f.d);
            database_evaluate(stats, database_electr_by_client_tariff2_daily, time_object->v.t, object->v.f.d);
            database_evaluate(stats, database_electr_by_client_tariff2_weekly, time_object->v.t, object->v.f.d);
        }

        object = rbtree_get(dsmr_->objects, OBIS_DEVICE1_LAST_5MIN_VALUE);
        if (object != NULL) {
            database_evaluate(stats, database_device1_last_5min_value_hourly, object->v.m.t, object->v.m.d);
            database_evaluate(stats, database_device1_last_5min_value_daily, object->v.m.t, object->v.m.d);
            database_evaluate(stats, database_device1_last_5min_value_weekly, object->v.m.t, object->v.m.d);
        }

        object = rbtree_get(dsmr_->objects, OBIS_DEVICE2_LAST_5MIN_VALUE);
        if (object != NULL) {
            database_evaluate(stats, database_device2_last_5min_value_hourly, object->v.m.t, object->v.m.d);
            database_evaluate(stats, database_device2_last_5min_value_daily, object->v.m.t, object->v.m.d);
            database_evaluate(stats, database_device2_last_5min_value_weekly, object->v.m.t, object->v.m.d);
        }

        object = rbtree_get(dsmr_->objects, OBIS_DEVICE3_LAST_5MIN_VALUE);
        if (object != NULL) {
            database_evaluate(stats, database_device3_last_5min_value_hourly, object->v.m.t, object->v.m.d);
            database_evaluate(stats, database_device3_last_5min_value_daily, object->v.m.t, object->v.m.d);
            database_evaluate(stats, database_device3_last_5min_value_weekly, object->v.m.t, object->v.m.d);
        }

        object = rbtree_get(dsmr_->objects, OBIS_DEVICE4_LAST_5MIN_VALUE);
        if (object != NULL) {
            database_evaluate(stats, database_device4_last_5min_value_hourly, object->v.m.t, object->v.m.d);
            database_evaluate(stats, database_device4_last_5min_value_daily, object->v.m.t, object->v.m.d);
            database_evaluate(stats, database_device4_last_5min_value_weekly, object->v.m.t, object->v.m.d);
        }
    }

    return 0;
}

