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
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#define FIFO_FILE       "tty"

char* p2msg5 =
"/ISk5\\2MT382-1000\r\n\
\r\n\
1-3:0.2.8(50)\r\n\
0-0:1.0.0(%s)\r\n\
0-0:96.1.1(4B384547303034303436333935353037)\r\n\
1-0:1.8.1(%010.3f*kWh)\r\n\
1-0:1.8.2(%010.3f*kWh)\r\n\
1-0:2.8.1(000000.000*kWh)\r\n\
1-0:2.8.2(000000.000*kWh)\r\n\
0-0:96.14.0(%04d)\r\n\
1-0:1.7.0(%06.3f*kW)\r\n\
1-0:2.7.0(00.000*kW)\r\n\
0-0:96.7.21(00004)\r\n\
0-0:96.7.9(00002)\r\n\
1-0:99.97.0(2)(0-0:96.7.19)(101208152415W)(0000000240*s)(101208151004W)(0000000301*s)\r\n\
1-0:32.32.0(00002)\r\n\
1-0:52.32.0(00001)\r\n\
1-0:72.32.0(00000)\r\n\
1-0:32.36.0(00000)\r\n\
1-0:52.36.0(00003)\r\n\
1-0:72.36.0(00000)\r\n\
0-0:96.13.0(303132333435363738393A3B3C3D3E3F303132333435363738393A3B3C3D3E3F303132333435363738393A3B3C3D3E3F303132333435363738393A3B3C3D3E3F303132333435363738393A3B3C3D3E3F)\r\n\
1-0:32.7.0(%05.1f*V)\r\n\
1-0:52.7.0(%05.1f*V)\r\n\
1-0:72.7.0(%05.1f*V)\r\n\
1-0:31.7.0(%03.0f*A)\r\n\
1-0:51.7.0(%03.0f*A)\r\n\
1-0:71.7.0(%03.0f*A)\r\n\
1-0:21.7.0(%06.3f*kW)\r\n\
1-0:41.7.0(%06.3f*kW)\r\n\
1-0:61.7.0(%06.3f*kW)\r\n\
1-0:22.7.0(00.000*kW)\r\n\
1-0:42.7.0(00.000*kW)\r\n\
1-0:62.7.0(00.000*kW)\r\n\
0-1:24.1.0(003)\r\n\
0-1:96.1.0(3232323241424344313233343536373839)\r\n\
0-1:24.2.1(%s)(12785.123*m3)\r\n\
!EF2F\r\n";

/*
/ISk5\\2MT382-1000
1-3:0.2.8(40)
0-0:1.0.0(101209113020W)
0-0:96.1.1(4B384547303034303436333935353037)
1-0:1.8.1(123456.789*kWh)
1-0:1.8.2(123456.789*kWh)
1-0:2.8.1(123456.789*kWh)
1-0:2.8.2(123456.789*kWh)
0-0:96.14.0(0002)
1-0:1.7.0(01.193*kW)
1-0:2.7.0(00.000*kW)
0-0:17.0.0(016.1*kW)
0-0:96.3.10(1)
0-0:96.7.21(00004)
0-0:96.7.9(00002)
1-0:99.97.0(2)(0-0:96.7.19)(101208152415W)(0000000240*s)(101208151004W)(0000000301*s)
1-0:32.32.0(00002)
1-0:52.32.0(00001)
1-0:72:32.0(00000)
1-0:32.36.0(00000)
1-0:52.36.0(00003)
1-0:72.36.0(00000)
0-0:96.13.1(3031203631203831)
0-0:96.13.0(303132333435363738393A3B3C3D3E3F303132333435363738393A3B3C3D3E3F303132333435363738393A3B3C3D3E3F303132333435363738393A3B3C3D3E3F303132333435363738393A3B3C3D3E3F)
1-0:31.7.0.255(001*A)
1-0:51.7.0.255(002*A)
1-0:71.7.0.255(003*A)
1-0:21.7.0.255(01.111*kW)
1-0:41.7.0.255(02.222*kW)
1-0:61.7.0.255(03.333*kW)
1-0:22.7.0.255(04.444*kW)
1-0:42.7.0.255(05.555*kW)
1-0:62.7.0.255(06.666*kW)
0-1:24.1.0(003)
0-1:96.1.0(3232323241424344313233343536373839)
0-1:24.2.1(101209110000W)(12785.123*m3)
0-1:24.4.0(1)
!E1D6
*/

char* p1msg =
"/KMP5 KA6U001660297912\r\n\
\r\n\
0-0:96.1.1(204B413655303031363630323937393132)\r\n\
1-0:1.8.1(%f*kWh)\r\n\
1-0:1.8.2(00005.000*kWh)\r\n\
1-0:2.8.1(00026.000*kWh)\r\n\
1-0:2.8.2(00001.000*kWh)\r\n\
0-0:96.14.0(0002)\r\n\
1-0:1.7.0(0000.03*kW)\r\n\
1-0:2.7.0(0000.00*kW)\r\n\
0-0:17.0.0(999*A)\r\n\
0-0:96.3.10(1)\r\n\
0-0:96.13.1()\r\n\
0-0:96.13.0()\r\n\
0-1:24.1.0(3)\r\n\
0-1:96.1.0(3238313031353431303034303232323131)\r\n\
0-1:24.3.0(121030140000)(00)(60)(1)(0-1:24.2.1)(m3)\r\n\
(00024.123)\r\n\
0-1:24.4.0(1)\r\n\
!\r\n\
";

double kwh1 = 24.0;
double kwh2 = 18.0;

int main() {
	FILE* fp;
	char buf[4096];
	char timebuf[32];
	//char* buf = p1msg;
	int i;
	double v1, a1c, p1, a1 = 0.0;
	double v2, a2c, p2, a2 = 0.0;
	double v3, a3c, p3, a3 = 0.0;
	double tc;
	double p;
	int t = 0;
	time_t tim = 12000000;
	struct tm* tmp;

	srand(time(0));

	umask(0);
	mknod(FIFO_FILE, S_IFIFO|0666, 0);
	signal(SIGPIPE, SIG_IGN);

	while (1) {
		fp = fopen(FIFO_FILE, "w");
		while (1) {
			v1 = ((double) rand() / RAND_MAX * 1) + 220.0;
			v2 = ((double) rand() / RAND_MAX * 1) + 220.0;
			v3 = ((double) rand() / RAND_MAX * 1) + 220.0;
			a1c = (double) rand() / RAND_MAX * 100;
			a2c = (double) rand() / RAND_MAX * 100;
			a3c = (double) rand() / RAND_MAX * 100;
			if (a1c > 95.0) {
				a1 = (double) rand() / RAND_MAX * 30;
			}
			if (a2c > 95.0) {
				a2 = (double) rand() / RAND_MAX * 30;
			}
			if (a3c > 95.0) {
				a3 = (double) rand() / RAND_MAX * 30;
			}
			p1 = a1 * v1 / 1000;
			p2 = a2 * v2 / 1000;
			p3 = a3 * v3 / 1000;
			tc = (double) rand() / RAND_MAX * 100;
			if (tc > 95.0) {
				t = 1 - t;
			}
			p = p1 + p2 + p3;
			if (t == 0) {
				kwh1 += (p / 1000);
			} else {
				kwh2 += (p / 1000);
			}

			tmp = localtime(&tim);
			strftime(timebuf, sizeof(timebuf), "%y%m%d%H%M%SW", tmp);
			tim += 10;

			//printf("v1=%f\n", v1);
			//printf("a1c=%f\n", a1c);
			//printf("a1=%f\n", a1);
			//printf("p1=%f\n", p1);
			//printf("kwh1=%f\n", kwh1);
			//printf("kwh2=%f\n", kwh2);
			snprintf(buf, sizeof(buf), p2msg5,
					timebuf,
					kwh1,
					kwh2,
					t + 1,
					p,
					v1, v2, v3,
					a1, a2, a3,
					p1, p2, p3,
					timebuf);
			i = fwrite(buf, strlen(buf), 1, fp);
			fflush(fp);
			sleep(1);
		}
		fclose(fp);
	}

	return i;
}

