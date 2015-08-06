/*============================================================================
  RAVM, a RISC-approximating virtual machine that fits in the L1 cache.
  Copyright (C) 2012-2013 by Zack T Smith.

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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <wchar.h>

#include "defs.h"

static uint32_t permissions = 0;
static uint32_t memory_size = MINIMUM_MEMORY_MB;

//----------------------------------------------------------------------------
// Name:	error
// Purpose:	Complain and exit.
//----------------------------------------------------------------------------
void error (char *s)
{
#ifndef __WIN32__
	fprintf (stderr, "Error: %s\n", s);
	exit (1);
#else
	wchar_t tmp [200];
	int i;
	for (i = 0; s[i]; i++)
		tmp[i] = s[i];
	tmp[i] = 0;
	MessageBoxW (0, tmp, L"Error", 0);
	ExitProcess (0);
#endif
}

//----------------------------------------------------------------------------
// Name:	mytime
// Purpose:	Reports time in microseconds.
//----------------------------------------------------------------------------
unsigned long mytime ()
{
#ifndef __WIN32__
	struct timeval tv;
	struct timezone tz;
	memset (&tz, 0, sizeof(struct timezone));
	gettimeofday (&tv, &tz);
	return 1000000 * tv.tv_sec + tv.tv_usec;
#else
	return 1000 * GetTickCount ();	// accurate enough.
#endif
}

//----------------------------------------------------------------------------
// Name:	usage
//----------------------------------------------------------------------------
void
usage ()
{
	exit (0);
}

//----------------------------------------------------------------------------
// Name:	callout_function 
// Purpose:	Provides VM program a means to access various services.
//----------------------------------------------------------------------------
uint32_t
callout_function (uint32_t which, uint32_t param1, uint32_t param2)
{
	switch (which) {
	case 0:
		// Request permission.
		return 0;
		break;
	case 1:
		break;
	case 2:
		break;
	default:
		printf ("Invalid callout %08x specified.\n", which);
	}
}

//----------------------------------------------------------------------------
// Name:	main
//----------------------------------------------------------------------------
int
main (int argc, char **argv)
{
	int i, chunk_size;

	permissions = 0;

	--argc;
	++argv;

	char *src = NULL;

	i = 0;
	while (i < argc) {
		char *s = argv [i++];
		if (!strcmp ("--help", s)) {
			usage ();
		}
		else if (i != argc-1 && !strcmp ("--memory", s)) {
			int mb = atoi (argv[i+1]);
			if (mb < MINIMUM_MEMORY_MB)
				mb = MINIMUM_MEMORY_MB;
			else if (mb > MAXIMUM_MEMORY_MB) 
				error ("Too much memory specified (units = megabytes).");
		}
		else {
			if ('-' == *s)
				usage ();
			else
				src = s;
		}
	}

	printf ("This is "PROGRAM_NAME" version "RELEASE".\n");
	printf ("Copyright (C) 2012-2013 by Zack T Smith.\n\n");
	printf ("This software is covered by the GNU Public License.\n");
	printf ("It is provided AS-IS, use at your own risk.\n");
	printf ("See the file COPYING for more information.\n\n");
	fflush (stdout);

	if (!src) 
		error ("No input file.");

	struct stat st;
	if (stat (src, &st)) {
		perror ("Input file");
		return 1;
	}

	//------------------------------
	// Get executable file length.
	//
	unsigned int size = st.st_size;
	FILE *f = fopen (src, "rb");
	if (!f) {
		perror ("Open file");
		return 2;
	}

	//------------------------------
	// Verify magic number present.
	//
	static uint32_t magic = 0;
	if (4 != fread (&magic, 1, 4, f)) {
		error ("Executable truncated.");
	}
	if (magic != MAGIC)
		error ("Program has bad magic number.");

	//------------------------------
	// Read the section sizes.
	//
	static uint32_t sizes [4];
	if (16 != fread (sizes, 1, 16, f)) {
		error ("Executable truncated.");
	}
	uint32_t program_length = sizes[0];
	uint32_t data_length = sizes[1];
	uint32_t aux_length = sizes[2];
	uint32_t aux2_length = sizes[3];
	if (!program_length)
		error ("Program length is zero.");
	if (program_length >= MAX_PROGRAM_LENGTH) 
		error ("Program length is excessive.");
	if (data_length >= MAX_DATA_SECTION_LENGTH)
		error ("Data section length is excessive.");

	char *memory = malloc ((memory_size<<20) + data_length);
	if (!memory) {
		perror (PROGRAM_NAME);
		return -4;
	}
	bzero (memory, memory_size<<20);

	//------------------------------
	// Read program bytes.
	//
	char *program = malloc (program_length);
	if (program_length != fread (program, 1, program_length, f)) {
		perror ("Read file");
		return 3;
	}

	//------------------------------
	// Read data section if any
	// directly into VM memory.
	//
	if (data_length) {
		if (data_length != fread (memory + data_length, 1, data_length, f)) {
			perror ("Read file");
			return 3;
		}
	}

	fclose (f);

#define STACKSIZE 1024
	char *stack = malloc (STACKSIZE);
	bzero (stack, STACKSIZE);

	//--------------------
	// Run the program.
	//
	int retval = Interpret (program, memory, stack, 
				program+size, memory+(memory_size<<20)+data_length, stack+STACKSIZE, 
				callout_function,
				memory_size << 20 /* data section location */, 
				data_length);

	free (program);
	free (memory);
	free (stack);

	//--------------------
	// Interpret results.
	//
	if (retval >= 0x8000) {
		retval &= 0x7fff;
		printf ("Exit code %u.\n", retval & 0x7fff);
		exit (retval);
	}
	else {
		switch (retval) {
			case RESULT_OK:
				puts ("OK."); 
				break;
			case RESULT_PROGRAM_BOUNDS:
				puts ("Program ran out of bounds."); 
				break;
			case RESULT_MEMORY_BOUNDS:
				puts ("Memory access out of bounds."); 
				break;
			case RESULT_STACK_BOUNDS:
				puts ("Stack overflow or underflow."); 
				break;
			case RESULT_STACK_UNDERFLOW:
				puts ("Stack underflow."); 
				break;
			case RESULT_STACK_OVERFLOW:
				puts ("Stack overflow."); 
				break;
			case RESULT_INVALID_ALLOCA_PARAM:
				puts ("Invalid alloc parameter."); 
				break;
			case RESULT_DIVIDE_BY_ZERO:
				puts ("Divide by zero."); 
				break;
			case RESULT_CALLOUT_IMPOSSIBLE:
				puts ("Callout impossible."); 
				break;

			default: printf ("Unknown error %d.\n", retval);
		}
	}
	
	return -retval;
}
