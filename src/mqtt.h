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

#ifndef MQTT_H
#define MQTT_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "dispatch.h"

typedef struct mqtt_struct_t* mqtt_t;

/*@null@*/ mqtt_t mqtt_init();
void mqtt_exit(mqtt_t);
int mqtt_publish(mqtt_t inst, char* topic, char* payload);
//int mqtt_subscribe(mqtt_t inst, char* topic);
int mqtt_open(mqtt_t inst, dispatch_t d, const char* name, const char* host, int port, int keepalive);

#endif // MQTT_H

