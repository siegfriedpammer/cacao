/* src/vm/jit/powerpc/darwin/md-os.c - machine dependent PowerPC Darwin functions

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
#include <signal.h>
#include <stdint.h>
#include <ucontext.h>

#include "vm/types.h"

#include "vm/jit/powerpc/codegen.h"
#include "vm/jit/powerpc/darwin/md-abi.h"

#include "threads/thread.hpp"

#include "vm/builtin.h"
#include "vm/global.h"
#include "vm/signallocal.h"

#include "vm/jit/asmpart.h"


/* md_signal_handler_sigsegv ***************************************************

   NullPointerException signal handler for hardware null pointer
   check.

*******************************************************************************/

void md_signal_handler_sigsegv(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t         *_uc;
	mcontext_t          _mc;
	ppc_thread_state_t *_ss;
	ptrint             *gregs;
	u1                 *pv;
	u1                 *sp;
	u1                 *ra;
	u1                 *xpc;
	u4                  mcode;
	int                 s1;
	int16_t             disp;
	int                 d;
	intptr_t            addr;
	intptr_t            val;
	int                 type;
	void               *p;

	_uc = (ucontext_t *) _p;
	_mc = _uc->uc_mcontext;
	_ss = &_mc->ss;

	/* immitate a gregs array */

	gregs = &_ss->r0;

	/* get register values */

	pv  = (u1 *) _ss->r13;
	sp  = (u1 *) _ss->r1;
	ra  = (u1 *) _ss->lr;                        /* this is correct for leafs */
	xpc = (u1 *) _ss->srr0;

	/* get exception-throwing instruction */

	mcode = *((u4 *) xpc);

	s1   = M_INSTR_OP2_IMM_A(mcode);
	disp = M_INSTR_OP2_IMM_I(mcode);
	d    = M_INSTR_OP2_IMM_D(mcode);

	val  = gregs[d];

	/* check for special-load */

	if (s1 == REG_ZERO) {
		/* we use the exception type as load displacement */

		type = disp;

		if (type == EXCEPTION_HARDWARE_COMPILER) {
			/* The XPC is the RA minus 4, because the RA points to the
			   instruction after the call. */

			xpc = ra - 4;
		}
	}
	else {
		/* This is a normal NPE: addr must be NULL and the NPE-type
		   define is 0. */

		addr = gregs[s1];
		type = EXCEPTION_HARDWARE_NULLPOINTER;

		if (addr != 0)
			vm_abort("md_signal_handler_sigsegv: faulting address is not NULL: addr=%p", addr);
	}

	/* Handle the type. */

	p = signal_handle(type, val, pv, sp, ra, xpc, _p);

	/* Set registers. */

	switch (type) {
	case EXCEPTION_HARDWARE_COMPILER:
		if (p != NULL) {
			_ss->r13  = (uintptr_t) p;                              /* REG_PV */
			_ss->srr0 = (uintptr_t) p;
			break;
		}

		/* Get and set the PV from the parent Java method. */

		pv = md_codegen_get_pv_from_pc(ra);

		_ss->r13 = (uintptr_t) pv;

		/* Get the exception object. */

		p = builtin_retrieve_exception();

		assert(p != NULL);

		/* fall-through */

	case EXCEPTION_HARDWARE_PATCHER:
		if (p == NULL)
			break;

		/* fall-through */
		
	default:
		_ss->r11  = (uintptr_t) p;
		_ss->r12  = (uintptr_t) xpc;
		_ss->srr0 = (uintptr_t) asm_handle_exception;
	}
}


/* md_signal_handler_sigtrap ***************************************************

   Signal handler for hardware-traps.

*******************************************************************************/

void md_signal_handler_sigtrap(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t         *_uc;
	mcontext_t          _mc;
	ppc_thread_state_t *_ss;
	ptrint             *gregs;
	u1                 *pv;
	u1                 *sp;
	u1                 *ra;
	u1                 *xpc;
	u4                  mcode;
	int                 s1;
	intptr_t            val;
	int                 type;
	void               *p;

 	_uc = (ucontext_t *) _p;
	_mc = _uc->uc_mcontext;
	_ss = &_mc->ss;

	/* immitate a gregs array */

	gregs = &_ss->r0;

	/* get register values */

	pv  = (u1 *) _ss->r13;
	sp  = (u1 *) _ss->r1;
	ra  = (u1 *) _ss->lr;                    /* this is correct for leafs */
	xpc = (u1 *) _ss->srr0;

	/* get exception-throwing instruction */

	mcode = *((u4 *) xpc);

	s1 = M_OP3_GET_A(mcode);

	/* for now we only handle ArrayIndexOutOfBoundsException */

	type = EXCEPTION_HARDWARE_ARRAYINDEXOUTOFBOUNDS;
	val  = gregs[s1];

	/* Handle the type. */

	p = signal_handle(type, val, pv, sp, ra, xpc, _p);

	/* set registers */

	_ss->r11  = (intptr_t) p;
	_ss->r12  = (intptr_t) xpc;
	_ss->srr0 = (intptr_t) asm_handle_exception;
}


/* md_signal_handler_sigusr2 ***************************************************

   Signal handler for profiling sampling.

*******************************************************************************/

void md_signal_handler_sigusr2(int sig, siginfo_t *siginfo, void *_p)
{
	threadobject       *t;
	ucontext_t         *_uc;
	mcontext_t          _mc;
	ppc_thread_state_t *_ss;
	u1                 *pc;

	t = THREADOBJECT;

	_uc = (ucontext_t *) _p;
	_mc = _uc->uc_mcontext;
	_ss = &_mc->ss;

	pc = (u1 *) _ss->srr0;

	t->pc = pc;
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
