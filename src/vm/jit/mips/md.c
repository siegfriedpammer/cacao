/* src/vm/jit/mips/md.c - machine dependent MIPS functions

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

   Authors: Andreas Krall
            Reinhard Grafl

   Changes: Christian Thalinger

   $Id: md.c 2711 2005-06-15 14:09:39Z twisti $

*/


#include <signal.h>
#include <sys/fpu.h>
#include <sys/mman.h>
#include <unistd.h>

#include "config.h"
#include "vm/jit/mips/md-abi.h"
#include "vm/jit/mips/types.h"

#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/options.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"


/* catch_NullPointerException **************************************************

   NullPointerException signal handler for hardware null pointer check.

*******************************************************************************/

void catch_NullPointerException(int sig, siginfo_t *siginfo, void *_p)
{
	sigset_t nsig;
	int      instr;
	long     faultaddr;
	java_objectheader *xptr;

	struct ucontext *_uc = (struct ucontext *) _p;
	mcontext_t *sigctx = &_uc->uc_mcontext;
	struct sigaction act;

	instr = *((s4 *) (sigctx->gregs[CTX_EPC]));
	faultaddr = sigctx->gregs[(instr >> 21) & 0x1f];

	if (faultaddr == 0) {
		/* Reset signal handler - necessary for SysV, does no harm for BSD */

		act.sa_sigaction = catch_NullPointerException;
		act.sa_flags = SA_SIGINFO;
		sigaction(sig, &act, NULL);

		sigemptyset(&nsig);
		sigaddset(&nsig, sig);
		sigprocmask(SIG_UNBLOCK, &nsig, NULL);           /* unblock signal    */
		
		xptr = new_nullpointerexception();

		sigctx->gregs[REG_ITMP1_XPTR] = (ptrint) xptr;
		sigctx->gregs[REG_ITMP2_XPC] = sigctx->gregs[CTX_EPC];
		sigctx->gregs[CTX_EPC] = (ptrint) asm_handle_exception;

	} else {
        faultaddr += (long) ((instr << 16) >> 16);

		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "faulting address: 0x%lx at 0x%lx\n",
								   (long) faultaddr,
								   (long) sigctx->gregs[CTX_EPC]);
	}
}


void init_exceptions(void)
{
	struct sigaction act;

	/* The Boehm GC initialization blocks the SIGSEGV signal. So we do a
	   dummy allocation here to ensure that the GC is initialized.
	*/
	heap_allocate(1, 0, NULL);

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

	/* Turn off flush-to-zero */
	{
		union fpc_csr n;
		n.fc_word = get_fpc_csr();
		n.fc_struct.flush = 0;
		set_fpc_csr(n.fc_word);
	}
}


#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void thread_restartcriticalsection(ucontext_t *uc)
{
	void *critical;

	critical = thread_checkcritical((void*) uc->uc_mcontext.gregs[CTX_EPC]);

	if (critical)
		uc->uc_mcontext.gregs[CTX_EPC] = (ptrint) critical;
}
#endif


void docacheflush(u1 *p, long bytelen)
{
	u1 *e = p + bytelen;
	long psize = sysconf(_SC_PAGESIZE);
	p -= (long) p & (psize - 1);
	e += psize - ((((long) e - 1) & (psize - 1)) + 1);
	bytelen = e-p;
	mprotect(p, bytelen, PROT_READ | PROT_WRITE | PROT_EXEC);
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
