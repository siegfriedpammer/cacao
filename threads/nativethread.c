#include "global.h"

#if defined(NATIVE_THREADS)

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include "config.h"
#include "thread.h"
#include "codegen.h"
#include "locks.h"
#include "tables.h"
#include "native.h"
#include "loader.h"
#include "builtin.h"
#include "asmpart.h"
#include "exceptions.h"
#include "toolbox/logging.h"
#include "toolbox/memory.h"
#include "toolbox/avl.h"
#include "mm/boehm.h"

#include "nat/java_lang_Object.h"
#include "nat/java_lang_Throwable.h"
#include "nat/java_lang_Thread.h"
#include "nat/java_lang_ThreadGroup.h"
#include "nat/java_lang_VMThread.h"

#include <pthread.h>
#include <semaphore.h>

#if !defined(__DARWIN__)
#if defined(__LINUX__)
#define GC_LINUX_THREADS
#elif defined(__MIPS__)
#define GC_IRIX_THREADS
#endif
#include "../mm/boehm-gc/include/gc.h"
#endif

#ifdef MUTEXSIM

/* We need this for older MacOSX (10.1.x) */

typedef struct {
	pthread_mutex_t mutex;
	pthread_t owner;
	int count;
} pthread_mutex_rec_t;

static void pthread_mutex_init_rec(pthread_mutex_rec_t *m)
{
	pthread_mutex_init(&m->mutex, NULL);
	m->count = 0;
}

static void pthread_mutex_destroy_rec(pthread_mutex_rec_t *m)
{
	pthread_mutex_destroy(&m->mutex);
}

static void pthread_mutex_lock_rec(pthread_mutex_rec_t *m)
{
	for (;;)
		if (!m->count)
		{
			pthread_mutex_lock(&m->mutex);
			m->owner = pthread_self();
			m->count++;
			break;
		} else {
			if (m->owner != pthread_self())
				pthread_mutex_lock(&m->mutex);
			else
			{
				m->count++;
				break;
			}
		}
}

static void pthread_mutex_unlock_rec(pthread_mutex_rec_t *m)
{
	if (!--m->count)
		pthread_mutex_unlock(&m->mutex);
}

#else /* MUTEXSIM */

#define pthread_mutex_lock_rec pthread_mutex_lock
#define pthread_mutex_unlock_rec pthread_mutex_unlock
#define pthread_mutex_rec_t pthread_mutex_t

#endif /* MUTEXSIM */

static void setPriority(pthread_t tid, int priority)
{
	struct sched_param schedp;
	int policy;

	pthread_getschedparam(tid, &policy, &schedp);
	schedp.sched_priority = priority;
	pthread_setschedparam(tid, policy, &schedp);
}

#include "machine-instr.h"

static struct avl_table *criticaltree;
static threadobject *mainthreadobj;

#ifndef HAVE___THREAD
pthread_key_t tkey_threadinfo;
#else
__thread threadobject *threadobj;
#endif

static pthread_mutex_rec_t compiler_mutex;
static pthread_mutex_rec_t tablelock;

void compiler_lock()
{
	pthread_mutex_lock_rec(&compiler_mutex);
}

void compiler_unlock()
{
	pthread_mutex_unlock_rec(&compiler_mutex);
}

void tables_lock()
{
    pthread_mutex_lock_rec(&tablelock);
}

void tables_unlock()
{
    pthread_mutex_unlock_rec(&tablelock);
}

static int criticalcompare(const void *pa, const void *pb, void *param)
{
	const threadcritnode *na = pa;
	const threadcritnode *nb = pb;

	if (na->mcodebegin < nb->mcodebegin)
		return -1;
	if (na->mcodebegin > nb->mcodebegin)
		return 1;
	return 0;
}

static const threadcritnode *findcritical(u1 *mcodeptr)
{
    struct avl_node *n = criticaltree->avl_root;
    const threadcritnode *m = NULL;
    if (!n)
        return NULL;
    for (;;)
    {
        const threadcritnode *d = n->avl_data;
        if (mcodeptr == d->mcodebegin)
            return d;
        if (mcodeptr < d->mcodebegin) {
            if (n->avl_link[0])
                n = n->avl_link[0];
            else
                return m;
        } else {
            if (n->avl_link[1]) {
                m = n->avl_data;
                n = n->avl_link[1];
            } else
                return n->avl_data;
        }
    }
}

void thread_registercritical(threadcritnode *n)
{
	avl_insert(criticaltree, n);
}

u1 *thread_checkcritical(u1 *mcodeptr)
{
	const threadcritnode *n = findcritical(mcodeptr);
	return (n && mcodeptr < n->mcodeend && mcodeptr > n->mcodebegin) ? n->mcoderestart : NULL;
}

static void thread_addstaticcritical()
{
	threadcritnode *n = &asm_criticalsections;

	while (n->mcodebegin)
		thread_registercritical(n++);
}

static pthread_mutex_t threadlistlock;

static pthread_mutex_t stopworldlock;
volatile int stopworldwhere;

static sem_t suspend_ack;
#if defined(__MIPS__)
static pthread_mutex_t suspend_ack_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t suspend_cond = PTHREAD_COND_INITIALIZER;
#endif

/*
 * where - 1 from within GC
           2 class numbering
 */
void lock_stopworld(int where)
{
	pthread_mutex_lock(&stopworldlock);
	stopworldwhere = where;
}

void unlock_stopworld()
{
	stopworldwhere = 0;
	pthread_mutex_unlock(&stopworldlock);
}

#if !defined(__DARWIN__)
/* Caller must hold threadlistlock */
static int cast_sendsignals(int sig, int count)
{
	/* Count threads */
	threadobject *tobj = mainthreadobj;
	nativethread *infoself = THREADINFO;

	if (count == 0)
		do {
			count++;
			tobj = tobj->info.next;
		} while (tobj != mainthreadobj);

	do {
		nativethread *info = &tobj->info;
		if (info != infoself)
			pthread_kill(info->tid, sig);
		tobj = tobj->info.next;
	} while (tobj != mainthreadobj);

	return count-1;
}

#else

static void cast_darwinstop()
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
			if (r != KERN_SUCCESS)
				panic("thread_suspend failed");

			r = thread_get_state(thread, flavor,
				(natural_t*)&thread_state, &thread_state_count);
			if (r != KERN_SUCCESS)
				panic("thread_get_state failed");

			thread_restartcriticalsection(&thread_state);

			r = thread_set_state(thread, flavor,
				(natural_t*)&thread_state, thread_state_count);
			if (r != KERN_SUCCESS)
				panic("thread_set_state failed");
		}
		tobj = tobj->info.next;
	} while (tobj != mainthreadobj);
}

static void cast_darwinresume()
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
			if (r != KERN_SUCCESS)
				panic("thread_resume failed");
		}
		tobj = tobj->info.next;
	} while (tobj != mainthreadobj);
}

#endif

#if defined(__MIPS__)
static void cast_irixresume()
{
	pthread_mutex_lock(&suspend_ack_lock);
	pthread_cond_broadcast(&suspend_cond);
	pthread_mutex_unlock(&suspend_ack_lock);
}
#endif

void cast_stopworld()
{
	int count, i;
	lock_stopworld(2);
	pthread_mutex_lock(&threadlistlock);
#if defined(__DARWIN__)
	cast_darwinstop();
#else
	count = cast_sendsignals(GC_signum1(), 0);
	for (i=0; i<count; i++)
		sem_wait(&suspend_ack);
#endif
	pthread_mutex_unlock(&threadlistlock);
}

void cast_startworld()
{
	pthread_mutex_lock(&threadlistlock);
#if defined(__DARWIN__)
	cast_darwinresume();
#elif defined(__MIPS__)
	cast_irixresume();
#else
	cast_sendsignals(GC_signum2(), -1);
#endif
	pthread_mutex_unlock(&threadlistlock);
	unlock_stopworld();
}

#if !defined(__DARWIN__)
static void sigsuspend_handler(ucontext_t *ctx)
{
	int sig;
	sigset_t sigs;
	
	thread_restartcriticalsection(ctx);

	/* Do as Boehm does. On IRIX a condition variable is used for wake-up
	   (not POSIX async-safe). */
#if defined(__IRIX__)
	pthread_mutex_lock(&suspend_ack_lock);
	sem_post(&suspend_ack);
	pthread_cond_wait(&suspend_cond, &suspend_ack_lock);
	pthread_mutex_unlock(&suspend_ack_lock);
#else
	sem_post(&suspend_ack);

	sig = GC_signum2();
	sigfillset(&sigs);
	sigdelset(&sigs, sig);
	sigsuspend(&sigs);
#endif
}

int cacao_suspendhandler(ucontext_t *ctx)
{
	if (stopworldwhere != 2)
		return 0;

	sigsuspend_handler(ctx);
	return 1;
}
#endif

static void setthreadobject(threadobject *thread)
{
#if !defined(HAVE___THREAD)
	pthread_setspecific(tkey_threadinfo, thread);
#else
	threadobj = thread;
#endif
}

static monitorLockRecord *dummyLR;

static void initPools();

/*
 * Initialize threads.
 */
void
initThreadsEarly()
{
#ifndef MUTEXSIM
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&compiler_mutex, &mutexattr);
	pthread_mutex_init(&tablelock, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);
#else
	pthread_mutex_init_rec(&compiler_mutex);
	pthread_mutex_init_rec(&tablelock);
#endif

	pthread_mutex_init(&threadlistlock, NULL);
	pthread_mutex_init(&stopworldlock, NULL);

	/* Allocate something so the garbage collector's signal handlers are  */
	/* installed. */
	heap_allocate(1, false, NULL);

	mainthreadobj = NEW(threadobject);
	memset(mainthreadobj, 0, sizeof(threadobject));
	mainthreadobj->info.tid = pthread_self();
#if !defined(HAVE___THREAD)
	pthread_key_create(&tkey_threadinfo, NULL);
#endif
	setthreadobject(mainthreadobj);
	initPools();

    criticaltree = avl_create(criticalcompare, NULL, NULL);
	thread_addstaticcritical();
	sem_init(&suspend_ack, 0, 0);

	/* Every newly created object's monitorPtr points here so we save a check
	 * against NULL */
	dummyLR = mem_alloc(sizeof(monitorLockRecord));
	dummyLR->o = NULL;
	dummyLR->ownerThread = NULL;
	dummyLR->waiting = false;
}

static pthread_attr_t threadattr;

static void freeLockRecordPools(lockRecordPool *);

void
initThreads(u1 *stackbottom)
{
	classinfo *threadclass;
	classinfo *threadgroupclass;
	java_lang_String *threadname;
	java_lang_Thread *mainthread;
	java_lang_ThreadGroup *threadgroup;
	threadobject *tempthread = mainthreadobj;
	methodinfo *method;

	threadclass = class_new(utf_new_char("java/lang/VMThread"));
	class_load(threadclass);
	class_link(threadclass);

	if (!threadclass)
		throw_exception_exit();

	freeLockRecordPools(mainthreadobj->ee.lrpool);
	/* This is kinda tricky, we grow the java.lang.Thread object so we can keep
	 * the execution environment there. No Thread object must have been created
	 * at an earlier time */
	threadclass->instancesize = sizeof(threadobject);

	/* Create a VMThread */
	mainthreadobj = (threadobject *) builtin_new(threadclass);

	if (!mainthreadobj)
		throw_exception_exit();

	FREE(tempthread, threadobject);
	initThread(&mainthreadobj->o);

	setthreadobject(mainthreadobj);

	initLocks();
	mainthreadobj->info.next = mainthreadobj;
	mainthreadobj->info.prev = mainthreadobj;

	threadname = javastring_new(utf_new_char("main"));

	/* Allocate and init ThreadGroup */
	threadgroupclass = class_new(utf_new_char("java/lang/ThreadGroup"));
	threadgroup =
		(java_lang_ThreadGroup *) native_new_and_init(threadgroupclass);

	if (!threadgroup)
		throw_exception_exit();

	/* Create a Thread */
	threadclass = class_new(utf_new_char("java/lang/Thread"));
	mainthread = (java_lang_Thread*) builtin_new(threadclass);
	mainthreadobj->o.thread = mainthread;

	if (!mainthread)
		throw_exception_exit();

	/* Call Thread constructor */
	method = class_resolveclassmethod(threadclass,
									  utf_new_char("<init>"),
									  utf_new_char("(Ljava/lang/VMThread;Ljava/lang/String;IZ)V"),
									  threadclass,
									  true);

	if (!method)
		throw_exception_exit();

	asm_calljavafunction(method, mainthread, mainthreadobj, threadname, (void*) 5);
	if (*exceptionptr)
		throw_exception_exit();

	mainthread->group = threadgroup;
	/* XXX This is a hack because the fourth argument was omitted */
	mainthread->daemon = false;

	/* Add mainthread to ThreadGroup */
	method = class_resolveclassmethod(threadgroupclass,
									  utf_new_char("addThread"),
									  utf_new_char("(Ljava/lang/Thread;)V"),
									  threadgroupclass,
									  true);

	if (!method)
		throw_exception_exit();

	asm_calljavafunction(method, threadgroup, mainthread, NULL, NULL);
	if (*exceptionptr)
		throw_exception_exit();

	setPriority(pthread_self(), 5);

	pthread_attr_init(&threadattr);
	pthread_attr_setdetachstate(&threadattr, PTHREAD_CREATE_DETACHED);
}

void initThread(java_lang_VMThread *t)
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

static void initThreadLocks(threadobject *);

typedef struct {
	threadobject *thread;
	sem_t *psem;
} startupinfo;

static void *threadstartup(void *t)
{
	startupinfo *startup = t;
	threadobject *thread = startup->thread;
	sem_t *psem = startup->psem;
	nativethread *info = &thread->info;
	threadobject *tnext;
	methodinfo *method;

	t = NULL;
#if defined(__DARWIN__)
	info->mach_thread = mach_thread_self();
#endif
	setthreadobject(thread);

	pthread_mutex_lock(&threadlistlock);
	info->prev = mainthreadobj;
	info->next = tnext = mainthreadobj->info.next;
	mainthreadobj->info.next = thread;
	tnext->info.prev = thread;
	pthread_mutex_unlock(&threadlistlock);

	initThreadLocks(thread);

	startup = NULL;
	sem_post(psem);

	setPriority(info->tid, thread->o.thread->priority);

	/* Find the run()V method and call it */
	method = class_resolveclassmethod(thread->o.header.vftbl->class,
									  utf_new_char("run"),
									  utf_new_char("()V"),
									  thread->o.header.vftbl->class,
									  true);

	/* if method != NULL, we had not exception */
	if (method) {
		asm_calljavafunction(method, thread, NULL, NULL, NULL);

	} else {
		throw_exception();
	}

	/* Allow lock record pools to be used by other threads. They cannot be
	 * deleted so we'd better not waste them. */
	freeLockRecordPools(thread->ee.lrpool);

	pthread_mutex_lock(&threadlistlock);
	info->next->info.prev = info->prev;
	info->prev->info.next = info->next;
	pthread_mutex_unlock(&threadlistlock);

	pthread_mutex_lock(&info->joinMutex);
	info->tid = 0;
	pthread_mutex_unlock(&info->joinMutex);
	pthread_cond_broadcast(&info->joinCond);

	return NULL;
}

void startThread(thread *t)
{
	nativethread *info = &((threadobject*) t->vmThread)->info;
	sem_t sem;
	startupinfo startup;

	startup.thread = (threadobject*) t->vmThread;
	startup.psem = &sem;

	sem_init(&sem, 0, 0);
	
	if (pthread_create(&info->tid, &threadattr, threadstartup, &startup))
		panic("pthread_create failed");

	/* Wait here until the thread has entered itself into the thread list */
	sem_wait(&sem);
	sem_destroy(&sem);
}

/* At the end of the program, we wait for all running non-daemon threads to die
 */

static threadobject *findNonDaemon(threadobject *thread)
{
	while (thread != mainthreadobj) {
		if (!thread->o.thread->daemon)
			return thread;
		thread = thread->info.prev;
	}

	return NULL;
}

void joinAllThreads()
{
	threadobject *thread;
	pthread_mutex_lock(&threadlistlock);
	while ((thread = findNonDaemon(mainthreadobj->info.prev)) != NULL) {
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

static void initLockRecord(monitorLockRecord *r, threadobject *t)
{
	r->lockCount = 1;
	r->ownerThread = t;
	r->queuers = 0;
	r->o = NULL;
	r->waiter = NULL;
	r->incharge = &dummyLR;
	r->waiting = false;
	sem_init(&r->queueSem, 0, 0);
	pthread_mutex_init(&r->resolveLock, NULL);
	pthread_cond_init(&r->resolveWait, NULL);
}

/* No lock record must ever be destroyed because there may still be references
 * to it.

static void destroyLockRecord(monitorLockRecord *r)
{
	sem_destroy(&r->queueSem);
	pthread_mutex_destroy(&r->resolveLock);
	pthread_cond_destroy(&r->resolveWait);
}
*/

void initLocks()
{
	initThreadLocks(mainthreadobj);
}

static void initThreadLocks(threadobject *thread)
{
	thread->ee.firstLR = NULL;
	thread->ee.lrpool = NULL;
	thread->ee.numlr = 0;
}

static lockRecordPool *allocNewLockRecordPool(threadobject *thread, int size)
{
	lockRecordPool *p = mem_alloc(sizeof(lockRecordPoolHeader) + sizeof(monitorLockRecord) * size);
	int i;

	p->header.size = size;
	for (i=0; i<size; i++) {
		initLockRecord(&p->lr[i], thread);
		p->lr[i].nextFree = &p->lr[i+1];
	}
	p->lr[i-1].nextFree = NULL;
	return p;
}

#define INITIALLOCKRECORDS 8

static pthread_mutex_t pool_lock;
static lockRecordPool *global_pool;

static void initPools()
{
	pthread_mutex_init(&pool_lock, NULL);
}

static lockRecordPool *allocLockRecordPool(threadobject *t, int size)
{
	pthread_mutex_lock(&pool_lock);
	if (global_pool) {
		int i;
		lockRecordPool *pool = global_pool;
		global_pool = pool->header.next;
		pthread_mutex_unlock(&pool_lock);

		for (i=0; i < pool->header.size; i++)
			pool->lr[i].ownerThread = t;
		
		return pool;
	}
	pthread_mutex_unlock(&pool_lock);

	return allocNewLockRecordPool(t, size);
}

static void freeLockRecordPools(lockRecordPool *pool)
{
	lockRecordPoolHeader *last;
	pthread_mutex_lock(&pool_lock);
	last = &pool->header;
	while (last->next)
		last = &last->next->header;
	last->next = global_pool;
	global_pool = pool;
	pthread_mutex_unlock(&pool_lock);
}

static monitorLockRecord *allocLockRecordSimple(threadobject *t)
{
	monitorLockRecord *r = t->ee.firstLR;

	if (!r) {
		int poolsize = t->ee.numlr ? t->ee.numlr * 2 : INITIALLOCKRECORDS;
		lockRecordPool *pool = allocLockRecordPool(t, poolsize);
		pool->header.next = t->ee.lrpool;
		t->ee.lrpool = pool;
		r = &pool->lr[0];
		t->ee.numlr += pool->header.size;
	}
	
	t->ee.firstLR = r->nextFree;
	return r;
}

static inline void recycleLockRecord(threadobject *t, monitorLockRecord *r)
{
	r->nextFree = t->ee.firstLR;
	t->ee.firstLR = r;
}

void initObjectLock(java_objectheader *o)
{
	o->monitorPtr = dummyLR;
}

static void queueOnLockRecord(monitorLockRecord *lr, java_objectheader *o)
{
	atomic_add(&lr->queuers, 1);
	MEMORY_BARRIER_AFTER_ATOMIC();
	while (lr->o == o)
		sem_wait(&lr->queueSem);
	atomic_add(&lr->queuers, -1);
}

static void freeLockRecord(monitorLockRecord *lr)
{
	int q;
	lr->o = NULL;
	MEMORY_BARRIER();
	q = lr->queuers;
	while (q--)
		sem_post(&lr->queueSem);
}

static inline void handleWaiter(monitorLockRecord *mlr, monitorLockRecord *lr)
{
	if (lr->waiting)
		mlr->waiter = lr;
}

monitorLockRecord *monitorEnter(threadobject *t, java_objectheader *o)
{
	for (;;) {
		monitorLockRecord *lr = o->monitorPtr;
		if (lr->o != o) {
			monitorLockRecord *nlr, *mlr = allocLockRecordSimple(t);
			mlr->o = o;
			if (mlr == lr) {
				MEMORY_BARRIER();
				nlr = o->monitorPtr;
				if (nlr == lr) {
					handleWaiter(mlr, lr);
					return mlr;
				}
			} else {
				if (lr->ownerThread != t)
					mlr->incharge = lr;
				MEMORY_BARRIER_BEFORE_ATOMIC();
				nlr = (void*) compare_and_swap((long*) &o->monitorPtr, (long) lr, (long) mlr);
			}
			if (nlr == lr) {
				if (mlr == lr || lr->o != o) {
					handleWaiter(mlr, lr);
					return mlr;
				}
				while (lr->o == o)
					queueOnLockRecord(lr, o);
				handleWaiter(mlr, lr);
				return mlr;
			}
			freeLockRecord(mlr);
			recycleLockRecord(t, mlr);
			queueOnLockRecord(nlr, o);
		} else {
			if (lr->ownerThread == t) {
				lr->lockCount++;
				return lr;
			}
			queueOnLockRecord(lr, o);
		}
	}
}

static void wakeWaiters(monitorLockRecord *lr)
{
	do {
		int q = lr->queuers;
		while (q--)
			sem_post(&lr->queueSem);
		lr = lr->waiter;
	} while (lr);
}

#define GRAB_LR(lr,t) \
    if (lr->ownerThread != t) { \
		lr = lr->incharge; \
	}

#define CHECK_MONITORSTATE(lr,t,mo,a) \
    if (lr->o != mo || lr->ownerThread != t) { \
		*exceptionptr = new_exception(string_java_lang_IllegalMonitorStateException); \
		a; \
	}

bool monitorExit(threadobject *t, java_objectheader *o)
{
	monitorLockRecord *lr = o->monitorPtr;
	GRAB_LR(lr, t);
	CHECK_MONITORSTATE(lr, t, o, return false);
	if (lr->lockCount > 1) {
		lr->lockCount--;
		return true;
	}
	if (lr->waiter) {
		monitorLockRecord *wlr = lr->waiter;
		if (o->monitorPtr != lr ||
			(void*) compare_and_swap((long*) &o->monitorPtr, (long) lr, (long) wlr) != lr)
		{
			monitorLockRecord *nlr = o->monitorPtr;
			nlr->waiter = wlr;
			STORE_ORDER_BARRIER();
		} else
			wakeWaiters(wlr);
		lr->waiter = NULL;
	}
	freeLockRecord(lr);
	recycleLockRecord(t, lr);
	return true;
}

static void removeFromWaiters(monitorLockRecord *lr, monitorLockRecord *wlr)
{
	do {
		if (lr->waiter == wlr) {
			lr->waiter = wlr->waiter;
			break;
		}
		lr = lr->waiter;
	} while (lr);
}

static inline bool timespec_less(const struct timespec *tv1, const struct timespec *tv2)
{
	return tv1->tv_sec < tv2->tv_sec || (tv1->tv_sec == tv2->tv_sec && tv1->tv_nsec < tv2->tv_nsec);
}

static bool timeIsEarlier(const struct timespec *tv)
{
	struct timeval tvnow;
	struct timespec tsnow;
	gettimeofday(&tvnow, NULL);
	tsnow.tv_sec = tvnow.tv_sec;
	tsnow.tv_nsec = tvnow.tv_usec * 1000;
	return timespec_less(&tsnow, tv);
}

bool waitWithTimeout(threadobject *t, monitorLockRecord *lr, struct timespec *wakeupTime)
{
	bool wasinterrupted;

	pthread_mutex_lock(&t->waitLock);
	t->isSleeping = true;
	if (wakeupTime->tv_sec || wakeupTime->tv_nsec)
		while (!t->interrupted && !t->signaled && timeIsEarlier(wakeupTime))
			pthread_cond_timedwait(&t->waitCond, &t->waitLock, wakeupTime);
	else
		while (!t->interrupted && !t->signaled)
			pthread_cond_wait(&t->waitCond, &t->waitLock);
	wasinterrupted = t->interrupted;
	t->interrupted = false;
	t->signaled = false;
	t->isSleeping = false;
	pthread_mutex_unlock(&t->waitLock);
	return wasinterrupted;
}

static void calcAbsoluteTime(struct timespec *tm, s8 millis, s4 nanos)
{
	if (millis || nanos) {
		struct timeval tv;
		long nsec;
		gettimeofday(&tv, NULL);
		tv.tv_sec += millis / 1000;
		millis %= 1000;
		nsec = tv.tv_usec * 1000 + (s4) millis * 1000000 + nanos;
		tm->tv_sec = tv.tv_sec + nsec / 1000000000;
		tm->tv_nsec = nsec % 1000000000;
	} else {
		tm->tv_sec = 0;
		tm->tv_nsec = 0;
	}
}

void monitorWait(threadobject *t, java_objectheader *o, s8 millis, s4 nanos)
{
	bool wasinterrupted;
	struct timespec wakeupTime;
	monitorLockRecord *mlr, *lr = o->monitorPtr;
	GRAB_LR(lr, t);
	CHECK_MONITORSTATE(lr, t, o, return);

	calcAbsoluteTime(&wakeupTime, millis, nanos);
	
	if (lr->waiter)
		wakeWaiters(lr->waiter);
	lr->waiting = true;
	STORE_ORDER_BARRIER();
	freeLockRecord(lr);
	wasinterrupted = waitWithTimeout(t, lr, &wakeupTime);
	mlr = monitorEnter(t, o);
	removeFromWaiters(mlr, lr);
	mlr->lockCount = lr->lockCount;
	lr->lockCount = 1;
	lr->waiting = false;
	lr->waiter = NULL;
	recycleLockRecord(t, lr);

	if (wasinterrupted)
		*exceptionptr = new_exception(string_java_lang_InterruptedException);
}

static void notifyOneOrAll(threadobject *t, java_objectheader *o, bool one)
{
	monitorLockRecord *lr = o->monitorPtr;
	GRAB_LR(lr, t);
	CHECK_MONITORSTATE(lr, t, o, return);
	do {
		threadobject *wthread;
		monitorLockRecord *wlr = lr->waiter;
		if (!wlr)
			break;
		wthread = wlr->ownerThread;
		pthread_mutex_lock(&wthread->waitLock);
		if (wthread->isSleeping)
			pthread_cond_signal(&wthread->waitCond);
		wthread->signaled = true;
		pthread_mutex_unlock(&wthread->waitLock);
		lr = wlr;
	} while (!one);
}

bool threadHoldsLock(threadobject *t, java_objectheader *o)
{
	monitorLockRecord *lr = o->monitorPtr;
	GRAB_LR(lr, t);
	return lr->o == o && lr->ownerThread == t;
}

void interruptThread(java_lang_VMThread *thread)
{
	threadobject *t = (threadobject*) thread;

	t->interrupted = true;
	pthread_mutex_lock(&t->waitLock);
	if (t->isSleeping)
		pthread_cond_signal(&t->waitCond);
	pthread_mutex_unlock(&t->waitLock);
}

bool interruptedThread()
{
	threadobject *t = (threadobject*) THREADOBJECT;
	bool intr = t->interrupted;
	t->interrupted = false;
	return intr;
}

bool isInterruptedThread(java_lang_VMThread *thread)
{
	threadobject *t = (threadobject*) thread;
	return t->interrupted;
}

void sleepThread(s8 millis, s4 nanos)
{
	bool wasinterrupted;
	threadobject *t = (threadobject*) THREADOBJECT;
	monitorLockRecord *lr;
	struct timespec wakeupTime;
	calcAbsoluteTime(&wakeupTime, millis, nanos);

	lr = allocLockRecordSimple(t);
	wasinterrupted = waitWithTimeout(t, lr, &wakeupTime);
	recycleLockRecord(t, lr);

	if (wasinterrupted)
		*exceptionptr = new_exception(string_java_lang_InterruptedException);
}

void yieldThread()
{
	sched_yield();
}

void setPriorityThread(thread *t, s4 priority)
{
	nativethread *info = &((threadobject*) t->vmThread)->info;
	setPriority(info->tid, priority);
}

void wait_cond_for_object(java_objectheader *o, s8 time, s4 nanos)
{
	threadobject *t = (threadobject*) THREADOBJECT;
	monitorWait(t, o, time, nanos);
}

void signal_cond_for_object(java_objectheader *o)
{
	threadobject *t = (threadobject*) THREADOBJECT;
	notifyOneOrAll(t, o, true);
}

void broadcast_cond_for_object(java_objectheader *o)
{
	threadobject *t = (threadobject*) THREADOBJECT;
	notifyOneOrAll(t, o, false);
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
 */
