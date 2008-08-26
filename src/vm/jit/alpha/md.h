/* src/vm/jit/alpha/md.h - machine dependent Alpha functions

   Copyright (C) 1996-2005, 2006, 2007, 2008
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


#ifndef _VM_JIT_ALPHA_MD_H
#define _VM_JIT_ALPHA_MD_H

#include "config.h"

#include <assert.h>
#include <stdint.h>

#include "vm/jit/alpha/codegen.h"

#include "vm/global.h"
#include "vm/vm.hpp"

#include "vm/jit/asmpart.h"
#include "vm/jit/codegen-common.hpp"


/* global variables ***********************************************************/

extern bool has_ext_instr_set;


/* inline functions ***********************************************************/

/* md_stacktrace_get_returnaddress *********************************************

   Returns the return address of the current stackframe, specified by
   the passed stack pointer and the stack frame size.

*******************************************************************************/

inline static void *md_stacktrace_get_returnaddress(void *sp, int32_t stackframesize)
{
	void *ra;

	/* On Alpha the return address is located on the top of the
	   stackframe. */

	ra = *((void **) (((uintptr_t) sp) + stackframesize - SIZEOF_VOID_P));

	return ra;
}


/* md_codegen_get_pv_from_pc ***************************************************

   Machine code:

   6b5b4000    jsr     (pv)
   277afffe    ldah    pv,-2(ra)
   237ba61c    lda     pv,-23012(pv)

*******************************************************************************/

inline static void *md_codegen_get_pv_from_pc(void *ra)
{
	uint32_t *pc;
	uint32_t  mcode;
	int       opcode;
	int32_t   disp;
	void     *pv;

	pc = (uint32_t *) ra;

	/* Get first instruction word after jump. */

	mcode = pc[0];

	/* Get opcode and displacement. */

	opcode = M_MEM_GET_Opcode(mcode);
	disp   = M_MEM_GET_Memory_disp(mcode);

	/* Check for short or long load (2 instructions). */

	switch (opcode) {
	case 0x08: /* LDA: TODO use define */
		assert((mcode >> 16) == 0x237a);

		pv = ((uint8_t *) pc) + disp;
		break;

	case 0x09: /* LDAH: TODO use define */
		pv = ((uint8_t *) pc) + (disp << 16);

		/* Get displacement of second instruction (LDA). */

		mcode = pc[1];

		assert((mcode >> 16) == 0x237b);

		disp = M_MEM_GET_Memory_disp(mcode);

		pv = ((uint8_t *) pv) + disp;
		break;

	default:
		vm_abort_disassemble(pc, 2, "md_codegen_get_pv_from_pc: unknown instruction %x", mcode);
		return NULL;
	}

	return pv;
}


/* md_cacheflush ***************************************************************

   Calls the system's function to flush the instruction and data
   cache.

*******************************************************************************/

inline static void md_cacheflush(void *addr, int nbytes)
{
	asm_cacheflush(addr, nbytes);
}


/* md_icacheflush **************************************************************

   Calls the system's function to flush the instruction cache.

*******************************************************************************/

inline static void md_icacheflush(void *addr, int nbytes)
{
	asm_cacheflush(addr, nbytes);
}


/* md_dcacheflush **************************************************************

   Calls the system's function to flush the data cache.

*******************************************************************************/

inline static void md_dcacheflush(void *addr, int nbytes)
{
	/* do nothing */
}

#endif /* _VM_JIT_ALPHA_MD_H */


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
