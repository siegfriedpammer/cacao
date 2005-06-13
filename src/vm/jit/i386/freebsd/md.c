/* src/vm/jit/i386/freebsd/md.c - machine dependent i386 FreeBSD functions

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Christian Thalinger

   Changes:

   $Id: md.c 2675 2005-06-13 16:20:03Z twisti $

*/


#include <stdlib.h>
#include <ucontext.h>

#include <machine/signal.h>

#include "config.h"
#include "vm/jit/x86_64/md-abi.h"

#include "vm/options.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"


/* catch_NullPointerException **************************************************

   NullPointerException signal handler for hardware null pointer check.

*******************************************************************************/

void catch_NullPointerException(int sig, siginfo_t *siginfo, void *_p)
{
	sigset_t nsig;

	ucontext_t *_uc = (ucontext_t *) _p;
	mcontext_t *sigctx = (mcontext_t *) &_uc->uc_mcontext;
	struct sigaction act;

	/* Reset signal handler - necessary for SysV, does no harm for BSD */

	act.sa_sigaction = catch_NullPointerException;
	act.sa_flags = SA_SIGINFO;
	sigaction(sig, &act, NULL);                          /* reinstall handler */

	sigemptyset(&nsig);
	sigaddset(&nsig, sig);
	sigprocmask(SIG_UNBLOCK, &nsig, NULL);           /* unblock signal    */

	sigctx->mc_ecx = sigctx->mc_eip;     /* REG_ITMP2_XPC*/
	sigctx->mc_eax = (u4) string_java_lang_NullPointerException;
	sigctx->mc_eip = (u4) asm_throw_and_handle_exception;
}


/* catch_ArithmeticException ***************************************************

   ArithmeticException signal handler for hardware divide by zero check.

*******************************************************************************/

void catch_ArithmeticException(int sig, siginfo_t *siginfo, void *_p)
{
	sigset_t nsig;

	ucontext_t *_uc = (ucontext_t *) _p;
	mcontext_t *sigctx = (mcontext_t *) &_uc->uc_mcontext;
	struct sigaction act;

	/* Reset signal handler - necessary for SysV, does no harm for BSD        */

	act.sa_sigaction = catch_ArithmeticException;
	act.sa_flags = SA_SIGINFO;
	sigaction(sig, &act, NULL);                          /* reinstall handler */

	sigemptyset(&nsig);
	sigaddset(&nsig, sig);
	sigprocmask(SIG_UNBLOCK, &nsig, NULL);               /* unblock signal    */

	sigctx->mc_ecx = sigctx->mc_eip;                 /* REG_ITMP2_XPC     */
	sigctx->mc_eip = (u4) asm_throw_and_handle_hardware_arithmetic_exception;
}


void init_exceptions(void)
{
	struct sigaction act;

	/* install signal handlers we need to convert to exceptions */
	sigemptyset(&act.sa_mask);

	if (!checknull) {
		act.sa_sigaction = catch_NullPointerException;
		act.sa_flags = SA_SIGINFO;

#if defined(SIGSEGV)
		sigaction(SIGSEGV, &act, NULL);
#endif

#if defined(SIGBUS)
		sigaction(SIGBUS, &act, NULL);
#endif
	}

#if defined(SIGFPE)
	act.sa_sigaction = catch_ArithmeticException;
	act.sa_flags = SA_SIGINFO;
	sigaction(SIGFPE, &act, NULL);
#endif
}


#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void thread_restartcriticalsection(ucontext_t *uc)
{
	void *critical;

	critical = thread_checkcritical((void *) uc->uc_mcontext.mc_eip);

	if (critical)
		uc->uc_mcontext.mc_eip = (ptrint) critical;
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
