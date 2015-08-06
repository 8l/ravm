/*============================================================================
  rasm, an assembler for RAVM.
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
#include <stdbool.h>

#include "defs.h"

#define ASSEMBLER_NAME "rasm"

#define MAX_LABELS (2000)
#define MAX_LINELEN (1024)
#define MAX_WORDS (MAX_LINELEN/2)

static uint32_t address = 0;
static uint32_t constants_length = 0;
static int pass_number = 0;
static int in_section = 't';

static int n_labels = 0;
static char *labels [MAX_LABELS];
static uint32_t label_addresses [MAX_LABELS];

#define DEST(XX) (((((unsigned)XX) & 255))<<0)
#define SRC(XX) (((((unsigned)XX) & 255))<<8)

enum {
	MAINLOOP = 0<<24,
	OP_DUMP = 1<<24,
	OP_EXIT = 2<<24,
	OP_LOAD16_SIGNED = 3<<24,
	OP_LOAD16_UNSIGNED = 4<<24,
	OP_LOAD32 = 5<<24,
	OP_LOAD8_SIGNED = 6<<24,
	OP_LOAD8_UNSIGNED = 7<<24,
	OP_MOV = 8<<24,
	OP_MOV_IMM16_SIGNED = 9<<24,
	OP_MOV_IMM32 = 10<<24,
	OP_MOV_IMM8_SIGNED = 11<<24,
	OP_STORE16 = 12<<24,
	OP_STORE32 = 13<<24,
	OP_STORE8 = 14<<24,
	OP_WRITE_MEMORY16 = 15<<24,
	OP_WRITE_MEMORY32 = 16<<24,
	OP_WRITE_MEMORY8 = 17<<24,
	OP_SAR = 18<<24,
	OP_SAR_IMM8 = 19<<24,
	OP_SHL = 20<<24,
	OP_SHL_IMM8 = 21<<24,
	OP_SHR = 22<<24,
	OP_SHR_IMM8 = 23<<24,
	OP_ADD = 24<<24,
	OP_ADD_IMM32 = 25<<24,
	OP_ADD_IMM8 = 26<<24,
	OP_DIV = 27<<24,
	OP_DIV_IMM8 = 28<<24,
	OP_IDIV = 29<<24,
	OP_IDIV_IMM8 = 30<<24,
	OP_IMOD = 31<<24,
	OP_IMOD_IMM8 = 32<<24,
	OP_IMUL = 33<<24,
	OP_IMUL_IMM8 = 34<<24,
	OP_MOD = 35<<24,
	OP_MOD_IMM8 = 36<<24,
	OP_MUL = 37<<24,
	OP_MUL_10 = 38<<24,
	OP_MUL_100 = 39<<24,
	OP_MUL_IMM8 = 40<<24,
	OP_NEG = 41<<24,
	OP_SUB = 42<<24,
	OP_SUB_IMM8 = 43<<24,
	OP_LOGICAL_AND = 44<<24,
	OP_LOGICAL_NOT = 45<<24,
	OP_LOGICAL_OR = 46<<24,
	OP_AND = 47<<24,
	OP_AND_IMM8 = 48<<24,
	OP_CLEAR_BIT_IMM8 = 49<<24,
	OP_INVERT_BIT_IMM8 = 50<<24,
	OP_NOT = 51<<24,
	OP_OR = 52<<24,
	OP_OR_IMM8 = 53<<24,
	OP_SET_BIT_IMM8 = 54<<24,
	OP_XOR = 55<<24,
	OP_XOR_IMM8 = 56<<24,
	OP_CALL = 57<<24,
	OP_CALL_REGISTER_INDIRECT = 58<<24,
	OP_CALL_RELATIVE_NEAR_BACKWARD = 59<<24,
	OP_CALL_RELATIVE_NEAR_FORWARD = 60<<24,
	OP_RET = 61<<24,
	OP_ALLOCA = 62<<24,
	OP_DROP = 63<<24,
	OP_GET_STACK_RELATIVE = 64<<24,
	OP_POP = 65<<24,
	OP_PUSH = 66<<24,
	OP_PUT_STACK_RELATIVE = 67<<24,
	OP_DECJNZ = 68<<24,
	OP_DECJNZ_NEAR = 69<<24,
	OP_JA = 70<<24,
	OP_JA_NEAR = 71<<24,
	OP_JAE = 72<<24,
	OP_JAE_NEAR = 73<<24,
	OP_JB = 74<<24,
	OP_JB_NEAR = 75<<24,
	OP_JBE = 76<<24,
	OP_JBE_NEAR = 77<<24,
	OP_JCLEAR = 78<<24,
	OP_JCLEAR_NEAR = 79<<24,
	OP_JE = 80<<24,
	OP_JE_NEAR = 81<<24,
	OP_JG = 82<<24,
	OP_JG_NEAR = 83<<24,
	OP_JGE = 84<<24,
	OP_JGE_NEAR = 85<<24,
	OP_JL = 86<<24,
	OP_JL_NEAR = 87<<24,
	OP_JLE = 88<<24,
	OP_JLE_NEAR = 89<<24,
	OP_JNE = 90<<24,
	OP_JNE_NEAR = 91<<24,
	OP_JNZ = 92<<24,
	OP_JNZ_NEAR = 93<<24,
	OP_JSET = 94<<24,
	OP_JSET_NEAR = 95<<24,
	OP_JUMP = 96<<24,
	OP_JUMP_NEAR = 97<<24,
	OP_JUMP_RELATIVE_NEAR = 98<<24,
	OP_JZ = 99<<24,
	OP_JZ_NEAR = 100<<24,
	OP_LOOP = 101<<24,
	OP_REPEAT = 102<<24,
	OP_PUTCHAR = 103<<24,
	OP_CALLOUT = 104<<24,
	OP_PRINT = 105<<24,
        OP_PRINTHEX = 106<<24
};

enum {
	ERR_USAGE=1,
	ERR_GENERIC=2,
	ERR_SYNTAX=3,
	ERR_INFILE=4,
	ERR_OUTFILE=5,
	ERR_INSTRUCTION=6,
	ERR_UNKNOWN_LABEL=7,
};

void usage (void)
{
	fprintf (stderr, "Usage: rasm input-file output-file\n");
	exit (ERR_USAGE);
}

void error (const char *str)
{
	fprintf (stderr, "Error: %s\n", str);
	exit (ERR_GENERIC);
}

void unknown_label (const char *str)
{
	fprintf (stderr, "Error: Unknown label \"%s\"\n", str);
	exit (ERR_UNKNOWN_LABEL);
}

void syntax (char **words, int n_words)
{
	int i;
	fprintf (stderr, "Syntax error in line: ");
	for (i = 0; i < n_words; i++) {
		fprintf (stderr, "%s ", words[i]);
	}
	fprintf (stderr, "\n");
	exit (ERR_SYNTAX);
}

void add_label (char *s)
{
	if (pass_number != 1) {
		//----------------------------------------
		// If we're not in Pass 1, we're not
		// defining labels any longer.
		//
		return;
	}

	printf ("LABEL %s is @ 0x%x\n", s, address);

	if (n_labels >= MAX_LABELS) 
		error ("Too many labels.");

	labels [n_labels] = strdup (s);
	label_addresses [n_labels] = address;
	n_labels++;
}

int lookup_label (char *s, uint32_t *return_address)
{
	if (pass_number != 2) {
		//----------------------------------------
		// If we're not in Pass 2, we're not
		// generating code and the destination
		// does not matter.
		//
		*return_address = 2000000000;
		return true;
	}

	if (!s || !return_address || !n_labels)
		return false;
	int i = 0;
	while (i < n_labels) {
		if (!strcasecmp (s, labels[i])) {
			*return_address = label_addresses[i];
			return true;
		}
		i++;
	}

	return false;
}

uint32_t parse_number (char *s)
{
//printf ("#=%s\n",s);
	if ('0' == *s && 'x' == s[1]) {
		union { uint32_t u; unsigned i; } u;
		sscanf (s+2, "%x", &u.i);
//printf ("# = 0x%x\n",u.u);
		return u.u;
	}
	if (isdigit ((int) *s)) {
		return atoi (s);
	}
	if ('-' == *s) {
		union { uint32_t u; int i; } u;
		u.i = atoi (s);
		return u.u;
	}
	error ("Bad number expression.");
}

int parse_register (char *s)
{
	if (!s)
		return -1;

	int len = strlen (s);
	if (len <= 1 || len > 4)
		return -1;

	if (!s)
		error ("Bad register expression.");

	if (tolower ((int)*s) != 'r') 
		error ("Bad register expression.");
	
	int reg = atoi (s+1);
	if (reg >= 256 || reg < 0)
		error ("Bad register expression.");

	return reg;
}

void
write_int32 (FILE *f, int v)
{
	if (pass_number == 2) {
		union {
			uint32_t v2;
			int32_t v;
		} u;
		u.v = v;
		fwrite ((void*) &u.v2, 1, 4, f);
	}
	address += 4;
}

void
write_int16 (FILE *f, int v)
{
	if (pass_number == 2) {
		union {
			uint32_t v2;
			int32_t v;
		} u;
		u.v = v;
		fwrite ((void*) &u.v2, 1, 2, f);
	}
	address += 2;
}

void
write_uint32 (FILE *f, uint32_t v)
{
	if (pass_number == 2) {
		unsigned long v2 = v;
		fwrite ((void*) &v2, 1, 4, f);
	}
	address += 4;
}

void
write_uint16 (FILE *f, uint16_t v)
{
	if (pass_number == 2) {
		unsigned short v2 = v;
		fwrite ((void*) &v2, 1, 2, f);
	}
	address += 2;
}

void
write_byte (FILE *f, uint32_t v)
{
	if (pass_number == 2) {
		fputc (v, f);
	}
	address++;
}

void
write_opcode (FILE *f, uint32_t op)
{
	write_uint32 (f, op);
	if (pass_number == 2) 
		; // printf ("Writing opcode %08x\n", op);
}

//-----------------------------------------------------------------------------
// Name:	readline
// Purpose:	Special-purpose readline. Filters out comments, commas.
// Returns:	# chars, or EOF.
//-----------------------------------------------------------------------------

static int
readline (FILE *f, char *line, int max)
{
	int ch=0, ix=0;

	if (!f || !line || max<2)
		return EOF;

	while (EOF != (ch = fgetc (f))) 
	{
		if (ch == '\r')
			continue;
		if (ch == '\n')
			break;
		if (ch == ',')	// Ignore comma char.
			continue;

		if (ch == '#' || ch == ';') {	
			//--------------------
			// Skip past comments.
			//
			while (EOF != (ch = fgetc (f))) {
				if (ch == '\n')
					break;
			}
			break;
		}

		line [ix++] = ch;
		if (ix == max-1)
			break;
	}

	line [ix] = 0;

	if (EOF == ch && !ix)
		return ch;

	return ix;
}

//-----------------------------------------------------------------------------
// Name:	break_line_into_words
// Purpose:	Breaks line into words, destroying the original line.
// Returns:	# words.
// Note:	Words are not strdup'd. Their pointers point into the line.
// Note:	Observes MAX_WORDS.
//-----------------------------------------------------------------------------

int
break_line_into_words (char *line /* Mutable */, char **words_return)
{
	int ch, ix = 0;
	int n_words = 0;
	while (n_words < MAX_WORDS && line [ix]) 
	{
		while (line[ix] && isspace (ch = line[ix])) 
			ix++;

		if (!line[ix])
			break;

		words_return [n_words++] = line + ix;

		while (line[ix] && !isspace (ch = line[ix])) 
			ix++;
		
		if (!line[ix])
			break;

		line [ix++] = 0;
	}
	return n_words;
}

uint32_t calculate_branch32 (char *label)
{
	uint32_t dest;
	if (!lookup_label (label, &dest)) 
		unknown_label (label);

	union {
		uint32_t d;
		int32_t i;
	} u;
	u.d = dest;
	u.i -= address;
//printf ("dest = %x, addr = %x\n", dest, address);
	if (pass_number == 2)
		printf ("BRANCH OFFSET 0x%x\n", (int) u.i);

	return u.d;
}

int
parse_data (char **words, int n_words, FILE *ouf)
{
}

int
parse_instruction (char **words, int n_words, FILE *ouf)
{
	char *word = words[0];

	int dest_reg = -1;
	int src_reg = -1;
	uint32_t immed = 0;
	bool have_immed = false;

	if (n_words > 1) {
		char *param = words[1];
		int len = strlen (param);
		if ('r'==tolower((int)*param) && len <= 4 && isdigit ((int) param[1])) {
			bool skip = false;
			if (len == 3 && !isdigit (param[2]))	// Special case of label like r9x.
				skip = true;
			if (!skip)
				dest_reg = 255 & parse_register (words[1]);
		}
	}
	if (n_words > 2) {
		if ('r'==tolower((int)*words[2]))
			src_reg = 255 & parse_register (words[2]);
		else if (isdigit((int)*words[2]) || '-' == *words[2]) {
			immed = parse_number (words[2]);
			have_immed = true;
		}
	}

	if (!strcasecmp ("exit", word)) {
		if (n_words != 1)
			syntax (words, n_words);
		write_opcode (ouf, OP_EXIT);
	}
	else if (!strcasecmp ("decjnz", word))
	{
		if (n_words != 3)
			syntax (words, n_words);

		uint32_t op = 0;

		switch (word[1]) {
			case 'n': op = OP_JNZ; break;
			case 'z': op = OP_JZ; break;
			case 'e': 
			case 'o': 
				  op = OP_DECJNZ;
				  break;
		}

		address += 4;
		uint32_t rel32 = calculate_branch32 (words[2]);
		address -= 4;

		write_opcode (ouf, op | DEST(dest_reg)); 
		write_uint32 (ouf, rel32);
	}
	else if (!strcasecmp ("decjnznear", word))
	{
		if (n_words != 3)
			syntax (words, n_words);

		uint32_t op = OP_DECJNZ_NEAR;

		address += 4;
		uint32_t rel32 = calculate_branch32 (words[2]);
		address -= 4;

		if (pass_number == 2) {
			if (rel32 < 128) {
				error ("DECJNZNEAR cannot branch forward.");
			}
			else
				if (rel32 > 0xffffff00) {
					rel32 &= 255;
					rel32 = 256 - rel32;
					rel32 &= 255;
					write_opcode (ouf, op | DEST(dest_reg) | SRC(rel32)); 
				} else {
					error ("DECJNZNEAR branch is out of range.");
				}
		}
	}
	else if (!strcasecmp ("calli", word)) {		// Call register-indirect.
		write_opcode (ouf, OP_CALL_REGISTER_INDIRECT | DEST(dest_reg) );
	}
	else if (!strcasecmp ("inc", word)) {
		write_opcode (ouf, OP_ADD_IMM8 | DEST(dest_reg) | SRC(1));
	}
	else if (!strcasecmp ("dec", word)) {
		write_opcode (ouf, OP_SUB_IMM8 | DEST(dest_reg) | SRC(1));
	}
	else if (!strcasecmp ("neg", word)) {
		write_opcode (ouf, OP_NEG | DEST(dest_reg));
	}
	else if (!strcasecmp ("not", word)) {
		write_opcode (ouf, OP_NOT | DEST(dest_reg));
	}
	else if (!strcasecmp ("nop", word)) {
		write_opcode (ouf, MAINLOOP);
	}
	else if (!strcasecmp ("mov", word)) {
		if (dest_reg < 0) 
			syntax (words, n_words);

		if (src_reg >= 0) {
			write_opcode (ouf, OP_MOV + DEST(dest_reg) + SRC(src_reg));
		} else {
			if (immed >= -128 && immed < 128) {
				write_opcode (ouf, OP_MOV_IMM8_SIGNED | DEST(dest_reg) | SRC(immed));
			} 
			else if (immed >= -32768 && immed < 32768) {
				union {
					unsigned int u;
					int i;
				} u;
				u.i = immed;
				u.u &= 32767;
				write_opcode (ouf, OP_MOV_IMM16_SIGNED | u.u); 
			} else {
				printf ("DEST REG %d, immed 0x%x\n", dest_reg,immed);
				write_opcode (ouf, OP_MOV_IMM32 | DEST(dest_reg));
				write_uint32 (ouf, immed);
			}
		}
	}
	else if (!strcasecmp ("repeat", word)) {
		if (n_words != 2) 
			syntax (words, n_words);

		write_opcode (ouf, OP_REPEAT | DEST(dest_reg));
	}
	else if (!strcasecmp ("loop", word)) {
		if (n_words != 3) 
			syntax (words, n_words);

		//-----------------------------
		// dest gets decremented
		// src holds address to jump to.
		//-----------------------------
		write_opcode (ouf, OP_LOOP | DEST(dest_reg) | SRC(src_reg));
	}
	else if (!strcasecmp ("lnot", word)) {
		write_opcode (ouf, OP_LOGICAL_NOT | DEST(dest_reg));
	}
	else if (!strcasecmp ("land", word)) {
		if (dest_reg < 0 || src_reg < 0) 
			syntax (words, n_words);

		write_opcode (ouf, OP_LOGICAL_AND + DEST(dest_reg) + SRC(src_reg));
	}
	else if (!strcasecmp ("lor", word)) {
		if (dest_reg < 0 || src_reg < 0) 
			syntax (words, n_words);

		write_opcode (ouf, OP_LOGICAL_OR + DEST(dest_reg) + SRC(src_reg));
	}
	else if (!strcasecmp ("write8", word)) {
		if (n_words != 3)
			syntax (words, n_words);

		uint32_t dest = parse_number (words[1]);
		uint32_t value = parse_number (words[2]);

		if (value < 256) {
			write_opcode (ouf, OP_WRITE_MEMORY8 + DEST(0) + SRC(value));
			write_uint32 (ouf, dest);
		} 
		else 
			error ("Value too large for write8.");
	}
	else if (!strcasecmp ("write16", word)) {
		if (n_words != 3)
			syntax (words, n_words);

		uint32_t dest = parse_number (words[1]);
		uint32_t value = parse_number (words[2]);

		if (value < 65536) {
			write_opcode (ouf, OP_WRITE_MEMORY16);
			write_uint32 (ouf, dest);
			write_uint32 (ouf, value);
		}
		else
			error ("Value too large for write8.");
	}
	else if (!strcasecmp ("write32", word)) {
		if (n_words != 3)
			syntax (words, n_words);

		uint32_t dest = parse_number (words[1]);
		uint32_t value = parse_number (words[2]);

		write_opcode (ouf, OP_WRITE_MEMORY32);
		write_uint32 (ouf, dest);
		write_uint32 (ouf, value);
	}
	else if (!strcasecmp ("load32", word)) {
		if (dest_reg < 0 || src_reg < 0) 
			syntax (words, n_words);

		write_opcode (ouf, OP_LOAD32 + DEST(dest_reg) + SRC(src_reg));
	}
	else if (!strcasecmp ("store32", word)) {
		if (dest_reg < 0 || src_reg < 0) 
			syntax (words, n_words);

		write_opcode (ouf, OP_STORE32 + DEST(dest_reg) + SRC(src_reg));
	}
	else if (!strcasecmp ("store16", word)) {
		if (dest_reg < 0 || src_reg < 0) 
			syntax (words, n_words);

		write_opcode (ouf, OP_STORE16 + DEST(dest_reg) + SRC(src_reg));
	}
	else if (!strcasecmp ("store8", word)) {
		if (dest_reg < 0 || src_reg < 0) 
			syntax (words, n_words);

		write_opcode (ouf, OP_STORE8 + DEST(dest_reg) + SRC(src_reg));
	}
	else if (!strcasecmp ("load8s", word)) {
		if (dest_reg < 0 || src_reg < 0) 
			syntax (words, n_words);

		write_opcode (ouf, OP_LOAD8_SIGNED + DEST(dest_reg) + SRC(src_reg));
	}
	else if (!strcasecmp ("load8", word)) {
		if (dest_reg < 0 || src_reg < 0) 
			syntax (words, n_words);

		write_opcode (ouf, OP_LOAD8_UNSIGNED + DEST(dest_reg) + SRC(src_reg));
	}
	else if (!strcasecmp ("load16s", word)) {
		if (dest_reg < 0 || src_reg < 0) 
			syntax (words, n_words);

		write_opcode (ouf, OP_LOAD16_SIGNED + DEST(dest_reg) + SRC(src_reg));
	}
	else if (!strcasecmp ("load16", word)) {
		if (dest_reg < 0 || src_reg < 0) 
			syntax (words, n_words);

		write_opcode (ouf, OP_LOAD16_UNSIGNED + DEST(dest_reg) + SRC(src_reg));
	}
	else if (!strcasecmp ("set", word)) {
		if (dest_reg < 0 || src_reg >= 0) 
			syntax (words, n_words);

		if (immed >= 32) 
			error ("Bit number is too large.");

		write_opcode (ouf, OP_SET_BIT_IMM8 | DEST(dest_reg) | SRC(immed));
	}
	else if (!strcasecmp ("clear", word)) {
		if (dest_reg < 0 || src_reg >= 0) 
			syntax (words, n_words);

		if (immed >= 32) 
			error ("Bit number is too large.");

		write_opcode (ouf, OP_CLEAR_BIT_IMM8 | DEST(dest_reg) | SRC(immed));
	}
	else if (!strcasecmp ("invert", word)) {
		if (dest_reg < 0 || src_reg >= 0) 
			syntax (words, n_words);

		if (immed >= 32) 
			error ("Bit number is too large.");

		write_opcode (ouf, OP_INVERT_BIT_IMM8 | DEST(dest_reg) | SRC(immed));
	}
	else if (!strcasecmp ("callout", word)) {
		if (n_words != 4 || dest_reg < 0 || src_reg < 0) 
			syntax (words, n_words);

		uint32_t func_number = parse_number (words[3]);

		if (func_number >= 256) {
			error ("Callout immediate value is too large.");
		} 
		write_opcode (ouf, OP_CALLOUT | DEST(dest_reg) | SRC(src_reg) | (func_number << 16));
	}
	else if (!strcasecmp ("add", word)) {
		if (dest_reg < 0) 
			syntax (words, n_words);

		if (src_reg >= 0) {
			write_opcode (ouf, OP_ADD + DEST(dest_reg) + SRC(src_reg));
		} else {
			if (immed >= 256) {
				error ("Immediate value is too large.");
			} 
			write_opcode (ouf, OP_ADD_IMM8 | DEST(dest_reg) | SRC(immed));
		}
	}
	else if (!strcasecmp ("sub", word)) {
		if (dest_reg < 0) 
			syntax (words, n_words);

		if (src_reg >= 0) {
			write_opcode (ouf, OP_SUB + DEST(dest_reg) + SRC(src_reg));
		} else {
			if (immed >= 256) {
				error ("Immediate value is too large.");
			} 
			write_opcode (ouf, OP_SUB_IMM8 | DEST(dest_reg) | SRC(immed));
		}
	}
	else if (!strcasecmp ("and", word)) {
		if (dest_reg < 0) 
			syntax (words, n_words);

		if (src_reg >= 0) {
			write_opcode (ouf, OP_AND + DEST(dest_reg) + SRC(src_reg));
		} else {
			if (immed >= 256) {
				error ("Immediate value is too large.");
			} 
			write_opcode (ouf, OP_AND_IMM8 | DEST(dest_reg) | SRC(immed));
		}
	}
	else if (!strcasecmp ("xor", word)) {
		if (dest_reg < 0) 
			syntax (words, n_words);

		if (src_reg >= 0) {
			write_opcode (ouf, OP_XOR + DEST(dest_reg) + SRC(src_reg));
		} else {
			if (immed >= 256) {
				error ("Immediate value is too large.");
			} 
			write_opcode (ouf, OP_XOR_IMM8 | DEST(dest_reg) | SRC(immed));
		}
	}
	else if (!strcasecmp ("or", word)) {
		if (dest_reg < 0) 
			syntax (words, n_words);

		if (src_reg >= 0) {
			write_opcode (ouf, OP_OR + DEST(dest_reg) + SRC(src_reg));
		} else {
			if (immed >= 256) {
				error ("Immediate value is too large.");
			} 
			write_opcode (ouf, OP_OR_IMM8 | DEST(dest_reg) | SRC(immed));
		}
	}
	else if (!strcasecmp ("get", word)) {
		if (dest_reg < 0) 
			syntax (words, n_words);

		write_opcode (ouf, OP_GET_STACK_RELATIVE | DEST(dest_reg) | SRC(immed));
	}
	else if (!strcasecmp ("put", word)) {
		if (dest_reg < 0) 
			syntax (words, n_words);

		write_opcode (ouf, OP_PUT_STACK_RELATIVE | DEST(dest_reg) | SRC(immed));
	}
	else if (!strcasecmp ("alloca", word)) {
		if (n_words != 2 || dest_reg >= 0) 
			syntax (words, n_words);

		immed = parse_number (words[1]);

		write_opcode (ouf, OP_ALLOCA | SRC(immed));
	}
	else if (!strcasecmp ("drop", word)) {
		if (n_words != 2 || dest_reg >= 0) 
			syntax (words, n_words);

		immed = parse_number (words[1]);

		write_opcode (ouf, OP_DROP | SRC(immed));
	}
	else if (!strcasecmp ("print", word)) {
		if (n_words != 3 || dest_reg < 0 || src_reg >= 0)
			syntax (words, n_words);
		uint32_t instruction = OP_PRINT | DEST(dest_reg) | SRC(immed);
		write_opcode (ouf, instruction);
	}
	else if (!strcasecmp ("printhex", word)) {
		if (n_words != 3 || dest_reg < 0 || src_reg >= 0)
			syntax (words, n_words);
		uint32_t instruction = OP_PRINTHEX | DEST(dest_reg) | SRC(immed);
		write_opcode (ouf, instruction);
	}
	else if (!strcasecmp ("putchar", word)) {
		if (n_words != 2 || dest_reg < 0)
			syntax (words, n_words);
		uint32_t instruction = OP_PUTCHAR | DEST(dest_reg);
		write_opcode (ouf, instruction);
	}
	else if (!strcasecmp ("push", word)) {
		uint32_t instruction = OP_PUSH | DEST(dest_reg);
		write_opcode (ouf, instruction);
	}
	else if (!strcasecmp ("pop", word)) {
		write_opcode (ouf, OP_POP | DEST(dest_reg));
	}
	else if (!strcasecmp ("mul", word)) {
		if (dest_reg < 0) 
			syntax (words, n_words);

		if (src_reg >= 0) {
			write_opcode (ouf, OP_MUL | DEST(dest_reg) | SRC(src_reg) );
		} else {
			if (immed >= 256) {
				error ("Immediate value is too large.");
			} 
			write_opcode (ouf, OP_MUL_IMM8 | DEST(dest_reg) | SRC(immed));
		}
	}
	else if (!strcasecmp ("imul", word)) {
		if (dest_reg < 0) 
			syntax (words, n_words);

		if (src_reg >= 0) {
			write_opcode (ouf, OP_IMUL + DEST(dest_reg) + SRC(src_reg));
		} else {
			if (immed >= 256) {
				error ("Immediate value is too large.");
			} 
			write_opcode (ouf, OP_IMUL_IMM8 | DEST(dest_reg) | SRC(immed));
		}
	}
	else if (!strcasecmp ("shl", word)) {
		if (dest_reg < 0) 
			syntax (words, n_words);

		if (src_reg >= 0) {
			write_opcode (ouf, OP_SHL + DEST(dest_reg) + SRC(src_reg));
		} else {
			if (immed >= 256) {
				error ("Immediate value is too large.");
			} 
			write_opcode (ouf, OP_SHL_IMM8 | DEST(dest_reg) | SRC(immed));
		}
	}
	else if (!strcasecmp ("shr", word)) {
		if (dest_reg < 0) 
			syntax (words, n_words);

		if (src_reg >= 0) {
			write_opcode (ouf, OP_SHR + DEST(dest_reg) + SRC(src_reg));
		} else {
			if (immed >= 256) {
				error ("Immediate value is too large.");
			} 
			write_opcode (ouf, OP_SHR_IMM8 | DEST(dest_reg) | SRC(immed));
		}
	}
	else if (!strcasecmp ("sar", word)) {
		if (dest_reg < 0) 
			syntax (words, n_words);

		if (src_reg >= 0) {
			write_opcode (ouf, OP_SAR + DEST(dest_reg) + SRC(src_reg));
		} else {
			if (immed >= 256) {
				error ("Immediate value is too large.");
			} 
			write_opcode (ouf, OP_SAR_IMM8 | DEST(dest_reg) | SRC(immed));
		}
	}
	else if (!strcasecmp ("div", word)) {
		if (dest_reg < 0) 
			syntax (words, n_words);

		if (src_reg >= 0) {
			write_opcode (ouf, OP_DIV + DEST(dest_reg) + SRC(src_reg));
		} else {
			if (immed >= 256) {
				error ("Immediate value is too large.");
			} 
			write_opcode (ouf, OP_DIV_IMM8 | DEST(dest_reg) | SRC(immed));
		}
	}
	else if (!strcasecmp ("idiv", word)) {
		if (dest_reg < 0) 
			syntax (words, n_words);

		if (src_reg >= 0) {
			write_opcode (ouf, OP_IDIV + DEST(dest_reg) + SRC(src_reg));
		} else {
			if (immed >= 128 && immed < 0xffffff80) {
				error ("Immediate value is too large.");
			} 
			write_opcode (ouf, OP_IDIV_IMM8 | DEST(dest_reg) | SRC(immed));
		}
	}
	else if (!strcasecmp ("mod", word)) {
		if (dest_reg < 0) 
			syntax (words, n_words);

		if (src_reg >= 0) {
			write_opcode (ouf, OP_MOD + DEST(dest_reg) + SRC(src_reg));
		} else {
			if (immed >= 256) {
				error ("Immediate value is too large.");
			} 
			write_opcode (ouf, OP_MOD_IMM8 | DEST(dest_reg) | SRC(immed));
		}
	}
	else if (!strcasecmp ("imod", word)) {
		if (dest_reg < 0) 
			syntax (words, n_words);

		if (src_reg >= 0) {
			write_opcode (ouf, OP_IMOD | DEST(dest_reg) | SRC(src_reg));
		} else {
			if (immed >= 128 && immed < 0xffffff80) {
				error ("Immediate value is too large.");
			} 
			write_opcode (ouf, OP_IMOD_IMM8 | DEST(dest_reg) | SRC(immed));
		}
	}
	else if (!strcasecmp ("jb", word)
			|| !strcasecmp ("jnz", word)
			|| !strcasecmp ("jz", word)
			|| !strcasecmp ("jb", word)
			|| !strcasecmp ("ja", word)
			|| !strcasecmp ("jae", word)
			|| !strcasecmp ("jl", word)
			|| !strcasecmp ("jle", word)
			|| !strcasecmp ("jg", word)
			|| !strcasecmp ("jge", word)
			|| !strcasecmp ("je", word)
			|| !strcasecmp ("jne", word))
	{
		if (n_words != 4) 
			syntax (words, n_words);

		int base = 0;
		if (!strcasecmp ("jz", word)) base = OP_JZ;
		else if (!strcasecmp ("jnz", word)) base = OP_JNZ;
		else if (!strcasecmp ("jb", word)) base = OP_JB;
		else if (!strcasecmp ("jbe", word)) base = OP_JBE;
		else if (!strcasecmp ("ja", word)) base = OP_JA;
		else if (!strcasecmp ("jae", word)) base = OP_JAE;
		else if (!strcasecmp ("jl", word)) base = OP_JL;
		else if (!strcasecmp ("jle", word)) base = OP_JLE;
		else if (!strcasecmp ("jg", word)) base = OP_JG;
		else if (!strcasecmp ("jge", word)) base = OP_JGE;
		else if (!strcasecmp ("je", word)) base = OP_JE;
		else if (!strcasecmp ("jne", word)) base = OP_JNE;
		else
			error ("Invalid branch operation.");

		write_opcode (ouf, base + DEST(dest_reg) + SRC(src_reg));

		uint32_t dest = calculate_branch32 (words[2]);
		write_uint32 (ouf, dest);
	}
	else if (!strcasecmp ("jbnear", word)
			|| !strcasecmp ("jnznear", word)
			|| !strcasecmp ("jznear", word)
			|| !strcasecmp ("jbnear", word)
			|| !strcasecmp ("janear", word)
			|| !strcasecmp ("jaenear", word)
			|| !strcasecmp ("jlnear", word)
			|| !strcasecmp ("jlenear", word)
			|| !strcasecmp ("jgnear", word)
			|| !strcasecmp ("jgenear", word)
			|| !strcasecmp ("jenear", word)
			|| !strcasecmp ("jnenear", word))
	{
		if (n_words != 4) 
			syntax (words, n_words);

		int op = 0;
		if (!strcasecmp ("jznear", word)) op = OP_JZ;
		else if (!strcasecmp ("jnznear", word)) op = OP_JNZ;
		else if (!strcasecmp ("jbnear", word)) op = OP_JB;
		else if (!strcasecmp ("jbenear", word)) op = OP_JBE;
		else if (!strcasecmp ("janear", word)) op = OP_JA;
		else if (!strcasecmp ("jaenear", word)) op = OP_JAE;
		else if (!strcasecmp ("jlnear", word)) op = OP_JL;
		else if (!strcasecmp ("jlenear", word)) op = OP_JLE;
		else if (!strcasecmp ("jgnear", word)) op = OP_JG;
		else if (!strcasecmp ("jgenear", word)) op = OP_JGE;
		else if (!strcasecmp ("jenear", word)) op = OP_JE;
		else if (!strcasecmp ("jnenear", word)) op = OP_JNE;
		else
			error ("Invalid branch operation.");

		address += 4;
		uint32_t rel32 = calculate_branch32 (words[2]);
		address -= 4;

		if (pass_number == 1) {
			write_opcode (ouf, 0); // Write dummy instruction.
		}
		else {
			if (rel32 < 128 || rel32 > 0xffffff80) {
				write_opcode (ouf, op | DEST(dest_reg) | SRC(rel32)); 
			} else {
				error ("Near branch is out of range.");
			}
		}
	}
	else if (!strcasecmp ("callnearf", word) 	// Call near forward
			|| !strcasecmp ("callf", word)) 
	{
		if (n_words != 2) 
			syntax (words, n_words);

		address += 4;
		uint32_t rel32 = calculate_branch32 (words[1]);
		address -= 4;

		if (rel32 < 256) {
			write_opcode (ouf, OP_CALL_RELATIVE_NEAR_FORWARD | SRC(rel32));
		} else if (rel32 >= 0xffffff00) {
			error ("Call forward used instead of call backward.");
		} else {
			if (pass_number == 2)
				error ("Call branch out of range.");

			write_opcode (ouf, 0); // Write dummy instruction.
		}
	}
	else if (!strcasecmp ("callnearb", word) 	// Call near backward
			|| !strcasecmp ("callb", word)) 
	{
		if (n_words != 2) 
			syntax (words, n_words);

		address += 4;
		uint32_t rel32 = calculate_branch32 (words[1]);
		address -= 4;

		if (rel32 < 256) {
			error ("Call backward used instead of call forward.");
		} else if (rel32 >= 0xffffff00) {
			rel32 &= 255;
			rel32 = 256 - rel32;
			rel32 &= 255;
			write_opcode (ouf, OP_CALL_RELATIVE_NEAR_BACKWARD | SRC(rel32));
		} else {
			if (pass_number == 2)
				error ("Call branch out of range.");

			write_opcode (ouf, 0); // Write dummy instruction.
		}
	}
	else if (!strcasecmp ("call", word)) {
		if (n_words != 2) 
			syntax (words, n_words);

		address += 4;
		uint32_t rel32 = calculate_branch32 (words[1]);
		address -= 4;

		write_opcode (ouf, OP_CALL);
		write_uint32 (ouf, rel32);
	}
	else if (!strcasecmp ("ret", word)) {
		if (n_words != 1) 
			syntax (words, n_words);
		write_opcode (ouf, OP_RET);
	}
	else if (!strcasecmp ("dump", word)) 
	{
		if (n_words != 1)
			syntax (words, n_words);
		write_opcode (ouf, OP_DUMP);
	}
	else if (!strcasecmp ("jmp", word)) {	// Absolute.
		if (n_words != 2) 
			syntax (words, n_words);

		write_opcode (ouf, OP_JUMP);
		uint32_t rel32 = calculate_branch32 (words[1]);
		write_uint32 (ouf, rel32);
	}
	else {
		fprintf (stderr, "Unknown instruction %s.\n", word);
		exit (ERR_INSTRUCTION);
	}

	return 0;
}

int
process (FILE *inf, FILE *ouf)
{
	if (pass_number != 1 && pass_number != 2)
		return false;

	int line_number = 0;
	char line [MAX_LINELEN];
	char *words [MAX_WORDS];

	int len=0;
	while (EOF != (len = readline (inf, line, 255))) 
	{
		line_number++; // Starts at 1.

		int n_words = break_line_into_words (line, words);
		if (!n_words)
			continue;

		int first_word = 0;
		char *word = words[0], *s;

		if (s = strchr (word, ':')) 
		{
			*s = 0; // Remove colon char.

			add_label (word);
			// first_word++;
			if (n_words == 1)
				continue;
			int i; 
			// free (word);
			for (i = 0; i < n_words-1; i++)
				words[i] = words[i+1];
			n_words--;
		}

		if (pass_number == 2) {
			printf ("@%08lx: ", (unsigned long)address);
			printf ("LINE %d: ", line_number);
			int i;
			for (i = 0; i < n_words; i++) 
				printf ("%s ", words[i]);
			puts ("");
		}

		word = words [first_word];
		if (!strcasecmp ("section", word)) {
			if (n_words != 2)
				syntax (words, n_words);
			if (!strcasecmp (words[1], "text")) {
				in_section = 't';
			}
			else if (!strcasecmp (words[1], "data")) {
				in_section = 'd';
			}
			else
				error ("Invalid section type.");
			continue;
		}

		switch (in_section) {
		case 't':
			parse_instruction (words, n_words, ouf);
			break;
		case 'd':
			parse_data (words, n_words, ouf);
			break;
		}

	}
	return 0;
}

int
main (int argc, const char **argv)
{
	if (argc < 2 || argc > 3)
		usage ();

        printf ("This is "ASSEMBLER_NAME" version "RELEASE".\n");
        printf ("Copyright (C) 2012-2013 by Zack T Smith.\n\n");
        printf ("This software is covered by the GNU Public License.\n");
        printf ("It is provided AS-IS, use at your own risk.\n");
        printf ("See the file COPYING for more information.\n\n");
        fflush (stdout);

	const char *inpath = argv[1];
	const char *outpath = argc == 3 ? argv[2] : "out.dat";

	FILE *inf = fopen (inpath, "rb");
	if (!inf) {
		perror (ASSEMBLER_NAME);
		exit (ERR_INFILE);
	}

	//----------------------------------------
	// Pass 1: Determine addresses of labels.
	//
	puts ("Pass 1");
	address = 0;
	pass_number = 1;
	process (inf, NULL);
	fclose (inf);

	inf = fopen (inpath, "rb");
	if (!inf) {
		perror (ASSEMBLER_NAME);
		exit (ERR_INFILE);
	}

	//------------------------------
	// Pass 2: Generate machine code.
	//
	puts ("\nPass 2");
	FILE *ouf = fopen (outpath, "wb");
	if (!ouf) {
		perror (ASSEMBLER_NAME);
		exit (ERR_OUTFILE);
	}

	// Write magic number.
	static uint32_t magic = MAGIC;
	fwrite (&magic, 1, 4, ouf);

	uint32_t sizes [4];
	sizes[0] = address;
	sizes[1] = constants_length;
	sizes[2] = 0;
	sizes[3] = 0;
	// Write section lengths.
	fwrite (sizes, 1, 16, ouf);

	// Write program bytes.
	address = 0;
	pass_number = 2;
	process (inf, ouf);

	return 0;
}

