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
#include <stdio.h>
#include "dsmr.h"
#ifdef FEATURE_MQTT
#include "mqtt.h"
#endif // FEATURE_MQTT


int publish_init(mqtt_t m) {
	(void) mqtt_will(m, "/dsmrd/meter/0", "undefined");
	(void) mqtt_will(m, "/dsmrd/meter/1", "undefined");
	(void) mqtt_will(m, "/dsmrd/meter/2", "undefined");
	(void) mqtt_will(m, "/dsmrd/meter/3", "undefined");
	(void) mqtt_will(m, "/dsmrd/meter/4", "undefined");
	return 0;
}

int publish(mqtt_t m, dsmr_t dsmr_) {
	obis_object_t object;
	obis_object_t tobject;
	char buf[1024];

	tobject = rbtree_get(dsmr_->objects, OBIS_DATETIME_STAMP);

	object = rbtree_get(dsmr_->objects, OBIS_ELECTR_TO_CLIENT_TARIFF1);
	(void) snprintf(buf, sizeof(buf), "{ \"timestamp\": %lu, \"value\": %.3f, \"unit\": \"%s\" }", tobject->v.t, object->v.f.d, object->v.f.s);
	(void) mqtt_publish(m, "/dsmrd/meter/0", buf);

	object = rbtree_get(dsmr_->objects, OBIS_ELECTR_TO_CLIENT_TARIFF2);
	(void) snprintf(buf, sizeof(buf), "{ \"timestamp\": %lu, \"value\": %.3f, \"unit\": \"%s\" }", tobject->v.t, object->v.f.d, object->v.f.s);
	(void) mqtt_publish(m, "/dsmrd/meter/1", buf);

	object = rbtree_get(dsmr_->objects, OBIS_ELECTR_BY_CLIENT_TARIFF1);
	(void) snprintf(buf, sizeof(buf), "{ \"timestamp\": %lu, \"value\": %.3f, \"unit\": \"%s\" }", tobject->v.t, object->v.f.d, object->v.f.s);
	(void) mqtt_publish(m, "/dsmrd/meter/2", buf);

	object = rbtree_get(dsmr_->objects, OBIS_ELECTR_BY_CLIENT_TARIFF2);
	(void) snprintf(buf, sizeof(buf), "{ \"timestamp\": %lu, \"value\": %.3f, \"unit\": \"%s\" }", tobject->v.t, object->v.f.d, object->v.f.s);
	(void) mqtt_publish(m, "/dsmrd/meter/3", buf);

	object = rbtree_get(dsmr_->objects, OBIS_DEVICE1_LAST_5MIN_VALUE);
	(void) snprintf(buf, sizeof(buf), "{ \"timestamp\": %lu, \"value\": %.3f }", object->v.m.t, object->v.m.d);
	(void) mqtt_publish(m, "/dsmrd/meter/4", buf);


	object = rbtree_get(dsmr_->objects, OBIS_ELECTR_TO_CLIENT_TARIFF1);
	(void) snprintf(buf, sizeof(buf), "%010.3f", object->v.f.d);
	(void) mqtt_publish(m, "/dsmrd/devices/0/tariffs/1/delivered", buf);

	object = rbtree_get(dsmr_->objects, OBIS_ELECTR_TO_CLIENT_TARIFF2);
	(void) snprintf(buf, sizeof(buf), "%010.3f", object->v.f.d);
	(void) mqtt_publish(m, "/dsmrd/devices/0/tariffs/2/delivered", buf);

	object = rbtree_get(dsmr_->objects, OBIS_ELECTR_BY_CLIENT_TARIFF1);
	(void) snprintf(buf, sizeof(buf), "%010.3f", object->v.f.d);
	(void) mqtt_publish(m, "/dsmrd/devices/0/tariffs/1/received", buf);

	object = rbtree_get(dsmr_->objects, OBIS_ELECTR_BY_CLIENT_TARIFF2);
	(void) snprintf(buf, sizeof(buf), "%010.3f", object->v.f.d);
	(void) mqtt_publish(m, "/dsmrd/devices/0/tariffs/2/received", buf);

	object = rbtree_get(dsmr_->objects, OBIS_DEVICE1_LAST_5MIN_VALUE);
	(void) snprintf(buf, sizeof(buf), "%010.3f", object->v.m.d);
	(void) mqtt_publish(m, "/dsmrd/devices/1/tariffs/1/received", buf);
	(void) snprintf(buf, sizeof(buf), "%lu", object->v.m.t);
	(void) mqtt_publish(m, "/dsmrd/devices/1/tariffs/1/timestamp", buf);

	object = rbtree_get(dsmr_->objects, OBIS_DATETIME_STAMP);
	(void) snprintf(buf, sizeof(buf), "%lu", object->v.t);
	(void) mqtt_publish(m, "/dsmrd/devices/0/timestamp", buf);


	object = rbtree_get(dsmr_->objects, OBIS_ELECTR_TO_CLIENT_TARIFF_INDICATOR);
	(void) snprintf(buf, sizeof(buf), "%d", object->v.i);
	(void) mqtt_publish(m, "/dsmrd/devices/0/indicator", buf);

	object = rbtree_get(dsmr_->objects, OBIS_ELECTR_POWER_DELIVERED);
	(void) snprintf(buf, sizeof(buf), "%06.3f", object->v.f.d);
	(void) mqtt_publish(m, "/dsmrd/devices/0/delivered", buf);

	object = rbtree_get(dsmr_->objects, OBIS_ELECTR_POWER_RECEIVED);
	(void) snprintf(buf, sizeof(buf), "%06.3f", object->v.f.d);
	(void) mqtt_publish(m, "/dsmrd/devices/0/received", buf);

	return 0;
}

