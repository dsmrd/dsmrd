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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "serial.h"
#include "logging.h"
#include "dispatch.h"

#define _POSIX_SOURCE 1 /* POSIX compliant source */

struct struct_serial_t {
	int fd;
	int (*decoder)(char*, ssize_t);
	struct termios newtio;
	struct termios oldtio;
	char device[256];
	int is_tty;
	dispatch_t dis;
};

static struct baudmap_t {
	int baud;
	int bits;
} baudmap[] = {
	{ 50,     B50     },
	{ 75,     B75     },
	{ 110,    B110    },
	{ 134,    B134    },
	{ 150,    B150    },
	{ 200,    B200    },
	{ 200,    B300    },
	{ 600,    B600    },
	{ 1200,   B1200   },
	{ 1800,   B1800   },
	{ 2400,   B2400   },
	{ 4800,   B4800   },
	{ 9600,   B9600   },
	{ 19200,  B19200  },
	{ 38400,  B38400  },
	{ 57600,  B57600  },
	{ 115200, B115200 },
	{ 230400, B230400 },
};


static int serial_read(void* inst);
static int serial_close(void* inst);


serial_t serial_init(char* device, serial_baud_t baud, int is_tty, int (decoder)(char*, ssize_t)) {
	serial_t inst;
	int i;
	int b=-1;

	info("Initializing P1 driver");

	inst = calloc(sizeof(struct struct_serial_t), 1);
	if (inst != NULL) {
		for (i=0; i<(int)(sizeof(baudmap)/sizeof(struct baudmap_t)); i++) {
			if (baudmap[i].baud == baud) {
				b = baudmap[i].bits;
				break;
			}
		}
		if (b == -1) {
			error("Cannot handle baudrate setting");
			free(inst);
			inst = NULL;
		} else {
			strncpy(inst->device, device, sizeof(inst->device));
			inst->newtio.c_cflag = (tcflag_t)(b | CS8 | CLOCAL | CREAD);
			inst->newtio.c_iflag = (tcflag_t)(IGNPAR | IGNBRK | IXANY);
			inst->newtio.c_oflag = 0;
			inst->newtio.c_lflag = 0;
			inst->newtio.c_cc[VTIME] = (cc_t)10;
			inst->newtio.c_cc[VMIN] = (cc_t)1;
			inst->decoder = decoder;
			inst->is_tty = is_tty;
		}
	}

	return inst;
}

int serial_open(serial_t inst, dispatch_t dis) {
	int rval;

	info("Opening serial port");
	inst->fd = open(inst->device, O_RDONLY | O_NOCTTY | O_NONBLOCK );
	if (inst->fd < 0) {
		error("Cannot open serial line: %s", strerror(errno));
		rval = -1;
	} else {
		if (inst->is_tty != 0) {
			rval = tcgetattr(inst->fd, &(inst->oldtio));
			if (rval != 0) {
				if (errno == ENOTTY) {
					warning("Cannot retrieve serial line settings: %s", strerror(errno));
					warning("Not a serial device? Disable using -t");
					rval = 0;
				} else {
					error("Cannot retrieve serial line settings: %s", strerror(errno));
				}
			} else {
				rval = tcflush(inst->fd, TCIFLUSH);
				if (rval != 0) {
					error("Cannot flush serial line: %s", strerror(errno));
				} else {
					rval = tcsetattr(inst->fd, TCSANOW, &(inst->newtio));
					if (rval != 0) {
						error("Cannot set serial line settings: %s", strerror(errno));
					}
				}
			}
		}

		rval = dispatch_register(dis, inst->fd, serial_read, NULL, NULL, serial_close, inst);
		if (rval != 0) {
			error("Cannot register serial port to dispatcher");
		}

		inst->dis = dis;
	}

	return rval;
}

static int serial_read(void* data) {
	serial_t inst = (serial_t) data;
	char buf[255];
	int rval;
	ssize_t sz;

	sz = read(inst->fd, buf, sizeof(buf)-1);
	if (sz < 0) {
		error("Cannot read from serial line: %s", strerror(errno));
		rval = -1;
	} else {
		if (sz > 0) {
			buf[sz]='\0';
			rval = inst->decoder(buf, sz);
		} else {
			error("Cannot read from serial line: no data available");
			rval = -1;
		}
	}

	return rval;
}

static int serial_close(void* data) {
	serial_t inst = (serial_t) data;
	int rval;

	(void) dispatch_unregister(inst->dis, inst);

	if (inst->is_tty != 0) {
		rval = tcsetattr(inst->fd, TCSANOW, &(inst->oldtio));
		if (rval < 0) {
			error("Cannot restore serial line settings: %s", strerror(errno));
		}
	}

	(void) close(inst->fd);

	free(inst);
	inst = NULL;

	info("Stopped P1 driver");

	return 0;
}

