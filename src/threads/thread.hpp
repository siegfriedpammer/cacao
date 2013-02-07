/* src/threads/thread.hpp - machine independent thread functions

   Copyright (C) 1996-2013
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


#ifndef _THREAD_HPP
#define _THREAD_HPP

#include "config.h"

#include "vm/types.h"

// Include early to get threadobject.
#if defined(ENABLE_THREADS)
# include "threads/posix/thread-posix.hpp"
#else
# include "threads/none/thread-none.hpp"
#endif

#include "vm/os.hpp"

#include "native/llni.hpp"

#include "threads/mutex.hpp"

#include "vm/global.h"
#include "vm/utf8.hpp"

// short-hand for '#ifdef ENABLE_THREADS' block
#if defined(ENABLE_THREADS)
	#define WITH_THREADS(X) X
#else
	#define WITH_THREADS(X)
#endif

/* only define the following stuff with thread enabled ************************/

#if defined(ENABLE_THREADS)

/* thread states **************************************************************/

#define THREAD_STATE_NEW              0
#define THREAD_STATE_RUNNABLE         1
#define THREAD_STATE_BLOCKED          2
#define THREAD_STATE_WAITING          3
#define THREAD_STATE_TIMED_WAITING    4
#define THREAD_STATE_TERMINATED       5
#define THREAD_STATE_PARKED           6
#define THREAD_STATE_TIMED_PARKED     7


/* thread priorities **********************************************************/

#define MIN_PRIORITY     1
#define NORM_PRIORITY    5
#define MAX_PRIORITY     10


/* debug **********************************************************************/

#if !defined(NDEBUG)
# define DEBUGTHREADS(message, thread) \
	do { \
		if (opt_DebugThreads) { \
			printf("[Thread %-16s: ", message); \
			thread_print_info(thread); \
			printf("]\n"); \
		} \
	} while (0)
#else
# define DEBUGTHREADS(message, thread)
#endif


/* global variables ***********************************************************/

#if defined(__LINUX__)
/* XXX Remove for exact-GC. */
extern bool threads_pthreads_implementation_nptl;
#endif


#ifdef __cplusplus

/* inline functions ***********************************************************/

/* thread_get_current_object **************************************************

   Return the Java object of the current thread.
   
   RETURN VALUE:
       the Java object

*******************************************************************************/

inline static java_handle_t *thread_get_current_object(void)
{
	threadobject  *t;
	java_handle_t *o;

	t = THREADOBJECT;
	o = LLNI_WRAP(t->object);

	return o;
}


/* cacaothread_get_state *******************************************************

   Returns the current state of the given thread.

   ARGUMENTS:
       t ... the thread to check

   RETURN:
       thread state

*******************************************************************************/

inline static int cacaothread_get_state(threadobject *t)
{
	return t->state;
}


/* thread_is_attached **********************************************************

   Returns if the given thread is attached to the VM.

   ARGUMENTS:
       t ... the thread to check

   RETURN:
       true .... the thread is attached to the VM
       false ... the thread is not

*******************************************************************************/

inline static bool thread_is_attached(threadobject *t)
{
	java_handle_t *o;

	o = LLNI_WRAP(t->object);

	return o != NULL;
}


/* thread_is_daemon ************************************************************

   Returns if the given thread is a daemon thread.

   ARGUMENTS:
       t ... the thread to check

   RETURN:
       true .... the thread is a daemon thread
       false ... the thread is not

*******************************************************************************/

inline static bool thread_is_daemon(threadobject *t)
{
	return (t->flags & THREAD_FLAG_DAEMON) != 0;
}


/* thread_current_is_attached **************************************************

   Returns if the current thread is attached to the VM.

   RETURN:
       true .... the thread is attached to the VM
       false ... the thread is not

*******************************************************************************/

inline static bool thread_current_is_attached(void)
{
	threadobject  *t;

	t = thread_get_current();

	if (t == NULL)
		return false;

	return thread_is_attached(t);
}


/* function prototypes ********************************************************/

void          threads_preinit(void);
void          threads_init(void);

void          thread_free(threadobject *t);

bool          threads_thread_start_internal(Utf8String name, functionptr f);
void          threads_thread_start(java_handle_t *object);

bool          thread_attach_current_thread(JavaVMAttachArgs *vm_aargs, bool isdaemon);
bool          thread_attach_current_external_thread(JavaVMAttachArgs *vm_aargs, bool isdaemon);
bool          thread_detach_current_thread(void);

bool          thread_detach_current_external_thread(void);

void          thread_fprint_name(threadobject *t, FILE *stream);
void          thread_print_info(threadobject *t);

intptr_t      threads_get_current_tid(void);

void          thread_set_state_runnable(threadobject *t);
void          thread_set_state_waiting(threadobject *t);
void          thread_set_state_timed_waiting(threadobject *t);
void          thread_set_state_parked(threadobject *t);
void          thread_set_state_timed_parked(threadobject *t);
void          thread_set_state_terminated(threadobject *t);

threadobject *thread_get_thread(java_handle_t *h);

bool          threads_thread_is_alive(threadobject *t);
bool          thread_is_interrupted(threadobject *t);
void          thread_set_interrupted(threadobject *t, bool interrupted);

/* implementation specific functions */

void          threads_impl_preinit(void);
void          threads_impl_init(void);

#if defined(ENABLE_GC_CACAO)
void          threads_mutex_gc_lock(void);
void          threads_mutex_gc_unlock(void);
#endif

void          threads_impl_thread_clear(threadobject *t);
void          threads_impl_thread_reuse(threadobject *t);
void          threads_impl_clear_heap_pointers(threadobject *t);
void          threads_impl_thread_start(threadobject *thread, functionptr f);

void          threads_yield(void);

#endif

#endif /* ENABLE_THREADS */

#endif // _THREAD_HPP

void          thread_handle_set_priority(java_handle_t *th, int);
bool          thread_handle_is_interrupted(java_handle_t *th);
void          thread_handle_interrupt(java_handle_t *th);
int           thread_handle_get_state(java_handle_t *th);

#if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)
#include "thread-classpath.hpp"
#elif defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)
#include "thread-openjdk.hpp"
#elif defined(WITH_JAVA_RUNTIME_LIBRARY_CLDC1_1)
#include "thread-cldc11.hpp"
#endif


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
