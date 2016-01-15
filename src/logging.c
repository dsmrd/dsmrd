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

#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <string.h>
#include "logging.h"


struct struct_logging_t {
	int verbose;
	int to_syslog;
	char facility[256];
};

struct struct_logging_t logging;


int logging_init(int to_syslog, int verbose, char* ident, int facility) {
	memset(&logging, 0, sizeof(struct struct_logging_t));
	logging.verbose = verbose;
	if (0 != to_syslog) {
		openlog(ident, 0, facility);
syslog(LOG_ERR, "DSMRd started");
		logging.to_syslog = 1;
	}
	return 0;
}

int logging_exit() {
	if (0 != logging.to_syslog) {
		closelog();
	}
	return 0;
}

int error(const char* fmt, ...) {
	char buf[256];
	va_list ap;

	va_start(ap, fmt);
	if (0 == logging.to_syslog) {
		(void) snprintf(buf, sizeof(buf), "E: %s\n", fmt);
		(void) vprintf(buf, ap);
	} else {
		vsyslog(LOG_ERR, fmt, ap);
	}
	va_end(ap);

	return 0;
}

int warning(const char* fmt, ...) {
	char buf[256];
	va_list ap;

	va_start(ap, fmt);
	if (0 == logging.to_syslog) {
		if (logging.verbose > 0) {
			(void) snprintf(buf, sizeof(buf), "W: %s\n", fmt);
			(void) vprintf(buf, ap);
		}
	} else {
		vsyslog(LOG_WARNING, fmt, ap);
	}
	va_end(ap);

	return 0;
}

int info(const char* fmt, ...) {
	char buf[256];
	va_list ap;

	va_start(ap, fmt);
	if (0 == logging.to_syslog) {
		if (logging.verbose > 1) {
			(void) snprintf(buf, sizeof(buf), "I: %s\n", fmt);
			(void) vprintf(buf, ap);
		}
	} else {
		vsyslog(LOG_INFO, fmt, ap);
	}
	va_end(ap);

	return 0;
}

int debug(const char* fmt, ...) {
	char buf[256];
	va_list ap;

	va_start(ap, fmt);
	if (0 == logging.to_syslog) {
		if (logging.verbose > 2) {
			(void) snprintf(buf, sizeof(buf), "D: %s\n", fmt);
			(void) vprintf(buf, ap);
		}
	} else {
		vsyslog(LOG_DEBUG, fmt, ap);
	}
	va_end(ap);

	return 0;
}

