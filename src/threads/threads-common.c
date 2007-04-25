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

   $Id: threads-common.c 7825 2007-04-25 20:31:57Z twisti $

*/


#include "config.h"

#include <assert.h>

#include "vm/types.h"

#include "native/jni.h"

#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_String.h"
#include "native/include/java_lang_Thread.h"

#if defined(WITH_CLASSPATH_GNU)
# include "native/include/java_lang_VMThread.h"
#endif

#include "threads/lock-common.h"
#include "threads/threads-common.h"

#include "threads/native/threads.h"

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


/* threads_create_thread *******************************************************

   Creates and initializes an internal thread data-structure.

*******************************************************************************/

threadobject *threads_create_thread(void)
{
	threadobject *thread;

	/* allocate internal thread data-structure */

#if defined(ENABLE_GC_BOEHM)
	thread = GCNEW_UNCOLLECTABLE(threadobject, 1);
#else
	thread = NEW(threadobject);
#endif

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		size_threadobject += sizeof(threadobject);
#endif

	/* initialize thread data structure */

	threads_init_threadobject(thread);
	lock_init_execution_env(thread);

	return thread;
}


/* threads_thread_create_internal **********************************************

   Creates an internal thread data-structure with the given name, plus
   necessary Java objects for the VM (e.g. finalizer-thread,
   signal-thread, ...).

*******************************************************************************/

threadobject *threads_thread_create_internal(utf *name)
{
	threadobject       *thread;
	java_lang_Thread   *t;
#if defined(WITH_CLASSPATH_GNU)
	java_lang_VMThread *vmt;
#endif

	/* create internal thread data-structure */

	thread = threads_create_thread();

	/* create the java thread object */

	t = (java_lang_Thread *) builtin_new(class_java_lang_Thread);

	if (t == NULL)
		return NULL;

#if defined(WITH_CLASSPATH_GNU)
	vmt = (java_lang_VMThread *) builtin_new(class_java_lang_VMThread);

	if (vmt == NULL)
		return NULL;

	vmt->thread = t;
	vmt->vmdata = (java_lang_Object *) thread;

	t->vmThread = vmt;
#elif defined(WITH_CLASSPATH_CLDC1_1)
	t->vm_thread = (java_lang_Object *) thread;
#endif

	thread->object     = t;
	thread->flags      = THREAD_FLAG_DAEMON;

	/* set java.lang.Thread fields */

	t->name     = (java_lang_String *) javastring_new(name);
#if defined(ENABLE_JAVASE)
	t->daemon   = true;
#endif
	t->priority = NORM_PRIORITY;

	/* return the thread data-structure */

	return thread;
}


/* threads_start_javathread ****************************************************

   Start a thread in the JVM. Only the java thread object exists so far.

   IN:
      object.....the java thread object java.lang.Thread

*******************************************************************************/

void threads_start_javathread(void *vobject)
{
	threadobject     *thread;
	java_lang_Thread *object;

	object = (java_lang_Thread *) vobject;

	/* create internal thread data-structure */

	thread = threads_create_thread();

	/* link the two objects together */

	thread->object = object;

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

	/* Actually start the thread.  Don't pass a function pointer
	   (NULL) since we want Thread.run()V here. */

	threads_start_thread(thread, NULL);
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


/* threads_thread_get_state ****************************************************

   Returns the current state of the given thread.

*******************************************************************************/

utf *threads_thread_get_state(threadobject *thread)
{
	utf *u;

	switch (thread->state) {
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
		vm_abort("threads_get_state: unknown thread state %d", thread->state);
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
	}

	return result;
}


/* threads_dump ****************************************************************

   Dumps info for all threads running in the JVM.  This function is
   called when SIGQUIT (<ctrl>-\) is sent to CACAO.

*******************************************************************************/

void threads_dump(void)
{
	threadobject     *thread;
	java_lang_Thread *t;
	utf              *name;

	/* XXX we should stop the world here */

	printf("Full thread dump CACAO "VERSION":\n");

	/* iterate over all started threads */

	thread = mainthreadobj;

	do {
		/* get thread object */

		t = thread->object;

		/* the thread may be currently in initalization, don't print it */

		if (t != NULL) {
			/* get thread name */

#if defined(ENABLE_JAVASE)
			name = javastring_toutf((java_objectheader *) t->name, false);
#elif defined(ENABLE_JAVAME_CLDC1_1)
			name = t->name;
#endif

			printf("\n\"");
			utf_display_printable_ascii(name);
			printf("\"");

			if (thread->flags & THREAD_FLAG_DAEMON)
				printf(" daemon");

			printf(" prio=%d", t->priority);

#if SIZEOF_VOID_P == 8
			printf(" tid=0x%016lx (%ld)",
				   (ptrint) thread->tid, (ptrint) thread->tid);
#else
			printf(" tid=0x%08lx (%ld)",
				   (ptrint) thread->tid, (ptrint) thread->tid);
#endif

			/* print thread state */

			switch (thread->state) {
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
				vm_abort("threads_dump: unknown thread state %d",
						 thread->state);
			}

			printf("\n");

			/* print trace of thread */

			threads_thread_print_stacktrace(thread);
		}

		thread = thread->next;
	} while ((thread != NULL) && (thread != mainthreadobj));
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
