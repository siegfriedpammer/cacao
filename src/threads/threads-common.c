/* src/threads/threads-common.c - machine independent thread functions

   Copyright (C) 2007 R. Grafl, A. Krall, C. Kruegel,
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

   $Id: threads-common.c 7963 2007-05-24 10:21:16Z twisti $

*/


#include "config.h"

#include <assert.h>

#include "vm/types.h"

#include "mm/memory.h"

#include "native/jni.h"

#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_String.h"
#include "native/include/java_lang_Thread.h"

#if defined(WITH_CLASSPATH_GNU)
# include "native/include/java_lang_VMThread.h"
#endif

#include "threads/critical.h"
#include "threads/lock-common.h"
#include "threads/threads-common.h"

#include "toolbox/list.h"

#include "vm/builtin.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"

#include "vm/jit/stacktrace.h"

#include "vmcore/class.h"

#if defined(ENABLE_STATISTICS)
# include "vmcore/options.h"
# include "vmcore/statistics.h"
#endif

#include "vmcore/utf8.h"


/* global variables ***********************************************************/

/* global threads list */
static list_t *list_threads;

/* global threads free-list */
static list_t *list_threads_free;


/* threads_preinit *************************************************************

   Do some early initialization of stuff required.

   ATTENTION: Do NOT use any Java heap allocation here, as gc_init()
   is called AFTER this function!

*******************************************************************************/

void threads_preinit(void)
{
	threadobject *mainthread;

	/* initialize the threads lists */

	list_threads      = list_create(OFFSET(threadobject, linkage));
	list_threads_free = list_create(OFFSET(threadobject, linkage));

	/* Initialize the threads implementation (sets the thinlock on the
	   main thread). */

	threads_impl_preinit();

	/* create internal thread data-structure for the main thread */

	mainthread = threads_thread_new();

	/* thread is a Java thread and running */

	mainthread->flags = THREAD_FLAG_JAVA;
	mainthread->state = THREAD_STATE_RUNNABLE;

	/* store the internal thread data-structure in the TSD */

	threads_set_current_threadobject(mainthread);

	/* initialize locking subsystems */

	lock_init();

	/* initialize the critical section */

	critical_init();
}


/* threads_list_first **********************************************************

   Return the first entry in the threads list.

   NOTE: This function does not lock the lists.

*******************************************************************************/

threadobject *threads_list_first(void)
{
	threadobject *t;

	t = list_first_unsynced(list_threads);

	return t;
}


/* threads_list_next ***********************************************************

   Return the next entry in the threads list.

   NOTE: This function does not lock the lists.

*******************************************************************************/

threadobject *threads_list_next(threadobject *t)
{
	threadobject *next;

	next = list_next_unsynced(list_threads, t);

	return next;
}


/* threads_list_get_non_daemons ************************************************

   Return the number of non-daemon threads.

   NOTE: This function does a linear-search over the threads list,
         because it's only used for joining the threads.

*******************************************************************************/

s4 threads_list_get_non_daemons(void)
{
	threadobject *t;
	s4            nondaemons;

	/* lock the threads lists */

	threads_list_lock();

	nondaemons = 0;

	for (t = threads_list_first(); t != NULL; t = threads_list_next(t)) {
		if (!(t->flags & THREAD_FLAG_DAEMON))
			nondaemons++;
	}

	/* unlock the threads lists */

	threads_list_unlock();

	return nondaemons;
}


/* threads_thread_new **********************************************************

   Allocates and initializes an internal thread data-structure and
   adds it to the threads list.

*******************************************************************************/

threadobject *threads_thread_new(void)
{
	threadobject *t;

	/* lock the threads-lists */

	threads_list_lock();

	/* try to get a thread from the free-list */

	t = list_first_unsynced(list_threads_free);

	/* is a free thread available? */

	if (t != NULL) {
		/* yes, remove it from the free list */

		list_remove_unsynced(list_threads_free, t);
	}
	else {
		/* no, allocate a new one */

#if defined(ENABLE_GC_BOEHM)
		t = GCNEW_UNCOLLECTABLE(threadobject, 1);
#else
		t = NEW(threadobject);
#endif

#if defined(ENABLE_STATISTICS)
		if (opt_stat)
			size_threadobject += sizeof(threadobject);
#endif

		/* clear memory */

		MZERO(t, threadobject, 1);

		/* set the threads-index */

		t->index = list_threads->size + 1;
	}

	/* pre-compute the thinlock-word */

	assert(t->index != 0);

	t->thinlock = lock_pre_compute_thinlock(t->index);
	t->state    = THREAD_STATE_NEW;

	/* initialize the implementation-specific bits */

	threads_impl_thread_new(t);

	/* add the thread to the threads-list */

	list_add_last_unsynced(list_threads, t);

	/* unlock the threads-lists */

	threads_list_unlock();

	return t;
}


/* threads_thread_free *********************************************************

   Frees an internal thread data-structure by removing it from the
   threads-list and adding it to the free-list.

   NOTE: The data-structure is NOT freed, the pointer keeps valid!

*******************************************************************************/

void threads_thread_free(threadobject *t)
{
	s4 index;

	/* lock the threads-lists */

	threads_list_lock();

	/* cleanup the implementation-specific bits */

	threads_impl_thread_free(t);

	/* remove the thread from the threads-list */

	list_remove_unsynced(list_threads, t);

	/* Clear memory, but keep the thread-index. */
	/* ATTENTION: Do this after list_remove, otherwise the linkage
	   pointers are invalid. */

	index = t->index;

	MZERO(t, threadobject, 1);

	t->index = index;

	/* add the thread to the free list */

	list_add_first_unsynced(list_threads_free, t);

	/* unlock the threads-lists */

	threads_list_unlock();
}


/* threads_thread_start_internal ***********************************************

   Start an internal thread in the JVM.  No Java thread objects exists
   so far.

   IN:
      name.......UTF-8 name of the thread
      f..........function pointer to C function to start

*******************************************************************************/

bool threads_thread_start_internal(utf *name, functionptr f)
{
	threadobject       *thread;
	java_lang_Thread   *t;
#if defined(WITH_CLASSPATH_GNU)
	java_lang_VMThread *vmt;
#endif

	/* create internal thread data-structure */

	thread = threads_thread_new();

	/* create the java thread object */

	t = (java_lang_Thread *) builtin_new(class_java_lang_Thread);

	if (t == NULL)
		return false;

#if defined(WITH_CLASSPATH_GNU)
	vmt = (java_lang_VMThread *) builtin_new(class_java_lang_VMThread);

	if (vmt == NULL)
		return false;

	vmt->thread = t;
	vmt->vmdata = (java_lang_Object *) thread;

	t->vmThread = vmt;
#elif defined(WITH_CLASSPATH_CLDC1_1)
	t->vm_thread = (java_lang_Object *) thread;
#endif

	thread->object = t;

	thread->flags = THREAD_FLAG_INTERNAL | THREAD_FLAG_DAEMON;

	/* set java.lang.Thread fields */

	t->name     = (java_lang_String *) javastring_new(name);
#if defined(ENABLE_JAVASE)
	t->daemon   = true;
#endif
	t->priority = NORM_PRIORITY;

	/* start the thread */

	threads_impl_thread_start(thread, f);

	/* everything's ok */

	return true;
}


/* threads_thread_start ********************************************************

   Start a Java thread in the JVM.  Only the java thread object exists
   so far.

   IN:
      object.....the java thread object java.lang.Thread

*******************************************************************************/

void threads_thread_start(java_lang_Thread *object)
{
	threadobject *thread;

	/* create internal thread data-structure */

	thread = threads_thread_new();

	/* link the two objects together */

	thread->object = object;

	/* this is a normal Java thread */

	thread->flags = THREAD_FLAG_JAVA;

#if defined(ENABLE_JAVASE)
	/* is this a daemon thread? */

	if (object->daemon == true)
		thread->flags |= THREAD_FLAG_DAEMON;
#endif

#if defined(WITH_CLASSPATH_GNU)
	assert(object->vmThread);
	assert(object->vmThread->vmdata == NULL);

	object->vmThread->vmdata = (java_lang_Object *) thread;
#elif defined(WITH_CLASSPATH_CLDC1_1)
	object->vm_thread = (java_lang_Object *) thread;
#endif

	/* Start the thread.  Don't pass a function pointer (NULL) since
	   we want Thread.run()V here. */

	threads_impl_thread_start(thread, NULL);
}


/* threads_thread_print_info ***************************************************

   Print information of the passed thread.
   
*******************************************************************************/

void threads_thread_print_info(threadobject *t)
{
	java_lang_Thread *object;
	utf              *name;

	assert(t->state != THREAD_STATE_NEW);

	/* the thread may be currently in initalization, don't print it */

	object = t->object;

	if (object != NULL) {
		/* get thread name */

#if defined(ENABLE_JAVASE)
		name = javastring_toutf((java_objectheader *) object->name, false);
#elif defined(ENABLE_JAVAME_CLDC1_1)
		name = object->name;
#endif

		printf("\"");
		utf_display_printable_ascii(name);
		printf("\"");

		if (t->flags & THREAD_FLAG_DAEMON)
			printf(" daemon");

		printf(" prio=%d", object->priority);

#if SIZEOF_VOID_P == 8
		printf(" t=0x%016lx tid=0x%016lx (%ld)",
			   (ptrint) t, (ptrint) t->tid, (ptrint) t->tid);
#else
		printf(" t=0x%08x tid=0x%08x (%d)",
			   (ptrint) t, (ptrint) t->tid, (ptrint) t->tid);
#endif

		printf(" index=%d", t->index);

		/* print thread state */

		switch (t->state) {
		case THREAD_STATE_NEW:
			printf(" new");
			break;
		case THREAD_STATE_RUNNABLE:
			printf(" runnable");
			break;
		case THREAD_STATE_BLOCKED:
			printf(" blocked");
			break;
		case THREAD_STATE_WAITING:
			printf(" waiting");
			break;
		case THREAD_STATE_TIMED_WAITING:
			printf(" waiting on condition");
			break;
		case THREAD_STATE_TERMINATED:
			printf(" terminated");
			break;
		default:
			vm_abort("threads_thread_print_info: unknown thread state %d",
					 t->state);
		}
	}
}


/* threads_get_current_tid *****************************************************

   Return the tid of the current thread.
   
   RETURN VALUE:
       the current tid

*******************************************************************************/

ptrint threads_get_current_tid(void)
{
	threadobject *thread;

	thread = THREADOBJECT;

	/* this may happen during bootstrap */

	if (thread == NULL)
		return 0;

	return (ptrint) thread->tid;
}


/* threads_thread_state_runnable ***********************************************

   Set the current state of the given thread to THREAD_STATE_RUNNABLE.

*******************************************************************************/

void threads_thread_state_runnable(threadobject *t)
{
	/* set the state inside the lock */

	threads_list_lock();

	t->state = THREAD_STATE_RUNNABLE;

	threads_list_unlock();
}


/* threads_thread_state_waiting ************************************************

   Set the current state of the given thread to THREAD_STATE_WAITING.

*******************************************************************************/

void threads_thread_state_waiting(threadobject *t)
{
	/* set the state in the lock */

	threads_list_lock();

	t->state = THREAD_STATE_WAITING;

	threads_list_unlock();
}


/* threads_thread_state_timed_waiting ******************************************

   Set the current state of the given thread to
   THREAD_STATE_TIMED_WAITING.

*******************************************************************************/

void threads_thread_state_timed_waiting(threadobject *t)
{
	/* set the state in the lock */

	threads_list_lock();

	t->state = THREAD_STATE_TIMED_WAITING;

	threads_list_unlock();
}


/* threads_thread_state_terminated *********************************************

   Set the current state of the given thread to
   THREAD_STATE_TERMINATED.

*******************************************************************************/

void threads_thread_state_terminated(threadobject *t)
{
	/* set the state in the lock */

	threads_list_lock();

	t->state = THREAD_STATE_TERMINATED;

	threads_list_unlock();
}


/* threads_thread_get_state ****************************************************

   Returns the current state of the given thread.

*******************************************************************************/

utf *threads_thread_get_state(threadobject *t)
{
	utf *u;

	switch (t->state) {
	case THREAD_STATE_NEW:
		u = utf_new_char("NEW");
		break;
	case THREAD_STATE_RUNNABLE:
		u = utf_new_char("RUNNABLE");
		break;
	case THREAD_STATE_BLOCKED:
		u = utf_new_char("BLOCKED");
		break;
	case THREAD_STATE_WAITING:
		u = utf_new_char("WAITING");
		break;
	case THREAD_STATE_TIMED_WAITING:
		u = utf_new_char("TIMED_WAITING");
		break;
	case THREAD_STATE_TERMINATED:
		u = utf_new_char("TERMINATED");
		break;
	default:
		vm_abort("threads_get_state: unknown thread state %d", t->state);

		/* keep compiler happy */

		u = NULL;
	}

	return u;
}


/* threads_thread_is_alive *****************************************************

   Returns if the give thread is alive.

*******************************************************************************/

bool threads_thread_is_alive(threadobject *thread)
{
	bool result;

	switch (thread->state) {
	case THREAD_STATE_NEW:
	case THREAD_STATE_TERMINATED:
		result = false;
		break;

	case THREAD_STATE_RUNNABLE:
	case THREAD_STATE_BLOCKED:
	case THREAD_STATE_WAITING:
	case THREAD_STATE_TIMED_WAITING:
		result = true;
		break;

	default:
		vm_abort("threads_is_alive: unknown thread state %d", thread->state);

		/* keep compiler happy */

		result = false;
	}

	return result;
}


/* threads_dump ****************************************************************

   Dumps info for all threads running in the JVM.  This function is
   called when SIGQUIT (<ctrl>-\) is sent to CACAO.

*******************************************************************************/

void threads_dump(void)
{
	threadobject *t;

	/* XXX we should stop the world here */

	/* lock the threads lists */

	threads_list_lock();

	printf("Full thread dump CACAO "VERSION":\n");

	/* iterate over all started threads */

	for (t = threads_list_first(); t != NULL; t = threads_list_next(t)) {
		/* print thread info */

		printf("\n");
		threads_thread_print_info(t);
		printf("\n");

		/* print trace of thread */

		threads_thread_print_stacktrace(t);
	}

	/* unlock the threads lists */

	threads_list_unlock();
}


/* threads_thread_print_stacktrace *********************************************

   Print the current stacktrace of the current thread.

*******************************************************************************/

void threads_thread_print_stacktrace(threadobject *thread)
{
	stackframeinfo   *sfi;
	stacktracebuffer *stb;
	s4                dumpsize;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* create a stacktrace for the passed thread */

	sfi = thread->_stackframeinfo;

	stb = stacktrace_create(sfi);

	/* print stacktrace */

	if (stb != NULL)
		stacktrace_print_trace_from_buffer(stb);
	else {
		puts("\t<<No stacktrace available>>");
		fflush(stdout);
	}

	dump_release(dumpsize);
}


/* threads_print_stacktrace ****************************************************

   Print the current stacktrace of the current thread.

*******************************************************************************/

void threads_print_stacktrace(void)
{
	threadobject *thread;

	thread = THREADOBJECT;

	threads_thread_print_stacktrace(thread);
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
 * vim:noexpandtab:sw=4:ts=4:
 */
