/* src/threads/native/threads.c - native threads support

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

   $Id: threads.c 8284 2007-08-10 08:58:39Z michi $

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

#include "vm/types.h"

#include "arch.h"

#if !defined(USE_FAKE_ATOMIC_INSTRUCTIONS)
# include "machine-instr.h"
#else
# include "threads/native/generic-primitives.h"
#endif

#include "mm/gc-common.h"
#include "mm/memory.h"

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"
#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_String.h"
#include "native/include/java_lang_Throwable.h"
#include "native/include/java_lang_Thread.h"

#if defined(ENABLE_JAVASE)
# include "native/include/java_lang_ThreadGroup.h"
#endif

#if defined(WITH_CLASSPATH_GNU)
# include "native/include/java_lang_VMThread.h"
#endif

#include "threads/lock-common.h"
#include "threads/threads-common.h"

#include "threads/native/threads.h"

#include "toolbox/logging.h"

#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"

#include "vm/jit/asmpart.h"

#include "vmcore/options.h"

#if defined(ENABLE_STATISTICS)
# include "vmcore/statistics.h"
#endif

#if !defined(__DARWIN__)
# if defined(__LINUX__)
#  define GC_LINUX_THREADS
# elif defined(__MIPS__)
#  define GC_IRIX_THREADS
# endif
# include <semaphore.h>
# if defined(ENABLE_GC_BOEHM)
#  include "mm/boehm-gc/include/gc.h"
# endif
#endif

#if defined(ENABLE_JVMTI)
#include "native/jvmti/cacaodbg.h"
#endif

#if defined(__DARWIN__)
/* Darwin has no working semaphore implementation.  This one is taken
   from Boehm-GC. */

/*
   This is a very simple semaphore implementation for darwin. It
   is implemented in terms of pthreads calls so it isn't async signal
   safe. This isn't a problem because signals aren't used to
   suspend threads on darwin.
*/
   
static int sem_init(sem_t *sem, int pshared, int value)
{
	if (pshared)
		assert(0);

	sem->value = value;
    
	if (pthread_mutex_init(&sem->mutex, NULL) < 0)
		return -1;

	if (pthread_cond_init(&sem->cond, NULL) < 0)
		return -1;

	return 0;
}

static int sem_post(sem_t *sem)
{
	if (pthread_mutex_lock(&sem->mutex) < 0)
		return -1;

	sem->value++;

	if (pthread_cond_signal(&sem->cond) < 0) {
		pthread_mutex_unlock(&sem->mutex);
		return -1;
	}

	if (pthread_mutex_unlock(&sem->mutex) < 0)
		return -1;

	return 0;
}

static int sem_wait(sem_t *sem)
{
	if (pthread_mutex_lock(&sem->mutex) < 0)
		return -1;

	while (sem->value == 0) {
		pthread_cond_wait(&sem->cond, &sem->mutex);
	}

	sem->value--;

	if (pthread_mutex_unlock(&sem->mutex) < 0)
		return -1;    

	return 0;
}

static int sem_destroy(sem_t *sem)
{
	if (pthread_cond_destroy(&sem->cond) < 0)
		return -1;

	if (pthread_mutex_destroy(&sem->mutex) < 0)
		return -1;

	return 0;
}
#endif /* defined(__DARWIN__) */


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

static methodinfo *method_thread_init;

/* the thread object of the current thread                                    */
/* This is either a thread-local variable defined with __thread, or           */
/* a thread-specific value stored with key threads_current_threadobject_key.  */
#if defined(HAVE___THREAD)
__thread threadobject *threads_current_threadobject;
#else
pthread_key_t threads_current_threadobject_key;
#endif

/* global mutex for the threads table */
static pthread_mutex_t mutex_threads_list;

/* global mutex for stop-the-world                                            */
static pthread_mutex_t stopworldlock;

/* global mutex and condition for joining threads on exit */
static pthread_mutex_t mutex_join;
static pthread_cond_t  cond_join;

/* XXX We disable that whole bunch of code until we have the exact-GC
   running. */

#if 0

/* this is one of the STOPWORLD_FROM_ constants, telling why the world is     */
/* being stopped                                                              */
static volatile int stopworldwhere;

/* semaphore used for acknowleding thread suspension                          */
static sem_t suspend_ack;
#if defined(__MIPS__)
static pthread_mutex_t suspend_ack_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t suspend_cond = PTHREAD_COND_INITIALIZER;
#endif

#endif /* 0 */

/* mutexes used by the fake atomic instructions                               */
#if defined(USE_FAKE_ATOMIC_INSTRUCTIONS)
pthread_mutex_t _atomic_add_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t _cas_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t _mb_lock = PTHREAD_MUTEX_INITIALIZER;
#endif


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

	vm_abort("sem_init failed: %s", strerror(errno));
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

	vm_abort("sem_wait failed: %s", strerror(errno));
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

	vm_abort("sem_post failed: %s", strerror(errno));
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
/* 	stopworldwhere = where; */
}


/* unlock_stopworld ************************************************************

   Release the stopworld lock.

******************************************************************************/

void unlock_stopworld(void)
{
/* 	stopworldwhere = 0; */
	pthread_mutex_unlock(&stopworldlock);
}

/* XXX We disable that whole bunch of code until we have the exact-GC
   running. */

#if 0

#if !defined(__DARWIN__)
/* Caller must hold threadlistlock */
static s4 threads_cast_sendsignals(s4 sig)
{
	threadobject *t;
	threadobject *self;
	s4            count;

	self = THREADOBJECT;

	/* iterate over all started threads */

	count = 0;

	for (t = threads_list_first(); t != NULL; t = threads_list_next(t)) {
		/* don't send the signal to ourself */

		if (t == self)
			continue;

		/* don't send the signal to NEW threads (because they are not
		   completely initialized) */

		if (t->state == THREAD_STATE_NEW)
			continue;

		/* send the signal */

		pthread_kill(t->tid, sig);

		/* increase threads count */

		count++;
	}

	return count;
}

#else

static void threads_cast_darwinstop(void)
{
	threadobject *tobj = mainthreadobj;
	threadobject *self = THREADOBJECT;

	do {
		if (tobj != self)
		{
			thread_state_flavor_t flavor = MACHINE_THREAD_STATE;
			mach_msg_type_number_t thread_state_count = MACHINE_THREAD_STATE_COUNT;
#if defined(__I386__)
			i386_thread_state_t thread_state;
#else
			ppc_thread_state_t thread_state;
#endif
			mach_port_t thread = tobj->mach_thread;
			kern_return_t r;

			r = thread_suspend(thread);

			if (r != KERN_SUCCESS)
				vm_abort("thread_suspend failed");

			r = thread_get_state(thread, flavor, (natural_t *) &thread_state,
								 &thread_state_count);

			if (r != KERN_SUCCESS)
				vm_abort("thread_get_state failed");

			md_critical_section_restart((ucontext_t *) &thread_state);

			r = thread_set_state(thread, flavor, (natural_t *) &thread_state,
								 thread_state_count);

			if (r != KERN_SUCCESS)
				vm_abort("thread_set_state failed");
		}

		tobj = tobj->next;
	} while (tobj != mainthreadobj);
}

static void threads_cast_darwinresume(void)
{
	threadobject *tobj = mainthreadobj;
	threadobject *self = THREADOBJECT;

	do {
		if (tobj != self)
		{
			mach_port_t thread = tobj->mach_thread;
			kern_return_t r;

			r = thread_resume(thread);

			if (r != KERN_SUCCESS)
				vm_abort("thread_resume failed");
		}

		tobj = tobj->next;
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

#if !defined(DISABLE_GC)

void threads_cast_stopworld(void)
{
#if !defined(__DARWIN__) && !defined(__CYGWIN__)
	s4 count, i;
#endif

	lock_stopworld(STOPWORLD_FROM_CLASS_NUMBERING);

	/* lock the threads lists */

	threads_list_lock();

#if defined(__DARWIN__)
	threads_cast_darwinstop();
#elif defined(__CYGWIN__)
	/* TODO */
	assert(0);
#else
	/* send all threads the suspend signal */

	count = threads_cast_sendsignals(GC_signum1());

	/* wait for all threads signaled to suspend */

	for (i = 0; i < count; i++)
		threads_sem_wait(&suspend_ack);
#endif

	/* ATTENTION: Don't unlock the threads-lists here so that
	   non-signaled NEW threads can't change their state and execute
	   code. */
}


void threads_cast_startworld(void)
{
#if defined(__DARWIN__)
	threads_cast_darwinresume();
#elif defined(__MIPS__)
	threads_cast_irixresume();
#elif defined(__CYGWIN__)
	/* TODO */
	assert(0);
#else
	(void) threads_cast_sendsignals(GC_signum2());
#endif

	/* unlock the threads lists */

	threads_list_unlock();

	unlock_stopworld();
}


#if !defined(__DARWIN__)
static void threads_sigsuspend_handler(ucontext_t *_uc)
{
	int sig;
	sigset_t sigs;

	/* XXX TWISTI: this is just a quick hack */
#if defined(ENABLE_JIT)
	md_critical_section_restart(_uc);
#endif

	/* Do as Boehm does. On IRIX a condition variable is used for wake-up
	   (not POSIX async-safe). */
#if defined(__IRIX__)
	pthread_mutex_lock(&suspend_ack_lock);
	threads_sem_post(&suspend_ack);
	pthread_cond_wait(&suspend_cond, &suspend_ack_lock);
	pthread_mutex_unlock(&suspend_ack_lock);
#elif defined(__CYGWIN__)
	/* TODO */
	assert(0);
#else
	threads_sem_post(&suspend_ack);

	sig = GC_signum2();
	sigfillset(&sigs);
	sigdelset(&sigs, sig);
	sigsuspend(&sigs);
#endif
}

#endif


/* This function is called from Boehm GC code. */

int cacao_suspendhandler(ucontext_t *_uc)
{
	if (stopworldwhere != STOPWORLD_FROM_CLASS_NUMBERING)
		return 0;

	threads_sigsuspend_handler(_uc);
	return 1;
}

#endif /* DISABLE_GC */

#endif /* 0 */


/* threads_set_current_threadobject ********************************************

   Set the current thread object.
   
   IN:
      thread.......the thread object to set

*******************************************************************************/

void threads_set_current_threadobject(threadobject *thread)
{
#if !defined(HAVE___THREAD)
	if (pthread_setspecific(threads_current_threadobject_key, thread) != 0)
		vm_abort("threads_set_current_threadobject: pthread_setspecific failed: %s", strerror(errno));
#else
	threads_current_threadobject = thread;
#endif
}


/* threads_impl_thread_new *****************************************************

   Initialize implementation fields of a threadobject.

   IN:
      t....the threadobject

*******************************************************************************/

void threads_impl_thread_new(threadobject *t)
{
	/* get the pthread id */

	t->tid = pthread_self();

	/* initialize the mutex and the condition */

	pthread_mutex_init(&(t->waitmutex), NULL);
	pthread_cond_init(&(t->waitcond), NULL);

#if defined(ENABLE_DEBUG_FILTER)
	/* Initialize filter counters */
	t->filterverbosecallctr[0] = 0;
	t->filterverbosecallctr[1] = 0;
#endif
}


/* threads_impl_thread_free ****************************************************

   Cleanup thread stuff.

   IN:
      t....the threadobject

*******************************************************************************/

void threads_impl_thread_free(threadobject *t)
{
	/* destroy the mutex and the condition */

	if (pthread_mutex_destroy(&(t->waitmutex)) != 0)
		vm_abort("threads_impl_thread_free: pthread_mutex_destroy failed: %s",
				 strerror(errno));

	if (pthread_cond_destroy(&(t->waitcond)) != 0)
		vm_abort("threads_impl_thread_free: pthread_cond_destroy failed: %s",
				 strerror(errno));
}


/* threads_get_current_threadobject ********************************************

   Return the threadobject of the current thread.
   
   RETURN VALUE:
       the current threadobject * (an instance of java.lang.Thread)

*******************************************************************************/

threadobject *threads_get_current_threadobject(void)
{
	return THREADOBJECT;
}


/* threads_impl_preinit ********************************************************

   Do some early initialization of stuff required.

   ATTENTION: Do NOT use any Java heap allocation here, as gc_init()
   is called AFTER this function!

*******************************************************************************/

void threads_impl_preinit(void)
{
	pthread_mutex_init(&stopworldlock, NULL);

	/* initialize exit mutex and condition (on exit we join all
	   threads) */

	pthread_mutex_init(&mutex_join, NULL);
	pthread_cond_init(&cond_join, NULL);

	/* initialize the threads-list mutex */

	pthread_mutex_init(&mutex_threads_list, NULL);

#if !defined(HAVE___THREAD)
	pthread_key_create(&threads_current_threadobject_key, NULL);
#endif

/* 	threads_sem_init(&suspend_ack, 0, 0); */
}


/* threads_list_lock ***********************************************************

   Enter the threads table mutex.

   NOTE: We need this function as we can't use an internal lock for
         the threads lists because the thread's lock is initialized in
         threads_table_add (when we have the thread index), but we
         already need the lock at the entry of the function.

*******************************************************************************/

void threads_list_lock(void)
{
	if (pthread_mutex_lock(&mutex_threads_list) != 0)
		vm_abort("threads_list_lock: pthread_mutex_lock failed: %s",
				 strerror(errno));
}


/* threads_list_unlock *********************************************************

   Leave the threads list mutex.

*******************************************************************************/

void threads_list_unlock(void)
{
	if (pthread_mutex_unlock(&mutex_threads_list) != 0)
		vm_abort("threads_list_unlock: pthread_mutex_unlock failed: %s",
				 strerror(errno));
}


/* threads_mutex_join_lock *****************************************************

   Enter the join mutex.

*******************************************************************************/

void threads_mutex_join_lock(void)
{
	if (pthread_mutex_lock(&mutex_join) != 0)
		vm_abort("threads_mutex_join_lock: pthread_mutex_lock failed: %s",
				 strerror(errno));
}


/* threads_mutex_join_unlock ***************************************************

   Leave the join mutex.

*******************************************************************************/

void threads_mutex_join_unlock(void)
{
	if (pthread_mutex_unlock(&mutex_join) != 0)
		vm_abort("threads_mutex_join_unlock: pthread_mutex_unlock failed: %s",
				 strerror(errno));
}


/* threads_init ****************************************************************

   Initializes the threads required by the JVM: main, finalizer.

*******************************************************************************/

bool threads_init(void)
{
	threadobject          *mainthread;
	java_objectheader     *threadname;
	java_lang_Thread      *t;
	java_objectheader     *o;

#if defined(ENABLE_JAVASE)
	java_lang_ThreadGroup *threadgroup;
	methodinfo            *m;
#endif

#if defined(WITH_CLASSPATH_GNU)
	java_lang_VMThread    *vmt;
#endif

	pthread_attr_t attr;

	/* get methods we need in this file */

#if defined(WITH_CLASSPATH_GNU)
	method_thread_init =
		class_resolveclassmethod(class_java_lang_Thread,
								 utf_init,
								 utf_new_char("(Ljava/lang/VMThread;Ljava/lang/String;IZ)V"),
								 class_java_lang_Thread,
								 true);
#else
	method_thread_init =
		class_resolveclassmethod(class_java_lang_Thread,
								 utf_init,
								 utf_new_char("(Ljava/lang/String;)V"),
								 class_java_lang_Thread,
								 true);
#endif

	if (method_thread_init == NULL)
		return false;

	/* Get the main-thread (NOTE: The main threads is always the first
	   thread in the list). */

	mainthread = threads_list_first();

	/* create a java.lang.Thread for the main thread */

	t = (java_lang_Thread *) builtin_new(class_java_lang_Thread);

	if (t == NULL)
		return false;

	/* set the object in the internal data structure */

	mainthread->object = t;

#if defined(ENABLE_INTRP)
	/* create interpreter stack */

	if (opt_intrp) {
		MSET(intrp_main_stack, 0, u1, opt_stacksize);
		mainthread->_global_sp = (Cell*) (intrp_main_stack + opt_stacksize);
	}
#endif

	threadname = javastring_new(utf_new_char("main"));

#if defined(ENABLE_JAVASE)
	/* allocate and init ThreadGroup */

	threadgroup = (java_lang_ThreadGroup *)
		native_new_and_init(class_java_lang_ThreadGroup);

	if (threadgroup == NULL)
		return false;
#endif

#if defined(WITH_CLASSPATH_GNU)
	/* create a java.lang.VMThread for the main thread */

	vmt = (java_lang_VMThread *) builtin_new(class_java_lang_VMThread);

	if (vmt == NULL)
		return false;

	/* set the thread */

	LLNI_field_set_ref(vmt, thread, t);
	LLNI_field_set_val(vmt, vmdata, (java_lang_Object *) mainthread);

	/* call java.lang.Thread.<init>(Ljava/lang/VMThread;Ljava/lang/String;IZ)V */
	o = (java_objectheader *) t;

	(void) vm_call_method(method_thread_init, o, vmt, threadname, NORM_PRIORITY,
						  false);

#elif defined(WITH_CLASSPATH_SUN)

	/* We trick java.lang.Thread.init, which sets the priority of the
	   current thread to the parent's one. */

	t->priority = NORM_PRIORITY;

#elif defined(WITH_CLASSPATH_CLDC1_1)

	/* set the thread */

	t->vm_thread = (java_lang_Object *) mainthread;

	/* call public Thread(String name) */

	o = (java_objectheader *) t;

	(void) vm_call_method(method_thread_init, o, threadname);
#else
# error unknown classpath configuration
#endif

	if (exceptions_get_exception())
		return false;

#if defined(ENABLE_JAVASE)
	LLNI_field_set_ref(t, group, threadgroup);

# if defined(WITH_CLASSPATH_GNU)
	/* add main thread to java.lang.ThreadGroup */

	m = class_resolveclassmethod(class_java_lang_ThreadGroup,
								 utf_addThread,
								 utf_java_lang_Thread__V,
								 class_java_lang_ThreadGroup,
								 true);

	o = (java_objectheader *) threadgroup;

	(void) vm_call_method(m, o, t);

	if (exceptions_get_exception())
		return false;
# else
#  warning Do not know what to do here
# endif
#endif

	threads_set_thread_priority(pthread_self(), NORM_PRIORITY);

	/* initialize the thread attribute object */

	if (pthread_attr_init(&attr) != 0)
		vm_abort("threads_init: pthread_attr_init failed: %s", strerror(errno));

	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0)
		vm_abort("threads_init: pthread_attr_setdetachstate failed: %s",
				 strerror(errno));

#if !defined(NDEBUG)
	if (opt_verbosethreads) {
		printf("[Starting thread ");
		threads_thread_print_info(mainthread);
		printf("]\n");
	}
#endif

	/* everything's ok */

	return true;
}


/* threads_startup_thread ******************************************************

   Thread startup function called by pthread_create.

   Thread which have a startup.function != NULL are marked as internal
   threads. All other threads are threated as normal Java threads.

   NOTE: This function is not called directly by pthread_create. The Boehm GC
         inserts its own GC_start_routine in between, which then calls
		 threads_startup.

   IN:
      arg..........the argument passed to pthread_create, ie. a pointer to
	               a startupinfo struct. CAUTION: When the `psem` semaphore
				   is posted, the startupinfo struct becomes invalid! (It
				   is allocated on the stack of threads_start_thread.)

******************************************************************************/

static void *threads_startup_thread(void *arg)
{
	startupinfo        *startup;
	threadobject       *thread;
#if defined(WITH_CLASSPATH_GNU)
	java_lang_VMThread *vmt;
#endif
	sem_t              *psem;
	classinfo          *c;
	methodinfo         *m;
	java_objectheader  *o;
	functionptr         function;

#if defined(ENABLE_INTRP)
	u1 *intrp_thread_stack;

	/* create interpreter stack */

	if (opt_intrp) {
		intrp_thread_stack = GCMNEW(u1, opt_stacksize);
		MSET(intrp_thread_stack, 0, u1, opt_stacksize);
	}
	else
		intrp_thread_stack = NULL;
#endif

	/* get passed startupinfo structure and the values in there */

	startup = arg;

	thread   = startup->thread;
	function = startup->function;
	psem     = startup->psem;

	/* Seems like we've encountered a situation where thread->tid was
	   not set by pthread_create. We alleviate this problem by waiting
	   for pthread_create to return. */

	threads_sem_wait(startup->psem_first);

#if defined(__DARWIN__)
	thread->mach_thread = mach_thread_self();
#endif

	/* store the internal thread data-structure in the TSD */

	threads_set_current_threadobject(thread);

	/* set our priority */

	threads_set_thread_priority(thread->tid, LLNI_field_direct(thread->object, priority));

	/* thread is completely initialized */

	threads_thread_state_runnable(thread);

	/* tell threads_startup_thread that we registered ourselves */
	/* CAUTION: *startup becomes invalid with this!             */

	startup = NULL;
	threads_sem_post(psem);

#if defined(ENABLE_INTRP)
	/* set interpreter stack */

	if (opt_intrp)
		thread->_global_sp = (Cell *) (intrp_thread_stack + opt_stacksize);
#endif

#if defined(ENABLE_JVMTI)
	/* fire thread start event */

	if (jvmti) 
		jvmti_ThreadStartEnd(JVMTI_EVENT_THREAD_START);
#endif

#if !defined(NDEBUG)
	if (opt_verbosethreads) {
		printf("[Starting thread ");
		threads_thread_print_info(thread);
		printf("]\n");
	}
#endif

	/* find and run the Thread.run()V method if no other function was passed */

	if (function == NULL) {
#if defined(WITH_CLASSPATH_GNU)
		/* We need to start the run method of
		   java.lang.VMThread. Since this is a final class, we can use
		   the class object directly. */

		c = class_java_lang_VMThread;
#elif defined(WITH_CLASSPATH_SUN) || defined(WITH_CLASSPATH_CLDC1_1)
		c = thread->object->header.vftbl->class;
#else
# error unknown classpath configuration
#endif

		m = class_resolveclassmethod(c, utf_run, utf_void__void, c, true);

		if (m == NULL)
			vm_abort("threads_startup_thread: run() method not found in class");

		/* set ThreadMXBean variables */

		_Jv_jvm->java_lang_management_ThreadMXBean_ThreadCount++;
		_Jv_jvm->java_lang_management_ThreadMXBean_TotalStartedThreadCount++;

		if (_Jv_jvm->java_lang_management_ThreadMXBean_ThreadCount >
			_Jv_jvm->java_lang_management_ThreadMXBean_PeakThreadCount)
			_Jv_jvm->java_lang_management_ThreadMXBean_PeakThreadCount =
				_Jv_jvm->java_lang_management_ThreadMXBean_ThreadCount;

#if defined(WITH_CLASSPATH_GNU)
		/* we need to start the run method of java.lang.VMThread */

		vmt = (java_lang_VMThread *) LLNI_field_direct(thread->object, vmThread);
		o   = (java_objectheader *) vmt;

#elif defined(WITH_CLASSPATH_SUN) || defined(WITH_CLASSPATH_CLDC1_1)
		o   = (java_objectheader *) thread->object;
#else
# error unknown classpath configuration
#endif

		/* run the thread */

		(void) vm_call_method(m, o);
	}
	else {
		/* set ThreadMXBean variables */

		_Jv_jvm->java_lang_management_ThreadMXBean_ThreadCount++;
		_Jv_jvm->java_lang_management_ThreadMXBean_TotalStartedThreadCount++;

		if (_Jv_jvm->java_lang_management_ThreadMXBean_ThreadCount >
			_Jv_jvm->java_lang_management_ThreadMXBean_PeakThreadCount)
			_Jv_jvm->java_lang_management_ThreadMXBean_PeakThreadCount =
				_Jv_jvm->java_lang_management_ThreadMXBean_ThreadCount;

		/* call passed function, e.g. finalizer_thread */

		(function)();
	}

#if !defined(NDEBUG)
	if (opt_verbosethreads) {
		printf("[Stopping thread ");
		threads_thread_print_info(thread);
		printf("]\n");
	}
#endif

#if defined(ENABLE_JVMTI)
	/* fire thread end event */

	if (jvmti)
		jvmti_ThreadStartEnd(JVMTI_EVENT_THREAD_END);
#endif

	if (!threads_detach_thread(thread))
		vm_abort("threads_startup_thread: threads_detach_thread failed");

	/* set ThreadMXBean variables */

	_Jv_jvm->java_lang_management_ThreadMXBean_ThreadCount--;

	return NULL;
}


/* threads_impl_thread_start ***************************************************

   Start a thread in the JVM.  Both (vm internal and java) thread
   objects exist.

   IN:
      thread....the thread object
	  f.........function to run in the new thread. NULL means that the
	            "run" method of the object `t` should be called

******************************************************************************/

void threads_impl_thread_start(threadobject *thread, functionptr f)
{
	sem_t          sem;
	sem_t          sem_first;
	pthread_attr_t attr;
	startupinfo    startup;
	int            ret;

	/* fill startupinfo structure passed by pthread_create to
	 * threads_startup_thread */

	startup.thread     = thread;
	startup.function   = f;              /* maybe we don't call Thread.run()V */
	startup.psem       = &sem;
	startup.psem_first = &sem_first;

	threads_sem_init(&sem, 0, 0);
	threads_sem_init(&sem_first, 0, 0);

	/* initialize thread attributes */

	if (pthread_attr_init(&attr) != 0)
		vm_abort("threads_impl_thread_start: pthread_attr_init failed: %s",
				 strerror(errno));

    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0)
		vm_abort("threads_impl_thread_start: pthread_attr_setdetachstate failed: %s",
				 strerror(errno));

	/* initialize thread stacksize */

	if (pthread_attr_setstacksize(&attr, opt_stacksize))
		vm_abort("threads_impl_thread_start: pthread_attr_setstacksize failed: %s",
				 strerror(errno));

	/* create the thread */

	ret = pthread_create(&(thread->tid), &attr, threads_startup_thread, &startup);

	/* destroy the thread attributes */

	if (pthread_attr_destroy(&attr) != 0)
		vm_abort("threads_impl_thread_start: pthread_attr_destroy failed: %s",
				 strerror(errno));

	/* check for pthread_create error */

	if (ret != 0)
		vm_abort("threads_impl_thread_start: pthread_create failed: %s",
				 strerror(errno));

	/* signal that pthread_create has returned, so thread->tid is valid */

	threads_sem_post(&sem_first);

	/* wait here until the thread has entered itself into the thread list */

	threads_sem_wait(&sem);

	/* cleanup */

	sem_destroy(&sem);
	sem_destroy(&sem_first);
}


/* threads_set_thread_priority *************************************************

   Set the priority of the given thread.

   IN:
      tid..........thread id
	  priority.....priority to set

******************************************************************************/

void threads_set_thread_priority(pthread_t tid, int priority)
{
	struct sched_param schedp;
	int policy;

	pthread_getschedparam(tid, &policy, &schedp);
	schedp.sched_priority = priority;
	pthread_setschedparam(tid, policy, &schedp);
}


/* threads_attach_current_thread ***********************************************

   Attaches the current thread to the VM.  Used in JNI.

*******************************************************************************/

bool threads_attach_current_thread(JavaVMAttachArgs *vm_aargs, bool isdaemon)
{
	threadobject          *thread;
	utf                   *u;
	java_objectheader     *s;
	java_objectheader     *o;
	java_lang_Thread      *t;

#if defined(ENABLE_JAVASE)
	java_lang_ThreadGroup *group;
	threadobject          *mainthread;
	classinfo             *c;
	methodinfo            *m;
#endif

#if defined(WITH_CLASSPATH_GNU)
	java_lang_VMThread    *vmt;
#endif

	/* Enter the join-mutex, so if the main-thread is currently
	   waiting to join all threads, the number of non-daemon threads
	   is correct. */

	threads_mutex_join_lock();

	/* create internal thread data-structure */

	thread = threads_thread_new();

	/* thread is a Java thread and running */

	thread->flags = THREAD_FLAG_JAVA;

	if (isdaemon)
		thread->flags |= THREAD_FLAG_DAEMON;

	/* The thread is flagged and (non-)daemon thread, we can leave the
	   mutex. */

	threads_mutex_join_unlock();

	/* create a java.lang.Thread object */

	t = (java_lang_Thread *) builtin_new(class_java_lang_Thread);

	/* XXX memory leak!!! */
	if (t == NULL)
		return false;

	thread->object = t;

	/* thread is completely initialized */

	threads_thread_state_runnable(thread);

#if !defined(NDEBUG)
	if (opt_verbosethreads) {
		printf("[Attaching thread ");
		threads_thread_print_info(thread);
		printf("]\n");
	}
#endif

#if defined(ENABLE_INTRP)
	/* create interpreter stack */

	if (opt_intrp) {
		MSET(intrp_main_stack, 0, u1, opt_stacksize);
		thread->_global_sp = (Cell *) (intrp_main_stack + opt_stacksize);
	}
#endif

#if defined(WITH_CLASSPATH_GNU)

	/* create a java.lang.VMThread object */

	vmt = (java_lang_VMThread *) builtin_new(class_java_lang_VMThread);

	/* XXX memory leak!!! */
	if (vmt == NULL)
		return false;

	/* set the thread */

	LLNI_field_set_ref(vmt, thread, t);
	LLNI_field_set_val(vmt, vmdata, (java_lang_Object *) thread);

#elif defined(WITH_CLASSPATH_SUN)

	vm_abort("threads_attach_current_thread: IMPLEMENT ME!");

#elif defined(WITH_CLASSPATH_CLDC1_1)

	LLNI_field_set_val(t, vm_thread, (java_lang_Object *) thread);

#else
# error unknown classpath configuration
#endif

	if (vm_aargs != NULL) {
		u     = utf_new_char(vm_aargs->name);
#if defined(ENABLE_JAVASE)
		group = (java_lang_ThreadGroup *) vm_aargs->group;
#endif
	}
	else {
		u     = utf_null;
#if defined(ENABLE_JAVASE)
		/* get the main thread */

		mainthread = threads_list_first();
		group = LLNI_field_direct(mainthread->object, group);
#endif
	}

	/* the the thread name */

	s = javastring_new(u);

	/* for convenience */

	o = (java_objectheader *) thread->object;

#if defined(WITH_CLASSPATH_GNU)
	(void) vm_call_method(method_thread_init, o, vmt, s, NORM_PRIORITY,
						  isdaemon);
#elif defined(WITH_CLASSPATH_CLDC1_1)
	(void) vm_call_method(method_thread_init, o, s);
#endif

	if (exceptions_get_exception())
		return false;

#if defined(ENABLE_JAVASE)
	/* store the thread group in the object */

	LLNI_field_direct(thread->object, group) = group;

	/* add thread to given thread-group */

	LLNI_class_get(group, c);

	m = class_resolveclassmethod(c,
								 utf_addThread,
								 utf_java_lang_Thread__V,
								 class_java_lang_ThreadGroup,
								 true);

	o = (java_objectheader *) group;

	(void) vm_call_method(m, o, t);

	if (exceptions_get_exception())
		return false;
#endif

	return true;
}


/* threads_detach_thread *******************************************************

   Detaches the passed thread from the VM.  Used in JNI.

*******************************************************************************/

bool threads_detach_thread(threadobject *thread)
{
#if defined(ENABLE_JAVASE)
	java_lang_ThreadGroup *group;
	classinfo             *c;
	methodinfo            *m;
	java_objectheader     *o;
	java_lang_Thread      *t;
#endif

	/* XXX implement uncaught exception stuff (like JamVM does) */

#if defined(ENABLE_JAVASE)
	/* remove thread from the thread group */

	group = LLNI_field_direct(thread->object, group);

	/* XXX TWISTI: should all threads be in a ThreadGroup? */

	if (group != NULL) {
		LLNI_class_get(group, c);

# if defined(WITH_CLASSPATH_GNU)
		m = class_resolveclassmethod(c,
									 utf_removeThread,
									 utf_java_lang_Thread__V,
									 class_java_lang_ThreadGroup,
									 true);
# elif defined(WITH_CLASSPATH_SUN)
		m = class_resolveclassmethod(c,
									 utf_remove,
									 utf_java_lang_Thread__V,
									 class_java_lang_ThreadGroup,
									 true);
# else
#  error unknown classpath configuration
# endif

		if (m == NULL)
			return false;

		o = (java_objectheader *) group;
		t = thread->object;

		(void) vm_call_method(m, o, t);

		if (exceptions_get_exception())
			return false;
	}
#endif

	/* thread is terminated */

	threads_thread_state_terminated(thread);

#if !defined(NDEBUG)
	if (opt_verbosethreads) {
		printf("[Detaching thread ");
		threads_thread_print_info(thread);
		printf("]\n");
	}
#endif

	/* Enter the join-mutex before calling threads_thread_free, so
	   threads_join_all_threads gets the correct number of non-daemon
	   threads. */

	threads_mutex_join_lock();

	/* free the vm internal thread object */

	threads_thread_free(thread);

	/* Signal that this thread has finished and leave the mutex. */

	pthread_cond_signal(&cond_join);
	threads_mutex_join_unlock();

	return true;
}


/* threads_join_all_threads ****************************************************

   Join all non-daemon threads.

*******************************************************************************/

void threads_join_all_threads(void)
{
	threadobject *t;

	/* get current thread */

	t = THREADOBJECT;

	/* this thread is waiting for all non-daemon threads to exit */

	threads_thread_state_waiting(t);

	/* enter join mutex */

	threads_mutex_join_lock();

	/* Wait for condition as long as we have non-daemon threads.  We
	   compare against 1 because the current (main thread) is also a
	   non-daemon thread. */

	while (threads_list_get_non_daemons() > 1)
		pthread_cond_wait(&cond_join, &mutex_join);

	/* leave join mutex */

	threads_mutex_join_unlock();
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

	if (gettimeofday(&tvnow, NULL) != 0)
		vm_abort("gettimeofday failed: %s\n", strerror(errno));

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

static bool threads_wait_with_timeout(threadobject *thread,
									  struct timespec *wakeupTime)
{
	bool wasinterrupted;

	/* acquire the waitmutex */

	pthread_mutex_lock(&thread->waitmutex);

	/* mark us as sleeping */

	thread->sleeping = true;

	/* wait on waitcond */

	if (wakeupTime->tv_sec || wakeupTime->tv_nsec) {
		/* with timeout */
		while (!thread->interrupted && !thread->signaled
			   && threads_current_time_is_earlier_than(wakeupTime))
		{
			threads_thread_state_timed_waiting(thread);

			pthread_cond_timedwait(&thread->waitcond, &thread->waitmutex,
								   wakeupTime);

			threads_thread_state_runnable(thread);
		}
	}
	else {
		/* no timeout */
		while (!thread->interrupted && !thread->signaled) {
			threads_thread_state_waiting(thread);

			pthread_cond_wait(&thread->waitcond, &thread->waitmutex);

			threads_thread_state_runnable(thread);
		}
	}

	/* check if we were interrupted */

	wasinterrupted = thread->interrupted;

	/* reset all flags */

	thread->interrupted = false;
	thread->signaled    = false;
	thread->sleeping    = false;

	/* release the waitmutex */

	pthread_mutex_unlock(&thread->waitmutex);

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

bool threads_wait_with_timeout_relative(threadobject *thread, s8 millis,
										s4 nanos)
{
	struct timespec wakeupTime;

	/* calculate the the (latest) wakeup time */

	threads_calc_absolute_time(&wakeupTime, millis, nanos);

	/* wait */

	return threads_wait_with_timeout(thread, &wakeupTime);
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


/* threads_thread_interrupt ****************************************************

   Interrupt the given thread.

   The thread gets the "waitcond" signal and 
   its interrupted flag is set to true.

   IN:
      thread............the thread to interrupt

*******************************************************************************/

void threads_thread_interrupt(threadobject *thread)
{
	/* Signal the thread a "waitcond" and tell it that it has been
	   interrupted. */

	pthread_mutex_lock(&thread->waitmutex);

	/* Interrupt blocking system call using a signal. */

	pthread_kill(thread->tid, SIGHUP);

	if (thread->sleeping)
		pthread_cond_signal(&thread->waitcond);

	thread->interrupted = true;

	pthread_mutex_unlock(&thread->waitmutex);
}


/* threads_check_if_interrupted_and_reset **************************************

   Check if the current thread has been interrupted and reset the
   interruption flag.

   RETURN VALUE:
      true, if the current thread had been interrupted

*******************************************************************************/

bool threads_check_if_interrupted_and_reset(void)
{
	threadobject *thread;
	bool intr;

	thread = THREADOBJECT;

	/* get interrupted flag */

	intr = thread->interrupted;

	/* reset interrupted flag */

	thread->interrupted = false;

	return intr;
}


/* threads_thread_has_been_interrupted *****************************************

   Check if the given thread has been interrupted

   IN:
      t............the thread to check

   RETURN VALUE:
      true, if the given thread had been interrupted

*******************************************************************************/

bool threads_thread_has_been_interrupted(threadobject *thread)
{
	return thread->interrupted;
}


/* threads_sleep ***************************************************************

   Sleep the current thread for the specified amount of time.

*******************************************************************************/

void threads_sleep(s8 millis, s4 nanos)
{
	threadobject    *thread;
	struct timespec  wakeupTime;
	bool             wasinterrupted;

	thread = THREADOBJECT;

	threads_calc_absolute_time(&wakeupTime, millis, nanos);

	wasinterrupted = threads_wait_with_timeout(thread, &wakeupTime);

	if (wasinterrupted)
		exceptions_throw_interruptedexception();
}


/* threads_yield ***************************************************************

   Yield to the scheduler.

*******************************************************************************/

void threads_yield(void)
{
	sched_yield();
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
