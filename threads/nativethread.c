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
#include "locks.h"
#include "tables.h"
#include "native.h"
#include "loader.h"
#include "builtin.h"
#include "asmpart.h"
#include "toolbox/loging.h"
#include "toolbox/memory.h"
#include "toolbox/avl.h"
#include "mm/boehm.h"

#include "nat/java_lang_Object.h"
#include "nat/java_lang_Throwable.h"
#include "nat/java_lang_Thread.h"
#include "nat/java_lang_ThreadGroup.h"

#include <pthread.h>
#include <semaphore.h>

#if defined(__LINUX__)
#define GC_LINUX_THREADS
#include "../mm/boehm-gc/include/gc.h"
#endif

#include "machine-instr.h"

static struct avl_table *criticaltree;
static threadobject *mainthreadobj;

#ifndef HAVE___THREAD
pthread_key_t tkey_threadinfo;
#else
__thread threadobject *threadobj;
#endif

static pthread_mutex_t cast_mutex = PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;
static pthread_mutex_t compiler_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static pthread_mutex_t tablelock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

void cast_lock()
{
	pthread_mutex_lock(&cast_mutex);
}

void cast_unlock()
{
	pthread_mutex_unlock(&cast_mutex);
}

void compiler_lock()
{
	pthread_mutex_lock(&compiler_mutex);
}

void compiler_unlock()
{
	pthread_mutex_unlock(&compiler_mutex);
}

void tables_lock()
{
    pthread_mutex_lock(&tablelock);
}

void tables_unlock()
{
    pthread_mutex_unlock(&tablelock);
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

static u1 *thread_checkcritical(u1 *mcodeptr)
{
	const threadcritnode *n = findcritical(mcodeptr);
	return (n && mcodeptr < n->mcodeend) ? n->mcodebegin : NULL;
}

static pthread_mutex_t stopworldlock = PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;
volatile int stopworldwhere;

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

static sem_t suspend_ack;

static void sigsuspend_handler(struct sigcontext *ctx)
{
	int sig;
	sigset_t sigs;
	void *critical;
	
#ifdef __I386__
	if ((critical = thread_checkcritical((void*) ctx->eip)) != NULL)
		ctx->eip = (long) critical;
#endif

	sem_post(&suspend_ack);

	sig = GC_signum2();
	sigfillset(&sigs);
	sigdelset(&sigs, sig);
	sigsuspend(&sigs);
}

int cacao_suspendhandler(struct sigcontext *ctx)
{
	if (stopworldwhere != 2)
		return 0;

	sigsuspend_handler(ctx);
	return 1;
}

static void setthreadobject(threadobject *thread)
{
#if !defined(HAVE___THREAD)
	pthread_setspecific(tkey_threadinfo, thread);
#else
	threadobj = thread;
#endif
}

/*
 * Initialize threads.
 */
void
initThreadsEarly()
{
	struct sigaction sa;
//	int sig1 = GC_signum1(), sig2 = GC_signum2();

	/* Allocate something so the garbage collector's signal handlers are  */
	/* initalized. */
	heap_allocate(1, false, NULL);

	mainthreadobj = NEW(threadobject);
	memset(mainthreadobj, 0, sizeof(threadobject));
#if !defined(HAVE___THREAD)
	pthread_key_create(&tkey_threadinfo, NULL);
#endif
	setthreadobject(mainthreadobj);

    criticaltree = avl_create(criticalcompare, NULL, NULL);
}

static void freeLockRecordPools(lockRecordPool *);

void
initThreads(u1 *stackbottom)
{
	classinfo *threadclass;
	java_lang_Thread *mainthread;
	threadobject *tempthread = mainthreadobj;

//	exit(1);

	threadclass = loader_load_sysclass(NULL,utf_new_char("java/lang/Thread"));
	assert(threadclass);
	freeLockRecordPools(mainthreadobj->ee.lrpool);
	threadclass->instancesize = sizeof(threadobject);
	mainthreadobj = (threadobject*) builtin_new(threadclass);
	assert(mainthreadobj);
	FREE(tempthread, threadobject);
	initThread(&mainthreadobj->o);

#if !defined(HAVE___THREAD)
	pthread_setspecific(tkey_threadinfo, mainthreadobj);
#else
	threadobj = mainthreadobj;
#endif

	mainthread = &mainthreadobj->o;
	initLocks();
	mainthreadobj->info.next = mainthreadobj;
	mainthreadobj->info.prev = mainthreadobj;

	mainthread->name=javastring_new(utf_new_char("main"));

	/* Allocate and init ThreadGroup */
	mainthread->group = (java_lang_ThreadGroup*)native_new_and_init(loader_load(utf_new_char("java/lang/ThreadGroup")));
	assert(mainthread->group != 0);
}

void initThread(java_lang_Thread *t)
{
	nativethread *info = &((threadobject*) t)->info;
	pthread_mutex_init(&info->joinMutex, NULL);
	pthread_cond_init(&info->joinCond, NULL);
}

static pthread_mutex_t threadlistlock = PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;

static void initThreadLocks(threadobject *);

static void *threadstartup(void *t)
{
	threadobject *thread = t;
	nativethread *info = &thread->info;
	threadobject *tnext;
	methodinfo *method;

	setthreadobject(thread);

	pthread_mutex_lock(&threadlistlock);
	info->prev = mainthreadobj;
	info->next = tnext = mainthreadobj->info.next;
	mainthreadobj->info.next = t;
	tnext->info.prev = t;
	pthread_mutex_unlock(&threadlistlock);

	initThreadLocks(thread);

	/* Find the run()V method and call it */
	method = class_findmethod(thread->o.header.vftbl->class,
							  utf_new_char("run"), utf_new_char("()V"));
	if (method == 0)
		panic("Cannot find method \'void run ()\'");

	asm_calljavafunction(method, thread, NULL, NULL, NULL);

    if (info->_exceptionptr) {
        utf_display((info->_exceptionptr)->vftbl->class->name);
        printf("\n");
    }

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

void startThread(threadobject *t)
{
	nativethread *info = &t->info;
	pthread_create(&info->tid, NULL, threadstartup, t);
	pthread_detach(info->tid);
}

void joinAllThreads()
{
	pthread_mutex_lock(&threadlistlock);
	while (mainthreadobj->info.prev != mainthreadobj) {
		nativethread *info = &mainthreadobj->info.prev->info;
		pthread_mutex_lock(&info->joinMutex);
		pthread_mutex_unlock(&threadlistlock);
		if (info->tid)
			pthread_cond_wait(&info->joinCond, &info->joinMutex);
		pthread_mutex_unlock(&info->joinMutex);
		pthread_mutex_lock(&threadlistlock);
	}
	pthread_mutex_unlock(&threadlistlock);
}

bool aliveThread(java_lang_Thread *t)
{
	return ((threadobject*) t)->info.tid != 0;
}

void sleepThread(s8 millis)
{
	struct timespec tv;
	tv.tv_sec = millis / 1000;
	tv.tv_nsec = millis % 1000 * 1000000;
	do { } while (nanosleep(&tv, &tv) == EINTR);
}

void yieldThread()
{
	sched_yield();
}

static void timedCondWait(pthread_cond_t *cond, pthread_mutex_t *mutex, s8 millis)
{
	struct timeval now;
	struct timespec desttime;
	gettimeofday(&now, NULL);
	desttime.tv_sec = millis / 1000;
	desttime.tv_nsec = millis % 1000 * 1000000;
	pthread_cond_timedwait(cond, mutex, &desttime);
}


#define NEUTRAL 0
#define LOCKED 1
#define WAITERS 2
#define BUSY 3

static void initExecutionEnvironment(ExecEnvironment *ee)
{
	pthread_mutex_init(&ee->metaLockMutex, NULL);
	pthread_cond_init(&ee->metaLockCond, NULL);
	pthread_mutex_init(&ee->monitorLockMutex, NULL);
	pthread_cond_init(&ee->monitorLockCond, NULL);
}

static void initLockRecord(monitorLockRecord *r, threadobject *t)
{
	r->owner = t;
	r->lockCount = 1;
	r->queue = NULL;
}

void initLocks()
{
	initThreadLocks(mainthreadobj);
}

static void initThreadLocks(threadobject *thread)
{
	int i;

	initExecutionEnvironment(&thread->ee);
	for (i=0; i<INITIALLOCKRECORDS; i++) {
		monitorLockRecord *r = &thread->ee.lr[i];
		initLockRecord(r, thread);
		r->nextFree = &thread->ee.lr[i+1];
	}
	thread->ee.lr[i-1].nextFree = NULL;
	thread->ee.firstLR = &thread->ee.lr[0];
}

static inline int lockState(long r)
{
	return r & 3;
}

static inline void *lockRecord(long r)
{
	return (void*) (r & ~3);
}

static inline long makeLockBits(void *r, long l)
{
	return ((long) r) | l;
}

static lockRecordPool *allocLockRecordPool(threadobject *thread, int size)
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

static void freeLockRecordPools(lockRecordPool *pool)
{
	while (pool) {
		lockRecordPool *n = pool->header.next;
		mem_free(pool, sizeof(lockRecordPoolHeader) + sizeof(monitorLockRecord) * pool->header.size);
		pool = n;
	}
}

static monitorLockRecord *allocLockRecord(threadobject *t)
{
	monitorLockRecord *r = t->ee.firstLR;

	if (!r) {
		int poolsize = t->ee.lrpool ? t->ee.lrpool->header.size * 2 : INITIALLOCKRECORDS * 2;
		lockRecordPool *pool = allocLockRecordPool(t, poolsize);
		pool->header.next = t->ee.lrpool;
		t->ee.lrpool = pool;
		r = &pool->lr[0];
	}
	
	t->ee.firstLR = r->nextFree;
	return r;
}

static void recycleLockRecord(threadobject *t, monitorLockRecord *r)
{
	r->nextFree = t->ee.firstLR;
	t->ee.firstLR = r;
}

static monitorLockRecord *appendToQueue(monitorLockRecord *queue, monitorLockRecord *lr)
{
	monitorLockRecord *queuestart = queue;
	if (!queue)
		return lr;
	while (queue->queue)
		queue = queue->queue;
	queue->queue = lr;
	return queuestart;
}

static monitorLockRecord *moveMyLRToFront(threadobject *t, monitorLockRecord *lr)
{
	monitorLockRecord *pred = NULL;
	monitorLockRecord *firstLR = lr;
	while (lr->owner != t) {
		pred = lr;
		lr = lr->queue;
	}
	if (!pred)
		return lr;
	pred->queue = lr->queue;
	lr->queue = firstLR;
	lr->storedBits = firstLR->storedBits;
	return lr;
}

static long getMetaLockSlow(threadobject *t, long predBits);
static void releaseMetaLockSlow(threadobject *t, long releaseBits);

static long getMetaLock(threadobject *t, java_objectheader *o)
{
	long busyBits = makeLockBits(t, BUSY);
	long lockBits = atomic_swap(&o->monitorBits, busyBits);
	return lockState(lockBits) != BUSY ? lockBits : getMetaLockSlow(t, lockBits);
}

static long getMetaLockSlow(threadobject *t, long predBits)
{
	long bits;
	threadobject *pred = lockRecord(predBits);
	pthread_mutex_lock(&pred->ee.metaLockMutex);
	if (!pred->ee.bitsForGrab) {
		pred->ee.succ = t;
		do {
			pthread_cond_wait(&pred->ee.metaLockCond, &pred->ee.metaLockMutex);
		} while (!t->ee.gotMetaLockSlow);
		t->ee.gotMetaLockSlow = false;
		bits = t->ee.metaLockBits;
	} else {
		bits = pred->ee.metaLockBits;
		pred->ee.bitsForGrab = false;
		pthread_cond_signal(&pred->ee.metaLockCond);
	}
	pthread_mutex_unlock(&pred->ee.metaLockMutex);
	return bits;
}

static void releaseMetaLock(threadobject *t, java_objectheader *o, long releaseBits)
{
	long busyBits = makeLockBits(t, BUSY);
	long lockBits = compare_and_swap(&o->monitorBits, busyBits, releaseBits);
	
	if (lockBits != busyBits)
		releaseMetaLockSlow(t, releaseBits);
}

static void releaseMetaLockSlow(threadobject *t, long releaseBits)
{
	pthread_mutex_lock(&t->ee.metaLockMutex);
	if (t->ee.succ) {
		assert(!t->ee.succ->ee.bitsForGrab);
		assert(!t->ee.bitsForGrab);
		assert(!t->ee.succ->ee.gotMetaLockSlow);
		t->ee.succ->ee.metaLockBits = releaseBits;
		t->ee.succ->ee.gotMetaLockSlow = true;
		t->ee.succ = NULL;
		pthread_cond_signal(&t->ee.metaLockCond);
	} else {
		t->ee.metaLockBits = releaseBits;
		t->ee.bitsForGrab = true;
		do {
			pthread_cond_wait(&t->ee.metaLockCond, &t->ee.metaLockMutex);
		} while (t->ee.bitsForGrab);
	}
	pthread_mutex_unlock(&t->ee.metaLockMutex);
}

static void monitorEnterSlow(threadobject *t, java_objectheader *o, long r);

void monitorEnter(threadobject *t, java_objectheader *o)
{
	long r = getMetaLock(t, o);
	int state = lockState(r);

	if (state == NEUTRAL) {
		monitorLockRecord *lr = allocLockRecord(t);
		lr->storedBits = r;
		releaseMetaLock(t, o, makeLockBits(lr, LOCKED));
	} else if (state == LOCKED) {
		monitorLockRecord *ownerLR = lockRecord(r);
		if (ownerLR->owner == t) {
			ownerLR->lockCount++;
			releaseMetaLock(t, o, r);
		} else {
			monitorLockRecord *lr = allocLockRecord(t);
			ownerLR->queue = appendToQueue(ownerLR->queue, lr);
			monitorEnterSlow(t, o, r);
		}
	} else if (state == WAITERS) {
		monitorLockRecord *lr = allocLockRecord(t);
		monitorLockRecord *firstWaiterLR = lockRecord(r);
		lr->queue = firstWaiterLR;
		lr->storedBits = firstWaiterLR->storedBits;
		releaseMetaLock(t, o, makeLockBits(lr, LOCKED));
	}
}

static void monitorEnterSlow(threadobject *t, java_objectheader *o, long r)
{
	monitorLockRecord *lr;
	monitorLockRecord *firstWaiterLR;
	while (lockState(r) == LOCKED) {
		pthread_mutex_lock(&t->ee.monitorLockMutex);
		releaseMetaLock(t, o, r);
		pthread_cond_wait(&t->ee.monitorLockCond, &t->ee.monitorLockMutex);
		pthread_mutex_unlock(&t->ee.monitorLockMutex);
		r = getMetaLock(t, o);
	}
	assert(lockState(r) == WAITERS);
	lr = moveMyLRToFront(t, lockRecord(r));
	releaseMetaLock(t, o, makeLockBits(lr, LOCKED));
}

static void monitorExitSlow(threadobject *t, java_objectheader *o, monitorLockRecord *lr);

void monitorExit(threadobject *t, java_objectheader *o)
{
	long r = getMetaLock(t, o);
	monitorLockRecord *ownerLR = lockRecord(r);
	int state = lockState(r);
	if (state == LOCKED && ownerLR->owner == t) {
		assert(ownerLR->lockCount >= 1);
		if (ownerLR->lockCount == 1) {
			if (ownerLR->queue == NULL) {
				assert(lockState(ownerLR->storedBits) == NEUTRAL);
				releaseMetaLock(t, o, ownerLR->storedBits);
			} else {
				ownerLR->queue->storedBits = ownerLR->storedBits;
				monitorExitSlow(t, o, ownerLR->queue);
				ownerLR->queue = NULL;
			}
			recycleLockRecord(t, ownerLR);
		} else {
			ownerLR->lockCount--;
			releaseMetaLock(t, o, r);
		}
	} else {
		releaseMetaLock(t, o, r);
		panic("Illegal monitor exception");
	}
}

static threadobject *wakeupEE(monitorLockRecord *lr)
{
	while (lr->queue && lr->queue->owner->ee.isWaitingForNotify)
		lr = lr->queue;
	return lr->owner;
}

static void monitorExitSlow(threadobject *t, java_objectheader *o, monitorLockRecord *lr)
{
	threadobject *wakeEE = wakeupEE(lr);
	if (wakeEE) {
		pthread_mutex_lock(&wakeEE->ee.monitorLockMutex);
		releaseMetaLock(t, o, makeLockBits(lr, WAITERS));
		pthread_cond_signal(&wakeEE->ee.monitorLockCond);
		pthread_mutex_unlock(&wakeEE->ee.monitorLockMutex);
	} else {
		releaseMetaLock(t, o, makeLockBits(lr, WAITERS));
	}
}

void monitorWait(threadobject *t, java_objectheader *o, s8 millis)
{
	long r = getMetaLock(t, o);
	monitorLockRecord *ownerLR = lockRecord(r);
	int state = lockState(r);
	if (state == LOCKED && ownerLR->owner == t) {
		pthread_mutex_lock(&t->ee.monitorLockMutex);
		t->ee.isWaitingForNotify = true;
		monitorExitSlow(t, o, ownerLR);
		if (millis == -1)
			pthread_cond_wait(&t->ee.monitorLockCond, &t->ee.monitorLockMutex);
		else
			timedCondWait(&t->ee.monitorLockCond, &t->ee.monitorLockMutex, millis);
		t->ee.isWaitingForNotify = false;
		pthread_mutex_unlock(&t->ee.monitorLockMutex);
		r = getMetaLock(t, o);
		monitorEnterSlow(t, o, r);
	} else {
		releaseMetaLock(t, o, r);
		panic("Illegal monitor exception");
	}
}

static void notifyOneOrAll(threadobject *t, java_objectheader *o, bool one)
{
	long r = getMetaLock(t, o);
	monitorLockRecord *ownerLR = lockRecord(r);
	int state = lockState(r);
	if (state == LOCKED && ownerLR->owner == t) {
		monitorLockRecord *q = ownerLR->queue;
		while (q) {
			if (q->owner->ee.isWaitingForNotify) {
				q->owner->ee.isWaitingForNotify = false;
				if (one)
					break;
			}
			q = q->queue;
		}
		releaseMetaLock(t, o, r);
	} else {
		releaseMetaLock(t, o, r);
		panic("Illegal monitor exception");
	}
}

void wait_cond_for_object(java_objectheader *o, s8 time)
{
	threadobject *t = (threadobject*) THREADOBJECT;
	monitorWait(t, o, time);
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
