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

#ifndef SERIAL_H
#define SERIAL_H

#include "dispatch.h"

typedef enum {
    BAUDRATE_38K4 = 0,
} serial_baud_t;

typedef struct struct_serial_t* serial_t;

serial_t serial_init(char* device, serial_baud_t baud, int enable_tc, int (decoder)(char*, ssize_t));
int serial_open(serial_t inst, dispatch_t dis);
int serial_read(void* inst);
int serial_close(void* inst);

#endif // SERIAL_H

