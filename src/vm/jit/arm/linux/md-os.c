/* src/vm/jit/arm/linux/md-os.c - machine dependent ARM Linux functions

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

#include <stdint.h>

#include "vm/types.h"

#include "vm/jit/disass.h"

#include "vm/jit/arm/md-abi.h"

#define ucontext broken_glibc_ucontext
#define ucontext_t broken_glibc_ucontext_t
#include <ucontext.h>
#undef ucontext
#undef ucontext_t

typedef struct ucontext {
   unsigned long     uc_flags;
   struct ucontext  *uc_link;
   stack_t           uc_stack;
   struct sigcontext uc_mcontext;
   sigset_t          uc_sigmask;
} ucontext_t;

#define scontext_t struct sigcontext

#include "threads/thread.h"

#include "vm/exceptions.h"
#include "vm/signallocal.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"

#include "vm/jit/asmpart.h"
#include "vm/jit/executionstate.h"
#include "vm/jit/stacktrace.h"
#include "vm/jit/trap.h"


/* md_signal_handler_sigsegv ***************************************************

   Signal handler for hardware exceptions.

*******************************************************************************/

void md_signal_handler_sigsegv(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t     *_uc;
	scontext_t     *_sc;
	u1             *pv;
	u1             *sp;
	u1             *ra;
	u1             *xpc;
	u4              mcode;
	intptr_t        addr;
	int             type;
	intptr_t        val;
	void           *p;

	_uc = (ucontext_t*) _p;
	_sc = &_uc->uc_mcontext;

	/* ATTENTION: glibc included messed up kernel headers we needed a
	   workaround for the ucontext structure. */

	pv  = (u1 *) _sc->arm_ip;
	sp  = (u1 *) _sc->arm_sp;
	ra  = (u1 *) _sc->arm_lr;                    /* this is correct for leafs */
	xpc = (u1 *) _sc->arm_pc;

	/* get exception-throwing instruction */

	if (xpc == NULL)
		vm_abort("md_signal_handler_sigsegv: the program counter is NULL");

	mcode = *((s4 *) xpc);

	/* This is a NullPointerException. */

	addr = *((s4 *) _sc + OFFSET(scontext_t, arm_r0)/4 + ((mcode >> 16) & 0x0f));
	type = addr;
	val  = 0;

	/* Handle the trap. */

	p = trap_handle(type, val, pv, sp, ra, xpc, _p);

	/* set registers */

	_sc->arm_r10 = (uintptr_t) p;
	_sc->arm_fp  = (uintptr_t) xpc;
	_sc->arm_pc  = (uintptr_t) asm_handle_exception;
}


/* md_signal_handler_sigill ****************************************************

   Illegal Instruction signal handler for hardware exception checks.

*******************************************************************************/

void md_signal_handler_sigill(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t     *_uc;
	scontext_t     *_sc;
	u1             *pv;
	u1             *sp;
	u1             *ra;
	u1             *xpc;
	u4              mcode;
	int             type;
	intptr_t        val;
	void           *p;

	_uc = (ucontext_t*) _p;
	_sc = &_uc->uc_mcontext;

	/* ATTENTION: glibc included messed up kernel headers we needed a
	   workaround for the ucontext structure. */

	pv  = (u1 *) _sc->arm_ip;
	sp  = (u1 *) _sc->arm_sp;
	ra  = (u1 *) _sc->arm_lr;                    /* this is correct for leafs */
	xpc = (u1 *) _sc->arm_pc;

	/* get exception-throwing instruction */

	mcode = *((u4 *) xpc);

	/* check for undefined instruction we use */

	if ((mcode & 0x0ff000f0) != 0x07f000f0) {
		log_println("md_signal_handler_sigill: unknown illegal instruction: inst=%x", mcode);
#if defined(ENABLE_DISASSEMBLER)
		DISASSINSTR(xpc);
#endif
		vm_abort("Aborting...");
	}

	type = (mcode >> 8) & 0x0fff;
	val  = *((s4 *) _sc + OFFSET(scontext_t, arm_r0)/4 + (mcode & 0x0f));

	/* Handle the trap. */

	p = trap_handle(type, val, pv, sp, ra, xpc, _p);

	/* set registers if we have an exception, continue execution
	   otherwise (this is needed for patchers to work) */

	if (p != NULL) {
		_sc->arm_r10 = (uintptr_t) p;
		_sc->arm_fp  = (uintptr_t) xpc;
		_sc->arm_pc  = (uintptr_t) asm_handle_exception;
	}
}


/* md_signal_handler_sigusr1 ***************************************************

   Signal handler for suspending threads.

*******************************************************************************/

#if defined(ENABLE_THREADS) && defined(ENABLE_GC_CACAO)
void md_signal_handler_sigusr1(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t *_uc;
	scontext_t *_sc;
	u1         *pc;
	u1         *sp;

	_uc = (ucontext_t *) _p;
	_sc = &_uc->uc_mcontext;

	/* get the PC and SP for this thread */
	pc = (u1 *) _sc->arm_pc;
	sp = (u1 *) _sc->arm_sp;

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
	threadobject *thread;
	ucontext_t   *_uc;
	scontext_t   *_sc;
	u1           *pc;

	thread = THREADOBJECT;

	_uc = (ucontext_t*) _p;
	_sc = &_uc->uc_mcontext;

	pc = (u1 *) _sc->arm_pc;

	thread->pc = pc;
}
#endif


/**
 * Read the given context into an executionstate.
 *
 * @param es      execution state
 * @param context machine context
 */
void md_executionstate_read(executionstate_t *es, void *context)
{
	vm_abort("md_executionstate_read: IMPLEMENT ME!");

#if 0
	ucontext_t *_uc;
	mcontext_t *_mc;
	int         i;

	_uc = (ucontext_t *) context;
	_mc = &_uc->uc_mcontext;

	/* read special registers */
	es->pc = (u1 *) _mc->sc_pc;
	es->sp = (u1 *) _mc->sc_regs[REG_SP];
	es->pv = (u1 *) _mc->sc_regs[REG_PV];
	es->ra = (u1 *) _mc->sc_regs[REG_RA];

	/* read integer registers */
	for (i = 0; i < INT_REG_CNT; i++)
		es->intregs[i] = _mc->sc_regs[i];

	/* read float registers */
	/* Do not use the assignment operator '=', as the type of
	 * the _mc->sc_fpregs[i] can cause invalid conversions. */

	assert(sizeof(_mc->sc_fpregs) == sizeof(es->fltregs));
	system_memcpy(&es->fltregs, &_mc->sc_fpregs, sizeof(_mc->sc_fpregs));
#endif
}


/**
 * Write the given executionstate back to the context.
 *
 * @param es      execution state
 * @param context machine context
 */
void md_executionstate_write(executionstate_t *es, void *context)
{
	vm_abort("md_executionstate_write: IMPLEMENT ME!");

#if 0
	ucontext_t *_uc;
	mcontext_t *_mc;
	int         i;

	_uc = (ucontext_t *) context;
	_mc = &_uc->uc_mcontext;

	/* write integer registers */
	for (i = 0; i < INT_REG_CNT; i++)
		_mc->sc_regs[i] = es->intregs[i];

	/* write float registers */
	/* Do not use the assignment operator '=', as the type of
	 * the _mc->sc_fpregs[i] can cause invalid conversions. */

	assert(sizeof(_mc->sc_fpregs) == sizeof(es->fltregs));
	system_memcpy(&_mc->sc_fpregs, &es->fltregs, sizeof(_mc->sc_fpregs));

	/* write special registers */
	_mc->sc_pc           = (ptrint) es->pc;
	_mc->sc_regs[REG_SP] = (ptrint) es->sp;
	_mc->sc_regs[REG_PV] = (ptrint) es->pv;
	_mc->sc_regs[REG_RA] = (ptrint) es->ra;
#endif
}


/* md_critical_section_restart *************************************************

   Search the critical sections tree for a matching section and set
   the PC to the restart point, if necessary.

*******************************************************************************/

#if defined(ENABLE_THREADS)
void md_critical_section_restart(ucontext_t *_uc)
{
	scontext_t *_sc;
	u1         *pc;
	u1         *npc;

	_sc = &_uc->uc_mcontext;

	pc = (u1 *) _sc->arm_pc;

	npc = critical_find_restart_point(pc);

	if (npc != NULL)
		_sc->arm_pc = (ptrint) npc;
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

