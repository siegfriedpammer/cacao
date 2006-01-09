/* src/vm/jit/x86_64/disass.c - wrapper functions for GNU binutils disassembler

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

   Changes: Christian Thalinger

   $Id: disass.c 4112 2006-01-09 19:21:25Z twisti $

*/


#include "config.h"

#include <stdio.h>

#include "vm/types.h"

#include "mm/memory.h"
#include "vm/jit/disass.h"


/* global variables ***********************************************************/

/* name table for 16 integer registers */

char *regs[] = {
	"rax",
	"rcx",
	"rdx",
	"rbx",
	"rsp",
	"rbp",
	"rsi",
	"rdi",
    "r8",
    "r9",
    "r10",
    "r11",
    "r12",
    "r13",
    "r14",
    "r15"
};


/* disassinstr *****************************************************************

   Outputs a disassembler listing of one machine code instruction on
   `stdout'.

   code: pointer to machine code

*******************************************************************************/

u1 *disassinstr(u1 *code)
{
	static disassemble_info info;
	static int dis_initialized;
	s4 seqlen;
	s4 i;

	if (!dis_initialized) {
		INIT_DISASSEMBLE_INFO(info, NULL, disass_printf);
		info.mach = bfd_mach_x86_64;
		dis_initialized = 1;
	}

	printf("0x%016lx:   ", (s8) code);

	disass_len = 0;

	seqlen = print_insn_i386((bfd_vma) code, &info);

	for (i = 0; i < seqlen; i++, code++) {
		printf("%02x ", *code);
	}
	
	for (; i < 10; i++) {
		printf("   ");
	}

	printf("   %s\n", disass_buf);

	return code;
}


/* disassemble *****************************************************************

   Outputs a disassembler listing of some machine code on `stdout'.

   start: pointer to first machine instruction
   end:   pointer after last machine instruction

*******************************************************************************/

void disassemble(u1 *start, u1 *end)
{
	disassemble_info info;

	INIT_DISASSEMBLE_INFO(info, NULL, disass_printf);
	info.mach = bfd_mach_x86_64;

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
