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
#include <stdio.h>
#include <libgen.h>
#include "options.h"

struct struct_options_t options;

int run_from_src(char* argv[], char** dir) {
	char arg0[PATH_MAX];
	char s[PATH_MAX];
	int rval;

	strncpy(arg0, argv[0], sizeof(arg0));
	*dir = dirname(arg0);
	snprintf(s, sizeof(s), "%s/%s", *dir, __FILE__);
	rval = (access(s, F_OK) != -1);

	return rval;
}

options_t options_init(int argc, char* argv[]) {
	int opt;
	char* dir;

	memset(&options, 0, sizeof(struct struct_options_t));

	if (run_from_src(argv, &dir)) {
		snprintf(options.wwwdir, sizeof(options.wwwdir), "%s/../www", dir);
	} else {
		snprintf(options.wwwdir, sizeof(options.wwwdir), "%s", WWWDIR);
	}
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

