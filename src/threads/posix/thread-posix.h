/* src/threads/posix/thread-posix.h - POSIX thread functions

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


#ifndef _THREAD_POSIX_H
#define _THREAD_POSIX_H

/* forward typedefs ***********************************************************/

typedef struct threadobject threadobject;


#include "config.h"

#include <pthread.h>
#include <ucontext.h>

#include "vm/types.h"

#include "mm/memory.h"

#include "native/localref.h"

#include "threads/mutex.h"

#include "threads/posix/lock.h"

#include "vm/global.h"
#include "vm/vm.h"

#if defined(ENABLE_GC_CACAO)
# include "vm/jit/replace.h"
#endif

#include "vm/jit/stacktrace.h"

#if defined(ENABLE_INTRP)
#include "vm/jit/intrp/intrp.h"
#endif

#if defined(__DARWIN__)
# include <mach/mach.h>

typedef struct {
	mutex_t mutex;
	pthread_cond_t cond;
	int value;
} sem_t;

#else
# include <semaphore.h>
#endif


/* current threadobject *******************************************************/

#if defined(HAVE___THREAD)

#define THREADSPECIFIC    __thread
#define THREADOBJECT      thread_current

extern __thread threadobject *thread_current;

#else /* defined(HAVE___THREAD) */

#define THREADSPECIFIC
#define THREADOBJECT \
	((threadobject *) pthread_getspecific(thread_current_key))

extern pthread_key_t thread_current_key;

#endif /* defined(HAVE___THREAD) */


/* threadobject ****************************************************************

   Struct holding thread local variables.

*******************************************************************************/

#define THREAD_FLAG_JAVA        0x01    /* a normal Java thread               */
#define THREAD_FLAG_INTERNAL    0x02    /* CACAO internal thread              */
#define THREAD_FLAG_DAEMON      0x04    /* daemon thread                      */
#define THREAD_FLAG_IN_NATIVE   0x08    /* currently executing native code    */

#define SUSPEND_REASON_JNI       1      /* suspended from JNI                 */
#define SUSPEND_REASON_STOPWORLD 2      /* suspended from stop-thw-world      */


struct threadobject {
	java_object_t        *object;       /* link to java.lang.Thread object    */

	ptrint                thinlock;     /* pre-computed thin lock value       */

	s4                    index;        /* thread index, starting with 1      */
	u4                    flags;        /* flag field                         */
	u4                    state;        /* state field                        */

	pthread_t             tid;          /* pthread id                         */

#if defined(__DARWIN__)
	mach_port_t           mach_thread;       /* Darwin thread id              */
#endif

	/* for the sable tasuki lock extension */
	bool                  flc_bit;
	struct threadobject  *flc_list;     /* FLC list head for this thread      */
	struct threadobject  *flc_next;     /* next pointer for FLC list          */
	java_handle_t        *flc_object;
	mutex_t               flc_lock;     /* controlling access to these fields */
	pthread_cond_t        flc_cond;

	/* these are used for the wait/notify implementation                      */
	mutex_t               waitmutex;
	pthread_cond_t        waitcond;

	mutex_t               suspendmutex; /* lock before suspending this thread */
	pthread_cond_t        suspendcond;  /* notify to resume this thread       */

	bool                  interrupted;
	bool                  signaled;
	bool                  sleeping;

	bool                  suspended;    /* is this thread suspended?          */
	s4                    suspend_reason; /* reason for suspending            */

	u1                   *pc;           /* current PC (used for profiling)    */

	java_object_t        *_exceptionptr;     /* current exception             */
	stackframeinfo_t     *_stackframeinfo;   /* current native stackframeinfo */
	localref_table       *_localref_table;   /* JNI local references          */

#if defined(ENABLE_INTRP)
	Cell                 *_global_sp;        /* stack pointer for interpreter */
#endif

#if defined(ENABLE_GC_CACAO)
	bool                  gc_critical;  /* indicates a critical section       */

	sourcestate_t        *ss;
	executionstate_t     *es;
#endif

	dumpinfo_t            dumpinfo;     /* dump memory info structure         */

#if defined(ENABLE_DEBUG_FILTER)
	u2                    filterverbosecallctr[2]; /* counters for verbose call filter */
#endif

#if !defined(NDEBUG)
	s4                    tracejavacallindent;
	u4                    tracejavacallcount;
#endif

	listnode_t            linkage;      /* threads-list                       */
	listnode_t            linkage_free; /* free-list                          */
};


/* native-world flags *********************************************************/

#if defined(ENABLE_GC_CACAO)
# define THREAD_NATIVEWORLD_ENTER THREADOBJECT->flags |=  THREAD_FLAG_IN_NATIVE
# define THREAD_NATIVEWORLD_EXIT  THREADOBJECT->flags &= ~THREAD_FLAG_IN_NATIVE
#else
# define THREAD_NATIVEWORLD_ENTER /*nop*/
# define THREAD_NATIVEWORLD_EXIT  /*nop*/
#endif


/* counter for verbose call filter ********************************************/

#if defined(ENABLE_DEBUG_FILTER)
#	define FILTERVERBOSECALLCTR (THREADOBJECT->filterverbosecallctr)
#endif

/* state for trace java call **************************************************/

#if !defined(NDEBUG)
#	define TRACEJAVACALLINDENT (THREADOBJECT->tracejavacallindent)
#	define TRACEJAVACALLCOUNT (THREADOBJECT->tracejavacallcount)
#endif


/* inline functions ***********************************************************/

/* thread_get_current **********************************************************

   Return the threadobject of the current thread.
   
   RETURN:
       the current threadobject *

*******************************************************************************/

inline static threadobject *thread_get_current(void)
{
	threadobject *t;

#if defined(HAVE___THREAD)
	t = thread_current;
#else
	t = (threadobject *) pthread_getspecific(thread_current_key);
#endif

	return t;
}


/* thread_set_current **********************************************************

   Set the current thread object.
   
   IN:
      t ... the thread object to set

*******************************************************************************/

inline static void thread_set_current(threadobject *t)
{
#if defined(HAVE___THREAD)
	thread_current = t;
#else
	int result;

	result = pthread_setspecific(thread_current_key, t);

	if (result != 0)
		vm_abort_errnum(result, "thread_set_current: pthread_setspecific failed");
#endif
}


inline static stackframeinfo_t *threads_get_current_stackframeinfo(void)
{
	return THREADOBJECT->_stackframeinfo;
}

inline static void threads_set_current_stackframeinfo(stackframeinfo_t *sfi)
{
	THREADOBJECT->_stackframeinfo = sfi;
}


/* functions ******************************************************************/

void threads_sem_init(sem_t *sem, bool shared, int value);
void threads_sem_wait(sem_t *sem);
void threads_sem_post(sem_t *sem);

void threads_start_thread(threadobject *thread, functionptr function);

void threads_set_thread_priority(pthread_t tid, int priority);

bool threads_detach_thread(threadobject *thread);

#if defined(ENABLE_GC_CACAO)
bool threads_suspend_thread(threadobject *thread, s4 reason);
void threads_suspend_ack(u1* pc, u1* sp);
bool threads_resume_thread(threadobject *thread);
#endif

void threads_join_all_threads(void);

void threads_sleep(int64_t millis, int32_t nanos);

void threads_wait_with_timeout_relative(threadobject *t, s8 millis, s4 nanos);

void threads_thread_interrupt(threadobject *thread);

#endif /* _THREAD_POSIX_H */


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
