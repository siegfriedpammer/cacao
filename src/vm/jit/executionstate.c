/* src/vm/jit/executionstate.c - execution-state handling

   Copyright (C) 2007, 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/


#include "config.h"

#include <stdint.h>
#include <stdio.h>

#include "md-abi.h"

#include "vm/descriptor.h"
#include "vm/os.hpp"

#include "vm/jit/abi.h"
#include "vm/jit/executionstate.h"


/* executionstate_sanity_check *************************************************

   Perform some sanity checks for the md_executionstate_read and
   md_executionstate_write functions.

*******************************************************************************/

#if !defined(NDEBUG)
void executionstate_sanity_check(void *context)
{
    /* estimate a minimum for the context size */

#if defined(HAS_ADDRESS_REGISTER_FILE)
#define MINIMUM_CONTEXT_SIZE  (SIZEOF_VOID_P    * ADR_REG_CNT \
		                       + sizeof(double) * FLT_REG_CNT \
							   + sizeof(int)    * INT_REG_CNT)
#else
#define MINIMUM_CONTEXT_SIZE  (SIZEOF_VOID_P    * INT_REG_CNT \
		                       + sizeof(double) * FLT_REG_CNT)
#endif

	executionstate_t es1;
	executionstate_t es2;
	executionstate_t es3;
	unsigned int i;
	unsigned char reference[MINIMUM_CONTEXT_SIZE];

	/* keep a copy of (a prefix of) the context for reference */

	os_memcpy(&reference, context, MINIMUM_CONTEXT_SIZE);

	/* different poisons */

	os_memset(&es1, 0xc9, sizeof(executionstate_t));
	os_memset(&es2, 0xb5, sizeof(executionstate_t));
	os_memset(&es3, 0x6f, sizeof(executionstate_t));

	md_executionstate_read(&es1, context);

	/* verify that item-by-item copying preserves the state */

	es2.pc = es1.pc;
	es2.sp = es1.sp;
	es2.pv = es1.pv;
	es2.ra = es1.ra;
	es2.code = es1.code;
	for (i = 0; i < INT_REG_CNT; ++i)
		es2.intregs[i] = es1.intregs[i];
	for (i = 0; i < FLT_REG_CNT; ++i)
		es2.fltregs[i] = es1.fltregs[i];
#if defined(HAS_ADDRESS_REGISTER_FILE)
	for (i = 0; i < ADR_REG_CNT; ++i)
		es2.adrregs[i] = es1.adrregs[i];
#endif

	/* write it back - this should not change the context */
	/* We cannot check that completely, unfortunately, as we don't know */
	/* the size of the (OS-dependent) context. */

	md_executionstate_write(&es2, context);

	/* Read it again, Sam! */

	md_executionstate_read(&es3, context);

	/* Compare. Note: Because of the NAN madness, we cannot compare
	 * doubles using '=='. */

	assert(es3.pc == es1.pc);
	assert(es3.sp == es1.sp);
	assert(es3.pv == es1.pv);
	for (i = 0; i < INT_REG_CNT; ++i)
		assert(es3.intregs[i] == es1.intregs[i]);
	for (i = 0; i < FLT_REG_CNT; ++i)
		assert(memcmp(es3.fltregs+i, es1.fltregs+i, sizeof(double)) == 0);
#if defined(HAS_ADDRESS_REGISTER_FILE)
	for (i = 0; i < ADR_REG_CNT; ++i)
		assert(es3.adrregs[i] == es1.adrregs[i]);
#endif

	/* i386 and x86_64 do not have an RA register */

#if defined(__I386__) || defined(__X86_64__)
	assert(es3.ra != es1.ra);
#else
	assert(es3.ra == es1.ra);
#endif

	/* "code" is not set by the md_* functions */

	assert(es3.code != es1.code);

	/* assert that we have not messed up the context */

	assert(memcmp(&reference, context, MINIMUM_CONTEXT_SIZE) == 0);
}
#endif


/* executionstate_println ******************************************************
 
   Print execution state
  
   IN:
       es...............the execution state to print
  
*******************************************************************************/

#if !defined(NDEBUG)
void executionstate_println(executionstate_t *es)
{
	uint64_t *sp;
	int       slots;
	int       extraslots;
	int       i;

	if (!es) {
		printf("(executionstate_t *)NULL\n");
		return;
	}

	printf("executionstate_t:\n");
	printf("\tpc = %p", es->pc);
	printf("  sp = %p", es->sp);
	printf("  pv = %p", es->pv);
	printf("  ra = %p\n", es->ra);

#if defined(ENABLE_DISASSEMBLER)
	for (i=0; i<INT_REG_CNT; ++i) {
		if (i%4 == 0)
			printf("\t");
		else
			printf(" ");
# if SIZEOF_VOID_P == 8
		printf("%-3s = %016lx", abi_registers_integer_name[i], es->intregs[i]);
# else
		printf("%-3s = %08x", abi_registers_integer_name[i], (unsigned) es->intregs[i]);
# endif
		if (i%4 == 3)
			printf("\n");
	}

	for (i=0; i<FLT_REG_CNT; ++i) {
		if (i%4 == 0)
			printf("\t");
		else
			printf(" ");
		printf("F%02d = %016llx",i,(unsigned long long)es->fltregs[i]);
		if (i%4 == 3)
			printf("\n");
	}

# if defined(HAS_ADDRESS_REGISTER_FILE)
	for (i=0; i<ADR_REG_CNT; ++i) {
		if (i%4 == 0)
			printf("\t");
		else
			printf(" ");
		printf("A%02d = %016llx",i,(unsigned long long)es->adrregs[i]);
		if (i%4 == 3)
			printf("\n");
	}
# endif
#endif

	sp = (uint64_t *) es->sp;

	extraslots = 2;

	if (es->code) {
		methoddesc *md = es->code->m->parseddesc;
		slots = es->code->stackframesize;
		extraslots = 1 + md->memuse;
	}
	else
		slots = 0;


	if (slots) {
		printf("\tstack slots(+%d) at sp:", extraslots);
		for (i=0; i<slots+extraslots; ++i) {
			if (i%4 == 0)
				printf("\n\t\t");
			printf("M%02d%c", i, (i >= slots) ? '(' : ' ');
			printf("%016llx",(unsigned long long)*sp++);
			printf("%c", (i >= slots) ? ')' : ' ');
		}
		printf("\n");
	}

	printf("\tcode: %p", (void*)es->code);
	if (es->code != NULL) {
		printf(" stackframesize=%d ", es->code->stackframesize);
		method_print(es->code->m);
	}
	printf("\n");

	printf("\n");
}
#endif


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
 * vim:noexpandtab:sw=4:ts=4:
 */
