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
#include <stdio.h>
#include <assert.h>
#include <regex.h>
#include "util.h"
#include "ini.h"

#define BUFMAX 4096

void ini_read(const char* filename, myfunc_t callback, void* data) {
	FILE* fp;
	char buf_with_cr[BUFMAX];
	int rval;
	regmatch_t match[5];
	regex_t eol;
	regex_t strip;
	regex_t empty;
	regex_t comment;
	regex_t key;
	regex_t section;
	char* sect = NULL;
	char sectionstr[4096] = "";

	rval = regcomp(&eol, "^(.*)\n$", REG_EXTENDED);
	assert(rval == 0);
	rval = regcomp(&strip, "^[ \t]*(.*[^ ])[ \t]*$", REG_EXTENDED);
	assert(rval == 0);
	rval = regcomp(&empty, "^[ \t]*$", REG_EXTENDED);
	assert(rval == 0);
	rval = regcomp(&comment, "^;.*$", REG_EXTENDED);
	assert(rval == 0);
	rval = regcomp(&key, "^([^=]*)=(.*)$", REG_EXTENDED);
	assert(rval == 0);
	rval = regcomp(&section, "^\\[(.*)\\] *$", REG_EXTENDED);
	assert(rval == 0);

	fp = fopen(filename, "r");
	if (fp == NULL) {
	} else {
		while (fgets(buf_with_cr, (int)sizeof(buf_with_cr), fp) != NULL) {
			char buf[4096] = "";

			rval = regexec(&eol, buf_with_cr, sizeof(match)/sizeof(match[0]), match, 0);
			if (rval == 0) {
				(void) regsubstr(buf, sizeof(buf), buf_with_cr, match, 1);

				rval = regexec(&empty, buf, sizeof(match)/sizeof(match[0]), match, 0);
				if (rval != 0) {
					rval = regexec(&comment, buf, sizeof(match)/sizeof(match[0]), match, 0);
					if (rval != 0) {
						rval = regexec(&section, buf, sizeof(match)/sizeof(match[0]), match, 0);
						if (rval == 0) {
							char unstripped_sectionstr[4096] = "";

							(void) regsubstr(unstripped_sectionstr, sizeof(unstripped_sectionstr), buf, match, 1);
							rval = regexec(&strip, unstripped_sectionstr, sizeof(match)/sizeof(match[0]), match, 0);
							assert(rval == 0);
							(void) regsubstr(sectionstr, sizeof(sectionstr), unstripped_sectionstr, match, 1);
							sect = sectionstr;
						} else {
							char keystr[4096] = "";
							char valstr[4096] = "";
							char skeystr[4096] = "";
							char svalstr[4096] = "";

							rval = regexec(&key, buf, sizeof(match)/sizeof(match[0]), match, 0);
							if (rval == 0) {
								(void) regsubstr(keystr, sizeof(keystr), buf, match, 1);
								(void) regsubstr(valstr, sizeof(valstr), buf, match, 2);

								rval = regexec(&strip, keystr, sizeof(match)/sizeof(match[0]), match, 0);
								assert(rval == 0);
								(void) regsubstr(skeystr, sizeof(skeystr), keystr, match, 1);

								rval = regexec(&strip, valstr, sizeof(match)/sizeof(match[0]), match, 0);
								assert(rval == 0);
								(void) regsubstr(svalstr, sizeof(svalstr), valstr, match, 1);

								callback(sect, skeystr, svalstr, data);
							} else {
								//printf("ERR: other: %s\n", buf);
							}
						}
					}
				}
			}
		}

		(void) fclose(fp);
	}

	regfree(&eol);
	regfree(&strip);
	regfree(&empty);
	regfree(&comment);
	regfree(&key);
	regfree(&section);
}

