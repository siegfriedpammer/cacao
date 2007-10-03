/* src/vm/jit/powerpc64/md.c - machine dependent PowerPC functions

   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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

#include <assert.h>
#include <stdint.h>

#include "vm/types.h"

#include "md-abi.h"

#include "vm/jit/powerpc64/codegen.h"

#include "vm/exceptions.h"
#include "vm/global.h"

#include "vm/jit/asmpart.h"
#include "vm/jit/codegen-common.h"
#include "vm/jit/jit.h"
#include "vm/jit/md.h"


/* md_init *********************************************************************

   Do some machine dependent initialization.

*******************************************************************************/

void md_init(void)
{
	/* nothing to do */
}


/* md_stacktrace_get_returnaddress *********************************************

   Returns the return address of the current stackframe, specified by
   the passed stack pointer and the stack frame size.

*******************************************************************************/

u1 *md_stacktrace_get_returnaddress(u1 *sp, u4 framesize)
{
	u1 *ra;

	/* on PowerPC the return address is located in the linkage area */

	ra = *((u1 **) (sp + framesize + LA_LR_OFFSET));

	return ra;
}


/* md_jit_method_patch_address *************************************************

   Gets the patch address of the currently compiled method. The offset
   is extracted from the load instruction(s) before the jump and added
   to the right base address (PV or REG_METHODPTR).

   INVOKESTATIC/SPECIAL:

   e9ceffb8    ld      r14,-72(r14)	
   7dc903a6    mtctr   r14		
   4e800421    bctrl

   INVOKEVIRTUAL:

FIXME   81830000    lwz     r12,0(r3)
   e9cc0000     ld      r14,0(r12)
   7dc903a6     mtctr   r14
   4e800421     bctrl


   INVOKEINTERFACE:

FIXME   81830000    lwz     r12,0(r3)
FIXME   818c0000    lwz     r12,0(r12)
FIXME   81ac0000    lwz     r13,0(r12)
  7dc903a6    mtctr   r14
  4e800421    bctrl

*******************************************************************************/

void *md_jit_method_patch_address(void *pv, void *ra, void *mptr)
{
	uint32_t *pc;
	uint32_t  mcode;
	int32_t   offset;
	void     *pa;

	/* Go back to the actual load instruction (3 instructions). */

	pc = ((uint32_t *) ra) - 3;

	/* get first instruction word (lwz) */

	mcode = pc[0];

	/* check if we have 2 instructions (addis, addi) */

	if ((mcode >> 16) == 0x3c19) {
		/* XXX write a regression for this */
		pa = NULL;
		assert(0);

		/* get displacement of first instruction (addis) */

		offset = (int32_t) (mcode << 16);

		/* get displacement of second instruction (addi) */

		mcode = pc[1];

		assert((mcode >> 16) != 0x6739);

		offset += (int16_t) (mcode & 0x0000ffff);
	}
	else {
		/* get the offset from the instruction */

		offset = (int16_t) (mcode & 0x0000ffff);

		/* check for load from PV */

		if ((mcode >> 16) == 0xe9ce) {	
			/* get the final data segment address */

			pa = ((uint8_t *) pv) + offset;
		}
		else if ((mcode >> 16) == 0xe9cc) { 
			/* in this case we use the passed method pointer */

			/* return NULL if no mptr was specified (used for replacement) */

			if (mptr == NULL)
				return NULL;

			pa = ((uint8_t *) mptr) + offset;
		}
		else {
			/* catch any problems */

			vm_abort("md_jit_method_patch_address: unknown instruction %x",
					 mcode);

			/* keep compiler happy */

			pa = NULL;
		}
	}

	return pa;
}


/* md_codegen_get_pv_from_pc ***************************************************

   Machine code:

   7d6802a6    mflr    r11
   39cbffe0    addi    r14,r11,-32

   or

   7d6802a6    mflr    r11
   3dcbffff    addis   r14,r11,-1
   39ce68b0    addi    r14,r13,26800

*******************************************************************************/

u1 *md_codegen_get_pv_from_pc(u1 *ra)
{
	u1 *pv;
	u4  mcode;
	s4  offset;

	/* get first instruction word after jump */

	mcode = *((u4 *) (ra + 1 * 4));

	/* check if we have 2 instructions (addis, addi) */

	if ((mcode >> 16) == 0x3dcb) {
		/* get displacement of first instruction (addis) */

		offset = (s4) (mcode << 16);

		/* get displacement of second instruction (addi) */

		mcode = *((u4 *) (ra + 2 * 4));

		/* check for addi instruction */

		assert((mcode >> 16) == 0x39ce);

		offset += (s2) (mcode & 0x0000ffff);
	}
	else if ((mcode >> 16) == 0x39cb) {
		/* get offset of first instruction (addi) */

		offset = (s2) (mcode & 0x0000ffff);
	}
	else {
		vm_abort("md_codegen_get_pv_from_pc: unknown instruction %x", mcode);

		/* keep compiler happy */

		offset = 0;
	}

	/* calculate PV via RA + offset */

	pv = ra + offset;

	return pv;
}


/* md_cacheflush ***************************************************************

   Calls the system's function to flush the instruction and data
   cache.

*******************************************************************************/

void md_cacheflush(u1 *addr, s4 nbytes)
{
	asm_cacheflush(addr, nbytes);
}


/* md_icacheflush **************************************************************

   Calls the system's function to flush the instruction cache.

*******************************************************************************/

void md_icacheflush(u1 *addr, s4 nbytes)
{
	asm_cacheflush(addr, nbytes);
}


/* md_dcacheflush **************************************************************

   Calls the system's function to flush the data cache.

*******************************************************************************/

void md_dcacheflush(u1 *addr, s4 nbytes)
{
	asm_cacheflush(addr, nbytes);
}


/* md_patch_replacement_point **************************************************

   Patch the given replacement point.

*******************************************************************************/

#if defined(ENABLE_REPLACEMENT)
void md_patch_replacement_point(u1 *pc, u1 *savedmcode, bool revert)
{
	u4 mcode;

	if (revert) {
		/* restore the patched-over instruction */
		*(u4*)(pc) = *(u4*)(savedmcode);
	}
	else {
		/* save the current machine code */
		*(u4*)(savedmcode) = *(u4*)(pc);

		/* build the machine code for the patch */
		mcode = (0x80000000 | (EXCEPTION_HARDWARE_PATCHER));

		/* write the new machine code */
		*(u4*)(pc) = (u4) mcode;
	}

	/* flush instruction cache */
    md_icacheflush(pc,4);
}
#endif /* defined(ENABLE_REPLACEMENT) */

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
