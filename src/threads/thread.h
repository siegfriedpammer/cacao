/* src/threads/thread.h - machine independent thread functions

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


#ifndef _THREADS_COMMON_H
#define _THREADS_COMMON_H

#include "config.h"

#include "vmcore/system.h"

#if defined(ENABLE_THREADS)
# include "threads/posix/thread-posix.h"
#else
# include "threads/none/threads.h"
#endif

#include "vm/types.h"

#include "vm/global.h"

#include "native/jni.h"
#include "native/llni.h"

#include "vmcore/utf8.h"


/* only define the following stuff with thread enabled ************************/

#if defined(ENABLE_THREADS)

/* thread states **************************************************************/

#define THREAD_STATE_NEW              0
#define THREAD_STATE_RUNNABLE         1
#define THREAD_STATE_BLOCKED          2
#define THREAD_STATE_WAITING          3
#define THREAD_STATE_TIMED_WAITING    4
#define THREAD_STATE_TERMINATED       5


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
			threads_thread_print_info(thread); \
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


/* inline functions ***********************************************************/

/* thread_get_object ***********************************************************

   Return the Java for the given thread.

   ARGUMENTS:
       t ... thread

   RETURN:
       the Java object

*******************************************************************************/

inline static java_handle_t *thread_get_object(threadobject *t)
{
	return LLNI_WRAP(t->object);
}


/* threads_thread_set_object ***************************************************

   Set the Java object for the given thread.

   ARGUMENTS:
       t ... thread
	   o ... Java object

*******************************************************************************/

inline static void thread_set_object(threadobject *t, java_handle_t *o)
{
	t->object = LLNI_DIRECT(o);
}


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
	o = thread_get_object(t);

	return o;
}


/* thread_is_attached **********************************************************

   Returns if the given thread is attached to the VM.

   RETURN:
       true .... the thread is attached to the VM
       false ... the thread is not

*******************************************************************************/

inline static bool thread_is_attached(threadobject *t)
{
	java_handle_t *o;

	o = thread_get_object(t);

	if (o != NULL)
		return true;
	else
		return false;
}


/* thread_is_daemon ************************************************************

   Returns if the given thread is a daemon thread.

   RETURN:
       true .... the thread is a daemon thread
       false ... the thread is not

*******************************************************************************/

inline static bool thread_is_daemon(threadobject *t)
{
	if (t->flags & THREAD_FLAG_DAEMON)
		return true;
	else
		return false;
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
	bool           result;

	t      = THREADOBJECT;
	result = thread_is_attached(t);

	return result;
}


/* function prototypes ********************************************************/

void          threads_preinit(void);
void          threads_init(void);

void          thread_free(threadobject *t);

bool          threads_thread_start_internal(utf *name, functionptr f);
void          threads_thread_start(java_handle_t *object);

bool          threads_attach_current_thread(JavaVMAttachArgs *vm_aargs, bool isdaemon);

void          thread_fprint_name(threadobject *t, FILE *stream);
void          threads_thread_print_info(threadobject *t);

intptr_t      threads_get_current_tid(void);

void          threads_thread_state_runnable(threadobject *t);
void          threads_thread_state_waiting(threadobject *t);
void          threads_thread_state_timed_waiting(threadobject *t);
void          threads_thread_state_terminated(threadobject *t);

utf          *threads_thread_get_state(threadobject *t);
threadobject *thread_get_thread(java_handle_t *h);

bool          threads_thread_is_alive(threadobject *t);

void          threads_dump(void);
void          threads_thread_print_stacktrace(threadobject *thread);
void          threads_print_stacktrace(void);


/* implementation specific functions */

void          threads_impl_preinit(void);
void          threads_impl_init(void);

#if defined(ENABLE_GC_CACAO)
void          threads_mutex_gc_lock(void);
void          threads_mutex_gc_unlock(void);
#endif

void          threads_mutex_join_lock(void);
void          threads_mutex_join_unlock(void);

void          threads_impl_thread_init(threadobject *t);
void          threads_impl_thread_clear(threadobject *t);
void          threads_impl_thread_reuse(threadobject *t);
void          threads_impl_thread_free(threadobject *t);
void          threads_impl_thread_start(threadobject *thread, functionptr f);

void          threads_yield(void);

#endif /* ENABLE_THREADS */

#endif /* _THREADS_COMMON_H */


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
