/* src/vm/jit/powerpc/linux/md-os.c - machine dependent PowerPC Linux functions

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

   $Id: md-os.c 5274 2006-08-24 09:29:44Z tbfg $

*/


#include "config.h"

#include <assert.h>
#include <ucontext.h>

#include "vm/types.h"

#include "vm/jit/powerpc/linux/md-abi.h"

#if defined(ENABLE_THREADS)
# include "threads/native/threads.h"
#endif

#include "vm/exceptions.h"
#include "vm/signallocal.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"

#if defined(ENABLE_PROFILING)
# include "vm/jit/profile/profile.h"
#endif

#include "vm/jit/stacktrace.h"


/* md_signal_handler_sigsegv ***************************************************

   NullPointerException signal handler for hardware null pointer
   check.

*******************************************************************************/

void md_signal_handler_sigsegv(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t  *_uc;
	mcontext_t  *_mc;
	u4           instr;
	s4           reg;
	ptrint       addr;
	u1          *pv;
	u1          *sp;
	u1          *ra;
	u1          *xpc;

 	_uc = (ucontext_t *) _p;
 	_mc = _uc->uc_mcontext.uc_regs;

	instr = *((u4 *) _mc->gregs[PT_NIP]);
	reg = (instr >> 16) & 0x1f;
	addr = _mc->gregs[reg];

	if (addr == 0) {
		pv  = (u1 *) _mc->gregs[REG_PV];
		sp  = (u1 *) _mc->gregs[REG_SP];
		ra  = (u1 *) _mc->gregs[PT_LNK];         /* this is correct for leafs */
		xpc = (u1 *) _mc->gregs[PT_NIP];

		_mc->gregs[REG_ITMP1_XPTR] =
			(ptrint) stacktrace_hardware_nullpointerexception(pv, sp, ra, xpc);

		_mc->gregs[REG_ITMP2_XPC] = (ptrint) xpc;
		_mc->gregs[PT_NIP] = (ptrint) asm_handle_exception;

	} else {
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Segmentation fault: 0x%08lx at 0x%08lx",
								   addr, _mc->gregs[PT_NIP]);
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
 	_mc = _uc->uc_mcontext.uc_regs;

	pc = (u1 *) _mc->gregs[PT_NIP];

	tobj->pc = pc;
}


void thread_restartcriticalsection(ucontext_t *_uc)
{
	mcontext_t *_mc;
	u1         *pc;
	void       *critical;

	_mc = _uc->uc_mcontext.uc_regs;

	pc = (u1 *) _mc->gregs[PT_NIP];

	critical = critical_find_restart_point(pc);

	if (critical)
		_mc->gregs[PT_NIP] = (ptrint) critical;
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
