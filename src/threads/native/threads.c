/* src/threads/native/threads.c - native threads support

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

   Authors: Stefan Ring

   Changes: Christian Thalinger
   			Edwin Steiner

   $Id: threads.c 4911 2006-05-14 12:15:12Z edwin $

*/


#include "config.h"

/* XXX cleanup these includes */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include <pthread.h>
#include <semaphore.h>

#include "vm/types.h"

#include "arch.h"

#if !defined(USE_MD_THREAD_STUFF)
#include "machine-instr.h"
#else
#include "threads/native/generic-primitives.h"
#endif

#include "mm/boehm.h"
#include "mm/memory.h"
#include "native/native.h"
#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_Throwable.h"
#include "native/include/java_lang_Thread.h"
#include "native/include/java_lang_ThreadGroup.h"
#include "native/include/java_lang_VMThread.h"
#include "threads/native/threads.h"
#include "toolbox/avl.h"
#include "toolbox/logging.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"
#include "vm/jit/asmpart.h"

#if !defined(__DARWIN__)
#if defined(__LINUX__)
#define GC_LINUX_THREADS
#elif defined(__MIPS__)
#define GC_IRIX_THREADS
#endif
#include "boehm-gc/include/gc.h"
#endif


/* internally used constants **************************************************/

/* CAUTION: Do not change these values. Boehm GC code depends on them.        */
#define STOPWORLD_FROM_GC               1
#define STOPWORLD_FROM_CLASS_NUMBERING  2


/* startupinfo *****************************************************************

   Struct used to pass info from threads_start_thread to 
   threads_startup_thread.

******************************************************************************/

typedef struct {
	threadobject *thread;      /* threadobject for this thread             */
	functionptr   function;    /* function to run in the new thread        */
	sem_t        *psem;        /* signals when thread has been entered     */
	                           /* in the thread list                       */
	sem_t        *psem_first;  /* signals when pthread_create has returned */
} startupinfo;


/* prototypes *****************************************************************/

static void threads_calc_absolute_time(struct timespec *tm, s8 millis, s4 nanos);


/******************************************************************************/
/* GLOBAL VARIABLES                                                           */
/******************************************************************************/

/* the main thread                                                            */
threadobject *mainthreadobj;

/* the thread object of the current thread                                    */
/* This is either a thread-local variable defined with __thread, or           */
/* a thread-specific value stored with key threads_current_threadobject_key.  */
#if defined(HAVE___THREAD)
__thread threadobject *threads_current_threadobject;
#else
pthread_key_t threads_current_threadobject_key;
#endif

/* global compiler mutex                                                      */
static pthread_mutex_rec_t compiler_mutex;

/* global mutex for changing the thread list                                  */
static pthread_mutex_t threadlistlock;

/* global mutex for stop-the-world                                            */
static pthread_mutex_t stopworldlock;

/* this is one of the STOPWORLD_FROM_ constants, telling why the world is     */
/* being stopped                                                              */
static volatile int stopworldwhere;

/* semaphore used for acknowleding thread suspension                          */
static sem_t suspend_ack;
#if defined(__MIPS__)
static pthread_mutex_t suspend_ack_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t suspend_cond = PTHREAD_COND_INITIALIZER;
#endif

static pthread_attr_t threadattr;

/* mutexes used by the fake atomic instructions                               */
#if defined(USE_MD_THREAD_STUFF)
pthread_mutex_t _atomic_add_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t _cas_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t _mb_lock = PTHREAD_MUTEX_INITIALIZER;
#endif


/******************************************************************************/
/* Recursive Mutex Implementation for Darwin                                  */
/******************************************************************************/

#if defined(MUTEXSIM)

/* We need this for older MacOSX (10.1.x) */

void pthread_mutex_init_rec(pthread_mutex_rec_t *m)
{
	pthread_mutex_init(&m->mutex, NULL);
	m->count = 0;
}

void pthread_mutex_destroy_rec(pthread_mutex_rec_t *m)
{
	pthread_mutex_destroy(&m->mutex);
}

void pthread_mutex_lock_rec(pthread_mutex_rec_t *m)
{
	for (;;) {
		if (!m->count)
		{
			pthread_mutex_lock(&m->mutex);
			m->owner = pthread_self();
			m->count++;
			break;
		}
		else {
			if (m->owner != pthread_self()) {
				pthread_mutex_lock(&m->mutex);
			}
			else {
				m->count++;
				break;
			}
		}
	}
}

void pthread_mutex_unlock_rec(pthread_mutex_rec_t *m)
{
	if (!--m->count)
		pthread_mutex_unlock(&m->mutex);
}

#endif /* defined(MUTEXSIM) */


/* threads_sem_init ************************************************************
 
   Initialize a semaphore. Checks against errors and interruptions.

   IN:
       sem..............the semaphore to initialize
	   shared...........true if this semaphore will be shared between processes
	   value............the initial value for the semaphore
   
*******************************************************************************/

void threads_sem_init(sem_t *sem, bool shared, int value)
{
	int r;

	assert(sem);

	do {
		r = sem_init(sem, shared, value);
		if (r == 0)
			return;
	} while (errno == EINTR);

	fprintf(stderr,"error: sem_init returned unexpected error %d: %s\n",
			errno, strerror(errno));
	abort();
}


/* threads_sem_wait ************************************************************
 
   Wait for a semaphore, non-interruptible.

   IMPORTANT: Always use this function instead of `sem_wait` directly, as
              `sem_wait` may be interrupted by signals!
  
   IN:
       sem..............the semaphore to wait on
   
*******************************************************************************/

void threads_sem_wait(sem_t *sem)
{
	int r;

	assert(sem);

	do {
		r = sem_wait(sem);
		if (r == 0)
			return;
	} while (errno == EINTR);

	fprintf(stderr,"error: sem_wait returned unexpected error %d: %s\n",
			errno, strerror(errno));
	abort();
}


/* threads_sem_post ************************************************************
 
   Increase the count of a semaphore. Checks for errors.

   IN:
       sem..............the semaphore to increase the count of
   
*******************************************************************************/

void threads_sem_post(sem_t *sem)
{
	int r;

	assert(sem);

	/* unlike sem_wait, sem_post is not interruptible */

	r = sem_post(sem);
	if (r == 0)
		return;

	fprintf(stderr,"error: sem_post returned unexpected error %d: %s\n",
			errno, strerror(errno));
	abort();
}


/* threads_set_thread_priority *************************************************

   Set the priority of the given thread.

   IN:
      tid..........thread id
	  priority.....priority to set

******************************************************************************/

static void threads_set_thread_priority(pthread_t tid, int priority)
{
	struct sched_param schedp;
	int policy;

	pthread_getschedparam(tid, &policy, &schedp);
	schedp.sched_priority = priority;
	pthread_setschedparam(tid, policy, &schedp);
}


/* compiler_lock ***************************************************************

   Enter the compiler lock.

******************************************************************************/

void compiler_lock(void)
{
	pthread_mutex_lock_rec(&compiler_mutex);
}


/* compiler_unlock *************************************************************

   Release the compiler lock.

******************************************************************************/

void compiler_unlock(void)
{
	pthread_mutex_unlock_rec(&compiler_mutex);
}


/* lock_stopworld **************************************************************

   Enter the stopworld lock, specifying why the world shall be stopped.

   IN:
      where........ STOPWORLD_FROM_GC              (1) from within GC
                    STOPWORLD_FROM_CLASS_NUMBERING (2) class numbering

******************************************************************************/

void lock_stopworld(int where)
{
	pthread_mutex_lock(&stopworldlock);
	stopworldwhere = where;
}


/* unlock_stopworld ************************************************************

   Release the stopworld lock.

******************************************************************************/

void unlock_stopworld(void)
{
	stopworldwhere = 0;
	pthread_mutex_unlock(&stopworldlock);
}

#if !defined(__DARWIN__)
/* Caller must hold threadlistlock */
static int threads_cast_sendsignals(int sig, int count)
{
	/* Count threads */
	threadobject *tobj = mainthreadobj;
	nativethread *infoself = THREADINFO;

	if (count == 0) {
		do {
			count++;
			tobj = tobj->info.next;
		} while (tobj != mainthreadobj);
	}

	do {
		nativethread *info = &tobj->info;
		if (info != infoself)
			pthread_kill(info->tid, sig);
		tobj = tobj->info.next;
	} while (tobj != mainthreadobj);

	return count-1;
}

#else

static void threads_cast_darwinstop(void)
{
	threadobject *tobj = mainthreadobj;
	nativethread *infoself = THREADINFO;

	do {
		nativethread *info = &tobj->info;
		if (info != infoself)
		{
			thread_state_flavor_t flavor = PPC_THREAD_STATE;
			mach_msg_type_number_t thread_state_count = PPC_THREAD_STATE_COUNT;
			ppc_thread_state_t thread_state;
			mach_port_t thread = info->mach_thread;
			kern_return_t r;

			r = thread_suspend(thread);
			if (r != KERN_SUCCESS) {
				log_text("thread_suspend failed");
				assert(0);
			}

			r = thread_get_state(thread, flavor,
				(natural_t*)&thread_state, &thread_state_count);
			if (r != KERN_SUCCESS) {
				log_text("thread_get_state failed");
				assert(0);
			}

			thread_restartcriticalsection(&thread_state);

			r = thread_set_state(thread, flavor,
				(natural_t*)&thread_state, thread_state_count);
			if (r != KERN_SUCCESS) {
				log_text("thread_set_state failed");
				assert(0);
			}
		}
		tobj = tobj->info.next;
	} while (tobj != mainthreadobj);
}

static void threads_cast_darwinresume(void)
{
	threadobject *tobj = mainthreadobj;
	nativethread *infoself = THREADINFO;

	do {
		nativethread *info = &tobj->info;
		if (info != infoself)
		{
			mach_port_t thread = info->mach_thread;
			kern_return_t r;

			r = thread_resume(thread);
			if (r != KERN_SUCCESS) {
				log_text("thread_resume failed");
				assert(0);
			}
		}
		tobj = tobj->info.next;
	} while (tobj != mainthreadobj);
}

#endif

#if defined(__MIPS__)
static void threads_cast_irixresume(void)
{
	pthread_mutex_lock(&suspend_ack_lock);
	pthread_cond_broadcast(&suspend_cond);
	pthread_mutex_unlock(&suspend_ack_lock);
}
#endif

void threads_cast_stopworld(void)
{
	int count, i;
	lock_stopworld(STOPWORLD_FROM_CLASS_NUMBERING);
	pthread_mutex_lock(&threadlistlock);
#if defined(__DARWIN__)
	threads_cast_darwinstop();
#else
	count = threads_cast_sendsignals(GC_signum1(), 0);
	for (i=0; i<count; i++)
		threads_sem_wait(&suspend_ack);
#endif
	pthread_mutex_unlock(&threadlistlock);
}

void threads_cast_startworld(void)
{
	pthread_mutex_lock(&threadlistlock);
#if defined(__DARWIN__)
	threads_cast_darwinresume();
#elif defined(__MIPS__)
	threads_cast_irixresume();
#else
	threads_cast_sendsignals(GC_signum2(), -1);
#endif
	pthread_mutex_unlock(&threadlistlock);
	unlock_stopworld();
}

#if !defined(__DARWIN__)
static void threads_sigsuspend_handler(ucontext_t *ctx)
{
	int sig;
	sigset_t sigs;

	/* XXX TWISTI: this is just a quick hack */
#if defined(ENABLE_JIT)
	thread_restartcriticalsection(ctx);
#endif

	/* Do as Boehm does. On IRIX a condition variable is used for wake-up
	   (not POSIX async-safe). */
#if defined(__IRIX__)
	pthread_mutex_lock(&suspend_ack_lock);
	threads_sem_post(&suspend_ack);
	pthread_cond_wait(&suspend_cond, &suspend_ack_lock);
	pthread_mutex_unlock(&suspend_ack_lock);
#else
	threads_sem_post(&suspend_ack);

	sig = GC_signum2();
	sigfillset(&sigs);
	sigdelset(&sigs, sig);
	sigsuspend(&sigs);
#endif
}

/* This function is called from Boehm GC code. */

int cacao_suspendhandler(ucontext_t *ctx)
{
	if (stopworldwhere != STOPWORLD_FROM_CLASS_NUMBERING)
		return 0;

	threads_sigsuspend_handler(ctx);
	return 1;
}
#endif


/* threads_set_current_threadobject ********************************************

   Set the current thread object.
   
   IN:
      thread.......the thread object to set

*******************************************************************************/

#if !defined(ENABLE_JVMTI)
static void threads_set_current_threadobject(threadobject *thread)
#else
void threads_set_current_threadobject(threadobject *thread)
#endif
{
#if !defined(HAVE___THREAD)
	pthread_setspecific(threads_current_threadobject_key, thread);
#else
	threads_current_threadobject = thread;
#endif
}


/* threads_get_current_threadobject ********************************************

   Return the threadobject of the current thread.
   
   RETURN VALUE:
       the current threadobject * (an instance of java.lang.VMThread)

*******************************************************************************/

threadobject *threads_get_current_threadobject(void)
{
	return THREADOBJECT;
}


/* threads_preinit *************************************************************

   Do some early initialization of stuff required.

*******************************************************************************/

void threads_preinit(void)
{
#ifndef MUTEXSIM
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&compiler_mutex, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);
#else
	pthread_mutex_init_rec(&compiler_mutex);
#endif

	pthread_mutex_init(&threadlistlock, NULL);
	pthread_mutex_init(&stopworldlock, NULL);

	/* Allocate something so the garbage collector's signal handlers
	   are installed. */
	heap_allocate(1, false, NULL);

	mainthreadobj = NEW(threadobject);
	mainthreadobj->info.tid = pthread_self();
#if !defined(HAVE___THREAD)
	pthread_key_create(&threads_current_threadobject_key, NULL);
#endif
	threads_set_current_threadobject(mainthreadobj);

	/* we need a working dummyLR before initializing the critical
	   section tree */

	lock_init();

	critical_init();

	threads_sem_init(&suspend_ack, 0, 0);
}


/* threads_init ****************************************************************

   Initializes the threads required by the JVM: main, finalizer.

*******************************************************************************/

bool threads_init(u1 *stackbottom)
{
	java_lang_String      *threadname;
	java_lang_Thread      *mainthread;
	java_lang_ThreadGroup *threadgroup;
	threadobject          *tempthread;
	methodinfo            *method;

	tempthread = mainthreadobj;

	lock_record_free_pools(mainthreadobj->ee.lrpool);

	/* This is kinda tricky, we grow the java.lang.Thread object so we
	   can keep the execution environment there. No Thread object must
	   have been created at an earlier time. */

	class_java_lang_VMThread->instancesize = sizeof(threadobject);

	/* create a VMThread */

	mainthreadobj = (threadobject *) builtin_new(class_java_lang_VMThread);

	if (!mainthreadobj)
		return false;

	FREE(tempthread, threadobject);

	threads_init_threadobject(&mainthreadobj->o);

	threads_set_current_threadobject(mainthreadobj);

	lock_init_execution_env(mainthreadobj);

	mainthreadobj->info.next = mainthreadobj;
	mainthreadobj->info.prev = mainthreadobj;

#if defined(ENABLE_INTRP)
	/* create interpreter stack */

	if (opt_intrp) {
		MSET(intrp_main_stack, 0, u1, opt_stacksize);
		mainthreadobj->info._global_sp = intrp_main_stack + opt_stacksize;
	}
#endif

	threadname = javastring_new(utf_new_char("main"));

	/* allocate and init ThreadGroup */

	threadgroup = (java_lang_ThreadGroup *)
		native_new_and_init(class_java_lang_ThreadGroup);

	if (!threadgroup)
		throw_exception_exit();

	/* create a Thread */

	mainthread = (java_lang_Thread *) builtin_new(class_java_lang_Thread);

	if (!mainthread)
		throw_exception_exit();

	mainthreadobj->o.thread = mainthread;

	/* call Thread.<init>(Ljava/lang/VMThread;Ljava/lang/String;IZ)V */

	method = class_resolveclassmethod(class_java_lang_Thread,
									  utf_init,
									  utf_new_char("(Ljava/lang/VMThread;Ljava/lang/String;IZ)V"),
									  class_java_lang_Thread,
									  true);

	if (!method)
		return false;

	(void) vm_call_method(method, (java_objectheader *) mainthread,
						  mainthreadobj, threadname, 5, false);

	if (*exceptionptr)
		return false;

	mainthread->group = threadgroup;

	/* add mainthread to ThreadGroup */

	method = class_resolveclassmethod(class_java_lang_ThreadGroup,
									  utf_new_char("addThread"),
									  utf_new_char("(Ljava/lang/Thread;)V"),
									  class_java_lang_ThreadGroup,
									  true);

	if (!method)
		return false;

	(void) vm_call_method(method, (java_objectheader *) threadgroup,
						  mainthread);

	if (*exceptionptr)
		return false;

	threads_set_thread_priority(pthread_self(), 5);

	pthread_attr_init(&threadattr);
	pthread_attr_setdetachstate(&threadattr, PTHREAD_CREATE_DETACHED);

	/* everything's ok */

	return true;
}


/* threads_init_threadobject **************************************************

   Initialize implementation fields of a java.lang.VMThread.

   IN:
      t............the java.lang.VMThread

******************************************************************************/

void threads_init_threadobject(java_lang_VMThread *t)
{
	threadobject *thread = (threadobject*) t;
	nativethread *info = &thread->info;
	info->tid = pthread_self();
	/* TODO destroy all those things */
	pthread_mutex_init(&info->joinMutex, NULL);
	pthread_cond_init(&info->joinCond, NULL);

	pthread_mutex_init(&thread->waitLock, NULL);
	pthread_cond_init(&thread->waitCond, NULL);
	thread->interrupted = false;
	thread->signaled = false;
	thread->isSleeping = false;
}


/* threads_startup_thread ******************************************************

   Thread startup function called by pthread_create.

   NOTE: This function is not called directly by pthread_create. The Boehm GC
         inserts its own GC_start_routine in between, which then calls
		 threads_startup.

   IN:
      t............the argument passed to pthread_create, ie. a pointer to
	               a startupinfo struct. CAUTION: When the `psem` semaphore
				   is posted, the startupinfo struct becomes invalid! (It
				   is allocated on the stack of threads_start_thread.)

******************************************************************************/

static void *threads_startup_thread(void *t)
{
	startupinfo  *startup;
	threadobject *thread;
	sem_t        *psem;
	nativethread *info;
	threadobject *tnext;
	methodinfo   *method;
	functionptr   function;

#if defined(ENABLE_INTRP)
	u1 *intrp_thread_stack;

	/* create interpreter stack */

	if (opt_intrp) {
		intrp_thread_stack = (u1 *) alloca(opt_stacksize);
		MSET(intrp_thread_stack, 0, u1, opt_stacksize);
	} else {
		intrp_thread_stack = NULL;
	}
#endif

	/* get passed startupinfo structure and the values in there */

	startup = t;
	t = NULL; /* make sure it's not used wrongly */

	thread   = startup->thread;
	function = startup->function;
	psem     = startup->psem;

	info = &thread->info;

	/* Seems like we've encountered a situation where info->tid was not set by
	 * pthread_create. We alleviate this problem by waiting for pthread_create
	 * to return. */
	threads_sem_wait(startup->psem_first);

	/* set the thread object */

#if defined(__DARWIN__)
	info->mach_thread = mach_thread_self();
#endif
	threads_set_current_threadobject(thread);

	/* insert the thread into the threadlist */

	pthread_mutex_lock(&threadlistlock);

	info->prev = mainthreadobj;
	info->next = tnext = mainthreadobj->info.next;
	mainthreadobj->info.next = thread;
	tnext->info.prev = thread;

	pthread_mutex_unlock(&threadlistlock);

	/* init data structures of this thread */

	lock_init_execution_env(thread);

	/* tell threads_startup_thread that we registered ourselves */
	/* CAUTION: *startup becomes invalid with this!             */

	startup = NULL;
	threads_sem_post(psem);

	/* set our priority */

	threads_set_thread_priority(info->tid, thread->o.thread->priority);

#if defined(ENABLE_INTRP)
	/* set interpreter stack */

	if (opt_intrp)
		THREADINFO->_global_sp = (void *) (intrp_thread_stack + opt_stacksize);
#endif

	/* find and run the Thread.run()V method if no other function was passed */

	if (function == NULL) {
		method = class_resolveclassmethod(thread->o.header.vftbl->class,
										  utf_run,
										  utf_void__void,
										  thread->o.header.vftbl->class,
										  true);

		if (!method)
			throw_exception();

		(void) vm_call_method(method, (java_objectheader *) thread);

	}
	else {
		/* call passed function, e.g. finalizer_thread */

		(function)();
	}

	/* Allow lock record pools to be used by other threads. They
	   cannot be deleted so we'd better not waste them. */

	lock_record_free_pools(thread->ee.lrpool);

	/* remove thread from thread list, do this inside a lock */

	pthread_mutex_lock(&threadlistlock);
	info->next->info.prev = info->prev;
	info->prev->info.next = info->next;
	pthread_mutex_unlock(&threadlistlock);

	/* reset thread id (lock on joinMutex? TWISTI) */

	pthread_mutex_lock(&info->joinMutex);
	info->tid = 0;
	pthread_mutex_unlock(&info->joinMutex);

	/* tell everyone that a thread has finished */

	pthread_cond_broadcast(&info->joinCond);

	return NULL;
}


/* threads_start_thread ********************************************************

   Start a thread in the JVM.

   IN:
      t............the java.lang.Thread object
	  function.....function to run in the new thread. NULL means that the
	               "run" method of the object `t` should be called

******************************************************************************/

void threads_start_thread(java_lang_Thread *t, functionptr function)
{
	nativethread *info;
	sem_t         sem;
	sem_t         sem_first;
	startupinfo   startup;

	info = &((threadobject *) t->vmThread)->info;

	/* fill startupinfo structure passed by pthread_create to
	 * threads_startup_thread */

	startup.thread     = (threadobject*) t->vmThread;
	startup.function   = function;       /* maybe we don't call Thread.run()V */
	startup.psem       = &sem;
	startup.psem_first = &sem_first;

	threads_sem_init(&sem, 0, 0);
	threads_sem_init(&sem_first, 0, 0);

	/* create the thread */

	if (pthread_create(&info->tid, &threadattr, threads_startup_thread,
					   &startup)) {
		log_text("pthread_create failed");
		assert(0);
	}

	/* signal that pthread_create has returned, so info->tid is valid */

	threads_sem_post(&sem_first);

	/* wait here until the thread has entered itself into the thread list */

	threads_sem_wait(&sem);

	/* cleanup */

	sem_destroy(&sem);
	sem_destroy(&sem_first);
}


/* threads_find_non_daemon_thread **********************************************

   Helper function used by threads_join_all_threads for finding non-daemon threads
   that are still running.

*******************************************************************************/

/* At the end of the program, we wait for all running non-daemon threads to die
 */

static threadobject *threads_find_non_daemon_thread(threadobject *thread)
{
	while (thread != mainthreadobj) {
		if (!thread->o.thread->daemon)
			return thread;
		thread = thread->info.prev;
	}

	return NULL;
}


/* threads_join_all_threads ****************************************************

   Join all non-daemon threads.

*******************************************************************************/

void threads_join_all_threads(void)
{
	threadobject *thread;
	pthread_mutex_lock(&threadlistlock);
	while ((thread = threads_find_non_daemon_thread(mainthreadobj->info.prev)) != NULL) {
		nativethread *info = &thread->info;
		pthread_mutex_lock(&info->joinMutex);
		pthread_mutex_unlock(&threadlistlock);
		while (info->tid)
			pthread_cond_wait(&info->joinCond, &info->joinMutex);
		pthread_mutex_unlock(&info->joinMutex);
		pthread_mutex_lock(&threadlistlock);
	}
	pthread_mutex_unlock(&threadlistlock);
}


/* threads_timespec_earlier ****************************************************

   Return true if timespec tv1 is earlier than timespec tv2.

   IN:
      tv1..........first timespec
	  tv2..........second timespec

   RETURN VALUE:
      true, if the first timespec is earlier

*******************************************************************************/

static inline bool threads_timespec_earlier(const struct timespec *tv1,
											const struct timespec *tv2)
{
	return (tv1->tv_sec < tv2->tv_sec)
				||
		(tv1->tv_sec == tv2->tv_sec && tv1->tv_nsec < tv2->tv_nsec);
}


/* threads_current_time_is_earlier_than ****************************************

   Check if the current time is earlier than the given timespec.

   IN:
      tv...........the timespec to compare against

   RETURN VALUE:
      true, if the current time is earlier

*******************************************************************************/

static bool threads_current_time_is_earlier_than(const struct timespec *tv)
{
	struct timeval tvnow;
	struct timespec tsnow;

	/* get current time */

	if (gettimeofday(&tvnow, NULL) != 0) {
		fprintf(stderr,"error: gettimeofday returned unexpected error %d: %s\n",
				errno, strerror(errno));
		abort();
	}

	/* convert it to a timespec */

	tsnow.tv_sec = tvnow.tv_sec;
	tsnow.tv_nsec = tvnow.tv_usec * 1000;

	/* compare current time with the given timespec */

	return threads_timespec_earlier(&tsnow, tv);
}


/* threads_wait_with_timeout ***************************************************

   Wait until the given point in time on a monitor until either
   we are notified, we are interrupted, or the time is up.

   IN:
      t............the current thread
	  wakeupTime...absolute (latest) wakeup time
	                   If both tv_sec and tv_nsec are zero, this function
					   waits for an unlimited amount of time.

   RETURN VALUE:
      true.........if the wait has been interrupted,
	  false........if the wait was ended by notification or timeout

*******************************************************************************/

static bool threads_wait_with_timeout(threadobject *t,
									  struct timespec *wakeupTime)
{
	bool wasinterrupted;

	/* acquire the waitLock */

	pthread_mutex_lock(&t->waitLock);

	/* mark us as sleeping */

	t->isSleeping = true;

	/* wait on waitCond */

	if (wakeupTime->tv_sec || wakeupTime->tv_nsec) {
		/* with timeout */
		while (!t->interrupted && !t->signaled
			   && threads_current_time_is_earlier_than(wakeupTime))
		{
			pthread_cond_timedwait(&t->waitCond, &t->waitLock, wakeupTime);
		}
	}
	else {
		/* no timeout */
		while (!t->interrupted && !t->signaled)
			pthread_cond_wait(&t->waitCond, &t->waitLock);
	}

	/* check if we were interrupted */

	wasinterrupted = t->interrupted;

	/* reset all flags */

	t->interrupted = false;
	t->signaled = false;
	t->isSleeping = false;

	/* release the waitLock */

	pthread_mutex_unlock(&t->waitLock);

	return wasinterrupted;
}


/* threads_wait_with_timeout_relative ******************************************

   Wait for the given maximum amount of time on a monitor until either
   we are notified, we are interrupted, or the time is up.

   IN:
      t............the current thread
	  millis.......milliseconds to wait
	  nanos........nanoseconds to wait

   RETURN VALUE:
      true.........if the wait has been interrupted,
	  false........if the wait was ended by notification or timeout

*******************************************************************************/

bool threads_wait_with_timeout_relative(threadobject *t,
										s8 millis,
										s4 nanos)
{
	struct timespec wakeupTime;

	/* calculate the the (latest) wakeup time */

	threads_calc_absolute_time(&wakeupTime, millis, nanos);

	/* wait */

	return threads_wait_with_timeout(t, &wakeupTime);
}


/* threads_calc_absolute_time **************************************************

   Calculate the absolute point in time a given number of ms and ns from now.

   IN:
      millis............milliseconds from now
	  nanos.............nanoseconds from now

   OUT:
      *tm...............receives the timespec of the absolute point in time

*******************************************************************************/

static void threads_calc_absolute_time(struct timespec *tm, s8 millis, s4 nanos)
{
	if ((millis != 0x7fffffffffffffffLLU) && (millis || nanos)) {
		struct timeval tv;
		long nsec;
		gettimeofday(&tv, NULL);
		tv.tv_sec += millis / 1000;
		millis %= 1000;
		nsec = tv.tv_usec * 1000 + (s4) millis * 1000000 + nanos;
		tm->tv_sec = tv.tv_sec + nsec / 1000000000;
		tm->tv_nsec = nsec % 1000000000;
	}
	else {
		tm->tv_sec = 0;
		tm->tv_nsec = 0;
	}
}


/* threads_interrupt_thread ****************************************************

   Interrupt the given thread.

   The thread gets the "waitCond" signal and 
   its interrupted flag is set to true.

   IN:
      thread............the thread to interrupt

*******************************************************************************/

void threads_interrupt_thread(java_lang_VMThread *thread)
{
	threadobject *t = (threadobject*) thread;

	/* signal the thread a "waitCond" and tell it that it has been */
	/* interrupted                                                 */

	pthread_mutex_lock(&t->waitLock);
	if (t->isSleeping)
		pthread_cond_signal(&t->waitCond);
	t->interrupted = true;
	pthread_mutex_unlock(&t->waitLock);
}


/* threads_check_if_interrupted_and_reset **************************************

   Check if the current thread has been interrupted and reset the
   interruption flag.

   RETURN VALUE:
      true, if the current thread had been interrupted

*******************************************************************************/

bool threads_check_if_interrupted_and_reset(void)
{
	threadobject *t;
	bool intr;

	t = (threadobject*) THREADOBJECT;

	intr = t->interrupted;

	t->interrupted = false;

	return intr;
}


/* threads_thread_has_been_interrupted *********************************************************

   Check if the given thread has been interrupted

   IN:
      t............the thread to check

   RETURN VALUE:
      true, if the given thread had been interrupted

*******************************************************************************/

bool threads_thread_has_been_interrupted(java_lang_VMThread *thread)
{
	threadobject *t;

	t = (threadobject*) thread;

	return t->interrupted;
}


/* threads_sleep ***************************************************************

   Sleep the current thread for the specified amount of time.

*******************************************************************************/

void threads_sleep(s8 millis, s4 nanos)
{
	threadobject       *t;
	struct timespec    wakeupTime;
	bool               wasinterrupted;

	t = (threadobject *) THREADOBJECT;

	threads_calc_absolute_time(&wakeupTime, millis, nanos);

	wasinterrupted = threads_wait_with_timeout(t, &wakeupTime);

	if (wasinterrupted)
		*exceptionptr = new_exception(string_java_lang_InterruptedException);
}


/* threads_yield *****************************************************************

   Yield to the scheduler.

*******************************************************************************/

void threads_yield(void)
{
	sched_yield();
}


/* threads_java_lang_Thread_set_priority ***********************************************************

   Set the priority for the given java.lang.Thread.

   IN:
      t............the java.lang.Thread
	  priority.....the priority

*******************************************************************************/

void threads_java_lang_Thread_set_priority(java_lang_Thread *t, s4 priority)
{
	nativethread *info = &((threadobject*) t->vmThread)->info;
	threads_set_thread_priority(info->tid, priority);
}


/* threads_dump ****************************************************************

   Dumps info for all threads running in the JVM. This function is
   called when SIGQUIT (<ctrl>-\) is sent to CACAO.

*******************************************************************************/

void threads_dump(void)
{
	threadobject       *tobj;
	java_lang_VMThread *vmt;
	nativethread       *nt;
	java_lang_Thread   *t;
	utf                *name;

	tobj = mainthreadobj;

	printf("Full thread dump CACAO "VERSION":\n");

	/* iterate over all started threads */

	do {
		/* get thread objects */

		vmt = &tobj->o;
		nt  = &tobj->info;
		t   = vmt->thread;

		/* the thread may be currently in initalization, don't print it */

		if (t) {
			/* get thread name */

			name = javastring_toutf(t->name, false);

			printf("\n\"");
			utf_display_printable_ascii(name);
			printf("\" ");

			if (t->daemon)
				printf("daemon ");

#if SIZEOF_VOID_P == 8
			printf("prio=%d tid=0x%016lx\n", t->priority, nt->tid);
#else
			printf("prio=%d tid=0x%08lx\n", t->priority, nt->tid);
#endif

			/* send SIGUSR1 to thread to print stacktrace */

			pthread_kill(nt->tid, SIGUSR1);

			/* sleep this thread a bit, so the signal can reach the thread */

			threads_sleep(10, 0);
		}

		tobj = tobj->info.next;
	} while (tobj && (tobj != mainthreadobj));
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
