/* src/vm/jit/sparc64/solaris/md-os.c - machine dependent SPARC Solaris functions

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
#include <ucontext.h>

/* work around name clash */
#undef REG_SP

#include "vm/types.h"

#include "vm/jit/sparc64/codegen.h"
#include "vm/jit/sparc64/md-abi.h"

#include "vm/exceptions.h"
#include "vm/signallocal.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/stacktrace.h"


ptrint md_get_reg_from_context(mcontext_t *_mc, u4 rindex)
{
	ptrint val;	
	s8     *window;
	
	
	/* return 0 for REG_ZERO */
	
	if (rindex == 0)
		return 0;
		
	
	if (rindex <= 15) {
		
		/* register is in global or out range, available in context */
		val = _mc->gregs[rindex + 3];
		
	}
	else {
		assert(rindex <= 31);
		
		/* register is local or in, need to fetch from regsave area on stack */

		window = (s8 *) (_mc->gregs[REG_O6] + BIAS);
		val = window[rindex - 16];

	}
	
	return val;
}


/* md_signal_handler_sigsegv ***************************************************

   NullPointerException signal handler for hardware null pointer
   check.

*******************************************************************************/

void md_signal_handler_sigsegv(int sig, siginfo_t *siginfo, void *_p)
{
	stackframeinfo  sfi;
	ucontext_t     *_uc;
	mcontext_t     *_mc;
	ptrint          addr;
	u1             *pv;
	u1             *sp;
	u1             *ra;
	u1             *xpc;
	u4              mcode;
	int             d;
	int             s1;
	int16_t         disp;
	intptr_t        val;
	int             type;
	void           *p;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;

	pv  = (u1 *) md_get_reg_from_context(_mc, REG_PV_CALLEE);
	sp  = (u1 *) _mc->gregs[REG_O6];
	ra  = (u1 *) md_get_reg_from_context(_mc, REG_RA_CALLEE);
	xpc = (u1 *) _mc->gregs[REG_PC];


	/* get exception-throwing instruction */	

	mcode = *((u4 *) xpc);

	d    = M_OP3_GET_RD(mcode);
	s1   = M_OP3_GET_RS(mcode);
	disp = M_OP3_GET_IMM(mcode);

	/* flush register windows? */
	
	val  = md_get_reg_from_context(_mc, d);

	/* check for special-load */

	if (s1 == REG_ZERO) {
		/* we use the exception type as load displacement */

		type = disp;
	}
	else {
		/* This is a normal NPE: addr must be NULL and the NPE-type
		   define is 0. */

		addr  = md_get_reg_from_context(_mc, s1);
		type = (int) addr;
	}

	/* create stackframeinfo */

	stacktrace_create_extern_stackframeinfo(&sfi, pv, sp, ra, xpc);

	/* Handle the type. */

	p = signal_handle(xpc, type, val);

	/* remove stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	/* set registers */

	_mc->gregs[REG_G2]  = (intptr_t) p;                     /* REG_ITMP2_XPTR */
	_mc->gregs[REG_G3]  = (intptr_t) xpc;                    /* REG_ITMP3_XPC */
	_mc->gregs[REG_PC]  = (intptr_t) asm_handle_exception;
	_mc->gregs[REG_nPC] = (intptr_t) asm_handle_exception + 4;	
}


#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void thread_restartcriticalsection(ucontext_t *_uc)
{
	mcontext_t *_mc;
	void       *critical;

	_mc = &_uc->uc_mcontext;

	critical = thread_checkcritical((void *) _mc->sc_pc);

	if (critical)
		_mc->sc_pc = (ptrint) critical;
}
#endif


/* md_icacheflush **************************************************************

   Calls the system's function to flush the instruction cache.

*******************************************************************************/

void md_icacheflush(u1 *addr, s4 nbytes)
{
	u1* end;
	
	end = addr + nbytes;
	
	/* zero the least significant 3 bits to align on a 64-bit boundary */
	addr = (u1 *) (((ptrint) addr) & -8l);
	
	while (addr < end) {
		__asm__ (
			"flush %0"
			:
			: "r"(addr)
			);
		addr += 8;
	}
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
