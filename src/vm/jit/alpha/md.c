/* src/vm/jit/alpha/md.c - machine dependent Alpha functions

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

   Authors: Andreas Krall
            Reinhard Grafl

   Changes: Joseph Wenninger
            Christian Thalinger

   $Id: md.c 4392 2006-01-31 15:35:22Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <ucontext.h>

#if defined(__LINUX__)
# include <asm/fpu.h>

extern unsigned long ieee_get_fp_control();
extern void ieee_set_fp_control(unsigned long fp_control);
#endif

#include "vm/types.h"

#include "vm/jit/alpha/md-abi.h"

#include "vm/exceptions.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/stacktrace.h"


/* global variables ***********************************************************/

bool has_ext_instr_set = false;             /* has instruction set extensions */


/* md_init *********************************************************************

   Do some machine dependent initialization.

*******************************************************************************/

void md_init(void)
{
	/* check for extended instruction set */

	has_ext_instr_set = !asm_md_init();

#if defined(__LINUX__)
	/* Linux on Digital Alpha needs an initialisation of the ieee
	   floating point control for IEEE compliant arithmetic (option
	   -mieee of GCC). Under Digital Unix this is done
	   automatically. */

	/* initialize floating point control */

	ieee_set_fp_control(ieee_get_fp_control()
						& ~IEEE_TRAP_ENABLE_INV
						& ~IEEE_TRAP_ENABLE_DZE
/*  						& ~IEEE_TRAP_ENABLE_UNF   we dont want underflow */
						& ~IEEE_TRAP_ENABLE_OVF);
#endif
}


/* md_stacktrace_get_returnaddress *********************************************

   Returns the return address of the current stackframe, specified by
   the passed stack pointer and the stack frame size.

*******************************************************************************/

u1 *md_stacktrace_get_returnaddress(u1 *sp, u4 framesize)
{
	u1 *ra;

	/* on Alpha the return address is located on the top of the stackframe */

	ra = *((u1 **) (sp + framesize - SIZEOF_VOID_P));

	return ra;
}


/* md_codegen_findmethod *******************************************************

   Machine code:

   6b5b4000    jsr     (pv)
   277afffe    ldah    pv,-2(ra)
   237ba61c    lda     pv,-23012(pv)

*******************************************************************************/

u1 *md_codegen_findmethod(u1 *ra)
{
	u1 *pv;
	u4  mcode;
	s4  offset;

	pv = ra;

	/* get first instruction word after jump */

	mcode = *((u4 *) ra);

	/* check if we have 2 instructions (ldah, lda) */

	if ((mcode >> 16) == 0x277a) {
		/* get displacement of first instruction (ldah) */

		offset = (s4) (mcode << 16);
		pv += offset;

		/* get displacement of second instruction (lda) */

		mcode = *((u4 *) (ra + 1 * 4));

		if ((mcode >> 16) != 0x237b) {
			log_text("No `lda pv,x(pv)' instruction found on return address!");
			assert(0);
		}

		offset = (s2) (mcode & 0x0000ffff);
		pv += offset;

	} else {
		/* get displacement of first instruction (lda) */

		if ((mcode >> 16) != 0x237a) {
			log_text("No `lda pv,x(ra)' instruction found on return address!");
			assert(0);
		}

		offset = (s2) (mcode & 0x0000ffff);
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
