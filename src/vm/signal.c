/* src/vm/signal.c - machine independent signal functions

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

   $Id: signal.c 5900 2006-11-04 17:30:44Z michi $

*/


#include "config.h"

#include <signal.h>
#include <stdlib.h>

#include "vm/types.h"

#if defined(ENABLE_THREADS)
# include "threads/native/threads.h"
#endif

#include "vm/signallocal.h"
#include "vm/options.h"
#include "vm/vm.h"
#include "vm/jit/stacktrace.h"


/* function prototypes ********************************************************/

void signal_handler_sighup(int sig, siginfo_t *siginfo, void *_p);
void signal_handler_sigint(int sig, siginfo_t *siginfo, void *_p);
void signal_handler_sigquit(int sig, siginfo_t *siginfo, void *_p);


/* signal_init *****************************************************************

   Initializes the signal subsystem and installs the signal handler.

*******************************************************************************/

void signal_init(void)
{
#if !defined(__CYGWIN__)
	struct sigaction act;

	/* Allocate something so the garbage collector's signal handlers
	   are installed. */

#if defined(ENABLE_GC_BOEHM)
	(void) GCNEW(u1);
#endif

	/* install signal handlers we need to convert to exceptions */

	sigemptyset(&act.sa_mask);

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (!opt_intrp) {
# endif
		/* catch NullPointerException/StackOverFlowException */

		if (!checknull) {
			act.sa_sigaction = md_signal_handler_sigsegv;
			act.sa_flags     = SA_NODEFER | SA_SIGINFO;

#if defined(SIGSEGV)
			sigaction(SIGSEGV, &act, NULL);
#endif

#if defined(SIGBUS)
			sigaction(SIGBUS, &act, NULL);
#endif
		}

		/* catch ArithmeticException */

#if defined(__I386__) || defined(__X86_64__)
		act.sa_sigaction = md_signal_handler_sigfpe;
		act.sa_flags     = SA_NODEFER | SA_SIGINFO;
		sigaction(SIGFPE, &act, NULL);
#endif
# if defined(ENABLE_INTRP)
	}
# endif
#endif /* !defined(ENABLE_INTRP) */

#if defined(ENABLE_THREADS)
	/* catch SIGHUP for threads_thread_interrupt */

	act.sa_sigaction = signal_handler_sighup;
	act.sa_flags     = 0;
	sigaction(SIGHUP, &act, NULL);
#endif

	/* catch SIGINT for exiting properly on <ctrl>-c */

	act.sa_sigaction = signal_handler_sigint;
	act.sa_flags     = SA_NODEFER | SA_SIGINFO;
	sigaction(SIGINT, &act, NULL);

#if defined(ENABLE_THREADS)
	/* catch SIGQUIT for thread dump */

# if !defined(__FREEBSD__)
	act.sa_sigaction = signal_handler_sigquit;
	act.sa_flags     = SA_SIGINFO;
	sigaction(SIGQUIT, &act, NULL);
# endif
#endif

#if defined(ENABLE_THREADS) && defined(ENABLE_PROFILING)
	/* install signal handler for profiling sampling */

	act.sa_sigaction = md_signal_handler_sigusr2;
	act.sa_flags     = SA_SIGINFO;
	sigaction(SIGUSR2, &act, NULL);
#endif

#endif /* !defined(__CYGWIN__) */
}


/* signal_handler_sighup *******************************************************

   This handler is required by threads_thread_interrupt and does
   nothing.

*******************************************************************************/

#if defined(ENABLE_THREADS)
void signal_handler_sighup(int sig, siginfo_t *siginfo, void *_p)
{
	/* do nothing */
}
#endif


/* signal_handler_sigquit ******************************************************

   Handle for SIGQUIT (<ctrl>-\) which print a stacktrace for every
   running thread.

*******************************************************************************/

#if defined(ENABLE_THREADS)
void signal_handler_sigquit(int sig, siginfo_t *siginfo, void *_p)
{
	/* do thread dump */

	threads_dump();
}
#endif


/* signal_handler_sigint *******************************************************

   Handler for SIGINT (<ctrl>-c) which shuts down CACAO properly with
   Runtime.exit(I)V.

*******************************************************************************/

void signal_handler_sigint(int sig, siginfo_t *siginfo, void *_p)
{
	/* if we are already in Runtime.exit(), just do it hardcore */

	if (vm_exiting) {
		fprintf(stderr, "Caught SIGINT while already shutting down. Shutdown aborted...\n");
		exit(0);
	}

	/* exit the vm properly */

	vm_exit(0);
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
