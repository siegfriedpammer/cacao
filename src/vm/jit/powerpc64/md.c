/* src/vm/jit/powerpc/md.c - machine dependent PowerPC functions

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
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

   Contact: cacao@cacaojvm.org

   Authors: Christian Thalinger

   Changes: Edwin Steiner

   $Id: md.c 5081 2006-07-06 13:59:01Z tbfg $

*/

#include "config.h"

#include <assert.h>

#include "vm/types.h"

#include "md-abi.h"

#include "vm/global.h"
#include "vm/jit/asmpart.h"

#if !defined(NDEBUG) && defined(ENABLE_DISASSEMBLER)
#include "vm/options.h" /* XXX debug */
#include "vm/jit/disass.h" /* XXX debug */
#endif


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


/* md_get_method_patch_address *************************************************

   Gets the patch address of the currently compiled method. The offset
   is extracted from the load instruction(s) before the jump and added
   to the right base address (PV or REG_METHODPTR).

   INVOKESTATIC/SPECIAL:

   81adffd4    lwz     r13,-44(r13)
   7da903a6    mtctr   r13
   4e800421    bctrl

   INVOKEVIRTUAL:

   81830000    lwz     r12,0(r3)
   81ac0000    lwz     r13,0(r12)
   7da903a6    mtctr   r13
   4e800421    bctrl

   INVOKEINTERFACE:

   81830000    lwz     r12,0(r3)
   818c0000    lwz     r12,0(r12)
   81ac0000    lwz     r13,0(r12)
   7da903a6    mtctr   r13
   4e800421    bctrl

*******************************************************************************/

u1 *md_get_method_patch_address(u1 *ra, stackframeinfo *sfi, u1 *mptr)
{
	u4  mcode;
	s4  offset;
	u1 *pa;

	/* go back to the actual load instruction (3 instructions) */

	ra = ra - 3 * 4;

	/* get first instruction word (lwz) */

	mcode = *((u4 *) ra);

	/* check if we have 2 instructions (addis, addi) */

	if ((mcode >> 16) == 0x3c19) {
		/* XXX write a regression for this */
		assert(0);

		/* get displacement of first instruction (addis) */

		offset = (s4) (mcode << 16);

		/* get displacement of second instruction (addi) */

		mcode = *((u4 *) (ra + 1 * 4));

		assert((mcode >> 16) != 0x6739);

		offset += (s2) (mcode & 0x0000ffff);

	} else {
		/* get the offset from the instruction */

		offset = (s2) (mcode & 0x0000ffff);

		/* check for load from PV */

		if ((mcode >> 16) == 0x81ad) {
			/* get the final data segment address */

			pa = sfi->pv + offset;

		} else if ((mcode >> 16) == 0x81ac) {
			/* in this case we use the passed method pointer */

			pa = mptr + offset;

		} else {
			/* catch any problems */

			assert(0);
		}
	}

	return pa;
}


/* md_codegen_findmethod *******************************************************

   Machine code:

   7d6802a6    mflr    r11
   39abffe0    addi    r13,r11,-32

   or

   7d6802a6    mflr    r11
   3dabffff    addis   r13,r11,-1
   39ad68b0    addi    r13,r13,26800

*******************************************************************************/

u1 *md_codegen_findmethod(u1 *ra)
{
	u1 *pv;
	u4  mcode;
	s4  offset;

	/* get first instruction word after jump */

	mcode = *((u4 *) (ra + 1 * 4));

	/* check if we have 2 instructions (addis, addi) */

	if ((mcode >> 16) == 0x3dab) {
		/* get displacement of first instruction (addis) */

		offset = (s4) (mcode << 16);

		/* get displacement of second instruction (addi) */

		mcode = *((u4 *) (ra + 2 * 4));

		/* check for addi instruction */

		assert((mcode >> 16) == 0x39ad);

		offset += (s2) (mcode & 0x0000ffff);

	} else {
		/* check for addi instruction */

		assert((mcode >> 16) == 0x39ab);

		/* get offset of first instruction (addi) */

		offset = (s2) (mcode & 0x0000ffff);
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

void md_patch_replacement_point(rplpoint *rp)
{
    u8 mcode;

	/* save the current machine code */
	mcode = *(u4*)rp->pc;

	/* write the new machine code */
    *(u4*)(rp->pc) = (u4) rp->mcode;

	/* store saved mcode */
	rp->mcode = mcode;
	
#if !defined(NDEBUG) && defined(ENABLE_DISASSEMBLER)
	{
		u1* u1ptr = rp->pc;
		DISASSINSTR(u1ptr);
		fflush(stdout);
	}
#endif
			
	/* flush instruction cache */
    md_icacheflush(rp->pc,4);
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
 * vim:noexpandtab:sw=4:ts=4:
 */
