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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "util.h"

/*@null@*/ char* regsubstr(char* dest, size_t n, const char* string, regmatch_t pmatch[], int idx) {
    regoff_t eo = pmatch[idx].rm_eo;
    regoff_t so = pmatch[idx].rm_so;
    size_t len = (size_t)(eo - so);
    size_t min = (len < (n-1)) ? len : (n-1);
    if (so == -1) {
		dest[0] = '\0';
		return NULL;
	}
    strncpy(dest, string+so, min);
    dest[min] = '\0';
    return dest;
}

