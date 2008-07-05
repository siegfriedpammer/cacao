/* src/vm/jit/powerpc64/linux/md-os.c - machine dependent PowerPC64 Linux functions

   Copyright (C) 1996-2005, 2006, 2007, 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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

#include "vm/jit/powerpc64/codegen.h"
#include "vm/jit/powerpc64/md.h"
#include "vm/jit/powerpc64/linux/md-abi.h"

#include "threads/thread.h"

#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/signallocal.h"

#include "vm/jit/asmpart.h"
#include "vm/jit/executionstate.h"

#if defined(ENABLE_PROFILING)
# include "vm/jit/optimizing/profile.h"
#endif

#include "vm/jit/stacktrace.h"
#include "vm/jit/trap.h"


/* md_signal_handler_sigsegv ***************************************************
 
	Signal handler for hardware-exceptions.

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
	int             s1;
	int16_t         disp;
	int             d;
	int             type;
	intptr_t        addr;
	intptr_t        val;
	void           *p;

 	_uc = (ucontext_t *) _p;
 	_mc = &(_uc->uc_mcontext);

	/* get register values */

	pv = (u1*) _mc->gp_regs[REG_PV];
	sp = (u1*) _mc->gp_regs[REG_SP];
	ra = (u1*) _mc->gp_regs[PT_LNK];                     /* correct for leafs */
	xpc =(u1*) _mc->gp_regs[PT_NIP];

	/* get the throwing instruction */

	mcode = *((u4*)xpc);

	s1   = M_INSTR_OP2_IMM_A(mcode);
	disp = M_INSTR_OP2_IMM_I(mcode);
	d    = M_INSTR_OP2_IMM_D(mcode);

	val  = _mc->gp_regs[d];

	if (s1 == REG_ZERO) {
		/* We use the exception type as load displacement. */
		type = disp;

		if (type == TRAP_COMPILER) {
			/* The XPC is the RA minus 1, because the RA points to the
			   instruction after the call. */

			xpc = ra - 4;
		}
	}
	else {
		/* Normal NPE. */
		addr = _mc->gp_regs[s1];
		type = (int) addr;
	}

	/* Handle the trap. */

	p = trap_handle(type, val, pv, sp, ra, xpc, _p);

	/* Set registers. */

	switch (type) {
	case TRAP_COMPILER:
		if (p != NULL) {
			_mc->gp_regs[REG_PV] = (uintptr_t) p;
			_mc->gp_regs[PT_NIP] = (uintptr_t) p;
			break;
		}

		/* Get and set the PV from the parent Java method. */

		pv = md_codegen_get_pv_from_pc(ra);

		_mc->gp_regs[REG_PV] = (uintptr_t) pv;

		/* Get the exception object. */

		p = builtin_retrieve_exception();

		assert(p != NULL);

		/* fall-through */

	case TRAP_PATCHER:
		if (p == NULL)
			break;

		/* fall-through */
		
	default:
		_mc->gp_regs[REG_ITMP1_XPTR] = (uintptr_t) p;
		_mc->gp_regs[REG_ITMP2_XPC]  = (uintptr_t) xpc;
		_mc->gp_regs[PT_NIP]         = (uintptr_t) asm_handle_exception;
	}
}


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
 	_mc = &(_uc->uc_mcontext);

	pc = (u1 *) _mc->gp_regs[PT_NIP];

	tobj->pc = pc;
}
#endif


/* md_executionstate_read ******************************************************

   Read the given context into an executionstate.

*******************************************************************************/

void md_executionstate_read(executionstate_t *es, void *context)
{
#if 0
	ucontext_t    *_uc;
	mcontext_t    *_mc;
	unsigned long *_gregs;
	s4              i;

	_uc = (ucontext_t *) context;

	_mc    = _uc->uc_mcontext.uc_regs;
	_gregs = _mc->gregs;

	/* read special registers */
	es->pc = (u1 *) _gregs[PT_NIP];
	es->sp = (u1 *) _gregs[REG_SP];
	es->pv = (u1 *) _gregs[REG_PV];
	es->ra = (u1 *) _gregs[PT_LNK];

	/* read integer registers */
	for (i = 0; i < INT_REG_CNT; i++)
		es->intregs[i] = _gregs[i];

	/* read float registers */
	/* Do not use the assignment operator '=', as the type of
	 * the _mc->fpregs[i] can cause invalid conversions. */

	assert(sizeof(_mc->fpregs.fpregs) == sizeof(es->fltregs));
	system_memcpy(&es->fltregs, &_mc->fpregs.fpregs, sizeof(_mc->fpregs.fpregs));
#endif

	vm_abort("md_executionstate_read: IMPLEMENT ME!");
}


/* md_executionstate_write *****************************************************

   Write the given executionstate back to the context.

*******************************************************************************/

void md_executionstate_write(executionstate_t *es, void *context)
{
#if 0
	ucontext_t    *_uc;
	mcontext_t    *_mc;
	unsigned long *_gregs;
	s4              i;

	_uc = (ucontext_t *) context;

	_mc    = _uc->uc_mcontext.uc_regs;
	_gregs = _mc->gregs;

	/* write integer registers */
	for (i = 0; i < INT_REG_CNT; i++)
		_gregs[i] = es->intregs[i];

	/* write float registers */
	/* Do not use the assignment operator '=', as the type of
	 * the _mc->fpregs[i] can cause invalid conversions. */

	assert(sizeof(_mc->fpregs.fpregs) == sizeof(es->fltregs));
	system_memcpy(&_mc->fpregs.fpregs, &es->fltregs, sizeof(_mc->fpregs.fpregs));

	/* write special registers */
	_gregs[PT_NIP] = (ptrint) es->pc;
	_gregs[REG_SP] = (ptrint) es->sp;
	_gregs[REG_PV] = (ptrint) es->pv;
	_gregs[PT_LNK] = (ptrint) es->ra;
#endif

	vm_abort("md_executionstate_write: IMPLEMENT ME!");
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
