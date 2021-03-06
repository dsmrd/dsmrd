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

AC_PREREQ([2.69])
AC_INIT([dsmrd], [0.9.2], [info@dsmrd.org])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AM_SILENT_RULES([yes])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

# Conditionals
AC_ARG_ENABLE([dns-sd],
[  --disable-dns-sd        disable DNS-SD autodiscovery (enabled by default, needs Avahi)],
[case "${enableval}" in
  no)  ena_dns_sd=false ;;
  yes) ena_dns_sd=true ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-dns-sd]) ;;
esac], [ena_dns_sd=true])
AM_CONDITIONAL([FEATURE_DNS_SD], [test x$ena_dns_sd = xtrue])
AM_COND_IF([FEATURE_DNS_SD], [AC_DEFINE([FEATURE_DNS_SD], [1], [Define to 1 to enable feature DNS-SD.])])

AC_ARG_ENABLE([mqtt],
[  --disable-mqtt          disable MQTT publish/subscribe (enabled by default, needs Mosquitto)],
[case "${enableval}" in
  no)  ena_mqtt=false ;;
  yes) ena_mqtt=true ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-mqtt]) ;;
esac], [ena_mqtt=true])
AM_CONDITIONAL([FEATURE_MQTT], [test x$ena_mqtt = xtrue])
AM_COND_IF([FEATURE_MQTT], [AC_DEFINE([FEATURE_MQTT], [1], [Define to 1 to enable feature MQTT.])])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_SEARCH_LIBS([cos], [m])
AM_COND_IF([FEATURE_DNS_SD], [
		AC_CHECK_LIB([avahi-common], [avahi_free], [], [AC_MSG_ERROR([No avahi-common library found])])
		AC_CHECK_LIB([avahi-client], [avahi_client_free], [], [AC_MSG_ERROR([No avahi-client library found])])
])
AM_COND_IF([FEATURE_MQTT], [
		AC_CHECK_LIB([mosquitto], [mosquitto_socket], [], [AC_MSG_ERROR([No mosquitto library found])])
])
AC_CHECK_LIB([sqlite3], [sqlite3_open])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([limits.h])
AC_CHECK_HEADERS([netinet/in.h])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADERS([strings.h])
AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([syslog.h])
AC_CHECK_HEADERS([termios.h])
AC_CHECK_HEADERS([unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([regcomp])
AC_CHECK_FUNCS([select])
AC_CHECK_FUNCS([socket])
AC_CHECK_FUNCS([strerror])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 www/Makefile
                 www/images/Makefile])
AC_OUTPUT

