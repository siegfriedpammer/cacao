/* src/vm/signal.c - machine independent signal functions

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

   $Id: signal.c 2968 2005-07-10 15:18:21Z twisti $

*/


#include <signal.h>

#include "config.h"

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
# include "threads/native/threads.h"
#endif

#include "vm/options.h"
#include "vm/jit/stacktrace.h"


/* function prototypes ********************************************************/

void signal_handler_sigsegv(int sig, siginfo_t *siginfo, void *_p);
void signal_handler_sigfpe(int sig, siginfo_t *siginfo, void *_p);
void signal_handler_sigquit(int sig, siginfo_t *siginfo, void *_p);
void signal_handler_sigusr1(int sig, siginfo_t *siginfo, void *_p);


/* signal_init *****************************************************************

   Initializes the signal subsystem and installs the signal handler.

*******************************************************************************/

void signal_init(void)
{
	struct sigaction act;

	/* install signal handlers we need to convert to exceptions */

	sigemptyset(&act.sa_mask);


	/* catch NullPointerException/StackOverFlowException */

	if (!checknull) {
		act.sa_sigaction = signal_handler_sigsegv;
		act.sa_flags = SA_NODEFER | SA_SIGINFO;

#if defined(SIGSEGV)
		sigaction(SIGSEGV, &act, NULL);
#endif

#if defined(SIGBUS)
		sigaction(SIGBUS, &act, NULL);
#endif
	}


	/* catch ArithmeticException */

#if defined(__I386__) || defined(__X86_64__)
	act.sa_sigaction = signal_handler_sigfpe;
	act.sa_flags = SA_NODEFER | SA_SIGINFO;
	sigaction(SIGFPE, &act, NULL);
#endif


	/* catch SIGQUIT for thread dump */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	act.sa_sigaction = signal_handler_sigquit;
	act.sa_flags = SA_SIGINFO;
	sigaction(SIGQUIT, &act, NULL);

	act.sa_sigaction = signal_handler_sigusr1;
	act.sa_flags = SA_SIGINFO;
	sigaction(SIGUSR1, &act, NULL);
#endif
}


/* signal_handler_sigquit ******************************************************

   XXX

*******************************************************************************/

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void signal_handler_sigquit(int sig, siginfo_t *siginfo, void *_p)
{
	/* do thread dump */

	thread_dump();
}
#endif


/* signal_handler_sigusr1 ******************************************************

   XXX

*******************************************************************************/

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void signal_handler_sigusr1(int sig, siginfo_t *siginfo, void *_p)
{
	/* call stacktrace function */

	stacktrace_dump_trace();
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
