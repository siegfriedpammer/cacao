/* src/vm/jit/powerpc/darwin/md-os.c - machine dependent PowerPC Darwin functions

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

   $Id: md-os.c 6123 2006-12-05 21:10:54Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <signal.h>
#include <ucontext.h>

#include "vm/types.h"

#include "vm/jit/powerpc/darwin/md-abi.h"

#include "vm/exceptions.h"
#include "vm/global.h"
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
	ucontext_t         *_uc;
	mcontext_t          _mc;
	ppc_thread_state_t *_ss;
	ptrint             *gregs;
	u4                  instr;
	s4                  reg;
	s4                  disp;
	ptrint              addr;
	u1                 *pv;
	u1                 *sp;
	u1                 *ra;
	u1                 *xpc;
	stackframeinfo      sfi;
	java_objectheader  *o;

	_uc = (ucontext_t *) _p;
	_mc = _uc->uc_mcontext;
	_ss = &_mc->ss;

	/* check for NullPointerException */

	gregs = &_ss->r0;

	instr = *((u4 *) _ss->srr0);
	reg   = (instr >> 16) & 31;
	disp  = (instr & 0xffff);
	addr  = gregs[reg];

	pv  = (u1 *) _ss->r13;
	sp  = (u1 *) _ss->r1;
	ra  = (u1 *) _ss->lr;                    /* this is correct for leafs */
	xpc = (u1 *) _ss->srr0;

	/* create stackframeinfo */

	stacktrace_create_extern_stackframeinfo(&sfi, pv, sp, ra, xpc);

	if (reg == REG_ZERO) {
		switch (disp) {
		case EXCEPTION_LOAD_DISP_ARITHMETIC:
			vm_abort("ArithmeticException");
			break;
		case EXCEPTION_LOAD_DISP_ARRAYINDEXOUTOFBOUNDS:
			log_println("ArrayIndexOutOfBoundsException");
			o = new_arrayindexoutofboundsexception(0);
			break;
		case EXCEPTION_LOAD_DISP_CLASSCAST:
			vm_abort("ClassCastException");
			break;
		default:
			vm_abort("unknown exception %d", disp);
		}
	}
	else if (addr == 0) {
		o = exceptions_new_nullpointerexception();
	}
	else {
		codegen_get_pv_from_pc(xpc);

		/* this should not happen */

		assert(0);
	}

	/* remove stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	_ss->r11  = (ptrint) o;
	_ss->r12  = (ptrint) xpc;
	_ss->srr0 = (ptrint) asm_handle_exception;
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


#if defined(ENABLE_THREADS)
void thread_restartcriticalsection(ucontext_t *_uc)
{
	mcontext_t          _mc;
	ppc_thread_state_t *_ss;
	void               *critical;

	_mc = _uc->uc_mcontext;
	_ss = &_mc->ss;

	critical = critical_find_restart_point((void *) _ss->srr0);

	if (critical)
		_ss->srr0 = (ptrint) critical;
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
