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

//---------------------------------------------------------------------------
// CHANGE LOG
// 0.1 ZS	REVM8: Simple 8-bit opcodes working, ~650 MIPS sans checks.
// 0.2 ZS	Got most instructions working, with bounds checks. ~420 MIPS.
// 0.3 ZS	Added more instructions e.g. reg-reg moves, division etc.
// 0.4 ZS	REVM16: Switchover to 16-bit opcodes begins.
// 0.5 ZS	REVM16: More work on implementing all 16-bit opcodes.
// 0.6 ZS	RXVM: Intermediate VM.
// 0.7 ZS	RYVM 832.
// 0.8 ZS	Settled on RYVM 832, renamed RAVM.
// 0.9 ZS	Added concept of a callout for I/O, also a constants area.
//---------------------------------------------------------------------------

#ifndef _DEFS_H
#define _DEFS_H

#define PROGRAM_NAME "ravm"
#define RELEASE "0.9"

#ifndef bool
typedef char bool;
enum { true = 1, false = 0 };
#endif

#define MAGIC (0xf17471fe)
#define MAX_PROGRAM_LENGTH (3 << 30) // 3 GB arbitrary
#define MAX_DATA_SECTION_LENGTH (400 * 1024 * 1024) // 400 MB arbitrary

#define MINIMUM_MEMORY_MB 1
#define MAXIMUM_MEMORY_MB 3800	/* System-dependent */

typedef uint32_t (Callout) (uint32_t, uint32_t, uint32_t);

extern int Interpret (void *program_start, 
			void *memory_start, 
			void *stack_start, 
			void *program_end, 
			void *memory_end, 
			void *stack_end,
			Callout *callout,
			uint32_t data_start, /* VM pointer */
			uint32_t data_length
			);
enum {
	RESULT_OK = 0,
	RESULT_PROGRAM_BOUNDS = 1,	// Instruction pointer went out of bounds.
	RESULT_MEMORY_BOUNDS = 2,	// Memory access ws out of bounds.
	RESULT_STACK_BOUNDS = 3,	// Stack overflow or underflow.
	RESULT_STACK_UNDERFLOW = 4,
	RESULT_STACK_OVERFLOW = 5,
	RESULT_INVALID_ALLOCA_PARAM = 6,
	RESULT_DIVIDE_BY_ZERO = 7,
	RESULT_CALLOUT_IMPOSSIBLE = 8,
};

#endif

