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

   $Id: md-os.c 4921 2006-05-15 14:24:36Z twisti $

*/


#include "config.h"

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
	ptrint              addr;
	u1                 *pv;
	u1                 *sp;
	u1                 *ra;
	u1                 *xpc;

	_uc = (ucontext_t *) _p;
	_mc = _uc->uc_mcontext;
	_ss = &_mc->ss;

	/* check for NullPointerException */

	gregs = &_ss->r0;

	instr = *((u4 *) _ss->srr0);
	reg = (instr >> 16) & 31;
	addr = gregs[reg];

	if (addr == 0) {
		pv  = (u1 *) _ss->r13;
		sp  = (u1 *) _ss->r1;
		ra  = (u1 *) _ss->lr;              /* this is correct for leafs */
		xpc = (u1 *) _ss->srr0;

		_ss->r11 =
			(ptrint) stacktrace_hardware_nullpointerexception(pv, sp, ra, xpc);

		_ss->r12 = (ptrint) xpc;
		_ss->srr0 = (ptrint) asm_handle_exception;

	} else {
		throw_cacao_exception_exit(string_java_lang_InternalError,
					   "Segmentation fault: 0x%08lx at 0x%08lx",
					   addr, _ss->srr0);
	}
}


#if defined(ENABLE_THREADS)
void thread_restartcriticalsection(ucontext_t *uc)
{
	/* XXX set pc to restart address */
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
