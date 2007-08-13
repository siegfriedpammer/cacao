/* src/vm/jit/powerpc64/linux/md-os.c - machine dependent PowerPC64 Linux functions

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

   $Id: md-os.c 8283 2007-08-09 15:10:05Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <stdint.h>
#include <ucontext.h>

#include "vm/types.h"

#include "vm/jit/powerpc64/codegen.h"
#include "vm/jit/powerpc64/linux/md-abi.h"

#if defined(ENABLE_THREADS)
# include "threads/native/threads.h"
#endif

#include "vm/exceptions.h"
#include "vm/signallocal.h"

#include "vm/jit/asmpart.h"

#if defined(ENABLE_PROFILING)
# include "vm/jit/optimizing/profile.h"
#endif

#include "vm/jit/stacktrace.h"


/* md_signal_handler_sigsegv ***************************************************
 
	Signal handler for hardware-exceptions.

*******************************************************************************/

void md_signal_handler_sigsegv(int sig, siginfo_t *siginfo, void *_p)
{
	stackframeinfo  sfi;
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
		/* we use the exception type as load displacement */
		type = disp;
	}
	else {
		/* normal NPE */
		addr = _mc->gp_regs[s1];
		type = (s4) addr;
	}

	/* create stackframeinfo */

	stacktrace_create_extern_stackframeinfo(&sfi, pv, sp, ra, xpc);

	/* Handle the type. */

	p = signal_handle(xpc, type, val);

	/* remove stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	_mc->gp_regs[REG_ITMP1]     = (intptr_t) p;
	_mc->gp_regs[REG_ITMP2_XPC] = (intptr_t) xpc;
	_mc->gp_regs[PT_NIP]        = (intptr_t) asm_handle_exception;
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

	_mc = &(_uc->uc_mcontext);

	pc = (u1 *) _mc->gp_regs[PT_NIP];

	npc = critical_find_restart_point(pc);

	if (npc != NULL)
		_mc->gp_regs[PT_NIP] = (ptrint) npc;
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
