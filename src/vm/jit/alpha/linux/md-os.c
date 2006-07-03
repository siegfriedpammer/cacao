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

   $Id: md-os.c 5069 2006-07-03 13:45:15Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <ucontext.h>

#include "vm/types.h"

#include "vm/jit/alpha/md-abi.h"

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

	instr = *((s4 *) (_mc->sc_pc));
	addr = _mc->sc_regs[(instr >> 16) & 0x1f];

	if (addr == 0) {
		pv  = (u1 *) _mc->sc_regs[REG_PV];
		sp  = (u1 *) _mc->sc_regs[REG_SP];
		ra  = (u1 *) _mc->sc_regs[REG_RA];       /* this is correct for leafs */
		xpc = (u1 *) _mc->sc_pc;

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


/* md_signal_handler_sigusr2 ***************************************************

   Signal handler for profiling sampling.

*******************************************************************************/

void md_signal_handler_sigusr2(int sig, siginfo_t *siginfo, void *_p)
{
	threadobject *tobj;
	ucontext_t   *_uc;
	mcontext_t   *_mc;
	u1           *pc;

	tobj = THREADOBJECT;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;

	pc = (u1 *) _mc->sc_pc;

	tobj->pc = pc;
}


#if defined(ENABLE_THREADS)
void thread_restartcriticalsection(ucontext_t *_uc)
{
	mcontext_t *_mc;
	u1         *pc;
	u1         *npc;

	_mc = &_uc->uc_mcontext;

	pc = (u1 *) _mc->sc_pc;

	npc = critical_find_restart_point(pc);

	if (npc != NULL)
		_mc->sc_pc = (ptrint) npc;
}
#endif


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
