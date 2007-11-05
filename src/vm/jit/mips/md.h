/* src/vm/jit/mips/md.h - machine dependent MIPS functions

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


#ifndef _VM_JIT_MIPS_MD_H
#define _VM_JIT_MIPS_MD_H

#include "config.h"

#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/cachectl.h>

#include "vm/types.h"

#include "vm/vm.h"


/* md_stacktrace_get_returnaddress *********************************************

   Returns the return address of the current stackframe, specified by
   the passed stack pointer and the stack frame size.

*******************************************************************************/

inline static void *md_stacktrace_get_returnaddress(void *sp, int32_t stackframesize)
{
	void *ra;

	/* On MIPS the return address is located on the top of the
	   stackframe. */

	ra = *((void **) (((uintptr_t) sp) + stackframesize - 8));

	return ra;
}


/* md_codegen_get_pv_from_pc ***************************************************

   Machine code:

   03c0f809    jalr     s8
   00000000    nop
   27feff9c    addiu    s8,ra,-100

*******************************************************************************/

inline static u1 *md_codegen_get_pv_from_pc(u1 *ra)
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
	}
	else {
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

inline static void md_cacheflush(void *addr, int nbytes)
{
	cacheflush(addr, nbytes, BCACHE);
}


/* md_icacheflush **************************************************************

   Calls the system's function to flush the instruction cache.

*******************************************************************************/

inline static void md_icacheflush(void *addr, int nbytes)
{
	cacheflush(addr, nbytes, ICACHE);
}


/* md_dcacheflush **************************************************************

   Calls the system's function to flush the data cache.

*******************************************************************************/

inline static void md_dcacheflush(void *addr, int nbytes)
{
	cacheflush(addr, nbytes, DCACHE);
}

#endif /* _VM_JIT_MIPS_MD_H */


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
