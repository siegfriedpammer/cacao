/* src/vm/jit/alpha/linux/md.c - machine dependent Alpha Linux functions

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

   $Id: md-os.c 4357 2006-01-22 23:33:38Z twisti $

*/

#include "config.h"

#include <assert.h>
#include <ucontext.h>

#undef REG_SP

#include "vm/types.h"

#include "vm/jit/sparc64/md-abi.h"

#include "vm/exceptions.h"
#include "vm/signallocal.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/stacktrace.h"


/* md_signal_handler_sigsegv ***************************************************

   NullPointerException signal handler for hardware null pointer
   check.

*******************************************************************************/

void md_signal_handler_sigsegv(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t  *_uc;
	mcontext_t  *_mc;
	u4           instr;
	ptrint       addr;
	u1          *pv;
	u1          *sp;
	u1          *ra;
	u1          *xpc;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;

	instr = *((s4 *) (_mc->gregs[REG_PC]));
	/*addr = _mc->sc_regs[(instr >> 16) & 0x1f];*/
	addr = 0;

	if (addr == 0) {

#if 0
		pv  = (u1 *) _mc->gregs[REG_G3];
		sp  = (u1 *) _mc->mc_fp;
		ra  = (u1 *) _mc->mc_i7;       /* this is correct for leafs */
		xpc = (u1 *) _mc->mc_gregs[MC_PC];

		_mc->mc_gregs[MC_G4] =
			(ptrint) stacktrace_hardware_nullpointerexception(pv, sp, ra, xpc);

		_mc->mc_gregs[MC_G5] = (ptrint) xpc;
		_mc->mc_gregs[MC_PC] = (ptrint) asm_handle_exception;
#endif
		assert(0);

	} else {
		addr += (long) ((instr << 16) >> 16);

		/*
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Segmentation fault: 0x%016lx at 0x%016lx\n",
								   addr, _mc->mc_gregs[MC_PC]);
								   */
		assert(0);
	}
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
