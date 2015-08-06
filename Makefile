
#============================================================================
#  RAVM, a RISC-inspired virtual machine that fits in the L1 cache.
#  Copyright (C) 2012-2013 by Zack T Smith.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
#  The author may be reached at 1@zsmith.co.
#============================================================================
 

SRC=main.c
OBJ=main.o
TARGET=ravm
AS=yasm 
ASMSRC=interpreter-x86.asm
ASMOBJ=interpreter-x86.o

${TARGET}:	${ASMSRC} main.c
	${AS} -f macho ${ASMSRC} -o ${ASMOBJ}
	gcc -m32 ${SRC} -o ${TARGET} ${ASMOBJ}

clean:
	rm -f ${ASMOBJ} rasm revm
	rm -rf *.dSYM *.dat

rasm:	assembler.c
	gcc -g assembler.c -o rasm

printhex:	printhex.c
	gcc printhex.c -o printhex

tests:
	gcc maketest.c -o maketest
	./maketest

