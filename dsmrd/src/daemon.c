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
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

int daemonize() {
	pid_t pid = 0;
	pid_t sid = 0;
	int rval;

	pid = fork();
	if (pid < 0) {
		printf("fork failed!\n");
		exit(EXIT_FAILURE);
	} else {
		if (pid > 0) {
			printf("pid of child process %d \n", pid);
			exit(EXIT_SUCCESS);
		} else {
			(void) umask(0);

			sid = setsid();
			if(sid < 0) {
				exit(EXIT_FAILURE);
			} else {
				rval = chdir("/");
				if (rval != 0) {
					exit(EXIT_FAILURE);
				} else {
					(void) close(STDIN_FILENO);
					(void) close(STDOUT_FILENO);
					(void) close(STDERR_FILENO);
				}
			}
		}
	}

	return 0;
}

