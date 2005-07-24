/* src/vm/jit/alpha/md.c - machine dependent Alpha functions

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

   Authors: Andreas Krall
            Reinhard Grafl

   Changes: Joseph Wenninger
            Christian Thalinger

   $Id: md.c 3107 2005-07-24 23:02:28Z twisti $

*/


#include <assert.h>
#include <ucontext.h>

#include "config.h"

#include "vm/jit/alpha/md-abi.h"
#include "vm/jit/alpha/types.h"

#include "vm/exceptions.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/stacktrace.h"


/* md_init *********************************************************************

   Do some machine dependent initialization.

*******************************************************************************/

void md_init(void)
{
#if 0
	/* XXX TWISTI: do we really need this? fptest's seem to work fine */

#if defined(__LINUX__)
/* Linux on Digital Alpha needs an initialisation of the ieee floating point
	control for IEEE compliant arithmetic (option -mieee of GCC). Under
	Digital Unix this is done automatically.
*/

#include <asm/fpu.h>

extern unsigned long ieee_get_fp_control();
extern void ieee_set_fp_control(unsigned long fp_control);

	/* initialize floating point control */

	ieee_set_fp_control(ieee_get_fp_control()
						& ~IEEE_TRAP_ENABLE_INV
						& ~IEEE_TRAP_ENABLE_DZE
/*  						& ~IEEE_TRAP_ENABLE_UNF   we dont want underflow */
						& ~IEEE_TRAP_ENABLE_OVF);
#endif
#endif

	/* nothing to do */
}


/* signal_handler_sigsegv ******************************************************

   NullPointerException signal handler for hardware null pointer check.

*******************************************************************************/

void signal_handler_sigsegv(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t  *_uc;
	mcontext_t  *_mc;
	u4           instr;
	ptrint       addr;
	u1          *pv;
	u1          *sp;
	functionptr  ra;
	functionptr  xpc;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;

	instr = *((s4 *) (_mc->sc_pc));
	addr = _mc->sc_regs[(instr >> 16) & 0x1f];

	if (addr == 0) {
		pv  = (u1 *) _mc->sc_regs[REG_PV];
		sp  = (u1 *) _mc->sc_regs[REG_SP];
		ra  = (functionptr) _mc->sc_regs[REG_RA]; /* this is correct for leafs*/
		xpc = (functionptr) _mc->sc_pc;

		_mc->sc_regs[REG_ITMP1_XPTR] =
			(ptrint) stacktrace_hardware_nullpointerexception(pv, sp, ra, xpc);

		_mc->sc_regs[REG_ITMP2_XPC] = (ptrint) xpc;
		_mc->sc_pc = (ptrint) asm_handle_exception;

	} else {
		addr += (long) ((instr << 16) >> 16);

		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Segmentation fault: 0x%016lx at 0x%016lx\n",
								   addr, _mc->sc_pc);
	}
}


#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void thread_restartcriticalsection(ucontext_t *uc)
{
	void *critical;

	critical = thread_checkcritical((void *) uc->uc_mcontext.sc_pc);

	if (critical)
		uc->uc_mcontext.sc_pc = (ptrint) critical;
}
#endif


/* md_stacktrace_get_returnaddress *********************************************

   Returns the return address of the current stackframe, specified by
   the passed stack pointer and the stack frame size.

*******************************************************************************/

functionptr md_stacktrace_get_returnaddress(u1 *sp, u4 framesize)
{
	functionptr ra;

	/* on Alpha the return address is located on the top of the stackframe */

	ra = (functionptr) *((u1 **) (sp + framesize - SIZEOF_VOID_P));

	return ra;
}


/* codegen_findmethod **********************************************************

   Machine code:

   6b5b4000    jsr     (pv)
   277afffe    ldah    pv,-2(ra)
   237ba61c    lda     pv,-23012(pv)

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
