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

#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include "options.h"

static struct struct_options_t options;

options_t options_init(int argc, char* argv[]) {
	int opt;

	memset(&options, 0, sizeof(struct struct_options_t));
	options.daemonize = 1;
	options.baud = 115200;
	options.port = 8888;
	options.is_tty = 1;
	strncpy(options.tty, "tty", sizeof(options.tty));

	while ((opt = getopt(argc, argv, "Deol:s:p:tv")) != -1) {
		switch (opt) {
			case 'D':
				options.daemonize = 0;
				break;
			case 'e':
				options.parity = PARITY_EVEN;
				break;
			case 'o':
				options.parity = PARITY_ODD;
				break;
			case 'l':
				strncpy(options.tty, optarg, sizeof(options.tty));
				break;
			case 's':
				options.baud = atoi(optarg);
				break;
			case 'p':
				options.port = atoi(optarg);
				break;
			case 't':
				options.is_tty = 0;
				break;
			case 'v':
				options.verbose++;
				break;
		}
	}

	return &options;
}

