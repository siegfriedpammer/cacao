/* src/vm/finalizer.c - finalizer linked list and thread

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

   $Id: finalizer.c 5123 2006-07-12 21:45:34Z twisti $

*/


#include "config.h"

#include <stdlib.h>

#include "vm/types.h"

#include "mm/memory.h"
#include "native/jni.h"
#include "native/include/java_lang_Thread.h"
#include "native/include/java_lang_VMThread.h"

#if defined(ENABLE_THREADS)
# include "threads/native/lock.h"
#endif

#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/options.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"
#include "vm/jit/asmpart.h"


/* global variables ***********************************************************/

#if defined(ENABLE_THREADS)
static java_lang_VMThread *finalizer_vmthread;
static java_objectheader *lock_finalizer_thread;
#endif


/* finalizer_init **************************************************************

   Initializes the finalizer global lock and the linked list.

*******************************************************************************/

bool finalizer_init(void)
{
#if defined(ENABLE_THREADS)
	lock_finalizer_thread = NEW(java_objectheader);

	lock_init_object_lock(lock_finalizer_thread);
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

		lock_monitor_enter(lock_finalizer_thread);

		/* wait forever (0, 0) on that object till we are signaled */
	
		lock_wait_for_object(lock_finalizer_thread, 0, 0);

		/* leave the lock */

		lock_monitor_exit(lock_finalizer_thread);

		/* and call the finalizers */

		gc_invoke_finalizers();
	}
}
#endif


/* finalizer_start_thread ******************************************************

   Starts the finalizer thread.

*******************************************************************************/

#if defined(ENABLE_THREADS)
bool finalizer_start_thread(void)
{
	java_lang_Thread *t;

	/* create the finalizer object */

	finalizer_vmthread =
		(java_lang_VMThread *) builtin_new(class_java_lang_VMThread);

	if (!finalizer_vmthread)
		return false;

	t = (java_lang_Thread *) builtin_new(class_java_lang_Thread);

	t->vmThread = finalizer_vmthread;
	t->name     = javastring_new_from_ascii("Finalizer");
	t->daemon   = true;
	t->priority = 5;

	finalizer_vmthread->thread = t;

	/* actually start the finalizer thread */

	threads_start_thread(t, finalizer_thread);

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
#if defined(ENABLE_THREADS)
	/* get the lock on the finalizer lock object, so we can call wait */

	lock_monitor_enter(lock_finalizer_thread);

	/* signal the finalizer thread */
	
	lock_notify_object(lock_finalizer_thread);

	/* leave the lock */

	lock_monitor_exit(lock_finalizer_thread);
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
	java_objectheader *ob;

	ob = (java_objectheader *) o;

	/* call the finalizer function */

	(void) vm_call_method(ob->vftbl->class->finalizer, ob);

	/* if we had an exception in the finalizer, ignore it */

	*exceptionptr = NULL;
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
