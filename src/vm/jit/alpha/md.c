/* src/vm/jit/alpha/md.c - machine dependent Alpha functions

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

   Changes: Joseph Wenninger
            Christian Thalinger

   $Id: md.c 2735 2005-06-17 13:38:35Z twisti $

*/


#include <assert.h>
#include <signal.h>
#include <ucontext.h>

#include "config.h"
#include "vm/jit/alpha/md-abi.h"
#include "vm/jit/alpha/types.h"

#include "vm/options.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"


/* catch_NullPointerException **************************************************

   NullPointerException signal handler for hardware null pointer check.

*******************************************************************************/

void catch_NullPointerException(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t       *_uc;
	mcontext_t       *_mc;
	struct sigaction act;
	sigset_t         nsig;
	int              instr;
	long             faultaddr;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;

	instr = *((s4 *) (_mc->sc_pc));
	faultaddr = _mc->sc_regs[(instr >> 16) & 0x1f];

	if (faultaddr == 0) {
		/* Reset signal handler - necessary for SysV, does no harm for BSD */
		act.sa_sigaction = catch_NullPointerException;
		act.sa_flags = SA_SIGINFO;
		sigaction(sig, &act, NULL);

		sigemptyset(&nsig);
		sigaddset(&nsig, sig);
		sigprocmask(SIG_UNBLOCK, &nsig, NULL);           /* unblock signal    */

		_mc->sc_regs[REG_ITMP1_XPTR] =
			(ptrint) string_java_lang_NullPointerException;

		_mc->sc_regs[REG_ITMP2_XPC] = _mc->sc_pc;
		_mc->sc_pc = (ptrint) asm_throw_and_handle_exception;

	} else {
		faultaddr += (long) ((instr << 16) >> 16);

		printf("faulting address: 0x%016lx\n", faultaddr);
		assert(0);
	}
}


#if defined(__OSF__)

void init_exceptions(void)
{

#else /* Linux */

/* Linux on Digital Alpha needs an initialisation of the ieee floating point
	control for IEEE compliant arithmetic (option -mieee of GCC). Under
	Digital Unix this is done automatically.
*/

#include <asm/fpu.h>

extern unsigned long ieee_get_fp_control();
extern void ieee_set_fp_control(unsigned long fp_control);

void init_exceptions(void)
{
	struct sigaction act;

	/* initialize floating point control */

	ieee_set_fp_control(ieee_get_fp_control()
						& ~IEEE_TRAP_ENABLE_INV
						& ~IEEE_TRAP_ENABLE_DZE
/*  						& ~IEEE_TRAP_ENABLE_UNF   we dont want underflow */
						& ~IEEE_TRAP_ENABLE_OVF);
#endif

	/* install signal handlers we need to convert to exceptions */

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


#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void thread_restartcriticalsection(ucontext_t *uc)
{
	void *critical;

	critical = thread_checkcritical((void *) uc->uc_mcontext.sc_pc);

	if (critical)
		uc->uc_mcontext.sc_pc = (ptrint) critical;
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
