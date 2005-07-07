/* src/vm/jit/powerpc/md.c - machine dependent PowerPC functions

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

   $Id: md.c 2922 2005-07-07 09:27:20Z twisti $

*/


#include "vm/jit/powerpc/types.h"
#include "vm/jit/powerpc/linux/md-abi.h"

#include "vm/global.h"


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

functionptr md_stacktrace_get_returnaddress(u1 *sp, u4 framesize)
{
	functionptr ra;

	/* on PowerPC the return address is located in the linkage area */

	ra = (functionptr) *((u1 **) (sp + framesize + LA_LR_OFFSET));

	return ra;
}


/* codegen_findmethod **********************************************************

   Machine code:

   7d6802a6    mflr  r11
   39abffe0    addi  r13,r11,-32

*******************************************************************************/

u1 *codegen_findmethod(u1 *ra)
{
	u1 *pv;
	u4  mcode;
	s2  offset;

	pv = ra;

	/* get offset of first instruction (addi) */

	mcode = *((u4 *) (ra + 1 * 4));
	offset = (s2) (mcode & 0x0000ffff);
	pv += offset;

	/* check for second instruction (addis) */

	mcode = *((u4 *) (ra + 2 * 4));

	if ((mcode >> 16) == 0x3dad) {
		offset = (s2) (mcode << 16);
		pv += offset;
	}

	return pv;
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
