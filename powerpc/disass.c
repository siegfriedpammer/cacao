#include "dis-asm.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

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
	myprintf(NULL, "0x%x", addr - (u4) info->application_data);
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

void disassinstr(int c, int pos)
{
	disassemble_info info;

	printf ("%6x: %8x  ", pos, c);

	INIT_DISASSEMBLE_INFO(info, NULL, myprintf);
	info.application_data = (PTR) ((u4) &c - pos);
	print_insn_big_powerpc((bfd_vma) &c, &info);
	printf ("\n");
}

void disassemble(int *code, int len)
{
	int p;
	disassemble_info info;

	INIT_DISASSEMBLE_INFO(info, NULL, myprintf);
	info.application_data = code;
	printf ("  --- disassembler listing ---\n");
	for (p = 0; p < len; p += 4, code++) {
		myprintf(NULL, "%6x: %08x  ", p, *code);
		print_insn_big_powerpc((bfd_vma) p, &info);
		myprintf(NULL, "\n");
	}
}
