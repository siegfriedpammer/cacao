/* src/threads/thread.cpp - machine independent thread functions

   Copyright (C) 2007, 2008
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

#include <assert.h>
#include <stdint.h>
#include <unistd.h>

#include "vm/types.h"

#include "mm/memory.h"

#if defined(ENABLE_GC_BOEHM)
/* We need to include Boehm's gc.h here for GC_register_my_thread and
   friends. */
# include "mm/boehm-gc/include/gc.h"
#endif

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"

#include "threads/lock-common.h"
#include "threads/threadlist.h"
#include "threads/thread.hpp"

#include "vm/builtin.h"
#include "vm/class.h"
#include "vm/exceptions.hpp"
#include "vm/globals.hpp"
#include "vm/javaobjects.hpp"
#include "vm/method.h"
#include "vm/options.h"
#include "vm/string.hpp"
#include "vm/utf8.h"
#include "vm/vm.hpp"

#if defined(ENABLE_STATISTICS)
# include "vm/statistics.h"
#endif

#include "vm/jit/stacktrace.hpp"


// FIXME
extern "C" {

/* global variables ***********************************************************/

static methodinfo    *thread_method_init;
static java_handle_t *threadgroup_system;
static java_handle_t *threadgroup_main;

#if defined(__LINUX__)
/* XXX Remove for exact-GC. */
bool threads_pthreads_implementation_nptl;
#endif


/* static functions ***********************************************************/

static void          thread_create_initial_threadgroups(void);
static void          thread_create_initial_thread(void);
static threadobject *thread_new(void);


/* threads_preinit *************************************************************

   Do some early initialization of stuff required.

*******************************************************************************/

void threads_preinit(void)
{
	threadobject *mainthread;
#if defined(__LINUX__) && defined(_CS_GNU_LIBPTHREAD_VERSION)
	char         *pathbuf;
	size_t        len;
#endif

	TRACESUBSYSTEMINITIALIZATION("threads_preinit");

#if defined(__LINUX__)
	/* XXX Remove for exact-GC. */

	/* On Linux we need to check the pthread implementation. */

	/* _CS_GNU_LIBPTHREAD_VERSION (GNU C library only; since glibc 2.3.2) */
	/* If the glibc is a pre-2.3.2 version, we fall back to
	   linuxthreads. */

# if defined(_CS_GNU_LIBPTHREAD_VERSION)
	len = confstr(_CS_GNU_LIBPTHREAD_VERSION, NULL, (size_t) 0);

	/* Some systems return as length 0 (maybe cross-compilation
	   related).  In this case we also fall back to linuxthreads. */

	if (len > 0) {
		pathbuf = MNEW(char, len);

		(void) confstr(_CS_GNU_LIBPTHREAD_VERSION, pathbuf, len);

		if (strstr(pathbuf, "NPTL") != NULL)
			threads_pthreads_implementation_nptl = true;
		else
			threads_pthreads_implementation_nptl = false;
	}
	else
		threads_pthreads_implementation_nptl = false;
# else
	threads_pthreads_implementation_nptl = false;
# endif
#endif

	/* Initialize the threads implementation (sets the thinlock on the
	   main thread). */

	threads_impl_preinit();

	/* Create internal thread data-structure for the main thread. */

	mainthread = thread_new();

	/* The main thread should always have index 1. */

	if (mainthread->index != 1)
		vm_abort("threads_preinit: main thread index not 1: %d != 1",
				 mainthread->index);

	/* thread is a Java thread and running */

	mainthread->flags |= THREAD_FLAG_JAVA;
	mainthread->state = THREAD_STATE_RUNNABLE;

	/* Store the internal thread data-structure in the TSD. */

	thread_set_current(mainthread);
}


/* threads_init ****************************************************************

   Initialize the main thread.

*******************************************************************************/

void threads_init(void)
{
	TRACESUBSYSTEMINITIALIZATION("threads_init");

	/* Create the system and main thread groups. */

	thread_create_initial_threadgroups();

	/* Cache the java.lang.Thread initialization method. */

#if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)

	thread_method_init =
		class_resolveclassmethod(class_java_lang_Thread,
								 utf_init,
								 utf_new_char("(Ljava/lang/VMThread;Ljava/lang/String;IZ)V"),
								 class_java_lang_Thread,
								 true);

#elif defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)

	thread_method_init =
		class_resolveclassmethod(class_java_lang_Thread,
								 utf_init,
								 utf_new_char("(Ljava/lang/ThreadGroup;Ljava/lang/String;)V"),
								 class_java_lang_Thread,
								 true);

#elif defined(WITH_JAVA_RUNTIME_LIBRARY_CLDC1_1)

	thread_method_init =
		class_resolveclassmethod(class_java_lang_Thread,
								 utf_init,
								 utf_java_lang_String__void,
								 class_java_lang_Thread,
								 true);

#else
# error unknown classpath configuration
#endif

	if (thread_method_init == NULL)
		vm_abort("threads_init: failed to resolve thread init method");

	thread_create_initial_thread();
}


/* thread_create_object ********************************************************

   Create a Java thread object for the given thread data-structure,
   initializes it and adds the thread to the threadgroup.

   ARGUMENTS:

       t ....... thread
       name .... thread name
       group ... threadgroup

   RETURN:

*******************************************************************************/

static bool thread_create_object(threadobject *t, java_handle_t *name, java_handle_t *group)
{
	/* Create a java.lang.Thread Java object. */

	java_handle_t* h = builtin_new(class_java_lang_Thread);

	if (h == NULL)
		return false;

	java_lang_Thread jlt(h);

	// Set the Java object in the thread data-structure.  This
	// indicates that the thread is attached to the VM.
	thread_set_object(t, jlt.get_handle());

#if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)

	h = builtin_new(class_java_lang_VMThread);

	if (h == NULL)
		return false;

	// Create and initialize a java.lang.VMThread object.
	java_lang_VMThread jlvmt(h, jlt.get_handle(), t);

	/* Call:
	   java.lang.Thread.<init>(Ljava/lang/VMThread;Ljava/lang/String;IZ)V */

	bool isdaemon = thread_is_daemon(t);

	(void) vm_call_method(thread_method_init, jlt.get_handle(), jlvmt.get_handle(),
						  name, NORM_PRIORITY, isdaemon);

	if (exceptions_get_exception())
		return false;

	// Set the ThreadGroup in the Java thread object.
	jlt.set_group(group);

	/* Add thread to the threadgroup. */

	classinfo* c;
	LLNI_class_get(group, c);

	methodinfo* m = class_resolveclassmethod(c,
											 utf_addThread,
											 utf_java_lang_Thread__V,
											 class_java_lang_ThreadGroup,
											 true);

	if (m == NULL)
		return false;

	(void) vm_call_method(m, group, jlt.get_handle());

	if (exceptions_get_exception())
		return false;

#elif defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)

	/* Set the priority.  java.lang.Thread.<init> requires it because
	   it sets the priority of the current thread to the parent's one
	   (which is the current thread in this case). */
	jlt.set_priority(NORM_PRIORITY);

	// Call: java.lang.Thread.<init>(Ljava/lang/ThreadGroup;Ljava/lang/String;)V

	(void) vm_call_method(thread_method_init, jlt.get_handle(), group, name);

	if (exceptions_get_exception())
		return false;

#elif defined(WITH_JAVA_RUNTIME_LIBRARY_CLDC1_1)

	// Set the thread data-structure in the Java thread object.
	jlt.set_vm_thread(t);

	// Call: public Thread(Ljava/lang/String;)V
	(void) vm_call_method(thread_method_init, jlt.get_handle(), name);

	if (exceptions_get_exception())
		return false;

#else
# error unknown classpath configuration
#endif

	return true;
}


/* thread_create_initial_threadgroups ******************************************

   Create the initial threadgroups.

   GNU Classpath:
       Create the main threadgroup only and set the system
       threadgroup to the main threadgroup.

   SUN:
       Create the system and main threadgroup.

   CLDC:
       This function is a no-op.

*******************************************************************************/

static void thread_create_initial_threadgroups(void)
{
#if defined(ENABLE_JAVASE)
# if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)

	/* Allocate and initialize the main thread group. */

	threadgroup_main = native_new_and_init(class_java_lang_ThreadGroup);

	if (threadgroup_main == NULL)
		vm_abort("thread_create_initial_threadgroups: failed to allocate main threadgroup");

	/* Use the same threadgroup for system as for main. */

	threadgroup_system = threadgroup_main;

# elif defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)

	java_handle_t *name;
	methodinfo    *m;

	/* Allocate and initialize the system thread group. */

	threadgroup_system = native_new_and_init(class_java_lang_ThreadGroup);

	if (threadgroup_system == NULL)
		vm_abort("thread_create_initial_threadgroups: failed to allocate system threadgroup");

	/* Allocate and initialize the main thread group. */

	threadgroup_main = builtin_new(class_java_lang_ThreadGroup);

	if (threadgroup_main == NULL)
		vm_abort("thread_create_initial_threadgroups: failed to allocate main threadgroup");

	name = javastring_new(utf_main);

	m = class_resolveclassmethod(class_java_lang_ThreadGroup,
								 utf_init,
								 utf_Ljava_lang_ThreadGroup_Ljava_lang_String__V,
								 class_java_lang_ThreadGroup,
								 true);

	if (m == NULL)
		vm_abort("thread_create_initial_threadgroups: failed to resolve threadgroup init method");

	(void) vm_call_method(m, threadgroup_main, threadgroup_system, name);

	if (exceptions_get_exception())
		vm_abort("thread_create_initial_threadgroups: exception while initializing main threadgroup");

# else
#  error unknown classpath configuration
# endif
#endif
}


/* thread_create_initial_thread ***********************************************

   Create the initial thread: main

*******************************************************************************/

static void thread_create_initial_thread(void)
{
	threadobject  *t;
	java_handle_t *name;

	/* Get the main-thread (NOTE: The main thread is always the first
	   thread in the list). */

	t = threadlist_first();

	/* The thread name. */

	name = javastring_new(utf_main);

#if defined(ENABLE_INTRP)
	/* create interpreter stack */

	if (opt_intrp) {
		MSET(intrp_main_stack, 0, u1, opt_stacksize);
		mainthread->_global_sp = (Cell*) (intrp_main_stack + opt_stacksize);
	}
#endif

	/* Create the Java thread object. */

	if (!thread_create_object(t, name, threadgroup_main))
		vm_abort("thread_create_initial_thread: failed to create Java object");

	/* Initialize the implementation specific bits. */

	threads_impl_init();

	DEBUGTHREADS("starting (main)", t);
}


/* thread_new ******************************************************************

   Allocates and initializes an internal thread data-structure and
   adds it to the threads list.

*******************************************************************************/

static threadobject *thread_new(void)
{
	int32_t       index;
	threadobject *t;
	
	/* Lock the thread lists */

	threadlist_lock();

	index = threadlist_get_free_index();

	/* Allocate a thread data structure. */

	/* First, try to get one from the free-list. */

	t = threadlist_free_first();

	if (t != NULL) {
		/* Remove from free list. */

		threadlist_free_remove(t);

		/* Equivalent of MZERO on the else path */

		threads_impl_thread_clear(t);
	}
	else {
#if defined(ENABLE_GC_BOEHM)
		t = GCNEW_UNCOLLECTABLE(threadobject, 1);
#else
		t = NEW(threadobject);
#endif

#if defined(ENABLE_STATISTICS)
		if (opt_stat)
			size_threadobject += sizeof(threadobject);
#endif

		/* Clear memory. */

		MZERO(t, threadobject, 1);

#if defined(ENABLE_GC_CACAO)
		/* Register reference to java.lang.Thread with the GC. */
		/* FIXME is it ok to do this only once? */

		gc_reference_register(&(t->object), GC_REFTYPE_THREADOBJECT);
		gc_reference_register(&(t->_exceptionptr), GC_REFTYPE_THREADOBJECT);
#endif

		/* Initialize the implementation-specific bits. */

		threads_impl_thread_init(t);
	}

	/* Pre-compute the thinlock-word. */

	assert(index != 0);

	t->index     = index;
	t->thinlock  = lock_pre_compute_thinlock(t->index);
	t->flags     = 0;
	t->state     = THREAD_STATE_NEW;

#if defined(ENABLE_GC_CACAO)
	t->flags    |= THREAD_FLAG_IN_NATIVE; 
#endif

	/* Initialize the implementation-specific bits. */

	threads_impl_thread_reuse(t);

	/* Add the thread to the thread list. */

	threadlist_add(t);

	/* Unlock the thread lists. */

	threadlist_unlock();

	return t;
}


/* thread_free *****************************************************************

   Remove the thread from the threads-list and free the internal
   thread data structure.  The thread index is added to the
   thread-index free-list.

   IN:
       t ... thread data structure

*******************************************************************************/

void thread_free(threadobject *t)
{
	/* Lock the thread lists. */

	threadlist_lock();

	/* Remove the thread from the thread-list. */

	threadlist_remove(t);

	/* Add the thread index to the free list. */

	threadlist_index_add(t->index);

	/* Set the reference to the Java object to NULL. */

	thread_set_object(t, NULL);

	/* Add the thread data structure to the free list. */

	threadlist_free_add(t);

	/* Unlock the thread lists. */

	threadlist_unlock();
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
	threadobject *t;

	/* Enter the join-mutex, so if the main-thread is currently
	   waiting to join all threads, the number of non-daemon threads
	   is correct. */

	threads_mutex_join_lock();

	/* Create internal thread data-structure. */

	t = thread_new();

	t->flags |= THREAD_FLAG_INTERNAL | THREAD_FLAG_DAEMON;

	/* The thread is flagged as (non-)daemon thread, we can leave the
	   mutex. */

	threads_mutex_join_unlock();

	/* Create the Java thread object. */

	if (!thread_create_object(t, javastring_new(name), threadgroup_system))
		return false;

	/* Start the thread. */

	threads_impl_thread_start(t, f);

	/* everything's ok */

	return true;
}


/* threads_thread_start ********************************************************

   Start a Java thread in the JVM.  Only the java thread object exists
   so far.

   IN:
      object.....the java thread object java.lang.Thread

*******************************************************************************/

void threads_thread_start(java_handle_t *object)
{
	java_lang_Thread jlt(object);

	/* Enter the join-mutex, so if the main-thread is currently
	   waiting to join all threads, the number of non-daemon threads
	   is correct. */

	threads_mutex_join_lock();

	/* Create internal thread data-structure. */

	threadobject* t = thread_new();

	/* this is a normal Java thread */

	t->flags |= THREAD_FLAG_JAVA;

#if defined(ENABLE_JAVASE)
	/* Is this a daemon thread? */

	if (jlt.get_daemon() == true)
		t->flags |= THREAD_FLAG_DAEMON;
#endif

	/* The thread is flagged and (non-)daemon thread, we can leave the
	   mutex. */

	threads_mutex_join_unlock();

	/* Link the two objects together. */

	thread_set_object(t, object);

#if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)

	/* Get the java.lang.VMThread object and do some sanity checks. */
	java_lang_VMThread jlvmt(jlt.get_vmThread());

	assert(jlvmt.get_handle() != NULL);
	assert(jlvmt.get_vmdata() == NULL);

	jlvmt.set_vmdata(t);

#elif defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)

	// Nothing to do.

#elif defined(WITH_JAVA_RUNTIME_LIBRARY_CLDC1_1)

	jlt.set_vm_thread(t);

#else
# error unknown classpath configuration
#endif

	/* Start the thread.  Don't pass a function pointer (NULL) since
	   we want Thread.run()V here. */

	threads_impl_thread_start(t, NULL);
}


/**
 * Attaches the current thread to the VM.
 *
 * @param vm_aargs Attach arguments.
 * @param isdaemon true if the attached thread should be a daemon
 *                 thread.
 *
 * @return true on success, false otherwise.
 */
bool thread_attach_current_thread(JavaVMAttachArgs *vm_aargs, bool isdaemon)
{
	bool           result;
	threadobject  *t;
	utf           *u;
	java_handle_t *name;
	java_handle_t *group;

    /* If the current thread has already been attached, this operation
	   is a no-op. */

	result = thread_current_is_attached();

	if (result == true)
		return true;

	/* Enter the join-mutex, so if the main-thread is currently
	   waiting to join all threads, the number of non-daemon threads
	   is correct. */

	threads_mutex_join_lock();

	/* Create internal thread data structure. */

	t = thread_new();

	/* Thread is a Java thread and running. */

	t->flags = THREAD_FLAG_JAVA;

	if (isdaemon)
		t->flags |= THREAD_FLAG_DAEMON;

	/* Store the internal thread data-structure in the TSD. */

	thread_set_current(t);

	/* The thread is flagged and (non-)daemon thread, we can leave the
	   mutex. */

	threads_mutex_join_unlock();

	DEBUGTHREADS("attaching", t);

	/* Get the thread name. */

	if (vm_aargs != NULL) {
		u = utf_new_char(vm_aargs->name);
	}
	else {
		u = utf_null;
	}

	name = javastring_new(u);

#if defined(ENABLE_JAVASE)
	/* Get the threadgroup. */

	if (vm_aargs != NULL)
		group = (java_handle_t *) vm_aargs->group;
	else
		group = NULL;

	/* If no threadgroup was given, use the main threadgroup. */

	if (group == NULL)
		group = threadgroup_main;
#endif

#if defined(ENABLE_INTRP)
	/* create interpreter stack */

	if (opt_intrp) {
		MSET(intrp_main_stack, 0, u1, opt_stacksize);
		thread->_global_sp = (Cell *) (intrp_main_stack + opt_stacksize);
	}
#endif

	/* Create the Java thread object. */

	if (!thread_create_object(t, name, group))
		return false;

	/* The thread is completely initialized. */

	thread_set_state_runnable(t);

	return true;
}


/**
 * Attaches the current external thread to the VM.  This function is
 * called by JNI's AttachCurrentThread.
 *
 * @param vm_aargs Attach arguments.
 * @param isdaemon true if the attached thread should be a daemon
 *                 thread.
 *
 * @return true on success, false otherwise.
 */
bool thread_attach_current_external_thread(JavaVMAttachArgs *vm_aargs, bool isdaemon)
{
	int result;

#if defined(ENABLE_GC_BOEHM)
	struct GC_stack_base sb;
#endif

#if defined(ENABLE_GC_BOEHM)
	/* Register the thread with Boehm-GC.  This must happen before the
	   thread allocates any memory from the GC heap.*/

	result = GC_get_stack_base(&sb);

	if (result != GC_SUCCESS)
		vm_abort("threads_attach_current_thread: GC_get_stack_base failed");

	GC_register_my_thread(&sb);
#endif

	result = thread_attach_current_thread(vm_aargs, isdaemon);

	if (result == false) {
#if defined(ENABLE_GC_BOEHM)
		/* Unregister the thread. */

		GC_unregister_my_thread();
#endif

		return false;
	}

	return true;
}


/**
 * Detaches the current external thread from the VM.  This function is
 * called by JNI's DetachCurrentThread.
 *
 * @return true on success, false otherwise.
 */
bool thread_detach_current_external_thread(void)
{
	int result;

	result = thread_detach_current_thread();

	if (result == false)
		return false;

#if defined(ENABLE_GC_BOEHM)
	/* Unregister the thread with Boehm-GC.  This must happen after
	   the thread allocates any memory from the GC heap. */

	/* Don't detach the main thread.  This is a workaround for
	   OpenJDK's java binary. */
	if (thread_get_current()->index != 1)
		GC_unregister_my_thread();
#endif

	return true;
}


/* thread_fprint_name **********************************************************

   Print the name of the given thread to the given stream.

   ARGUMENTS:
       t ........ thread data-structure
       stream ... stream to print to

*******************************************************************************/

void thread_fprint_name(threadobject *t, FILE *stream)
{
	if (thread_get_object(t) == NULL)
		vm_abort("");

	java_lang_Thread jlt(thread_get_object(t));

#if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)

	java_handle_t* name = jlt.get_name();
	javastring_fprint(name, stream);

#elif defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK) || defined(WITH_JAVA_RUNTIME_LIBRARY_CLDC1_1)

	/* FIXME: In OpenJDK and CLDC the name is a char[]. */
	java_chararray_t *name;

	/* FIXME This prints to stdout. */
	utf_display_printable_ascii(utf_null);

#else
# error unknown classpath configuration
#endif
}


/* thread_print_info ***********************************************************

   Print information of the passed thread.

   ARGUMENTS:
       t ... thread data-structure.

*******************************************************************************/

void thread_print_info(threadobject *t)
{
	java_lang_Thread jlt(thread_get_object(t));

	/* Print as much as we can when we are in state NEW. */

	if (jlt.get_handle() != NULL) {
		/* Print thread name. */

		printf("\"");
		thread_fprint_name(t, stdout);
		printf("\"");
	}
	else {
	}

	if (thread_is_daemon(t))
		printf(" daemon");

	if (jlt.get_handle() != NULL) {
		printf(" prio=%d", jlt.get_priority());
	}

#if SIZEOF_VOID_P == 8
	printf(" t=0x%016lx tid=0x%016lx (%ld)",
		   (ptrint) t, (ptrint) t->tid, (ptrint) t->tid);
#else
	printf(" t=0x%08x tid=0x%08x (%d)",
		   (ptrint) t, (ptrint) t->tid, (ptrint) t->tid);
#endif

	printf(" index=%d", t->index);

	/* Print thread state. */

	int state = cacaothread_get_state(t);

	switch (state) {
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
		vm_abort("thread_print_info: unknown thread state %d", state);
	}
}


/* threads_get_current_tid *****************************************************

   Return the tid of the current thread.
   
   RETURN VALUE:
       the current tid

*******************************************************************************/

intptr_t threads_get_current_tid(void)
{
	threadobject *thread;

	thread = THREADOBJECT;

	/* this may happen during bootstrap */

	if (thread == NULL)
		return 0;

	return (intptr_t) thread->tid;
}


/* thread_set_state_runnable ***************************************************

   Set the current state of the given thread to THREAD_STATE_RUNNABLE.

   NOTE: If the thread has already terminated, don't set the state.
         This is important for threads_detach_thread.

*******************************************************************************/

void thread_set_state_runnable(threadobject *t)
{
	/* Set the state inside a lock. */

	threadlist_lock();

	if (t->state != THREAD_STATE_TERMINATED) {
		t->state = THREAD_STATE_RUNNABLE;

		DEBUGTHREADS("is RUNNABLE", t);
	}

	threadlist_unlock();
}


/* thread_set_state_waiting ****************************************************

   Set the current state of the given thread to THREAD_STATE_WAITING.

   NOTE: If the thread has already terminated, don't set the state.
         This is important for threads_detach_thread.

*******************************************************************************/

void thread_set_state_waiting(threadobject *t)
{
	/* Set the state inside a lock. */

	threadlist_lock();

	if (t->state != THREAD_STATE_TERMINATED) {
		t->state = THREAD_STATE_WAITING;

		DEBUGTHREADS("is WAITING", t);
	}

	threadlist_unlock();
}


/* thread_set_state_timed_waiting **********************************************

   Set the current state of the given thread to
   THREAD_STATE_TIMED_WAITING.

   NOTE: If the thread has already terminated, don't set the state.
         This is important for threads_detach_thread.

*******************************************************************************/

void thread_set_state_timed_waiting(threadobject *t)
{
	/* Set the state inside a lock. */

	threadlist_lock();

	if (t->state != THREAD_STATE_TERMINATED) {
		t->state = THREAD_STATE_TIMED_WAITING;

		DEBUGTHREADS("is TIMED_WAITING", t);
	}

	threadlist_unlock();
}


/* thread_set_state_terminated *************************************************

   Set the current state of the given thread to
   THREAD_STATE_TERMINATED.

*******************************************************************************/

void thread_set_state_terminated(threadobject *t)
{
	/* Set the state inside a lock. */

	threadlist_lock();

	t->state = THREAD_STATE_TERMINATED;

	DEBUGTHREADS("is TERMINATED", t);

	threadlist_unlock();
}


/* thread_get_thread **********************************************************

   Return the thread data structure of the given Java thread object.

   ARGUMENTS:
       h ... java.lang.{VM}Thread object

   RETURN VALUE:
       the thread object

*******************************************************************************/

threadobject *thread_get_thread(java_handle_t *h)
{
#if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)

	java_lang_VMThread jlvmt(h);
	threadobject* t = jlvmt.get_vmdata();

#elif defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)

	/* XXX This is just a quick hack. */
	threadobject* t;
	bool          equal;

	threadlist_lock();

	for (t = threadlist_first(); t != NULL; t = threadlist_next(t)) {
		LLNI_equals(t->object, h, equal);

		if (equal == true)
			break;
	}

	threadlist_unlock();

#elif defined(WITH_JAVA_RUNTIME_LIBRARY_CLDC1_1)

	log_println("threads_get_thread: IMPLEMENT ME!");
	threadobject* t = NULL;

#else
# error unknown classpath configuration
#endif

	return t;
}


/* threads_thread_is_alive *****************************************************

   Returns if the give thread is alive.

*******************************************************************************/

bool threads_thread_is_alive(threadobject *t)
{
	int state;

	state = cacaothread_get_state(t);

	switch (state) {
	case THREAD_STATE_NEW:
	case THREAD_STATE_TERMINATED:
		return false;

	case THREAD_STATE_RUNNABLE:
	case THREAD_STATE_BLOCKED:
	case THREAD_STATE_WAITING:
	case THREAD_STATE_TIMED_WAITING:
		return true;

	default:
		vm_abort("threads_thread_is_alive: unknown thread state %d", state);
	}

	/* keep compiler happy */

	return false;
}


/* threads_dump ****************************************************************

   Dumps info for all threads running in the JVM.  This function is
   called when SIGQUIT (<ctrl>-\) is sent to CACAO.

*******************************************************************************/

void threads_dump(void)
{
	threadobject *t;

	/* XXX we should stop the world here */

	/* Lock the thread lists. */

	threadlist_lock();

	printf("Full thread dump CACAO "VERSION":\n");

	/* iterate over all started threads */

	for (t = threadlist_first(); t != NULL; t = threadlist_next(t)) {
		/* ignore threads which are in state NEW */
		if (t->state == THREAD_STATE_NEW)
			continue;

#if defined(ENABLE_GC_CACAO)
		/* Suspend the thread. */
		/* XXX Is the suspend reason correct? */

		if (threads_suspend_thread(t, SUSPEND_REASON_JNI) == false)
			vm_abort("threads_dump: threads_suspend_thread failed");
#endif

		/* Print thread info. */

		printf("\n");
		thread_print_info(t);
		printf("\n");

		/* Print trace of thread. */

		stacktrace_print_of_thread(t);

#if defined(ENABLE_GC_CACAO)
		/* Resume the thread. */

		if (threads_resume_thread(t) == false)
			vm_abort("threads_dump: threads_resume_thread failed");
#endif
	}

	/* Unlock the thread lists. */

	threadlist_unlock();
}

} // extern "C"


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
