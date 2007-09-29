/* src/vm/jit/alpha/linux/md-os.c - machine dependent Alpha Linux functions

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

#include "vm/types.h"

#include "vm/jit/alpha/codegen.h"
#include "vm/jit/alpha/md-abi.h"

#if defined(ENABLE_THREADS)
# include "threads/native/threads.h"
#endif

#include "vm/signallocal.h"

#include "vm/jit/asmpart.h"
#include "vm/jit/stacktrace.h"


/* md_signal_handler_sigsegv ***************************************************

   NullPointerException signal handler for hardware null pointer
   check.

*******************************************************************************/

void md_signal_handler_sigsegv(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t     *_uc;
	mcontext_t     *_mc;
	u1             *pv;
	u1             *sp;
	u1             *ra;
	u1             *xpc;
	u4              mcode;
	s4              d;
	s4              s1;
	s4              disp;
	intptr_t        val;
	intptr_t        addr;
	int             type;
	void           *p;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;

	pv  = (u1 *) _mc->sc_regs[REG_PV];
	sp  = (u1 *) _mc->sc_regs[REG_SP];
	ra  = (u1 *) _mc->sc_regs[REG_RA];           /* this is correct for leafs */
	xpc = (u1 *) _mc->sc_pc;

	/* get exception-throwing instruction */

	mcode = *((u4 *) xpc);

	d    = M_MEM_GET_Ra(mcode);
	s1   = M_MEM_GET_Rb(mcode);
	disp = M_MEM_GET_Memory_disp(mcode);

	val  = _mc->sc_regs[d];

	/* check for special-load */

	if (s1 == REG_ZERO) {
		/* we use the exception type as load displacement */

		type = disp;
	}
	else {
		/* This is a normal NPE: addr must be NULL and the NPE-type
		   define is 0. */

		addr = _mc->sc_regs[s1];
		type = (int) addr;
	}

	/* Handle the type. */

	p = signal_handle(type, val, pv, sp, ra, xpc, _p);

	/* set registers */

	if (p != NULL) {
		_mc->sc_regs[REG_ITMP1_XPTR] = (intptr_t) p;
		_mc->sc_regs[REG_ITMP2_XPC]  = (intptr_t) xpc;
		_mc->sc_pc                   = (intptr_t) asm_handle_exception;
	}
}


/* md_signal_handler_sigusr1 ***************************************************

   Signal handler for suspending threads.

*******************************************************************************/

#if defined(ENABLE_THREADS) && defined(ENABLE_GC_CACAO)
void md_signal_handler_sigusr1(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t *_uc;
	mcontext_t *_mc;
	u1         *pc;
	u1         *sp;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;

	/* get the PC and SP for this thread */
	pc = (u1 *) _mc->sc_pc;
	sp = (u1 *) _mc->sc_regs[REG_SP];

	/* now suspend the current thread */
	threads_suspend_ack(pc, sp);
}
#endif


/* md_signal_handler_sigusr2 ***************************************************

   Signal handler for profiling sampling.

*******************************************************************************/

#if defined(ENABLE_THREADS)
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
#endif


/* md_replace_executionstate_read **********************************************

   Read the given context into an executionstate for Replacement.

*******************************************************************************/

#if defined(ENABLE_REPLACEMENT)
void md_replace_executionstate_read(executionstate_t *es, void *context)
{
	ucontext_t *_uc;
	mcontext_t *_mc;
	s4          i;

	_uc = (ucontext_t *) context;
	_mc = &_uc->uc_mcontext;

	/* read special registers */
	es->pc = (u1 *) _mc->sc_pc;
	es->sp = (u1 *) _mc->sc_regs[REG_SP];
	es->pv = (u1 *) _mc->sc_regs[REG_PV];

	/* read integer registers */
	for (i = 0; i < INT_REG_CNT; i++)
		es->intregs[i] = _mc->sc_regs[i];

	/* read float registers */
	for (i = 0; i < FLT_REG_CNT; i++)
		es->fltregs[i] = _mc->sc_fpregs[i];
}
#endif


/* md_replace_executionstate_write *********************************************

   Write the given executionstate back to the context for Replacement.

*******************************************************************************/

#if defined(ENABLE_REPLACEMENT)
void md_replace_executionstate_write(executionstate_t *es, void *context)
{
	ucontext_t *_uc;
	mcontext_t *_mc;
	s4          i;

	_uc = (ucontext_t *) context;
	_mc = &_uc->uc_mcontext;

	/* write integer registers */
	for (i = 0; i < INT_REG_CNT; i++)
		_mc->sc_regs[i] = es->intregs[i];

	/* write float registers */
	for (i = 0; i < FLT_REG_CNT; i++)
		_mc->sc_fpregs[i] = es->fltregs[i];

	/* write special registers */
	_mc->sc_pc           = (ptrint) es->pc;
	_mc->sc_regs[REG_SP] = (ptrint) es->sp;
	_mc->sc_regs[REG_PV] = (ptrint) es->pv;
}
#endif


/* md_critical_section_restart *************************************************

   Search the critical sections tree for a matching section and set
   the PC to the restart point, if necessary.

*******************************************************************************/

#if defined(ENABLE_THREADS)
void md_critical_section_restart(ucontext_t *_uc)
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
