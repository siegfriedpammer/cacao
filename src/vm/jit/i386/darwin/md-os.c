/* src/vm/jit/i386/darwin/md-os.c - machine dependent i386 Darwin functions

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

#include "vm/jit/i386/codegen.h"
#include "vm/jit/i386/md.h"

#include "threads/thread.h"

#include "vm/builtin.h"
#include "vm/global.h"
#include "vm/signallocal.h"
#include "vm/stringlocal.h"

#include "vm/jit/asmpart.h"
#include "vm/jit/executionstate.h"
#include "vm/jit/stacktrace.h"
#include "vm/jit/trap.h"

#include "vm/jit/i386/codegen.h"


/* md_signal_handler_sigsegv ***************************************************

   Signal handler for hardware exceptions.

*******************************************************************************/

void md_signal_handler_sigsegv(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t          *_uc;
	mcontext_t           _mc;
	u1                  *pv;
	i386_thread_state_t *_ss;
	u1                  *sp;
	u1                  *ra;
	u1                  *xpc;
	u1                   opc;
	u1                   mod;
	u1                   rm;
	int                  d;
	int32_t              disp;
	intptr_t             val;
	int                  type;
	void                *p;
	java_object_t       *o;

	_uc = (ucontext_t *) _p;
	_mc = _uc->uc_mcontext;
	_ss = &_mc->__ss;

	pv  = NULL;                 /* is resolved during stackframeinfo creation */
	sp  = (u1 *) _ss->__esp;
	xpc = (u1 *) _ss->__eip;
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

		val = (d == 0) ? _ss->__eax :
			((d == 1) ? _ss->__ecx :
			((d == 2) ? _ss->__edx :
			((d == 3) ? _ss->__ebx :
			((d == 4) ? _ss->__esp :
			((d == 5) ? _ss->__ebp :
			((d == 6) ? _ss->__esi : _ss->__edi))))));

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
	}

	/* Handle the trap. */

	p = trap_handle(type, val, pv, sp, ra, xpc, _p);

	/* Set registers. */

	if (type == TRAP_COMPILER) {
		if (p == NULL) { 
			o = builtin_retrieve_exception();

			_ss->__esp = (uintptr_t) sp;  /* Remove RA from stack. */

			_ss->__eax = (uintptr_t) o;
			_ss->__ecx = (uintptr_t) xpc;           /* REG_ITMP2_XPC */
			_ss->__eip = (uintptr_t) asm_handle_exception;
		}
		else {
			_ss->__eip = (uintptr_t) p;
		}
	}
	else {
		_ss->__eax = (uintptr_t) p;
		_ss->__ecx = (uintptr_t) xpc;            /* REG_ITMP2_XPC */
		_ss->__eip = (uintptr_t) asm_handle_exception;
	}
}


/* md_signal_handler_sigfpe ****************************************************

   ArithmeticException signal handler for hardware divide by zero
   check.

*******************************************************************************/

void md_signal_handler_sigfpe(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t          *_uc;
	mcontext_t           _mc;
	u1                  *pv;
	i386_thread_state_t *_ss;
	u1                  *sp;
	u1                  *ra;
	u1                  *xpc;
	int                  type;
	intptr_t             val;
	void                *p;


	_uc = (ucontext_t *) _p;
	_mc = _uc->uc_mcontext;
	_ss = &_mc->__ss;

	pv  = NULL;                 /* is resolved during stackframeinfo creation */
	sp  = (u1 *) _ss->__esp;
	xpc = (u1 *) _ss->__eip;
	ra  = xpc;                          /* return address is equal to xpc     */

	/* This is an ArithmeticException */

	type = TRAP_ArithmeticException;
	val  = 0;

	/* Handle the trap. */

	p = trap_handle(type, val, pv, sp, ra, xpc, _p);

	/* Set registers. */

	_ss->__eax = (uintptr_t) p;
	_ss->__ecx = (uintptr_t) xpc;            /* REG_ITMP2_XPC */
	_ss->__eip = (uintptr_t) asm_handle_exception;
}


/* md_signal_handler_sigusr2 ***************************************************

   Signal handler for profiling sampling.

*******************************************************************************/

void md_signal_handler_sigusr2(int sig, siginfo_t *siginfo, void *_p)
{
	threadobject        *t;
	ucontext_t          *_uc;
	mcontext_t           _mc;
	i386_thread_state_t *_ss;
	u1                  *pc;

	t = THREADOBJECT;

	_uc = (ucontext_t *) _p;
	_mc = _uc->uc_mcontext;
	_ss = &_mc->__ss;

	pc = (u1 *) _ss->__eip;

	t->pc = pc;
}


/* md_signal_handler_sigill ****************************************************

   Signal handler for hardware patcher traps (ud2).

*******************************************************************************/

void md_signal_handler_sigill(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t          *_uc;
	mcontext_t           _mc;
	u1                  *pv;
	i386_thread_state_t *_ss;
	u1                  *sp;
	u1                  *ra;
	u1                  *xpc;
	int                  type;
	intptr_t             val;
	void                *p;


	_uc = (ucontext_t *) _p;
	_mc = _uc->uc_mcontext;
	_ss = &_mc->__ss;

	pv  = NULL;                 /* is resolved during stackframeinfo creation */
	sp  = (u1 *) _ss->__esp;
	xpc = (u1 *) _ss->__eip;
	ra  = xpc;                          /* return address is equal to xpc     */

	type = TRAP_PATCHER;
	val  = 0;

	/* Handle the trap. */

	p = trap_handle(type, val, pv, sp, ra, xpc, _p);

	/* Set registers. */

	if (p != NULL) {
		_ss->__eax = (uintptr_t) p;
		_ss->__ecx = (uintptr_t) xpc;            /* REG_ITMP2_XPC */
		_ss->__eip = (uintptr_t) asm_handle_exception;
	}
}

/* md_executionstate_read ******************************************************

   Read the given context into an executionstate.

*******************************************************************************/

void md_executionstate_read(executionstate_t *es, void *context)
{
	ucontext_t          *_uc;
	mcontext_t           _mc; 
	i386_thread_state_t *_ss;
	int                  i;

	_uc = (ucontext_t *) context;
	_mc = _uc->uc_mcontext;
	_ss = &_mc->__ss;

	/* read special registers */
	es->pc = (u1 *) _ss->__eip;
	es->sp = (u1 *) _ss->__esp;
	es->pv = NULL;                   /* pv must be looked up via AVL tree */

	/* read integer registers */
	for (i = 0; i < INT_REG_CNT; i++)
		es->intregs[i] = (i == 0) ? _ss->__eax :
			((i == 1) ? _ss->__ecx :
			((i == 2) ? _ss->__edx :
			((i == 3) ? _ss->__ebx :
			((i == 4) ? _ss->__esp :
			((i == 5) ? _ss->__ebp :
			((i == 6) ? _ss->__esi : _ss->__edi))))));

	/* read float registers */
	for (i = 0; i < FLT_REG_CNT; i++)
		es->fltregs[i] = 0xdeadbeefdeadbeefULL;
}


/* md_executionstate_write *****************************************************

   Write the given executionstate back to the context.

*******************************************************************************/

void md_executionstate_write(executionstate_t *es, void *context)
{
	ucontext_t*          _uc;
	mcontext_t           _mc;
	i386_thread_state_t* _ss;
	int                  i;

	_uc = (ucontext_t *) context;
	_mc = _uc->uc_mcontext;
	_ss = &_mc->__ss;

	/* write integer registers */
	for (i = 0; i < INT_REG_CNT; i++)
		*((i == 0) ? &_ss->__eax :
		 ((i == 1) ? &_ss->__ecx :
		 ((i == 2) ? &_ss->__edx :
		 ((i == 3) ? &_ss->__ebx :
		 ((i == 4) ? &_ss->__esp :
		 ((i == 5) ? &_ss->__ebp :
		 ((i == 6) ? &_ss->__esi : &_ss->__edi))))))) = es->intregs[i];

	/* write special registers */
	_ss->__eip = (ptrint) es->pc;
	_ss->__esp = (ptrint) es->sp;
}


/* md_critical_section_restart *************************************************

   Search the critical sections tree for a matching section and set
   the PC to the restart point, if necessary.

*******************************************************************************/

#if defined(ENABLE_THREADS)
void thread_restartcriticalsection(ucontext_t *_uc)
{
	mcontext_t           _mc;
	i386_thread_state_t *_ss;
	u1                  *pc;
	void                *rpc;

	_mc = _uc->uc_mcontext;
	_ss = &_mc->__ss;

	pc = (u1 *) _ss->__eip;

	rpc = critical_find_restart_point(pc);

	if (rpc != NULL)
		_ss->__eip = (ptrint) rpc;
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
