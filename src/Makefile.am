#
# DSMRd - Dutch Smart Meter Requirements Daemon
# Copyright (C)2016  M.J. de Wit
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

wwwdir = $(pkgdatadir)/www

bin_PROGRAMS = dsmrd
dsmrd_SOURCES = main.c
dsmrd_SOURCES += dsmr.h
dsmrd_SOURCES += dsmr.c
dsmrd_SOURCES += logging.h
dsmrd_SOURCES += logging.c
dsmrd_SOURCES += options.h
dsmrd_SOURCES += options.c
dsmrd_SOURCES += serial.h
dsmrd_SOURCES += serial.c
dsmrd_SOURCES += daemon.h
dsmrd_SOURCES += daemon.c
dsmrd_SOURCES += dispatch.h
dsmrd_SOURCES += dispatch.c
dsmrd_SOURCES += accept.h
dsmrd_SOURCES += accept.c
dsmrd_SOURCES += http.h
dsmrd_SOURCES += http.c
dsmrd_SOURCES += util.h
dsmrd_SOURCES += util.c
dsmrd_SOURCES += crc16.h
dsmrd_SOURCES += crc16.c
dsmrd_SOURCES += ini.h
dsmrd_SOURCES += ini.c
if FEATURE_DNS_SD
dsmrd_SOURCES += dnssd.h
dsmrd_SOURCES += dnssd.c
endif
dsmrd_SOURCES += list.h
dsmrd_SOURCES += list.c
dsmrd_SOURCES += rbtree.h
dsmrd_SOURCES += rbtree.c
dsmrd_SOURCES += rest.h
dsmrd_SOURCES += rest.c
if FEATURE_MQTT
dsmrd_SOURCES += mqtt.h
dsmrd_SOURCES += mqtt.c
endif
dsmrd_SOURCES += database.h
dsmrd_SOURCES += database.c
dsmrd_SOURCES += publish.h
dsmrd_SOURCES += publish.c

noinst_PROGRAMS = p1sim
p1sim_SOURCES = p1sim.c

AM_CPPFLAGS = -Wall -g
AM_CPPFLAGS += -DWWWDIR='"$(wwwdir)"'
AM_CPPFLAGS += -DSYSCONFDIR='"$(sysconfdir)"'
AM_CPPFLAGS += -DLOCALSTATEDIR='"$(localstatedir)"'

