/* src/vm/jit/x86_64/linux/md-os.c - machine dependent x86_64 Linux functions

   Copyright (C) 2007, 2008
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


#define _GNU_SOURCE

#include "config.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <ucontext.h>

#include "vm/types.h"

#include "vm/jit/x86_64/codegen.h"
#include "vm/jit/x86_64/md.h"

#include "threads/thread.h"

#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/signallocal.h"

#include "vm/jit/asmpart.h"
#include "vm/jit/executionstate.h"
#include "vm/jit/stacktrace.h"


/* md_signal_handler_sigsegv ***************************************************

   Signal handler for hardware exception.

*******************************************************************************/

void md_signal_handler_sigsegv(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t     *_uc;
	mcontext_t     *_mc;
	void           *pv;
	u1             *sp;
	u1             *ra;
	u1             *xpc;
	u1              opc;
	u1              mod;
	u1              rm;
	s4              d;
	s4              disp;
	int             type;
	intptr_t        val;
	void           *p;
	java_object_t  *o;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;

	/* ATTENTION: Don't use CACAO's internal REG_* defines as they are
	   different to the ones in <ucontext.h>. */

	pv  = NULL;                 /* is resolved during stackframeinfo creation */
	sp  = (u1 *) _mc->gregs[REG_RSP];
	xpc = (u1 *) _mc->gregs[REG_RIP];
	ra  = xpc;                              /* return address is equal to XPC */

#if 0
	/* check for StackOverflowException */

	threads_check_stackoverflow(sp);
#endif

	/* get exception-throwing instruction */

	opc = M_ALD_MEM_GET_OPC(xpc);
	mod = M_ALD_MEM_GET_MOD(xpc);
	rm  = M_ALD_MEM_GET_RM(xpc);

	/* for values see emit_mov_mem_reg and emit_mem */

	if ((opc == 0x8b) && (mod == 0) && (rm == 4)) {
		/* this was a hardware-exception */

		d    = M_ALD_MEM_GET_REG(xpc);
		disp = M_ALD_MEM_GET_DISP(xpc);

		/* we use the exception type as load displacement */

		type = disp;

		/* XXX FIX ME! */

		/* ATTENTION: The _mc->gregs layout is even worse than on
		   i386! See /usr/include/sys/ucontext.h.  We need a
		   switch-case here... */

		switch (d) {
		case 0:  /* REG_RAX == 13 */
			d = REG_RAX;
			break;
		case 1:  /* REG_RCX == 14 */
			d = REG_RCX;
			break;
		case 2:  /* REG_RDX == 12 */
			d = REG_RDX;
			break;
		case 3:  /* REG_RBX == 11 */
			d = REG_RBX;
			break;
		case 4:  /* REG_RSP == 15 */
			d = REG_RSP;
			break;
		case 5:  /* REG_RBP == 10 */
			d = REG_RBP;
			break;
		case 6:  /* REG_RSI == 9  */
			d = REG_RSI;
			break;
		case 7:  /* REG_RDI == 8  */
			d = REG_RDI;
			break;
		case 8:  /* REG_R8  == 0  */
		case 9:  /* REG_R9  == 1  */
		case 10: /* REG_R10 == 2  */
		case 11: /* REG_R11 == 3  */
		case 12: /* REG_R12 == 4  */
		case 13: /* REG_R13 == 5  */
		case 14: /* REG_R14 == 6  */
		case 15: /* REG_R15 == 7  */
			d = d - 8;
			break;
		}

		val = _mc->gregs[d];

		if (type == EXCEPTION_HARDWARE_COMPILER) {
			/* The PV from the compiler stub is equal to the XPC. */

			pv = xpc;

			/* We use a framesize of zero here because the call pushed
			   the return addres onto the stack. */

			ra = md_stacktrace_get_returnaddress(sp, 0);

			/* Skip the RA on the stack. */

			sp = sp + 1 * SIZEOF_VOID_P;

			/* The XPC is the RA minus 1, because the RA points to the
			   instruction after the call. */

			xpc = ra - 3;
		}
	}
	else {
		/* this was a normal NPE */

		type = EXCEPTION_HARDWARE_NULLPOINTER;
		val  = 0;
	}

	/* Handle the type. */

	p = signal_handle(type, val, pv, sp, ra, xpc, _p);

	/* Set registers. */

	if (type == EXCEPTION_HARDWARE_COMPILER) {
		if (p == NULL) {
			o = builtin_retrieve_exception();

			_mc->gregs[REG_RSP] = (uintptr_t) sp;    /* Remove RA from stack. */

			_mc->gregs[REG_RAX] = (uintptr_t) o;
			_mc->gregs[REG_R10] = (uintptr_t) xpc;           /* REG_ITMP2_XPC */
			_mc->gregs[REG_RIP] = (uintptr_t) asm_handle_exception;
		}
		else {
			_mc->gregs[REG_RIP] = (uintptr_t) p;
		}
	}
	else {
		_mc->gregs[REG_RAX] = (uintptr_t) p;
		_mc->gregs[REG_R10] = (uintptr_t) xpc;               /* REG_ITMP2_XPC */
		_mc->gregs[REG_RIP] = (uintptr_t) asm_handle_exception;
	}
}


/* md_signal_handler_sigfpe ****************************************************

   ArithmeticException signal handler for hardware divide by zero
   check.

*******************************************************************************/

void md_signal_handler_sigfpe(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t     *_uc;
	mcontext_t     *_mc;
	u1             *pv;
	u1             *sp;
	u1             *ra;
	u1             *xpc;
	int             type;
	intptr_t        val;
	void           *p;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;

	/* ATTENTION: Don't use CACAO's internal REG_* defines as they are
	   different to the ones in <ucontext.h>. */

	pv  = NULL;
	sp  = (u1 *) _mc->gregs[REG_RSP];
	xpc = (u1 *) _mc->gregs[REG_RIP];
	ra  = xpc;                          /* return address is equal to xpc     */

	/* this is an ArithmeticException */

	type = EXCEPTION_HARDWARE_ARITHMETIC;
	val  = 0;

	/* Handle the type. */

	p = signal_handle(type, val, pv, sp, ra, xpc, _p);

	/* set registers */

	_mc->gregs[REG_RAX] = (intptr_t) p;
	_mc->gregs[REG_R10] = (intptr_t) xpc;                    /* REG_ITMP2_XPC */
	_mc->gregs[REG_RIP] = (intptr_t) asm_handle_exception;
}


/* md_signal_handler_sigill ****************************************************

   Signal handler for patchers.

*******************************************************************************/

void md_signal_handler_sigill(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t     *_uc;
	mcontext_t     *_mc;
	u1             *pv;
	u1             *sp;
	u1             *ra;
	u1             *xpc;
	int             type;
	intptr_t        val;
	void           *p;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;

	/* ATTENTION: Don't use CACAO's internal REG_* defines as they are
	   different to the ones in <ucontext.h>. */

	pv  = NULL;
	sp  = (u1 *) _mc->gregs[REG_RSP];
	xpc = (u1 *) _mc->gregs[REG_RIP];
	ra  = xpc;                          /* return address is equal to xpc     */

	/* This is a patcher. */

	type = EXCEPTION_HARDWARE_PATCHER;
	val  = 0;

	/* Handle the type. */

	p = signal_handle(type, val, pv, sp, ra, xpc, _p);

	/* set registers */

	if (p != NULL) {
		_mc->gregs[REG_RAX] = (intptr_t) p;
		_mc->gregs[REG_R10] = (intptr_t) xpc;                /* REG_ITMP2_XPC */
		_mc->gregs[REG_RIP] = (intptr_t) asm_handle_exception;
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

	/* ATTENTION: Don't use CACAO's internal REG_* defines as they are
	   different to the ones in <ucontext.h>. */

	/* get the PC and SP for this thread */
	pc = (u1 *) _mc->gregs[REG_RIP];
	sp = (u1 *) _mc->gregs[REG_RSP];

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

	/* ATTENTION: Don't use CACAO's internal REG_* defines as they are
	   different to the ones in <ucontext.h>. */

	pc = (u1 *) _mc->gregs[REG_RIP];

	t->pc = pc;
}
#endif


/* md_executionstate_read ******************************************************

   Read the given context into an executionstate.

*******************************************************************************/

void md_executionstate_read(executionstate_t *es, void *context)
{
	ucontext_t *_uc;
	mcontext_t *_mc;
	s4          i;
	s4          d;

	_uc = (ucontext_t *) context;
	_mc = &_uc->uc_mcontext;

	/* read special registers */
	es->pc = (u1 *) _mc->gregs[REG_RIP];
	es->sp = (u1 *) _mc->gregs[REG_RSP];
	es->pv = NULL;

	/* read integer registers */
	for (i = 0; i < INT_REG_CNT; i++) {
		/* XXX FIX ME! */

		switch (i) {
		case 0:  /* REG_RAX == 13 */
			d = REG_RAX;
			break;
		case 1:  /* REG_RCX == 14 */
			d = REG_RCX;
			break;
		case 2:  /* REG_RDX == 12 */
			d = REG_RDX;
			break;
		case 3:  /* REG_RBX == 11 */
			d = REG_RBX;
			break;
		case 4:  /* REG_RSP == 15 */
			d = REG_RSP;
			break;
		case 5:  /* REG_RBP == 10 */
			d = REG_RBP;
			break;
		case 6:  /* REG_RSI == 9  */
			d = REG_RSI;
			break;
		case 7:  /* REG_RDI == 8  */
			d = REG_RDI;
			break;
		case 8:  /* REG_R8  == 0  */
		case 9:  /* REG_R9  == 1  */
		case 10: /* REG_R10 == 2  */
		case 11: /* REG_R11 == 3  */
		case 12: /* REG_R12 == 4  */
		case 13: /* REG_R13 == 5  */
		case 14: /* REG_R14 == 6  */
		case 15: /* REG_R15 == 7  */
			d = i - 8;
			break;
		}

		es->intregs[i] = _mc->gregs[d];
	}

	/* read float registers */
	for (i = 0; i < FLT_REG_CNT; i++)
		es->fltregs[i] = 0xdeadbeefdeadbeefL;
}


/* md_executionstate_write *****************************************************

   Write the given executionstate back to the context.

*******************************************************************************/

void md_executionstate_write(executionstate_t *es, void *context)
{
	ucontext_t *_uc;
	mcontext_t *_mc;
	s4          i;
	s4          d;

	_uc = (ucontext_t *) context;
	_mc = &_uc->uc_mcontext;

	/* write integer registers */
	for (i = 0; i < INT_REG_CNT; i++) {
		/* XXX FIX ME! */

		switch (i) {
		case 0:  /* REG_RAX == 13 */
			d = REG_RAX;
			break;
		case 1:  /* REG_RCX == 14 */
			d = REG_RCX;
			break;
		case 2:  /* REG_RDX == 12 */
			d = REG_RDX;
			break;
		case 3:  /* REG_RBX == 11 */
			d = REG_RBX;
			break;
		case 4:  /* REG_RSP == 15 */
			d = REG_RSP;
			break;
		case 5:  /* REG_RBP == 10 */
			d = REG_RBP;
			break;
		case 6:  /* REG_RSI == 9  */
			d = REG_RSI;
			break;
		case 7:  /* REG_RDI == 8  */
			d = REG_RDI;
			break;
		case 8:  /* REG_R8  == 0  */
		case 9:  /* REG_R9  == 1  */
		case 10: /* REG_R10 == 2  */
		case 11: /* REG_R11 == 3  */
		case 12: /* REG_R12 == 4  */
		case 13: /* REG_R13 == 5  */
		case 14: /* REG_R14 == 6  */
		case 15: /* REG_R15 == 7  */
			d = i - 8;
			break;
		}

		_mc->gregs[d] = es->intregs[i];
	}

	/* write special registers */
	_mc->gregs[REG_RIP] = (ptrint) es->pc;
	_mc->gregs[REG_RSP] = (ptrint) es->sp;
}


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

	/* ATTENTION: Don't use CACAO's internal REG_* defines as they are
	   different to the ones in <ucontext.h>. */

	pc = (u1 *) _mc->gregs[REG_RIP];

	npc = critical_find_restart_point(pc);

	if (npc != NULL)
		_mc->gregs[REG_RIP] = (ptrint) npc;
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
 * vim:noexpandtab:sw=4:ts=4:
 */
