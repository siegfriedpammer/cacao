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

   $Id: disass.c 1976 2005-03-03 11:25:06Z twisti $

*/


#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "disass.h"
#include "dis-asm.h"

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


void myprintf(PTR p, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	fflush(stdout);
}


int buffer_read_memory(bfd_vma memaddr, bfd_byte *myaddr, unsigned int length, struct disassemble_info *info)
{
	memcpy(myaddr, (void*) memaddr, length);
	return 0;
}


void perror_memory(int status, bfd_vma memaddr, struct disassemble_info *info)
{
	assert(0);
}


void generic_print_address(bfd_vma addr, struct disassemble_info *info)
{
/*  	myprintf(NULL, "0x%x", addr - (u4) info->application_data); */
	myprintf(NULL, "0x08%x", addr);
}


int generic_symbol_at_address(bfd_vma addr, struct disassemble_info *info)
{
	assert(0);
}


unsigned long bfd_getb32(void *buf)
{
	return *(unsigned long *) buf;
}


unsigned long bfd_getl32(void *buf)
{
	return *(unsigned long *) buf;
}


void sprintf_vma(char *buf, bfd_vma disp)
{
	sprintf(buf, "0x%x", disp);
}


void disassinstr(s4 *code)
{
	disassemble_info info;

	printf("0x%08x:   %08x    ", (s4) code, *code);

	INIT_DISASSEMBLE_INFO(info, NULL, myprintf);
/*     	info.application_data = (u4) code - pos; */
	print_insn_big_powerpc((bfd_vma) code, &info);
	printf("\n");
}


void disassemble(s4 *code, s4 len)
{
	s4 i;
	disassemble_info info;

	INIT_DISASSEMBLE_INFO(info, NULL, myprintf);
/*    	info.application_data = code; */
	printf ("  --- disassembler listing ---\n");
	for (i = 0; i < len; i += 4, code++)
		disassinstr(code);
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
