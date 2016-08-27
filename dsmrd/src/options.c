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
#include "ini.h"
#include "options.h"

struct struct_options_t options;

static int run_from_src(char* argv[], /*@out@*/ char** dir) {
	char arg0[PATH_MAX];
	char s[PATH_MAX];
	int rval;

	strncpy(arg0, argv[0], sizeof(arg0));
	*dir = dirname(arg0);
	(void) snprintf(s, sizeof(s), "%s/%s", *dir, __FILE__);
	rval = (access(s, F_OK) != -1) ? 1 : 0;

	return rval;
}

void myfunc(const char* section, const char* key, const char* value, void* data) {
	options_t options = (options_t) data;

	if (section == NULL) {
		if (strcmp(key, "ignore crc") == 0) {
			options->ignore_crc = atoi(value);
		} else {
			if (strcmp(key, "daemonize") == 0) {
				options->daemonize = atoi(value);
			} else {
				if (strcmp(key, "verbose") == 0) {
					options->verbose = atoi(value);
				} else {
					printf("Illegal [global] option (key=%s)\n", key);
				}
			}
		}
	} else {
		if (strcmp(section, "serial") == 0) {
			if (strcmp(key, "parity") == 0) {
				options->parity = atoi(value);
			} else {
				if (strcmp(key, "line") == 0) {
					strncpy(options->tty, value, sizeof(options->tty));
				} else {
					if (strcmp(key, "baud") == 0) {
						options->baud = atoi(value);
					} else {
						if (strcmp(key, "is tty") == 0) {
							options->is_tty = atoi(value);
						} else {
							printf("Illegal [%s] option (key=%s)\n", section, key);
						}
					}
				}
			}
		} else {
			if (strcmp(section, "www") == 0) {
				if (strcmp(key, "port") == 0) {
					options->port = atoi(value);
				} else {
					printf("Illegal [%s] option (key=%s)\n", section, key);
				}
			} else {
				printf("Illegal option (section=%s)\n", section);
			}
		}
	}
}

options_t options_init(int argc, char* argv[]) {
	int opt;
	char* dir;

	memset(&options, 0, sizeof(struct struct_options_t));

	if (run_from_src(argv, &dir) != 0) {
		(void) snprintf(options.wwwdir, sizeof(options.wwwdir), "%s/../www", dir);
	} else {
		(void) snprintf(options.wwwdir, sizeof(options.wwwdir), "%s", WWWDIR);
	}
	options.daemonize = 1;
	options.baud = 115200;
	options.port = 8888;
	options.is_tty = 1;
	strncpy(options.tty, "tty", sizeof(options.tty));

	ini_read("/etc/dsmrd.conf", myfunc, &options);
	ini_read("./dsmrd.conf", myfunc, &options);

	while ((opt = getopt(argc, argv, "cDeol:s:p:tv")) != -1) {
		switch (opt) {
			case 'c':
				options.ignore_crc = 1;
				break;
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

