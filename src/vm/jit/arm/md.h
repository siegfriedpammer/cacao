/* src/vm/jit/arm/md.h - machine dependent Arm functions

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


#ifndef _VM_JIT_ARM_MD_H
#define _VM_JIT_ARM_MD_H

#include "config.h"

#include <assert.h>
#include <stdint.h>

#include "vm/types.h"

#include "vm/jit/asmpart.h"
#include "vm/jit/codegen-common.hpp"


/* md_stacktrace_get_returnaddress *********************************************

   Returns the return address of the current stackframe, specified by
   the passed stack pointer and the stack frame size.

*******************************************************************************/

inline static void *md_stacktrace_get_returnaddress(void *sp, int32_t stackframesize)
{
	void *ra;

	/* On ARM the return address is located on the top of the
	   stackframe. */
	/* ATTENTION: This is only true for non-leaf methods!!! */

	ra = *((void **) (((uintptr_t) sp) + stackframesize - SIZEOF_VOID_P));

	return ra;
}


/* md_codegen_get_pv_from_pc ***************************************************

   TODO: document me

*******************************************************************************/

inline static void* md_codegen_get_pv_from_pc(void* ra)
{
	uint32_t* pc;
	uintptr_t pv;
	uint32_t mcode;
	int mcode_idx;

	pc = (uint32_t*) ra;
	pv = (uintptr_t) ra;

	/* this can either be a RECOMPUTE_IP in JIT code or a fake in asm_calljavafunction */
	mcode_idx = 0;
	mcode = pc[0];

	/* if this was shifted by 18 bits, we have to load additional instructions */
	if ((mcode & 0xfff0ff00) == 0xe240c700 /*sub ip,??,#__*/) {
		pv -= (uintptr_t) ((mcode & 0x000000ff) << 18);
		mcode = pc[++mcode_idx];
	}

	/* if this was shifted by 10 bits, we have to load additional instructions */
	if ((mcode & 0xfff0ff00) == 0xe240cb00 /*sub ip,??,#__*/) {
		pv -= (uintptr_t) ((mcode & 0x000000ff) << 10);
		mcode = pc[++mcode_idx];
	}

	/* this is the default path with just one instruction, shifted by 2 or no bits */
	if ((mcode & 0xfff0ff00) == 0xe240cf00 /*sub ip,??,#__*/)
		pv -= (uintptr_t) ((mcode & 0x000000ff) << 2);
	else if ((mcode & 0xffffff00) == 0xe24fc000 /*sub ip,pc,#__*/)
		pv -= (uintptr_t) (mcode & 0x000000ff);
	else {
		/* if this happens, we got an unexpected instruction at (*ra) */
		vm_abort("Unable to find method: %p (instr=%x)", ra, mcode);
	}

	/* we used PC-relative adressing; but now it is LR-relative */
	pv += 8;

	/* if we found our method the data segment has to be valid */
	/* we check this by looking up the IsLeaf field, which has to be boolean */
/* 	assert( *((s4*)pv-8) == (s4)true || *((s4*)pv-8) == (s4)false );  */

	return (void*) pv;
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

#endif /* _VM_JIT_ARM_MD_H */


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
