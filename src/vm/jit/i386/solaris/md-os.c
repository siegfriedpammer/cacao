/* src/vm/jit/i386/solaris/md-os.c - machine dependent i386 Solaris functions

   Copyright (C) 2008
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

#include <stdint.h>
#include <ucontext.h>

#include "vm/types.h"

#include "vm/jit/i386/codegen.h"
#include "vm/jit/i386/md.h"

#include "threads/thread.hpp"

#include "vm/jit/builtin.hpp"
#include "vm/signallocal.hpp"

#include "vm/jit/asmpart.h"
#include "vm/jit/executionstate.h"
#include "vm/jit/stacktrace.hpp"
#include "vm/jit/trap.hpp"


/* md_signal_handler_sigsegv ***************************************************

   Signal handler for hardware exceptions.

*******************************************************************************/

void md_signal_handler_sigsegv(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t     *_uc;
	mcontext_t     *_mc;
	u1             *pv;
	u1             *sp;
	u1             *ra;
	u1             *xpc;
	u1              opc;
	u1              mod;
	u1              rm;
	s4              d;
	s4              disp;
	ptrint          val;
	s4              type;
	void           *p;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;

	pv  = NULL;                 /* is resolved during stackframeinfo creation */
	sp  = (u1 *) _mc->gregs[ESP];
	xpc = (u1 *) _mc->gregs[EIP];
	ra  = xpc;                              /* return address is equal to XPC */

	/* get exception-throwing instruction */

	opc = M_ALD_MEM_GET_OPC(xpc);
	mod = M_ALD_MEM_GET_MOD(xpc);
	rm  = M_ALD_MEM_GET_RM(xpc);

	/* for values see emit_mov_mem_reg and emit_mem */

	if ((opc == 0x8b) && (mod == 0) && (rm == 5)) {
		/* this was a hardware-exception */

		d    = M_ALD_MEM_GET_REG(xpc);
		disp = M_ALD_MEM_GET_DISP(xpc);

		/* we use the exception type as load displacement */

		type = disp;

		/* ATTENTION: The _mc->gregs layout is completely crazy!  The
		   registers are reversed starting with number 4 for REG_EDI
		   (see /usr/include/sys/ucontext.h).  We have to convert that
		   here. */

		val = _mc->gregs[EAX - d];

		if (type == TRAP_COMPILER) {
			/* The PV from the compiler stub is equal to the XPC. */

			pv = xpc;

			/* We use a framesize of zero here because the call pushed
			   the return addres onto the stack. */

			ra = md_stacktrace_get_returnaddress(sp, 0);

			/* Skip the RA on the stack. */

			sp = sp + 1 * SIZEOF_VOID_P;

			/* The XPC is the RA minus 2, because the RA points to the
			   instruction after the call. */

			xpc = ra - 2;
		}
	}
	else {
		/* this was a normal NPE */

		type = TRAP_NullPointerException;
		val  = 0;
	}

	/* Handle the trap. */

	p = trap_handle(type, val, pv, sp, ra, xpc, _p);

	/* Set registers. */

	if (type == TRAP_COMPILER) {
		if (p == NULL) {
			_mc->gregs[ESP] = (uintptr_t) sp;    /* Remove RA from stack. */
		}
	}
}


/* md_signal_handler_sigfpe ****************************************************

   ArithmeticException signal handler for hardware divide by zero
   check.

*******************************************************************************/

void md_signal_handler_sigfpe(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t *_uc;
	mcontext_t *_mc;
	u1         *pv;
	u1         *sp;
	u1         *ra;
	u1         *xpc;
	s4          type;
	ptrint      val;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;

	pv  = NULL;                 /* is resolved during stackframeinfo creation */
	sp  = (u1 *) _mc->gregs[ESP];
	xpc = (u1 *) _mc->gregs[EIP];
	ra  = xpc;                          /* return address is equal to xpc     */

	/* This is an ArithmeticException. */

	type = TRAP_ArithmeticException;
	val  = 0;

	/* Handle the trap. */

	trap_handle(type, val, pv, sp, ra, xpc, _p);
}


/* md_signal_handler_sigill ****************************************************

   Signal handler for hardware patcher traps (ud2).

*******************************************************************************/

void md_signal_handler_sigill(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t *_uc;
	mcontext_t *_mc;
	u1         *pv;
	u1         *sp;
	u1         *ra;
	u1         *xpc;
	s4          type;
	ptrint      val;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;

	pv  = NULL;                 /* is resolved during stackframeinfo creation */
	sp  = (u1 *) _mc->gregs[ESP];
	xpc = (u1 *) _mc->gregs[EIP];
	ra  = xpc;                            /* return address is equal to xpc   */

	// Check if the trap instruction is valid.
	// TODO Move this into patcher_handler.
	if (patcher_is_valid_trap_instruction_at(xpc) == false) {
		// Check if the PC has been patched during our way to this
		// signal handler (see PR85).
		if (patcher_is_patched_at(xpc) == true)
			return;

		// We have a problem...
		log_println("md_signal_handler_sigill: Unknown illegal instruction at 0x%lx", xpc);
#if defined(ENABLE_DISASSEMBLER)
		(void) disassinstr(xpc);
#endif
		vm_abort("Aborting...");
	}

	type = TRAP_PATCHER;
	val  = 0;

	/* Handle the trap. */

	trap_handle(type, val, pv, sp, ra, xpc, _p);
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
	pc = (u1 *) _mc->gregs[EIP];
	sp = (u1 *) _mc->gregs[ESP];

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
	threadobject *t;
	ucontext_t   *_uc;
	mcontext_t   *_mc;
	u1           *pc;

	t = THREADOBJECT;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;

	pc = (u1 *) _mc->gregs[EIP];

	t->pc = pc;
}
#endif


/* md_executionstate_read ******************************************************

   Read the given context into an executionstate for Replacement.

*******************************************************************************/

void md_executionstate_read(executionstate_t *es, void *context)
{
	ucontext_t *_uc;
	mcontext_t *_mc;
	s4          i;

	_uc = (ucontext_t *) context;
	_mc = &_uc->uc_mcontext;

	/* read special registers */
	es->pc = (u1 *) _mc->gregs[EIP];
	es->sp = (u1 *) _mc->gregs[ESP];
	es->pv = NULL;                   /* pv must be looked up via AVL tree */

	/* read integer registers */
	for (i = 0; i < INT_REG_CNT; i++)
		es->intregs[i] = _mc->gregs[EAX - i];

	/* read float registers */
	for (i = 0; i < FLT_REG_CNT; i++)
		es->fltregs[i] = 0xdeadbeefdeadbeefULL;
}


/* md_executionstate_write *****************************************************

   Write the given executionstate back to the context for Replacement.

*******************************************************************************/

void md_executionstate_write(executionstate_t *es, void *context)
{
	ucontext_t *_uc;
	mcontext_t *_mc;
	s4          i;

	_uc = (ucontext_t *) context;
	_mc = &_uc->uc_mcontext;

	/* write integer registers */
	for (i = 0; i < INT_REG_CNT; i++)
		_mc->gregs[EAX - i] = es->intregs[i];

	/* write special registers */
	_mc->gregs[EIP] = (ptrint) es->pc;
	_mc->gregs[ESP] = (ptrint) es->sp;
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
