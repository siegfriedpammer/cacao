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

   Changes:

   $Id: md.c 4357 2006-01-22 23:33:38Z twisti $

*/

#include "config.h"
#include "vm/types.h"

#include "md-abi.h"

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

u1 *md_stacktrace_get_returnaddress(u1 *sp, u4 framesize)
{
	u1 *ra;

	/* on PowerPC the return address is located in the linkage area */

	ra = *((u1 **) (sp + framesize + LA_LR_OFFSET));

	return ra;
}


/* md_codegen_findmethod *******************************************************

   Machine code:

   7d6802a6    mflr  r11
   39abffe0    addi  r13,r11,-32

*******************************************************************************/

u1 *md_codegen_findmethod(u1 *ra)
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
