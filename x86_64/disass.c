/* x86_64/disass.c *************************************************************

    Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

    See file COPYRIGHT for information on usage and disclaimer of warranties

    Wrapper functions to call the disassembler from the GNU binutils.

    Authors: Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at
             Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
             Christian Thalinger EMAIL: cacao@complang.tuwien.ac.at

    Last Change: $Id: disass.c 388 2003-07-10 16:49:05Z twisti $

*******************************************************************************/

#include "dis-asm.h"
#include "dis-stuff.h"

static u1 *codestatic = 0;
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


/* function disassinstr ********************************************************

	outputs a disassembler listing of one machine code instruction on 'stdout'
	c:   instructions machine code
	pos: instructions address relative to method start

*******************************************************************************/

static void disassinstr(u1 *code, int pos)
{
	static disassemble_info info;
	static int dis_initialized;
	int seqlen;

	if (!dis_initialized) {
		INIT_DISASSEMBLE_INFO(info, NULL, myprintf);
		info.mach = bfd_mach_x86_64;
		dis_initialized = 1;
	}

	printf("0x%016x:   ", code);
	mylen = 0;
	seqlen = print_insn_i386((bfd_vma) code, &info);
	{
		int i;
		if (seqlen > 8) {
			for (i = 0; i < 8; i++) {
				printf("%02x ", *(code++));
			}
			printf("   %s\n", mylinebuf);
			printf("0x%016x:   ", code);
			for (; i < seqlen; i++) {
				printf("%02x ", *(code++));
			}
			printf("\n");

		} else {
			for (i = 0; i < seqlen; i++) {
				printf("%02x ", *(code++));
			}
			for (; i < 8; i++) {
				printf("   ");
			}
			printf("   %s\n", mylinebuf);
		}
	}
	codestatic = code - 1;
	pstatic = pos + seqlen - 1;
}


/* function disassemble ********************************************************

	outputs a disassembler listing of some machine code on 'stdout'
	code: pointer to first instruction
	len:  code size (number of instructions * 4)

*******************************************************************************/

static void disassemble(u1 *code, int len)
{
	int p;
	disassemble_info info;

	INIT_DISASSEMBLE_INFO(info, NULL, myprintf);
	info.mach = bfd_mach_x86_64;

	printf("  --- disassembler listing ---\n");
	for (p = 0; p < len; p++) {
		code += print_insn_i386((bfd_vma) code, &info);
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
