/* src/vm/signal.c - machine independent signal functions

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

   $Id: signal.c 7280 2007-02-03 19:34:10Z twisti $

*/


#include "config.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

#if defined(__DARWIN__)
/* If we compile with -ansi on darwin, <sys/types.h> is not
 included. So let's do it here. */
# include <sys/types.h>
#endif

#include "vm/types.h"

#include "mm/memory.h"

#include "native/jni.h"
#include "native/include/java_lang_Thread.h"

#if defined(WITH_CLASSPATH_GNU)
# include "native/include/java_lang_VMThread.h"
#endif

#if defined(ENABLE_THREADS)
# include "threads/native/threads.h"
#endif

#include "vm/builtin.h"
#include "vm/signallocal.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"
#include "vm/jit/stacktrace.h"

#include "vmcore/options.h"


/* global variables ***********************************************************/

#if defined(ENABLE_THREADS)
static threadobject *thread_signal;
#endif


/* function prototypes ********************************************************/

void signal_handler_sighup(int sig, siginfo_t *siginfo, void *_p);


/* signal_init *****************************************************************

   Initializes the signal subsystem and installs the signal handler.

*******************************************************************************/

void signal_init(void)
{
#if !defined(__CYGWIN__)
	int              pagesize;
	sigset_t         mask;
	struct sigaction act;

	/* mmap a memory page at address 0x0, so our hardware-exceptions
	   work. */

	pagesize = getpagesize();

	(void) memory_mmap_anon(NULL, pagesize, PROT_NONE, MAP_PRIVATE | MAP_FIXED);

#if 0
	/* Block the following signals (SIGINT for <ctrl>-c, SIGQUIT for
	   <ctrl>-\).  We enable them later in signal_thread, but only for
	   this thread. */

	if (sigemptyset(&mask) != 0)
		vm_abort("signal_init: sigemptyset failed: %s", strerror(errno));

	if (sigaddset(&mask, SIGINT) != 0)
		vm_abort("signal_init: sigaddset failed: %s", strerror(errno));

#if !defined(__FREEBSD__)
	if (sigaddset(&mask, SIGQUIT) != 0)
		vm_abort("signal_init: sigaddset failed: %s", strerror(errno));
#endif

	if (sigprocmask(SIG_BLOCK, &mask, NULL) != 0)
		vm_abort("signal_init: sigprocmask failed: %s", strerror(errno));
#endif

#if defined(ENABLE_GC_BOEHM)
	/* Allocate something so the garbage collector's signal handlers
	   are installed. */

	(void) GCNEW(u1);
#endif

	/* Install signal handlers for signals we want to catch in all
	   threads. */

	sigemptyset(&act.sa_mask);

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (!opt_intrp) {
# endif
		/* SIGSEGV handler */

		act.sa_sigaction = md_signal_handler_sigsegv;
		act.sa_flags     = SA_NODEFER | SA_SIGINFO;

#if defined(SIGSEGV)
		sigaction(SIGSEGV, &act, NULL);
#endif

#if defined(SIGBUS)
		sigaction(SIGBUS, &act, NULL);
#endif

#if SUPPORT_HARDWARE_DIVIDE_BY_ZERO
		/* SIGFPE handler */

		act.sa_sigaction = md_signal_handler_sigfpe;
		act.sa_flags     = SA_NODEFER | SA_SIGINFO;
		sigaction(SIGFPE, &act, NULL);
#endif
# if defined(ENABLE_INTRP)
	}
# endif
#endif /* !defined(ENABLE_INTRP) */

#if defined(ENABLE_THREADS)
	/* SIGHUP handler for threads_thread_interrupt */

	act.sa_sigaction = signal_handler_sighup;
	act.sa_flags     = 0;
	sigaction(SIGHUP, &act, NULL);
#endif

#if defined(ENABLE_THREADS) && defined(ENABLE_PROFILING)
	/* SIGUSR2 handler for profiling sampling */

	act.sa_sigaction = md_signal_handler_sigusr2;
	act.sa_flags     = SA_SIGINFO;
	sigaction(SIGUSR2, &act, NULL);
#endif

#endif /* !defined(__CYGWIN__) */
}


/* signal_thread ************************************************************

   This thread sets the signal mask to catch the user input signals
   (SIGINT, SIGQUIT).  We use such a thread, so we don't get the
   signals on every single thread running.  Especially, this makes
   problems on slow machines.

*******************************************************************************/

static void signal_thread(void)
{
	sigset_t mask;
	int      sig;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
#if !defined(__FREEBSD__)
	sigaddset(&mask, SIGQUIT);
#endif

	while (true) {
		/* just wait for a signal */

		sigwait(&mask, &sig);
		log_println("signal caught: %d", sig);

		switch (sig) {
		case SIGINT:
			/* exit the vm properly */

			vm_exit(0);
			break;

		case SIGQUIT:
			/* print a thread dump */

			threads_dump();

#if defined(ENABLE_STATISTICS)
			if (opt_stat)
				statistics_print_memory_usage();
#endif
			break;
		}
	}

	/* this should not happen */

	vm_abort("signal_thread: this thread should not exit!");
}


/* signal_start_thread *********************************************************

   Starts the signal handler thread.

*******************************************************************************/

bool signal_start_thread(void)
{
#if defined(ENABLE_THREADS)
#if defined(WITH_CLASSPATH_GNU)
	java_lang_VMThread *vmt;
#endif

	/* create the finalizer object */

	thread_signal = (threadobject *) builtin_new(class_java_lang_Thread);

	if (thread_signal == NULL)
		return false;

#if defined(WITH_CLASSPATH_GNU)
	vmt = (java_lang_VMThread *) builtin_new(class_java_lang_VMThread);

	vmt->thread = (java_lang_Thread *) thread_signal;

	thread_signal->o.vmThread = vmt;
#endif

	thread_signal->flags      = THREAD_FLAG_DAEMON;

	thread_signal->o.name     = javastring_new_from_ascii("Signal Handler");
#if defined(ENABLE_JAVASE)
	thread_signal->o.daemon   = true;
#endif
	thread_signal->o.priority = 5;

	/* actually start the finalizer thread */

	threads_start_thread(thread_signal, signal_thread);

	/* everything's ok */

	return true;
#else
#warning FIX ME!
#endif
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
