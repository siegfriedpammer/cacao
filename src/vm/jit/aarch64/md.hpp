/* src/vm/jit/aarch64/md.hpp - machine dependent Aarch64 functions

   Copyright (C) 1996-2013
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


#ifndef VM_JIT_AARCH64_MD_HPP_
#define VM_JIT_AARCH64_MD_HPP_ 1

#include "config.h"

#include <cassert>
#include <stdint.h>

#include "vm/jit/aarch64/codegen.hpp"

#include "vm/global.hpp"
#include "vm/vm.hpp"

#include "vm/jit/asmpart.hpp"
#include "vm/jit/code.hpp"
#include "vm/jit/codegen-common.hpp"
#include "vm/jit/disass.hpp"


/* global variables ***********************************************************/

extern bool has_ext_instr_set;


/* inline functions ***********************************************************/

/**
 * Returns the size (in bytes) of the current stackframe, specified by
 * the passed codeinfo structure.
 */
inline static int32_t md_stacktrace_get_framesize(codeinfo* code)
{
	// Check for the asm_vm_call_method special case.
	if (code == NULL)
		return 0;

	// On Aarch64 we use 8-byte stackslots with a 16-byte aligned stack
    u4 stacksize = code->stackframesize * 8;
    stacksize += stacksize % 16;
	return stacksize;
}


/* md_stacktrace_get_returnaddress *********************************************

   Returns the return address of the current stackframe, specified by
   the passed stack pointer and the stack frame size.

*******************************************************************************/

inline static void *md_stacktrace_get_returnaddress(void *sp, int32_t stackframesize)
{
	void *ra;

	/* On Aarch64 the return address is located on the top of the
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
	uint32_t *pc = (uint32_t *) ra;
	void     *pv = NULL;

	/* Get first instruction word after jump. */
	uint32_t mcode = pc[0];
    
    /* Check for SUB with immediate, as immediate is only 12 bit wide, we
     * might have 2 consecutive subs where the first is shifted */
    u1 high = (mcode >> 24) & 0xff;
    if (high == 0xD1) {
        u1 shift = (mcode >> 22) & 0x3;
        s4 offset = (mcode >> 10) & 0xfff;
        if (shift) {
            // as this instruction is shifted, shift the offset by 12 bits 
            // and add the offset of the next sub instruction
            offset = (offset << 12);
            offset += (pc[1] >> 10) & 0xfff;
        }
        pv = ((uint8_t *) pc) - offset;
    } else {
		vm_abort_disassemble(pc, 2, "md_codegen_get_pv_from_pc: unknown instruction %x", mcode);
    }

	return pv;
}


/* md_cacheflush ***************************************************************

   Calls the system's function to flush the instruction and data
   cache.

*******************************************************************************/

extern void asm_flush_icache_range(void *start, void *end) __asm__("asm_flush_icache_range");
extern void asm_flush_dcache_range(void *start, void *end) __asm__("asm_flush_dcache_range");

inline static void md_cacheflush(void *addr, int nbytes)
{
    asm_flush_icache_range(addr, ((char*)addr + nbytes + 8));
    asm_flush_dcache_range(addr, ((char*)addr + nbytes + 8));
}


/* md_icacheflush **************************************************************

   Calls the system's function to flush the instruction cache.

*******************************************************************************/

inline static void md_icacheflush(void *addr, int nbytes)
{
    asm_flush_icache_range(addr, ((char*)addr + nbytes + 8));
}


/* md_dcacheflush **************************************************************

   Calls the system's function to flush the data cache.

*******************************************************************************/

inline static void md_dcacheflush(void *addr, int nbytes)
{
    asm_flush_dcache_range(addr, ((char*)addr + nbytes + 8));
}

#endif // VM_JIT_AARCH64_MD_HPP_


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
