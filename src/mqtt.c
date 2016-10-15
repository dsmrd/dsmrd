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
#include <assert.h>
#include <time.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "mosquitto.h"
#include "dispatch.h" 
#include "logging.h" 
#include "mqtt.h" 


struct mqtt_struct_t {
	struct mosquitto* mosq;
	dispatch_t dispatch;
	dispatch_timer_t timer;
	int connected;
};


static int mqtt_read(void* userdata);
static int mqtt_write(void* userdata);
static int mqtt_close(void* userdata);
static void mqtt_misc(void* userdata);

static void on_log(/*@unused@*/ struct mosquitto *mosq, /*@unused@*/ void *obj, int level, const char* str) {
	//mqtt_t inst = (mqtt_t) obj;

	switch (level) {
		case MOSQ_LOG_INFO:
		case MOSQ_LOG_NOTICE:
			info("mosquitto: %s", str);
			break;
		case MOSQ_LOG_WARNING:
			warning("mosquitto: %s", str);
			break;
		case MOSQ_LOG_ERR:
			error("mosquitto: %s", str);
			break;
		case MOSQ_LOG_DEBUG:
		default:
			debug("mosquitto: %s", str);
			break;
	}
}

static void on_connect(/*@unused@*/ struct mosquitto *mosq, void *obj, /*@unused@*/ int rc) {
	mqtt_t inst = (mqtt_t) obj;

	inst->connected = 1;

	debug("MQTT connect: %s", mosquitto_connack_string(rc));
}

static void on_disconnect(/*@unused@*/ struct mosquitto *mosq, void *obj, /*@unused@*/ int rc) {
	mqtt_t inst = (mqtt_t) obj;
	int rval;

	debug("MQTT disconnect: %s", mosquitto_connack_string(rc));

	rval = dispatch_unregister(inst->dispatch, inst);
	assert(rval == 0);

	inst->connected = 0;
}

static void on_publish(/*@unused@*/ struct mosquitto *mosq, /*@unused@*/ void *obj, /*@unused@*/ int mid) {
	//mqtt_t inst = (mqtt_t) obj;

	debug("publish... mid=%d", mid);
}

static void on_message(/*@unused@*/ struct mosquitto *mosq, /*@unused@*/ void *obj, /*@unused@*/ const struct mosquitto_message *msg) {
	//mqtt_t inst = (mqtt_t) obj;

	debug("message... mid=%d, topic=%s, payload=%s, payloadlen=%d, qos=%d, retail=%d", msg->mid, msg->topic, msg->payload, msg->payloadlen, msg->qos, msg->retain);

}

static void on_subscribe(/*@unused@*/ struct mosquitto *mosq, /*@unused@*/ void *obj, /*@unused@*/ int mid, /*@unused@*/ int qos_count, /*@unused@*/ const int* granted_qos) {
	//mqtt_t inst = (mqtt_t) obj;

	debug("subscribe... mid=%d, qos_count=%d", mid, qos_count);
}

static void on_unsubscribe(/*@unused@*/ struct mosquitto *mosq, /*@unused@*/ void *obj, /*@unused@*/ int mid) {
	//mqtt_t inst = (mqtt_t) obj;

	debug("unsubscribe... mid=%d", mid);
}

static mqtt_t iii;
static void alarm_handler(/*@unused@*/ int signum) {
	int rval;
	int fd;

	if (iii->connected == 0) {
		rval = mosquitto_reconnect(iii->mosq);
		if (rval != MOSQ_ERR_SUCCESS) {
			debug("Cannot reconnect: '%s'", mosquitto_strerror(rval));
		} else {
			fd = mosquitto_socket(iii->mosq);
			assert(fd != 0);

			rval = dispatch_register(iii->dispatch, fd, mqtt_read, mqtt_write, NULL, mqtt_close, /*mqtt_misc,*/ iii);
			assert(rval == 0);
		}
	}
}

mqtt_t mqtt_init() {
	mqtt_t inst;
	int rval;
	int major = 0;
	int minor = 0;
	int revision = 0;
	struct sigaction new_action, old_action;
	struct itimerval itv;

	inst = (mqtt_t) calloc(sizeof(struct mqtt_struct_t), 1);
	iii = inst;

	(void) mosquitto_lib_version(&major, &minor, &revision);
	info("Use mosquitto v%d.%d r%d", major, minor, revision);

	rval = mosquitto_lib_init();
	if (rval != MOSQ_ERR_SUCCESS) {
		debug("Cannot initialize library: '%s'", mosquitto_strerror(rval));
	}

	memset(&new_action, 0, sizeof(new_action));
	new_action.sa_handler = alarm_handler;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;
    rval = sigaction(SIGALRM, &new_action, &old_action);
    if (rval != 0) {
        error("Cannot register signal handler: %s", strerror(errno));
	}

	itv.it_value.tv_sec = 1;
	itv.it_value.tv_usec = 0;
	itv.it_interval.tv_sec = 2;
	itv.it_interval.tv_usec = 0;
	setitimer(0, &itv, NULL);

	return inst;
}

void mqtt_exit(mqtt_t inst) {
	int rval;

	mosquitto_destroy(inst->mosq);

	rval = mosquitto_lib_cleanup();
	if (rval != MOSQ_ERR_SUCCESS) {
		debug("Cannot cleanup library: '%s'", mosquitto_strerror(rval));
	}
}

static int mqtt_read(void* userdata) {
	mqtt_t inst = (mqtt_t) userdata;
	int rval;

	rval = mosquitto_loop_read(inst->mosq, 1);
	switch (rval) {
		case MOSQ_ERR_SUCCESS:
			break;
		case MOSQ_ERR_CONN_LOST:
		default:
			error("mosquitto_loop_read: %s", mosquitto_strerror(rval));
			break;
	}

	return 0;
}

static int mqtt_write(void* userdata) {
	mqtt_t inst = (mqtt_t) userdata;
	int rval;

	rval = mosquitto_loop_write(inst->mosq, 1);
	switch (rval) {
		case MOSQ_ERR_SUCCESS:
			break;
		case MOSQ_ERR_CONN_LOST:
		default:
			error("mosquitto_loop_write: %s", mosquitto_strerror(rval));
			break;
	}

	return 0;
}

static void mqtt_misc(void* userdata) {
	mqtt_t inst = (mqtt_t) userdata;
	int rval;

	rval = mosquitto_loop_misc(inst->mosq);
	switch (rval) {
		case MOSQ_ERR_SUCCESS:
			break;
		case MOSQ_ERR_CONN_LOST:
		default:
			error("mosquitto_loop_misc: %s", mosquitto_strerror(rval));
			break;
	}
}

static int mqtt_close(void* userdata) {
	mqtt_t inst = (mqtt_t) userdata;
	int rval;

	info("Stopping MQTT service");

	rval = mosquitto_disconnect(inst->mosq);
	if (rval != MOSQ_ERR_SUCCESS) {
		error("Cannot disconnect mosquitto: %s", mosquitto_strerror(rval));
	}

	return rval;
}

int mqtt_open(mqtt_t inst, dispatch_t d, const char* name, const char* host, int port) {
	int rval;
	int fd;

	inst->mosq = mosquitto_new(name, true, inst);
	assert(inst->mosq != NULL);

	inst->dispatch = d;

	mosquitto_log_callback_set(inst->mosq, on_log);
	mosquitto_connect_callback_set(inst->mosq, on_connect);
	mosquitto_disconnect_callback_set(inst->mosq, on_disconnect);
	mosquitto_publish_callback_set(inst->mosq, on_publish);
	mosquitto_message_callback_set(inst->mosq, on_message);
	mosquitto_subscribe_callback_set(inst->mosq, on_subscribe);
	mosquitto_unsubscribe_callback_set(inst->mosq, on_unsubscribe);

	rval = mosquitto_connect(inst->mosq, host, port, 10);
	assert(rval == 0);

	fd = mosquitto_socket(inst->mosq);
	assert(fd != 0);

	rval = dispatch_register(d, fd, mqtt_read, mqtt_write, NULL, mqtt_close, /*mqtt_misc,*/ inst);
	assert(rval == 0);

	inst->timer = dispatch_create_timer(d, 100000, mqtt_misc, inst);
	assert(inst->timer != 0);

	return rval;
}


int mqtt_publish(mqtt_t inst, char* topic, char* payload) {
	int rval;
	int qos = 0;
	int retain = 0;
	int mid = 0;

	rval = mosquitto_publish(inst->mosq, &mid, topic, (int)strlen(payload), payload, qos, retain);
	if (rval != MOSQ_ERR_SUCCESS) {
		debug("Cannot publish: '%s'", mosquitto_strerror(rval));
	}
	assert(rval == 0);

	return rval;
}

/*
int mqtt_subscribe(mqtt_t inst, char* topic) {
	int rval;
	int qos = 0;
	int mid;

	rval = mosquitto_subscribe(inst->mosq, &mid, topic, qos);
	if (rval != MOSQ_ERR_SUCCESS) {
		debug("Cannot subscribe: '%s'", mosquitto_strerror(rval));
	}
	assert(rval == 0);

	return rval;
}
*/

