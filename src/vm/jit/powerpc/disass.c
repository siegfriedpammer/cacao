/* src/vm/jit/powerpc/disass.c - wrapper functions for GNU binutils disassembler

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Andreas  Krall
            Reinhard Grafl

   Changes: Stefan Ring
            Christian Thalinger

   $Id: disass.c 4114 2006-01-09 20:19:02Z twisti $

*/


#include "config.h"

#include <dis-asm.h>
#include <stdio.h>

#include "vm/types.h"

#include "vm/jit/disass.h"


char *regs[] = {
	"r0",
	"r1",
	"r2",
	"r3",
	"r4",
	"r5",
	"r6",
	"r7",
	"r8",
	"r9",
	"r10",
	"r11",
	"r12",
	"r13",
	"r14",
	"r15",
	"r16",
	"r17",
	"r18",
	"r19",
	"r20",
	"r21",
	"r22",
	"r23",
	"r24",
	"r25",
	"r26",
	"r27",
	"r28",
	"r29",
	"r30",
	"r31",
};


/* disassinstr *****************************************************************

   Outputs a disassembler listing of one machine code instruction on
   `stdout'.

   code: pointer to machine code

*******************************************************************************/

u1 *disassinstr(u1 *code)
{
	disassemble_info info;

	INIT_DISASSEMBLE_INFO(info, NULL, disass_printf);

	printf("0x%08x:   %08x    ", (s4) code, *((s4 *) code));

	print_insn_big_powerpc((bfd_vma) code, &info);

	printf("\n");

	return code + 4;
}


/* disassemble *****************************************************************

   Outputs a disassembler listing of some machine code on `stdout'.

   start: pointer to first instruction
   end:   pointer to last instruction

*******************************************************************************/

void disassemble(u1 *start, u1 *end)
{
	disassemble_info info;

	INIT_DISASSEMBLE_INFO(info, NULL, disass_printf);

	printf("  --- disassembler listing ---\n");
	for (; start < end; )
		start = disassinstr(start);
}


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
