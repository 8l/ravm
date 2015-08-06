
/*============================================================================
  printhex, a simple od-like utiity.
  Copyright (C) 2012 by Zack T Smith.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  The author may be reached at 1@zsmith.co.
 *===========================================================================*/

#include <stdio.h>
int
main (int c, char **v)
{
	if (c != 2) {
		puts ("Usage: printhex [file]");
		return 0;
	}

	int n = 0;

	FILE *f = fopen (v[1], "rb");
	if (f) {
		int ch;
		while (EOF != (ch = fgetc (f))) {
			if (!(n & 15))
				printf ("%08x: ", n);
			printf ("%02x ", ch);
			n++;
			if (!(n & 15))
				puts ("");
		}
		fclose (f);
	}

	if (n & 15)
		puts ("");

	return 0;
}

