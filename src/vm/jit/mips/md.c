/* src/vm/jit/mips/md.c - machine dependent MIPS functions

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

   Changes: 

   $Id: md.c 4640 2006-03-16 17:24:18Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <unistd.h>
#include <sys/cachectl.h>
#include <sys/mman.h>

#include "vm/types.h"

#include "toolbox/logging.h"
#include "vm/global.h"
#include "vm/jit/stacktrace.h"


void docacheflush(u1 *p, long bytelen)
{
	u1 *e = p + bytelen;
	long psize = sysconf(_SC_PAGESIZE);
	p -= (long) p & (psize - 1);
	e += psize - ((((long) e - 1) & (psize - 1)) + 1);
	bytelen = e-p;
	mprotect(p, bytelen, PROT_READ | PROT_WRITE | PROT_EXEC);
}


/* md_stacktrace_get_returnaddress *********************************************

   Returns the return address of the current stackframe, specified by
   the passed stack pointer and the stack frame size.

*******************************************************************************/

u1 *md_stacktrace_get_returnaddress(u1 *sp, u4 framesize)
{
	u1 *ra;

	/* on MIPS the return address is located on the top of the stackframe */

	/* XXX change this if we ever want to use 4-byte stackslots */
	/* ra = *((u1 **) (sp + framesize - SIZEOF_VOID_P)); */
	ra = *((u1 **) (sp + framesize - 8));

	return ra;
}


/* md_assembler_get_patch_address **********************************************

   Gets the patch address of the currently compiled method. The offset
   is extracted from the load instruction(s) before the jump and added
   to the right base address (PV or REG_METHODPTR).

*******************************************************************************/

u1 *md_assembler_get_patch_address(u1 *ra, stackframeinfo *sfi, u1 *mptr)
{
	u4  mcode;
	s4  offset;
	u1 *pa;

	/* go back to the actual load instruction (3 instructions on MIPS) */

	ra -= 3 * 4;

	/* get first instruction word on current PC */

	mcode = *((u4 *) ra);

	/* check if we have 2 instructions (lui) */

	if ((mcode >> 16) == 0x3c19) {
		/* XXX write a regression for this */
		assert(0);

		/* get displacement of first instruction (lui) */

		offset = (s4) (mcode << 16);

		/* get displacement of second instruction (daddiu) */

		mcode = *((u4 *) (ra + 1 * 4));

		assert((mcode >> 16) != 0x6739);

		offset += (s2) (mcode & 0x0000ffff);

	} else {
		/* get first instruction (ld) */

		mcode = *((u4 *) ra);

		/* get the offset from the instruction */

		offset = (s2) (mcode & 0x0000ffff);

		/* check for call with REG_METHODPTR: ld s8,x(t9) */

#if SIZEOF_VOID_P == 8
		if ((mcode >> 16) == 0xdf3e) {
#else
		if ((mcode >> 16) == 0x8f3e) {
#endif
			/* in this case we use the passed method pointer */

			pa = mptr + offset;

		} else {
			/* in the normal case we check for a `ld s8,x(s8)' instruction */

#if SIZEOF_VOID_P == 8
			assert((mcode >> 16) == 0xdfde);
#else
			assert((mcode >> 16) == 0x8fde);
#endif

			/* and get the final data segment address */

			pa = sfi->pv + offset;
		}
	}

	return pa;
}


/* md_codegen_findmethod *******************************************************

   Machine code:

   03c0f809    jalr     s8
   00000000    nop
   27feff9c    addiu    s8,ra,-100

*******************************************************************************/

u1 *md_codegen_findmethod(u1 *ra)
{
	u1 *pv;
	u4  mcode;
	s4  offset;

	/* get the offset of the instructions */

	/* get first instruction word after jump */

	mcode = *((u4 *) ra);

	/* check if we have 2 instructions (lui, daddiu) */

	if ((mcode >> 16) == 0x3c19) {
		/* get displacement of first instruction (lui) */

		offset = (s4) (mcode << 16);

		/* get displacement of second instruction (daddiu) */

		mcode = *((u4 *) (ra + 1 * 4));

#if SIZEOF_VOID_P == 8
		assert((mcode >> 16) == 0x6739);
#else	
		assert((mcode >> 16) == 0x2739);
#endif

		offset += (s2) (mcode & 0x0000ffff);

	} else {
		/* get offset of first instruction (daddiu) */

		mcode = *((u4 *) ra);

#if SIZEOF_VOID_P == 8
		assert((mcode >> 16) == 0x67fe);
#else
		assert((mcode >> 16) == 0x27fe);
#endif

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
	cacheflush(addr, nbytes, BCACHE);
}


/* md_icacheflush **************************************************************

   Calls the system's function to flush the instruction cache.

*******************************************************************************/

void md_icacheflush(u1 *addr, s4 nbytes)
{
	cacheflush(addr, nbytes, ICACHE);
}


/* md_dcacheflush **************************************************************

   Calls the system's function to flush the data cache.

*******************************************************************************/

void md_dcacheflush(u1 *addr, s4 nbytes)
{
	cacheflush(addr, nbytes, DCACHE);
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
