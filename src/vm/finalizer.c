/* src/vm/finalizer.c - finalizer linked list and thread

   Copyright (C) 1996-2005, 2006, 2007, 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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

#include <stdlib.h>

#include "vm/types.h"

#include "mm/memory.h"

#include "threads/lock-common.h"
#include "threads/thread.hpp"

#include "vm/builtin.h"
#include "vm/exceptions.hpp"
#include "vm/global.h"
#include "vm/vm.hpp"

#include "vm/jit/asmpart.h"

#include "vmcore/options.h"


/* global variables ***********************************************************/

#if defined(ENABLE_THREADS)
static java_object_t *lock_thread_finalizer;
#endif


/* finalizer_init **************************************************************

   Initializes the finalizer global lock and the linked list.

*******************************************************************************/

bool finalizer_init(void)
{
	TRACESUBSYSTEMINITIALIZATION("finalizer_init");

#if defined(ENABLE_THREADS)
	lock_thread_finalizer = NEW(java_object_t);

	LOCK_INIT_OBJECT_LOCK(lock_thread_finalizer);
#endif

	/* everything's ok */

	return true;
}


/* finalizer_thread ************************************************************

   This thread waits on an object for a notification and the runs the
   finalizers (finalizer thread).  This is necessary because of a
   possible deadlock in the GC.

*******************************************************************************/

#if defined(ENABLE_THREADS)
static void finalizer_thread(void)
{
	while (true) {
		/* get the lock on the finalizer lock object, so we can call wait */

		LOCK_MONITOR_ENTER(lock_thread_finalizer);

		/* wait forever on that object till we are signaled */
	
		LOCK_WAIT_FOREVER(lock_thread_finalizer);

		/* leave the lock */

		LOCK_MONITOR_EXIT(lock_thread_finalizer);

#if !defined(NDEBUG)
		if (opt_DebugFinalizer)
			log_println("[finalizer thread    : status=awake]");
#endif

		/* and call the finalizers */

		gc_invoke_finalizers();

#if !defined(NDEBUG)
		if (opt_DebugFinalizer)
			log_println("[finalizer thread    : status=sleeping]");
#endif
	}
}
#endif


/* finalizer_start_thread ******************************************************

   Starts the finalizer thread.

*******************************************************************************/

#if defined(ENABLE_THREADS)
bool finalizer_start_thread(void)
{
	utf *name;

	name = utf_new_char("Finalizer");

	if (!threads_thread_start_internal(name, finalizer_thread))
		return false;

	/* everything's ok */

	return true;
}
#endif


/* finalizer_notify ************************************************************

   Notifies the finalizer thread that it should run the
   gc_invoke_finalizers from the GC.

*******************************************************************************/

void finalizer_notify(void)
{
#if !defined(NDEBUG)
	if (opt_DebugFinalizer)
		log_println("[finalizer notified]");
#endif

#if defined(ENABLE_THREADS)
	/* get the lock on the finalizer lock object, so we can call wait */

	LOCK_MONITOR_ENTER(lock_thread_finalizer);

	/* signal the finalizer thread */
	
	LOCK_NOTIFY(lock_thread_finalizer);

	/* leave the lock */

	LOCK_MONITOR_EXIT(lock_thread_finalizer);
#else
	/* if we don't have threads, just run the finalizers */

	gc_invoke_finalizers();
#endif
}


/* finalizer_run ***************************************************************

   Actually run the finalizer functions.

*******************************************************************************/

void finalizer_run(void *o, void *p)
{
	java_handle_t *h;
	classinfo     *c;

	h = (java_handle_t *) o;

#if !defined(ENABLE_GC_CACAO) && defined(ENABLE_HANDLES)
	/* XXX this is only a dirty hack to make Boehm work with handles */

	h = LLNI_WRAP((java_object_t *) h);
#endif

	LLNI_class_get(h, c);

#if !defined(NDEBUG)
	if (opt_DebugFinalizer) {
		log_start();
		log_print("[finalizer running   : o=%p p=%p class=", o, p);
		class_print(c);
		log_print("]");
		log_finish();
	}
#endif

	/* call the finalizer function */

	(void) vm_call_method(c->finalizer, h);

#if !defined(NDEBUG)
	if (opt_DebugFinalizer && (exceptions_get_exception() != NULL)) {
		log_println("[finalizer exception]");
		exceptions_print_stacktrace();
	}
#endif

	/* if we had an exception in the finalizer, ignore it */

	exceptions_clear_exception();
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
