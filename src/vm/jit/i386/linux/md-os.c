/* src/vm/jit/i386/linux/md-os.c - machine dependent i386 Linux functions

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


#define _GNU_SOURCE                   /* include REG_ defines from ucontext.h */

#include "config.h"

#include <stdint.h>
#include <ucontext.h>

#include "vm/types.h"

#include "vm/jit/i386/codegen.h"
#include "vm/jit/i386/md.h"

#include "threads/threads-common.h"

#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/signallocal.h"
#include "vm/stringlocal.h"

#include "vm/jit/asmpart.h"
#include "vm/jit/stacktrace.h"


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
	java_object_t  *o;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;

	pv  = NULL;                 /* is resolved during stackframeinfo creation */
	sp  = (u1 *) _mc->gregs[REG_ESP];
	xpc = (u1 *) _mc->gregs[REG_EIP];
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

		val = _mc->gregs[REG_EAX - d];

		if (type == EXCEPTION_HARDWARE_COMPILER) {
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

		type = EXCEPTION_HARDWARE_NULLPOINTER;
		val  = 0;
	}

	/* Handle the type. */

	p = signal_handle(type, val, pv, sp, ra, xpc, _p);

	/* Set registers. */

	if (type == EXCEPTION_HARDWARE_COMPILER) {
		if (p == NULL) {
			o = builtin_retrieve_exception();

			_mc->gregs[REG_ESP] = (uintptr_t) sp;    /* Remove RA from stack. */

			_mc->gregs[REG_EAX] = (uintptr_t) o;
			_mc->gregs[REG_ECX] = (uintptr_t) xpc;           /* REG_ITMP2_XPC */
			_mc->gregs[REG_EIP] = (uintptr_t) asm_handle_exception;
		}
		else {
			_mc->gregs[REG_EIP] = (uintptr_t) p;
		}
	}
	else {
		_mc->gregs[REG_EAX] = (intptr_t) p;
		_mc->gregs[REG_ECX] = (intptr_t) xpc;                /* REG_ITMP2_XPC */
		_mc->gregs[REG_EIP] = (intptr_t) asm_handle_exception;
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
	s4              type;
	ptrint          val;
	void           *p;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;

	pv  = NULL;                 /* is resolved during stackframeinfo creation */
	sp  = (u1 *) _mc->gregs[REG_ESP];
	xpc = (u1 *) _mc->gregs[REG_EIP];
	ra  = xpc;                          /* return address is equal to xpc     */

	/* this is an ArithmeticException */

	type = EXCEPTION_HARDWARE_ARITHMETIC;
	val  = 0;

	/* Handle the type. */

	p = signal_handle(type, val, pv, sp, ra, xpc, _p);

	/* set registers */

	_mc->gregs[REG_EAX] = (intptr_t) p;
	_mc->gregs[REG_ECX] = (intptr_t) xpc;                    /* REG_ITMP2_XPC */
	_mc->gregs[REG_EIP] = (intptr_t) asm_handle_exception;
}


/* md_signal_handler_sigill ****************************************************

   Signal handler for hardware patcher traps (ud2).

*******************************************************************************/

void md_signal_handler_sigill(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t        *_uc;
	mcontext_t        *_mc;
	u1                *pv;
	u1                *sp;
	u1                *ra;
	u1                *xpc;
	s4                 type;
	ptrint             val;
	void              *p;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;

	pv  = NULL;                 /* is resolved during stackframeinfo creation */
	sp  = (u1 *) _mc->gregs[REG_ESP];
	xpc = (u1 *) _mc->gregs[REG_EIP];
	ra  = xpc;                            /* return address is equal to xpc   */

	/* this is an ArithmeticException */

	type = EXCEPTION_HARDWARE_PATCHER;
	val  = 0;

	/* generate appropriate exception */

	p = signal_handle(type, val, pv, sp, ra, xpc, _p);

	/* set registers (only if exception object ready) */

	if (p != NULL) {
		_mc->gregs[REG_EAX] = (ptrint) p;
		_mc->gregs[REG_ECX] = (ptrint) xpc;                      /* REG_ITMP2_XPC */
		_mc->gregs[REG_EIP] = (ptrint) asm_handle_exception;
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
	pc = (u1 *) _mc->gregs[REG_EIP];
	sp = (u1 *) _mc->gregs[REG_ESP];

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

	pc = (u1 *) _mc->gregs[REG_EIP];

	t->pc = pc;
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
	es->pc = (u1 *) _mc->gregs[REG_EIP];
	es->sp = (u1 *) _mc->gregs[REG_ESP];
	es->pv = NULL;                   /* pv must be looked up via AVL tree */

	/* read integer registers */
	for (i = 0; i < INT_REG_CNT; i++)
		es->intregs[i] = _mc->gregs[REG_EAX - i];

	/* read float registers */
	for (i = 0; i < FLT_REG_CNT; i++)
		es->fltregs[i] = 0xdeadbeefdeadbeefULL;
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
		_mc->gregs[REG_EAX - i] = es->intregs[i];

	/* write special registers */
	_mc->gregs[REG_EIP] = (ptrint) es->pc;
	_mc->gregs[REG_ESP] = (ptrint) es->sp;
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

	pc = (u1 *) _mc->gregs[REG_EIP];

	npc = critical_find_restart_point(pc);

	if (npc != NULL)
		_mc->gregs[REG_EIP] = (ptrint) npc;
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
