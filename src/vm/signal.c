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

*/


#include "config.h"

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(__DARWIN__)
/* If we compile with -ansi on darwin, <sys/types.h> is not
 included. So let's do it here. */
# include <sys/types.h>
#endif

#include "arch.h"

#include "mm/memory.h"

#if defined(ENABLE_THREADS)
# include "threads/threads-common.h"
#else
# include "threads/none/threads.h"
#endif

#include "toolbox/logging.h"

#include "vm/exceptions.h"
#include "vm/signallocal.h"
#include "vm/vm.h"

#include "vm/jit/codegen-common.h"
#include "vm/jit/disass.h"
#include "vm/jit/patcher-common.h"

#include "vmcore/options.h"

#if defined(ENABLE_STATISTICS)
# include "vmcore/statistics.h"
#endif


/* function prototypes ********************************************************/

void signal_handler_sighup(int sig, siginfo_t *siginfo, void *_p);


/* signal_init *****************************************************************

   Initializes the signal subsystem and installs the signal handler.

*******************************************************************************/

bool signal_init(void)
{
#if !defined(__CYGWIN__)
	sigset_t mask;

#if defined(__LINUX__) && defined(ENABLE_THREADS)
	/* XXX Remove for exact-GC. */
	if (threads_pthreads_implementation_nptl) {
#endif

	/* Block the following signals (SIGINT for <ctrl>-c, SIGQUIT for
	   <ctrl>-\).  We enable them later in signal_thread, but only for
	   this thread. */

	if (sigemptyset(&mask) != 0)
		vm_abort("signal_init: sigemptyset failed: %s", strerror(errno));

#if !defined(WITH_CLASSPATH_SUN)
	/* Let OpenJDK handle SIGINT itself. */

	if (sigaddset(&mask, SIGINT) != 0)
		vm_abort("signal_init: sigaddset failed: %s", strerror(errno));
#endif

#if !defined(__FREEBSD__)
	if (sigaddset(&mask, SIGQUIT) != 0)
		vm_abort("signal_init: sigaddset failed: %s", strerror(errno));
#endif

	if (sigprocmask(SIG_BLOCK, &mask, NULL) != 0)
		vm_abort("signal_init: sigprocmask failed: %s", strerror(errno));

#if defined(__LINUX__) && defined(ENABLE_THREADS)
	/* XXX Remove for exact-GC. */
	}
#endif

#if defined(ENABLE_GC_BOEHM)
	/* Allocate something so the garbage collector's signal handlers
	   are installed. */

	(void) GCNEW(int);
#endif

	/* Install signal handlers for signals we want to catch in all
	   threads. */

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (!opt_intrp) {
# endif
		/* SIGSEGV handler */

		signal_register_signal(SIGSEGV, (functionptr) md_signal_handler_sigsegv,
							   SA_NODEFER | SA_SIGINFO);

#  if defined(SIGBUS)
		signal_register_signal(SIGBUS, (functionptr) md_signal_handler_sigsegv,
							   SA_NODEFER | SA_SIGINFO);
#  endif

#  if SUPPORT_HARDWARE_DIVIDE_BY_ZERO
		/* SIGFPE handler */

		signal_register_signal(SIGFPE, (functionptr) md_signal_handler_sigfpe,
							   SA_NODEFER | SA_SIGINFO);
#  endif

#  if defined(__ARM__) || defined(__S390__)
		/* XXX use better defines for that (in arch.h) */
		/* SIGILL handler */

		signal_register_signal(SIGILL, (functionptr) md_signal_handler_sigill,
							   SA_NODEFER | SA_SIGINFO);
#  endif

#  if defined(__POWERPC__)
		/* XXX use better defines for that (in arch.h) */
		/* SIGTRAP handler */

		signal_register_signal(SIGTRAP, (functionptr) md_signal_handler_sigtrap,
							   SA_NODEFER | SA_SIGINFO);
#  endif
# if defined(ENABLE_INTRP)
	}
# endif
#endif /* !defined(ENABLE_INTRP) */

#if defined(ENABLE_THREADS)
	/* SIGHUP handler for threads_thread_interrupt */

	signal_register_signal(SIGHUP, (functionptr) signal_handler_sighup, 0);
#endif

#if defined(ENABLE_THREADS) && defined(ENABLE_GC_CACAO)
	/* SIGUSR1 handler for the exact GC to suspend threads */

	signal_register_signal(SIGUSR1, (functionptr) md_signal_handler_sigusr1,
						   SA_SIGINFO);
#endif

#if defined(ENABLE_THREADS) && defined(ENABLE_PROFILING)
	/* SIGUSR2 handler for profiling sampling */

	signal_register_signal(SIGUSR2, (functionptr) md_signal_handler_sigusr2,
						   SA_SIGINFO);
#endif

#endif /* !defined(__CYGWIN__) */

	return true;
}


/* signal_register_signal ******************************************************

   Register the specified handler with the specified signal.

*******************************************************************************/

void signal_register_signal(int signum, functionptr handler, int flags)
{
	struct sigaction act;

	void (*function)(int, siginfo_t *, void *);

	function = (void (*)(int, siginfo_t *, void *)) handler;

	if (sigemptyset(&act.sa_mask) != 0)
		vm_abort("signal_register_signal: sigemptyset failed: %s",
				 strerror(errno));

	act.sa_sigaction = function;
	act.sa_flags     = flags;

	if (sigaction(signum, &act, NULL) != 0)
		vm_abort("signal_register_signal: sigaction failed: %s",
				 strerror(errno));
}


/* signal_handle ***************************************************************

   Handles the signal caught by a signal handler and calls the correct
   function.

*******************************************************************************/

void *signal_handle(void *xpc, int type, intptr_t val)
{
	void          *p;
	int32_t        index;
	java_object_t *o;

	switch (type) {
	case EXCEPTION_HARDWARE_NULLPOINTER:
		p = exceptions_new_nullpointerexception();
		break;

	case EXCEPTION_HARDWARE_ARITHMETIC:
		p = exceptions_new_arithmeticexception();
		break;

	case EXCEPTION_HARDWARE_ARRAYINDEXOUTOFBOUNDS:
		index = (s4) val;
		p = exceptions_new_arrayindexoutofboundsexception(index);
		break;

	case EXCEPTION_HARDWARE_ARRAYSTORE:
		p = exceptions_new_arraystoreexception();
		break;

	case EXCEPTION_HARDWARE_CLASSCAST:
		o = (java_object_t *) val;
		p = exceptions_new_classcastexception(o);
		break;

	case EXCEPTION_HARDWARE_EXCEPTION:
		p = exceptions_fillinstacktrace();
		break;

	case EXCEPTION_HARDWARE_PATCHER:
#if defined(ENABLE_REPLACEMENT)
		if (replace_me_wrapper(xpc)) {
			p = NULL;
			break;
		}
#endif
		p = patcher_handler(xpc);
		break;

	default:
		/* Let's try to get a backtrace. */

		codegen_get_pv_from_pc(xpc);

		/* If that does not work, print more debug info. */

		log_println("exceptions_new_hardware_exception: unknown exception type %d", type);

#if SIZEOF_VOID_P == 8
		log_println("PC=0x%016lx", xpc);
#else
		log_println("PC=0x%08x", xpc);
#endif

#if defined(ENABLE_DISASSEMBLER)
		log_println("machine instruction at PC:");
		disassinstr(xpc);
#endif

		vm_abort("Exiting...");

		/* keep compiler happy */

		p = NULL;
	}

	return p;
}


/* signal_thread ************************************************************

   This thread sets the signal mask to catch the user input signals
   (SIGINT, SIGQUIT).  We use such a thread, so we don't get the
   signals on every single thread running.

*******************************************************************************/

static void signal_thread(void)
{
	threadobject *t;
	sigset_t      mask;
	int           sig;

	t = THREADOBJECT;

	if (sigemptyset(&mask) != 0)
		vm_abort("signal_thread: sigemptyset failed: %s", strerror(errno));

#if !defined(WITH_CLASSPATH_SUN)
	/* Let OpenJDK handle SIGINT itself. */

	if (sigaddset(&mask, SIGINT) != 0)
		vm_abort("signal_thread: sigaddset failed: %s", strerror(errno));
#endif

#if !defined(__FREEBSD__)
	if (sigaddset(&mask, SIGQUIT) != 0)
		vm_abort("signal_thread: sigaddset failed: %s", strerror(errno));
#endif

	for (;;) {
		/* just wait for a signal */

#if defined(ENABLE_THREADS)
		threads_thread_state_waiting(t);
#endif

		/* XXX We don't check for an error here, although the man-page
		   states sigwait does not return an error (which is wrong!),
		   but it seems to make problems with Boehm-GC.  We should
		   revisit this code with our new exact-GC. */

/* 		if (sigwait(&mask, &sig) != 0) */
/* 			vm_abort("signal_thread: sigwait failed: %s", strerror(errno)); */
		(void) sigwait(&mask, &sig);

#if defined(ENABLE_THREADS)
		threads_thread_state_runnable(t);
#endif

		/* Handle the signal. */

		signal_thread_handler(sig);
	}
}


/* signal_thread_handler *******************************************************

   Handles the signals caught in the signal handler thread.  Also used
   from sun.misc.Signal with OpenJDK.

*******************************************************************************/

void signal_thread_handler(int sig)
{
	switch (sig) {
	case SIGINT:
		/* exit the vm properly */

		vm_exit(0);
		break;

	case SIGQUIT:
		/* print a thread dump */
#if defined(ENABLE_THREADS)
		threads_dump();
#endif

#if defined(ENABLE_STATISTICS)
		if (opt_stat)
			statistics_print_memory_usage();
#endif
		break;
	}
}


/* signal_start_thread *********************************************************

   Starts the signal handler thread.

*******************************************************************************/

bool signal_start_thread(void)
{
#if defined(ENABLE_THREADS)
	utf *name;

	name = utf_new_char("Signal Handler");

	if (!threads_thread_start_internal(name, signal_thread))
		return false;

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
