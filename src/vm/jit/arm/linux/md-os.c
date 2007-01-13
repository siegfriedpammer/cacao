/* src/vm/jit/arm/linux/md.c - machine dependent arm linux functions

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

   Authors: Michael Starzinger
            Christian Thalinger

   $Id: md.c 166 2006-01-22 23:38:44Z twisti $

*/


#include "config.h"

#include <assert.h>

#include "vm/types.h"

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

#include "mm/memory.h"
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
	ucontext_t *_uc;
	/*mcontext_t *_mc;*/
	scontext_t *_sc;
	u4          instr;
	ptrint      addr;
	ptrint      base;
	u1          *pv;
	u1          *sp;
	u1          *ra;
	u1          *xpc;

	_uc = (ucontext_t*) _p;
	_sc = &_uc->uc_mcontext;

	/* ATTENTION: glibc included messed up kernel headers */
	/* we needed a workaround for the ucontext structure */

	addr = (ptrint) siginfo->si_addr;
	/*xpc = (u1*) _mc->gregs[REG_PC];*/
	xpc = (u1*) _sc->arm_pc;

	instr = *((s4*) xpc);
	base = *((s4*) _sc + OFFSET(scontext_t, arm_r0)/4 + ((instr >> 16) & 0x0f));

	if (base == 0) {
		pv  = (u1*) _sc->arm_ip;
		sp  = (u1*) _sc->arm_sp;
		ra  = (u1*) _sc->arm_lr; /* this is correct for leafs */

		_sc->arm_r10 = (ptrint) stacktrace_hardware_nullpointerexception(pv, sp, ra, xpc);
		_sc->arm_fp = (ptrint) xpc;
		_sc->arm_pc = (ptrint) asm_handle_exception;
	}
	else {
		codegen_get_pv_from_pc(xpc);

		/* this should not happen */

		assert(0);
	}
}


/* thread_restartcriticalsection ***********************************************

   TODO: document me

*******************************************************************************/

#if defined(ENABLE_THREADS)
void thread_restartcriticalsection(ucontext_t *_uc)
{
	scontext_t *_sc;
	void       *critical;

	_sc = &_uc->uc_mcontext;

	critical = critical_find_restart_point((void *) _sc->arm_pc);

	if (critical)
		_sc->arm_pc = (ptrint) critical;
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
