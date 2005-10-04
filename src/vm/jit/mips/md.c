/* src/vm/jit/mips/md.c - machine dependent MIPS functions

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Christian Thalinger

   Changes: 

   $Id: md.c 3323 2005-10-04 18:33:30Z twisti $

*/


#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>

#include "config.h"
#include "vm/types.h"

#include "vm/global.h"


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

functionptr md_stacktrace_get_returnaddress(u1 *sp, u4 framesize)
{
	functionptr ra;

	/* on MIPS the return address is located on the top of the stackframe */

	ra = (functionptr) *((u1 **) (sp + framesize - SIZEOF_VOID_P));

	return ra;
}


/* codegen_findmethod **********************************************************

   Machine code:

   6b5b4000    jsr     (pv)
   237affe8    lda     pv,-24(ra)

*******************************************************************************/

functionptr codegen_findmethod(functionptr pc)
{
	u1 *ra;
	u1 *pv;
	u4  mcode;
	s4  offset;

	ra = (u1 *) pc;
	pv = ra;

	/* get first instruction word after jump */

	mcode = *((u4 *) ra);

	/* check if we have 2 instructions (ldah, lda) */

	if ((mcode >> 16) == 0x3c19) {
		/* get displacement of first instruction (lui) */

		offset = (s4) (mcode << 16);
		pv += offset;

		/* get displacement of second instruction (daddiu) */

		mcode = *((u4 *) (ra + 1 * 4));

		if ((mcode >> 16) != 0x6739) {
			log_text("No `daddiu' instruction found on return address!");
			assert(0);
		}

		offset = (s2) (mcode & 0x0000ffff);
		pv += offset;

	} else {
		/* get offset of first instruction (daddiu) */

		mcode = *((u4 *) ra);

		if ((mcode >> 16) != 0x67fe) {
			log_text("No `daddiu s8,ra,x' instruction found on return address!");
			assert(0);
		}

		offset = (s2) (mcode & 0x0000ffff);
		pv += offset;
	}

	return (functionptr) pv;
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
