/* src/vm/jit/powerpc/linux/md.c - machine dependent PowerPC Linux functions

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

   $Id: md.c 2688 2005-06-14 17:39:59Z twisti $

*/


#include <stdlib.h>
#include <signal.h>
#include <ucontext.h>

#include "vm/jit/powerpc/types.h"
#include "vm/jit/powerpc/linux/md-abi.h"

#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/options.h"
#include "vm/jit/asmpart.h"


/* catch_NullPointerException **************************************************

   NullPointerException signal handler for hardware null pointer check.

*******************************************************************************/

void catch_NullPointerException(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t       *uc;
	mcontext_t       *mc;
	struct sigaction  act;
	sigset_t          nsig;

	java_objectheader *xptr;

 	uc = (ucontext_t *) _p;
 	mc = uc->uc_mcontext.uc_regs;

	act.sa_sigaction = catch_NullPointerException;
	act.sa_flags = SA_SIGINFO;
	sigaction(sig, &act, NULL);

	xptr = new_nullpointerexception();

	sigemptyset(&nsig);
	sigaddset(&nsig, sig);
	sigprocmask(SIG_UNBLOCK, &nsig, NULL);               /* unblock signal    */

	mc->gregs[REG_ITMP1_XPTR] = (ptrint) xptr;
	mc->gregs[REG_ITMP2_XPC] = mc->gregs[PT_NIP];
	mc->gregs[PT_NIP] = (ptrint) asm_handle_exception;
}


/* init_exceptions *************************************************************

   Installs the OS dependent signal handlers.

*******************************************************************************/

void init_exceptions(void)
{
	struct sigaction act;

	GC_dirty_init(1);

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
