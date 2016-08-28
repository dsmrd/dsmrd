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

#ifndef OPTIONS_H
#define OPTIONS_H

#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 256
#endif // PATH_MAX

typedef enum {
	B9K6,
	B19K2,
	B38K4,
	B115K2,
} options_baud_t;

typedef enum {
	SA1, // 1 start bit
} options_startbits_t;

typedef enum {
	D8, // 8 data bits
} options_databits_t;

typedef enum {
	PARITY_NONE, // no parity
	PARITY_EVEN,
	PARITY_ODD
} options_parity_t;

typedef enum {
	SO1, // 1 stop bit
} options_stopbits_t;

struct struct_options_t {
	int verbose;
	int daemonize;
	char tty[256];
	int baud;
	int port;
	int is_tty;
	int ignore_crc;
	options_startbits_t startbits;
	options_parity_t parity;
	options_stopbits_t stopbits;
    char wwwdir[PATH_MAX];
};

typedef struct struct_options_t* options_t;

extern struct struct_options_t options;

options_t options_init(int argc, char* argv[]);

#endif // OPTIONS_H

