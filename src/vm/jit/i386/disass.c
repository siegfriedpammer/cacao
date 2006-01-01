/* src/vm/jit/i386/disass.c - wrapper functions for GNU binutils disassembler

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

   $Id: disass.c 4033 2006-01-01 18:30:53Z twisti $

*/


#include "config.h"

#include <assert.h>
/* #include <dis-asm.h> */
#include <stdarg.h>
#include <string.h>

#include "vm/types.h"

#include "vm/jit/i386/dis-asm.h"

#include "mm/memory.h"
#include "vm/jit/disass.h"


char mylinebuf[512];
int mylen;


char *regs[] = {
	"eax",
	"ecx",
	"edx",
	"ebx",
	"esp",
	"ebp",
	"esi",
	"edi"
};


/* myprintf ********************************************************************

   Required by i386-dis.c, prints the stuff into a buffer.

*******************************************************************************/

void myprintf(PTR p, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	mylen += vsprintf(mylinebuf + mylen, fmt, ap);
	va_end(ap);
}

/* buffer_read_memory **********************************************************

   Required by i386-dis.c, copy some stuff to another memory.

*******************************************************************************/

int buffer_read_memory(bfd_vma memaddr, bfd_byte *myaddr, unsigned int length, struct disassemble_info *info)
{
	MCOPY(myaddr, (void *) memaddr, u1, length);

	return 0;
}


/* generic_symbol_at_address ***************************************************

   Required by i386-dis.c, just return 1.

*******************************************************************************/

int generic_symbol_at_address(bfd_vma addr, struct disassemble_info *info)
{
	return 1;
}


/* generic_print_address *******************************************************

   Required by i386-dis.c, just print the address.

*******************************************************************************/

void generic_print_address(bfd_vma addr, struct disassemble_info *info)
{
	myprintf(info->stream, "0x%08x", addr);
}


/* perror_memory ***************************************************************

   Required by i386-dis.c, jsut assert in case.

*******************************************************************************/

void perror_memory(int status, bfd_vma memaddr, struct disassemble_info *info)
{
	assert(0);
}


/* disassinstr *****************************************************************

   Outputs a disassembler listing of one machine code instruction on
   'stdout'.

   code: instructions machine code

*******************************************************************************/

u1 *disassinstr(u1 *code)
{
	static disassemble_info info;
	static int dis_initialized;
	s4 seqlen;
	s4 i;

	if (!dis_initialized) {
		INIT_DISASSEMBLE_INFO(info, NULL, myprintf);
		info.mach = bfd_mach_i386_i386;
		dis_initialized = 1;
	}

	printf("0x%08x:   ", (s4) code);
	mylen = 0;
	seqlen = print_insn_i386((bfd_vma) code, &info);

	for (i = 0; i < seqlen; i++, code++) {
		printf("%02x ", *code);
	}

	for (; i < 8; i++) {
		printf("   ");
	}

	printf("   %s\n", mylinebuf);

	return code;
}



/* disassemble *****************************************************************

   Outputs a disassembler listing of some machine code on 'stdout'.

   start: pointer to first machine instruction
   end:   pointer after last machine instruction

*******************************************************************************/

void disassemble(u1 *start, u1 *end)
{
	disassemble_info info;

	INIT_DISASSEMBLE_INFO(info, NULL, myprintf);
	info.mach = bfd_mach_i386_i386;

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
