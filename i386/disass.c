/* disass.c ********************************************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	A very primitive disassembler for Alpha machine code for easy debugging.

	Authors: Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at
	         Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1998/11/18

*******************************************************************************/

#include "dis-asm.h"
#include "dis-stuff.h"

static s4 *codestatic = 0;
static int pstatic = 0;

char mylinebuf[512];
int mylen;


static void myprintf(PTR p, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	mylen += vsprintf(mylinebuf + mylen, fmt, ap);
	va_end(ap);
}

static int buffer_read_memory(bfd_vma memaddr, bfd_byte *myaddr, unsigned int length, struct disassemble_info *info)
{
	if (length == 1)
		*myaddr = *((u1 *) memaddr);
	else
		memcpy(myaddr, (void *) memaddr, length);
	return 0;
}


/* name table for 8 integer registers */

static char *regs[] = {
	"eax",
	"ecx",
	"edx",
	"ebx",
	"esp",
	"ebp",
	"esi",
	"edi"
};


/* function disassinstr ********************************************************

	outputs a disassembler listing of one machine code instruction on 'stdout'
	c:   instructions machine code
	pos: instructions address relative to method start

*******************************************************************************/

static void disassinstr(int c, int pos)
{
	static disassemble_info info;
	static int dis_initialized;
	s4 *code = (s4 *) c;
	int seqlen;

	if (!dis_initialized) {
		INIT_DISASSEMBLE_INFO(info, NULL, myprintf);
		info.mach = bfd_mach_i386_i386;
		dis_initialized = 1;
	}

	printf("%6x:   ", pos);
	mylen = 0;
	seqlen = print_insn_i386((bfd_vma) code, &info);
	{
		int i;
		for (i = 0; i < seqlen; i++)
			printf("%02x ", *((u1 *) code)++);
		for (; i < 8; i++)
			printf("   ");
	}
	codestatic = (u1 *) code - 1;
	pstatic = pos + seqlen - 1;
	printf("%s\n", mylinebuf);
}


/* function disassemble ********************************************************

	outputs a disassembler listing of some machine code on 'stdout'
	code: pointer to first instruction
	len:  code size (number of instructions * 4)

*******************************************************************************/

static void disassemble(int *code, int len)
{
	int p;
	disassemble_info info;

	INIT_DISASSEMBLE_INFO(info, NULL, myprintf);
	info.mach = bfd_mach_i386_i386;

	printf("  --- disassembler listing ---\n");
	for (p = 0; p < len; p++) {
		(u1 *) code += print_insn_i386((bfd_vma) code, &info);
		myprintf(NULL, "\n");
	}
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
