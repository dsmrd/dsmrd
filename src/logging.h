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

#ifndef LOGGING_H
#define LOGGING_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <syslog.h>

int logging_init(int to_syslog, int verbose, char* ident, int facility);
int logging_exit();
void error(const char* fmt, ...);
void warning(const char* fmt, ...);
void info(const char* fmt, ...);
#define debug(...) debug1(__FILE__, __LINE__, __VA_ARGS__)
void debug1(const char* file, int line, const char* fmt, ...);

#endif // LOGGING_H

