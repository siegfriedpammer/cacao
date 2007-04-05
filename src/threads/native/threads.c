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

   $Id: threads.c 7669 2007-04-05 11:39:58Z twisti $

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

#include "threads/threads-common.h"

#include "threads/native/threads.h"

#include "toolbox/avl.h"
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

#define THREADS_INITIAL_TABLE_SIZE      8


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

static void threads_table_init(void);
static s4 threads_table_add(threadobject *thread);
static void threads_table_remove(threadobject *thread);
static void threads_calc_absolute_time(struct timespec *tm, s8 millis, s4 nanos);

#if !defined(NDEBUG) && 0
static void threads_table_dump(FILE *file);
#endif

/******************************************************************************/
/* GLOBAL VARIABLES                                                           */
/******************************************************************************/

/* the main thread                                                            */
threadobject *mainthreadobj;

static methodinfo *method_thread_init;

/* the thread object of the current thread                                    */
/* This is either a thread-local variable defined with __thread, or           */
/* a thread-specific value stored with key threads_current_threadobject_key.  */
#if defined(HAVE___THREAD)
__thread threadobject *threads_current_threadobject;
#else
pthread_key_t threads_current_threadobject_key;
#endif

/* global threads table                                                       */
static threads_table_t threads_table;

/* global compiler mutex                                                      */
static pthread_mutex_t compiler_mutex;

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


/* compiler_lock ***************************************************************

   Enter the compiler lock.

******************************************************************************/

void compiler_lock(void)
{
	pthread_mutex_lock(&compiler_mutex);
}


/* compiler_unlock *************************************************************

   Release the compiler lock.

******************************************************************************/

void compiler_unlock(void)
{
	pthread_mutex_unlock(&compiler_mutex);
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
	threadobject *self = THREADOBJECT;

	if (count == 0) {
		do {
			count++;
			tobj = tobj->next;
		} while (tobj != mainthreadobj);
	}

	assert(tobj == mainthreadobj);

	do {
		if (tobj != self)
			pthread_kill(tobj->tid, sig);
		tobj = tobj->next;
	} while (tobj != mainthreadobj);

	return count - 1;
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

			thread_restartcriticalsection((ucontext_t *) &thread_state);

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
	int count, i;
#endif

	lock_stopworld(STOPWORLD_FROM_CLASS_NUMBERING);
	pthread_mutex_lock(&threadlistlock);

#if defined(__DARWIN__)
	threads_cast_darwinstop();
#elif defined(__CYGWIN__)
	/* TODO */
	assert(0);
#else
	count = threads_cast_sendsignals(GC_signum1(), 0);
	for (i = 0; i < count; i++)
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
#elif defined(__CYGWIN__)
	/* TODO */
	assert(0);
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

/* This function is called from Boehm GC code. */

int cacao_suspendhandler(ucontext_t *ctx)
{
	if (stopworldwhere != STOPWORLD_FROM_CLASS_NUMBERING)
		return 0;

	threads_sigsuspend_handler(ctx);
	return 1;
}
#endif

#endif /* DISABLE_GC */

/* threads_set_current_threadobject ********************************************

   Set the current thread object.
   
   IN:
      thread.......the thread object to set

*******************************************************************************/

static void threads_set_current_threadobject(threadobject *thread)
{
#if !defined(HAVE___THREAD)
	pthread_setspecific(threads_current_threadobject_key, thread);
#else
	threads_current_threadobject = thread;
#endif
}


/* threads_init_threadobject **************************************************

   Initialize implementation fields of a threadobject.

   IN:
      thread............the threadobject

******************************************************************************/

static void threads_init_threadobject(threadobject *thread)
{
	thread->tid = pthread_self();

	thread->index = 0;

	/* TODO destroy all those things */
	pthread_mutex_init(&(thread->joinmutex), NULL);
	pthread_cond_init(&(thread->joincond), NULL);

	pthread_mutex_init(&(thread->waitmutex), NULL);
	pthread_cond_init(&(thread->waitcond), NULL);

	thread->interrupted = false;
	thread->signaled = false;
	thread->sleeping = false;
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


/* threads_preinit *************************************************************

   Do some early initialization of stuff required.

   ATTENTION: Do NOT use any Java heap allocation here, as gc_init()
   is called AFTER this function!

*******************************************************************************/

void threads_preinit(void)
{
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&compiler_mutex, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);

	pthread_mutex_init(&threadlistlock, NULL);
	pthread_mutex_init(&stopworldlock, NULL);

	mainthreadobj = NEW(threadobject);

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		size_threadobject += sizeof(threadobject);
#endif

	mainthreadobj->object   = NULL;
	mainthreadobj->tid      = pthread_self();
	mainthreadobj->index    = 1;
	mainthreadobj->thinlock = lock_pre_compute_thinlock(mainthreadobj->index);
	
#if !defined(HAVE___THREAD)
	pthread_key_create(&threads_current_threadobject_key, NULL);
#endif
	threads_set_current_threadobject(mainthreadobj);

	threads_sem_init(&suspend_ack, 0, 0);

	/* initialize the threads table */

	threads_table_init();

	/* initialize subsystems */

	lock_init();

	critical_init();
}


/* threads_init ****************************************************************

   Initializes the threads required by the JVM: main, finalizer.

*******************************************************************************/

bool threads_init(void)
{
	java_lang_String      *threadname;
	threadobject          *tempthread;
	java_objectheader     *o;

#if defined(ENABLE_JAVASE)
	java_lang_ThreadGroup *threadgroup;
	methodinfo            *m;
	java_lang_Thread      *t;
#endif

#if defined(WITH_CLASSPATH_GNU)
	java_lang_VMThread    *vmt;
#endif

	tempthread = mainthreadobj;

	/* XXX We have to find a new way to free lock records */
	/*     with the new locking algorithm.                */
	/* lock_record_free_pools(mainthreadobj->ee.lockrecordpools); */

#if 0
	/* This is kinda tricky, we grow the java.lang.Thread object so we
	   can keep the execution environment there. No Thread object must
	   have been created at an earlier time. */

	class_java_lang_Thread->instancesize = sizeof(threadobject);
#endif

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

	/* create a vm internal thread object for the main thread */
	/* XXX Michi: we do not need to do this here, we could use the one
	       created by threads_preinit() */

#if defined(ENABLE_GC_CACAO)
	mainthreadobj = NEW(threadobject);

# if defined(ENABLE_STATISTICS)
	if (opt_stat)
		size_threadobject += sizeof(threadobject);
# endif
#else
	mainthreadobj = GCNEW(threadobject);
#endif

	if (mainthreadobj == NULL)
		return false;

	/* create a java.lang.Thread for the main thread */

	mainthreadobj->object = (java_lang_Thread *) builtin_new(class_java_lang_Thread);

	if (mainthreadobj->object == NULL)
		return false;

	FREE(tempthread, threadobject);

	threads_init_threadobject(mainthreadobj);
	threads_set_current_threadobject(mainthreadobj);
	lock_init_execution_env(mainthreadobj);

	/* thread is running */

	mainthreadobj->state = THREAD_STATE_RUNNABLE;

	mainthreadobj->next = mainthreadobj;
	mainthreadobj->prev = mainthreadobj;

	threads_table_add(mainthreadobj);

	/* mark main thread as Java thread */

	mainthreadobj->flags = THREAD_FLAG_JAVA;

#if defined(ENABLE_INTRP)
	/* create interpreter stack */

	if (opt_intrp) {
		MSET(intrp_main_stack, 0, u1, opt_stacksize);
		mainthreadobj->_global_sp = (Cell*) (intrp_main_stack + opt_stacksize);
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

	vmt->thread = mainthreadobj->object;
	vmt->vmdata = (java_lang_Object *) mainthreadobj;

	/* call java.lang.Thread.<init>(Ljava/lang/VMThread;Ljava/lang/String;IZ)V */
	o = (java_objectheader *) mainthreadobj->object;

	(void) vm_call_method(method_thread_init, o, vmt, threadname, NORM_PRIORITY,
						  false);
#elif defined(WITH_CLASSPATH_CLDC1_1)
	/* set the thread */

	mainthreadobj->object->vm_thread = (java_lang_Object *) mainthreadobj;

	/* call public Thread(String name) */

	o = (java_objectheader *) mainthreadobj->object;

	(void) vm_call_method(method_thread_init, o, threadname);
#endif

	if (*exceptionptr)
		return false;

#if defined(ENABLE_JAVASE)
	mainthreadobj->object->group = threadgroup;

	/* add main thread to java.lang.ThreadGroup */

	m = class_resolveclassmethod(class_java_lang_ThreadGroup,
								 utf_addThread,
								 utf_java_lang_Thread__V,
								 class_java_lang_ThreadGroup,
								 true);

	o = (java_objectheader *) threadgroup;
	t = mainthreadobj->object;

	(void) vm_call_method(m, o, t);

	if (*exceptionptr)
		return false;

#endif

	threads_set_thread_priority(pthread_self(), NORM_PRIORITY);

	/* initialize the thread attribute object */

	if (pthread_attr_init(&threadattr)) {
		log_println("pthread_attr_init failed: %s", strerror(errno));
		return false;
	}

	pthread_attr_setdetachstate(&threadattr, PTHREAD_CREATE_DETACHED);

	/* everything's ok */

	return true;
}


/* threads_table_init *********************************************************

   Initialize the global threads table.

******************************************************************************/

static void threads_table_init(void)
{
	s4 size;
	s4 i;

	size = THREADS_INITIAL_TABLE_SIZE;

	threads_table.size = size;
	threads_table.table = MNEW(threads_table_entry_t, size);

	/* link the entries in a freelist */

	for (i=0; i<size; ++i) {
		threads_table.table[i].nextfree = i+1;
	}

	/* terminate the freelist */

	threads_table.table[size-1].nextfree = 0; /* index 0 is never free */
}


/* threads_table_add **********************************************************

   Add a thread to the global threads table. The index is entered in the
   threadobject. The thinlock value for the thread is pre-computed.

   IN:
      thread............the thread to add

   RETURN VALUE:
      The table index for the newly added thread. This value has also been
	  entered in the threadobject.

   PRE-CONDITION:
      The caller must hold the threadlistlock!

******************************************************************************/

static s4 threads_table_add(threadobject *thread)
{
	s4 index;
	s4 oldsize;
	s4 newsize;
	s4 i;

	/* table[0] serves as the head of the freelist */

	index = threads_table.table[0].nextfree;

	/* if we got a free index, use it */

	if (index) {
got_an_index:
		threads_table.table[0].nextfree = threads_table.table[index].nextfree;
		threads_table.table[index].thread = thread;
		thread->index = index;
		thread->thinlock = lock_pre_compute_thinlock(index);
		return index;
	}

	/* we must grow the table */

	oldsize = threads_table.size;
	newsize = oldsize * 2;

	threads_table.table = MREALLOC(threads_table.table, threads_table_entry_t,
								   oldsize, newsize);
	threads_table.size = newsize;

	/* link the new entries to a free list */

	for (i=oldsize; i<newsize; ++i) {
		threads_table.table[i].nextfree = i+1;
	}

	/* terminate the freelist */

	threads_table.table[newsize-1].nextfree = 0; /* index 0 is never free */

	/* use the first of the new entries */

	index = oldsize;
	goto got_an_index;
}


/* threads_table_remove *******************************************************

   Remove a thread from the global threads table.

   IN:
      thread............the thread to remove

   PRE-CONDITION:
      The caller must hold the threadlistlock!

******************************************************************************/

static void threads_table_remove(threadobject *thread)
{
	s4 index;

	index = thread->index;

	/* put the index into the freelist */

	threads_table.table[index] = threads_table.table[0];
	threads_table.table[0].nextfree = index;

	/* delete the index in the threadobject to discover bugs */
#if !defined(NDEBUG)
	thread->index = 0;
#endif
}


/* threads_startup_thread ******************************************************

   Thread startup function called by pthread_create.

   Thread which have a startup.function != NULL are marked as internal
   threads. All other threads are threated as normal Java threads.

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
	startupinfo        *startup;
	threadobject       *thread;
#if defined(WITH_CLASSPATH_GNU)
	java_lang_VMThread *vmt;
#endif
	sem_t              *psem;
	threadobject       *tnext;
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

	startup = t;
	t = NULL; /* make sure it's not used wrongly */

	thread   = startup->thread;
	function = startup->function;
	psem     = startup->psem;

	/* Seems like we've encountered a situation where thread->tid was not set by
	 * pthread_create. We alleviate this problem by waiting for pthread_create
	 * to return. */
	threads_sem_wait(startup->psem_first);

	/* set the thread object */

#if defined(__DARWIN__)
	thread->mach_thread = mach_thread_self();
#endif
	threads_set_current_threadobject(thread);

	/* thread is running */

	thread->state = THREAD_STATE_RUNNABLE;

	/* insert the thread into the threadlist and the threads table */

	pthread_mutex_lock(&threadlistlock);

	thread->prev = mainthreadobj;
	thread->next = tnext = mainthreadobj->next;
	mainthreadobj->next = thread;
	tnext->prev = thread;

	threads_table_add(thread);

	pthread_mutex_unlock(&threadlistlock);

	/* init data structures of this thread */

	lock_init_execution_env(thread);

	/* tell threads_startup_thread that we registered ourselves */
	/* CAUTION: *startup becomes invalid with this!             */

	startup = NULL;
	threads_sem_post(psem);

	/* set our priority */

	threads_set_thread_priority(thread->tid, thread->object->priority);

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

	/* find and run the Thread.run()V method if no other function was passed */

	if (function == NULL) {
		/* this is a normal Java thread */

		thread->flags |= THREAD_FLAG_JAVA;

#if defined(WITH_CLASSPATH_GNU)
		/* We need to start the run method of
		   java.lang.VMThread. Since this is a final class, we can use
		   the class object directly. */

		c   = class_java_lang_VMThread;
#elif defined(WITH_CLASSPATH_CLDC1_1)
		c   = thread->object->header.vftbl->class;
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

		vmt = (java_lang_VMThread *) thread->object->vmThread;
		o   = (java_objectheader *) vmt;

#elif defined(WITH_CLASSPATH_CLDC1_1)
		o   = (java_objectheader *) thread->object;
#endif

		/* run the thread */

		(void) vm_call_method(m, o);
	}
	else {
		/* this is an internal thread */

		thread->flags |= THREAD_FLAG_INTERNAL;

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

#if defined(ENABLE_JVMTI)
	/* fire thread end event */

	if (jvmti)
		jvmti_ThreadStartEnd(JVMTI_EVENT_THREAD_END);
#endif

	threads_detach_thread(thread);

	/* set ThreadMXBean variables */

	_Jv_jvm->java_lang_management_ThreadMXBean_ThreadCount--;

	return NULL;
}


/* threads_start_javathread ***************************************************

   Start a thread in the JVM. Only the java thread object exists so far.

   IN:
      object.....the java thread object java.lang.Thread

******************************************************************************/

void threads_start_javathread(java_lang_Thread *object)
{
	threadobject *thread;

	/* create the vm internal threadobject */

	thread = NEW(threadobject);

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		size_threadobject += sizeof(threadobject);
#endif

	/* link the two objects together */

	thread->object = object;

#if defined(WITH_CLASSPATH_GNU)
	assert(object->vmThread);
	assert(object->vmThread->vmdata == NULL);
	object->vmThread->vmdata = (java_lang_Object *) thread;
#elif defined(WITH_CLASSPATH_CLDC1_1)
	object->vm_thread = (java_lang_Object *) thread;
#endif

	/* actually start the thread */
	/* don't pass a function pointer (NULL) since we want Thread.run()V here */

	threads_start_thread(thread, NULL);
}


/* threads_start_thread ********************************************************

   Start a thread in the JVM. Both (vm internal and java) thread objects exist.

   IN:
      thread.......the thread object
	  function.....function to run in the new thread. NULL means that the
	               "run" method of the object `t` should be called

******************************************************************************/

void threads_start_thread(threadobject *thread, functionptr function)
{
	sem_t          sem;
	sem_t          sem_first;
	pthread_attr_t attr;
	startupinfo    startup;

	/* fill startupinfo structure passed by pthread_create to
	 * threads_startup_thread */

	startup.thread     = thread;
	startup.function   = function;       /* maybe we don't call Thread.run()V */
	startup.psem       = &sem;
	startup.psem_first = &sem_first;

	threads_sem_init(&sem, 0, 0);
	threads_sem_init(&sem_first, 0, 0);

	/* initialize thread attribute object */

	if (pthread_attr_init(&attr))
		vm_abort("pthread_attr_init failed: %s", strerror(errno));

	/* initialize thread stacksize */

	if (pthread_attr_setstacksize(&attr, opt_stacksize))
		vm_abort("pthread_attr_setstacksize failed: %s", strerror(errno));

	/* create the thread */

	if (pthread_create(&(thread->tid), &attr, threads_startup_thread, &startup))
		vm_abort("pthread_create failed: %s", strerror(errno));

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
	java_lang_String      *s;
	java_objectheader     *o;
	java_lang_Thread      *t;

#if defined(ENABLE_JAVASE)
	java_lang_ThreadGroup *group;
	methodinfo            *m;
#endif

#if defined(WITH_CLASSPATH_GNU)
	java_lang_VMThread    *vmt;
#endif

	/* create a vm internal thread object */

	thread = NEW(threadobject);

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		size_threadobject += sizeof(threadobject);
#endif

	if (thread == NULL)
		return false;

	/* create a java.lang.Thread object */

	t = (java_lang_Thread *) builtin_new(class_java_lang_Thread);

	if (t == NULL)
		return false;

	thread->object = t;

	threads_init_threadobject(thread);
	threads_set_current_threadobject(thread);
	lock_init_execution_env(thread);

	/* thread is running */

	thread->state = THREAD_STATE_RUNNABLE;

	/* insert the thread into the threadlist and the threads table */

	pthread_mutex_lock(&threadlistlock);

	thread->prev        = mainthreadobj;
	thread->next        = mainthreadobj->next;
	mainthreadobj->next = thread;
	thread->next->prev  = thread;

	threads_table_add(thread);

	pthread_mutex_unlock(&threadlistlock);

	/* mark thread as Java thread */

	thread->flags = THREAD_FLAG_JAVA;

	if (isdaemon)
		thread->flags |= THREAD_FLAG_DAEMON;

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

	if (vmt == NULL)
		return false;

	/* set the thread */

	vmt->thread = t;
	vmt->vmdata = (java_lang_Object *) thread;
#elif defined(WITH_CLASSPATH_CLDC1_1)
	t->vm_thread = (java_lang_Object *) thread;
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
		group = mainthreadobj->object->group;
#endif
	}

	s = javastring_new(u);

	o = (java_objectheader *) thread->object;

#if defined(WITH_CLASSPATH_GNU)
	(void) vm_call_method(method_thread_init, o, vmt, s, NORM_PRIORITY,
						  isdaemon);
#elif defined(WITH_CLASSPATH_CLDC1_1)
	(void) vm_call_method(method_thread_init, o, s);
#endif

	if (*exceptionptr)
		return false;

#if defined(ENABLE_JAVASE)
	/* store the thread group in the object */

	thread->object->group = group;

	/* add thread to given thread-group */

	m = class_resolveclassmethod(group->header.vftbl->class,
								 utf_addThread,
								 utf_java_lang_Thread__V,
								 class_java_lang_ThreadGroup,
								 true);

	o = (java_objectheader *) group;

	(void) vm_call_method(m, o, t);

	if (*exceptionptr)
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
	methodinfo            *m;
	java_objectheader     *o;
	java_lang_Thread      *t;
#endif

	/* Allow lock record pools to be used by other threads. They
	   cannot be deleted so we'd better not waste them. */

	/* XXX We have to find a new way to free lock records */
	/*     with the new locking algorithm.                */
	/* lock_record_free_pools(thread->ee.lockrecordpools); */

	/* XXX implement uncaught exception stuff (like JamVM does) */

#if defined(ENABLE_JAVASE)
	/* remove thread from the thread group */

	group = thread->object->group;

	/* XXX TWISTI: should all threads be in a ThreadGroup? */

	if (group != NULL) {
		m = class_resolveclassmethod(group->header.vftbl->class,
									 utf_removeThread,
									 utf_java_lang_Thread__V,
									 class_java_lang_ThreadGroup,
									 true);

		if (m == NULL)
			return false;

		o = (java_objectheader *) group;
		t = thread->object;

		(void) vm_call_method(m, o, t);

		if (*exceptionptr)
			return false;
	}
#endif

	/* thread is terminated */

	thread->state = THREAD_STATE_TERMINATED;

	/* remove thread from thread list and threads table, do this
	   inside a lock */

	pthread_mutex_lock(&threadlistlock);

	thread->next->prev = thread->prev;
	thread->prev->next = thread->next;

	threads_table_remove(thread);

	pthread_mutex_unlock(&threadlistlock);

	/* reset thread id (lock on joinmutex? TWISTI) */

	pthread_mutex_lock(&(thread->joinmutex));
	thread->tid = 0;
	pthread_mutex_unlock(&(thread->joinmutex));

	/* tell everyone that a thread has finished */

	pthread_cond_broadcast(&(thread->joincond));

	/* free the vm internal thread object */

	FREE(thread, threadobject);

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		size_threadobject -= sizeof(threadobject);
#endif

	return true;
}


/* threads_find_non_daemon_thread **********************************************

   Helper function used by threads_join_all_threads for finding
   non-daemon threads that are still running.

*******************************************************************************/

/* At the end of the program, we wait for all running non-daemon
   threads to die. */

static threadobject *threads_find_non_daemon_thread(threadobject *thread)
{
	while (thread != mainthreadobj) {
		if (!(thread->flags & THREAD_FLAG_DAEMON))
			return thread;

		thread = thread->prev;
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

	while ((thread = threads_find_non_daemon_thread(mainthreadobj->prev)) != NULL) {
		pthread_mutex_lock(&(thread->joinmutex));

		pthread_mutex_unlock(&threadlistlock);

		while (thread->tid)
			pthread_cond_wait(&(thread->joincond), &(thread->joinmutex));

		pthread_mutex_unlock(&(thread->joinmutex));

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
			thread->state = THREAD_STATE_TIMED_WAITING;

			pthread_cond_timedwait(&thread->waitcond, &thread->waitmutex,
								   wakeupTime);

			thread->state = THREAD_STATE_RUNNABLE;
		}
	}
	else {
		/* no timeout */
		while (!thread->interrupted && !thread->signaled) {
			thread->state = THREAD_STATE_WAITING;

			pthread_cond_wait(&thread->waitcond, &thread->waitmutex);

			thread->state = THREAD_STATE_RUNNABLE;
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


/* threads_table_dump *********************************************************

   Dump the threads table for debugging purposes.

   IN:
      file..............stream to write to

******************************************************************************/

#if !defined(NDEBUG) && 0
static void threads_table_dump(FILE *file)
{
	s4 i;
	s4 size;
	ptrint index;

	pthread_mutex_lock(&threadlistlock);

	size = threads_table.size;

	fprintf(file, "======== THREADS TABLE (size %d) ========\n", size);

	for (i=0; i<size; ++i) {
		index = threads_table.table[i].nextfree;

		fprintf(file, "%4d: ", i);

		if (index < size) {
			fprintf(file, "free, nextfree = %d\n", (int) index);
		}
		else {
			fprintf(file, "thread %p\n", (void*) threads_table.table[i].thread);
		}
	}

	fprintf(file, "======== END OF THREADS TABLE ========\n");

	pthread_mutex_unlock(&threadlistlock);
}
#endif

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
