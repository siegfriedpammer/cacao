/* src/vm/jit/i386/md.c - machine dependent i386 functions

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

   $Id: md.c 4806 2006-04-21 11:07:43Z twisti $

*/


#include "config.h"
#include "vm/types.h"

#include <assert.h>

#include "vm/global.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/codegen-common.h"

#if !defined(NDEBUG) && defined(ENABLE_DISASSEMBLER)
#include "vm/options.h" /* XXX debug */
#include "vm/jit/disass.h" /* XXX debug */
#endif


/* md_init *********************************************************************

   Do some machine dependent initialization.

*******************************************************************************/

void md_init(void)
{
	(void) asm_md_init();
}


/* md_stacktrace_get_returnaddress *********************************************

   Returns the return address of the current stackframe, specified by
   the passed stack pointer and the stack frame size.

*******************************************************************************/

u1 *md_stacktrace_get_returnaddress(u1 *sp, u4 framesize)
{
	u1 *ra;

	/* on i386 the return address is above the current stack frame */

	ra = *((u1 **) (sp + framesize));

	return ra;
}


/* md_get_method_patch_address *************************************************

   Gets the patch address of the currently compiled method. The offset
   is extracted from the load instruction(s) before the jump and added
   to the right base address (PV or REG_METHODPTR).

   INVOKESTATIC/SPECIAL:

   b9 30 00 49 b7             mov    $0xb7490030,%ecx
   ff d1                      call   *%ecx

   INVOKEVIRTUAL:

   8b 08                      mov    (%eax),%ecx
   8b 81 00 00 00 00          mov    0x0(%ecx),%eax
   ff d0                      call   *%eax

   INVOKEINTERFACE:

   8b 00                      mov    (%eax),%eax
   8b 88 00 00 00 00          mov    0x0(%eax),%ecx
   8b 81 00 00 00 00          mov    0x0(%ecx),%eax
   ff d0                      call   *%eax

*******************************************************************************/

u1 *md_get_method_patch_address(u1 *ra, stackframeinfo *sfi, u1 *mptr)
{
	u1  mcode;
	s4  offset;
	u1 *pa;                             /* patch address                      */

	/* go back to the actual call instruction (2-bytes) */

	ra = ra - 2;

	/* get the last byte of the call */

	mcode = ra[1];

	/* check for the different calls */

	/* INVOKESTATIC/SPECIAL */

	if (mcode == 0xd1) {
		/* patch address is 8-bytes before the call instruction */

		pa = ra - 4;

	} else if (mcode == 0xd0) {
		/* INVOKEVIRTUAL/INTERFACE */

		/* Get the offset from the instruction (the offset address is
		   4-bytes before the call instruction). */

		offset = *((s4 *) (ra - 4));

		/* add the offset to the method pointer */

		pa = mptr + offset;

	} else {
		/* catch any problems */

		assert(0);
	}

	return pa;
}


/* md_codegen_findmethod *******************************************************

   On this architecture just a wrapper function to codegen_findmethod.

*******************************************************************************/

u1 *md_codegen_findmethod(u1 *ra)
{
	u1 *pv;

	/* the the start address of the function which contains this
       address from the method table */

	pv = codegen_findmethod(ra);

	return pv;
}


/* md_cacheflush ***************************************************************

   Calls the system's function to flush the instruction and data
   cache.

*******************************************************************************/

void md_cacheflush(u1 *addr, s4 nbytes)
{
	/* do nothing */
}


/* md_icacheflush **************************************************************

   Calls the system's function to flush the instruction cache.

*******************************************************************************/

void md_icacheflush(u1 *addr, s4 nbytes)
{
	/* do nothing */
}


/* md_dcacheflush **************************************************************

   Calls the system's function to flush the data cache.

*******************************************************************************/

void md_dcacheflush(u1 *addr, s4 nbytes)
{
	/* do nothing */
}


/* md_patch_replacement_point **************************************************

   Patch the given replacement point.

*******************************************************************************/

void md_patch_replacement_point(rplpoint *rp)
{
    u8 mcode;

	/* XXX this is probably unsafe! */

	/* save the current machine code */
	mcode = *(u8*)rp->pc;

	/* write spinning instruction */
	*(u2*)(rp->pc) = 0xebfe;

	/* write 5th byte */
	rp->pc[4] = (rp->mcode >> 32);

	/* write first word */
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
			
    /* XXX if required asm_cacheflush(rp->pc,8); */
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
