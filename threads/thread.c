/* -*- mode: c; tab-width: 4; c-basic-offset: 4 -*- */
/*
 * thread.c
 * Thread support.
 *
 * Copyright (c) 1996 T. J. Wilkinson & Associates, London, UK.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * Written by Tim Wilkinson <tim@tjwassoc.demon.co.uk>, 1996.
 */

#include <assert.h>

#include <sys/types.h>
#include <sys/mman.h>                   /* for mprotect */
#include <unistd.h>

#include "thread.h"
#include "locks.h"
#include "sysdep/defines.h"
#include "sysdep/threads.h"

#include "../tables.h"
#include "../native.h"
#include "../loader.h"
#include "../builtin.h"
#include "../asmpart.h"

static classinfo *class_java_lang_ThreadDeath;

#if 1
#define DBG(s)
#define SDBG(s)
#else
#define DBG(s)                 s
#define SDBG(s)                s
#endif

#if defined(USE_INTERNAL_THREADS)

thread* currentThread = NULL;
thread* mainThread;
thread* threadQhead[MAX_THREAD_PRIO + 1];
thread* threadQtail[MAX_THREAD_PRIO + 1];

thread* liveThreads = NULL;
thread* alarmList;

int blockInts;
bool needReschedule;

ctx contexts[MAXTHREADS];

/* Number of threads alive, also counting daemons */
static int talive;

/* Number of daemon threads alive */
static int tdaemon;

static void firstStartThread(void);

void reschedule(void);

/* Setup default thread stack size - this can be overwritten if required */
int threadStackSize = THREADSTACKSIZE;

static thread* startDaemon(void* func, char* nm, int stackSize);

/*
 * Allocate the stack for a thread
 */
void
allocThreadStack (thread *tid, int size)
{
    int pageSize = getpagesize(),
		result;
    unsigned long pageBegin;

    CONTEXT(tid).stackMem = malloc(size + 2 * pageSize);
    assert(CONTEXT(tid).stackMem != 0);
    CONTEXT(tid).stackEnd = CONTEXT(tid).stackMem + size + 2 * pageSize;
    
    pageBegin = (unsigned long)(CONTEXT(tid).stackMem) + pageSize - 1;
    pageBegin = pageBegin - pageBegin % pageSize;

    result = mprotect((void*)pageBegin, pageSize, PROT_NONE);
    assert(result == 0);

    CONTEXT(tid).stackBase = (u1*)pageBegin + pageSize;
}

/*
 * Free the stack for a thread
 */
void
freeThreadStack (thread *tid)
{
    if (!(CONTEXT(tid).flags & THREAD_FLAGS_NOSTACKALLOC))
    {
		int pageSize = getpagesize(),
			result;
		unsigned long pageBegin;

		pageBegin = (unsigned long)(CONTEXT(tid).stackMem) + pageSize - 1;
		pageBegin = pageBegin - pageBegin % pageSize;
	
		result = mprotect((void*)pageBegin, pageSize,
						  PROT_READ | PROT_WRITE | PROT_EXEC);
		assert(result == 0);

		free(CONTEXT(tid).stackMem);
    }
    CONTEXT(tid).stackMem = 0;
    CONTEXT(tid).stackBase = 0;
    CONTEXT(tid).stackEnd = 0;
}

/*
 * Initialize threads.
 */
void
initThreads(u1 *stackbottom)
{
	thread *the_main_thread;
    int i;

    initLocks();

    for (i = 0; i < MAXTHREADS; ++i)
		contexts[i].free = true;

    /* Allocate a thread to be the main thread */
    liveThreads = the_main_thread = (thread*)builtin_new(loader_load(unicode_new_char("java/lang/Thread")));
    assert(the_main_thread != 0);
	heap_addreference((void **) &liveThreads);
    
    the_main_thread->PrivateInfo = 1;
    CONTEXT(the_main_thread).free = false;

    the_main_thread->name = javastring_new(unicode_new_char("main"));
    the_main_thread->priority = NORM_THREAD_PRIO;
    CONTEXT(the_main_thread).priority = (u1)the_main_thread->priority;
    CONTEXT(the_main_thread).exceptionptr = 0;
    the_main_thread->next = 0;
    CONTEXT(the_main_thread).status = THREAD_SUSPENDED;
    CONTEXT(the_main_thread).stackBase = CONTEXT(the_main_thread).stackEnd = stackbottom;
    THREADINFO(&CONTEXT(the_main_thread));

    DBG( printf("main thread %p base %p end %p\n", 
				the_main_thread,
				CONTEXT(the_main_thread).stackBase,
				CONTEXT(the_main_thread).stackEnd); );

	CONTEXT(the_main_thread).flags = THREAD_FLAGS_NOSTACKALLOC;
	CONTEXT(the_main_thread).nextlive = 0;
	the_main_thread->single_step = 0;
	the_main_thread->daemon = 0;
	the_main_thread->stillborn = 0;
	the_main_thread->target = 0;
	the_main_thread->interruptRequested = 0;
	the_main_thread->group =
		(threadGroup*)builtin_new(loader_load(unicode_new_char("java/lang/ThreadGroup")));
	/* we should call the constructor */
	assert(the_main_thread->group != 0);

	talive++;

	/* Load exception classes */
	class_java_lang_ThreadDeath = loader_load(unicode_new_char("java/lang/ThreadDeath"));

	DBG( fprintf(stderr, "finishing initThreads\n"); );

    mainThread = currentThread = the_main_thread;

	/* Add thread into runQ */
	iresumeThread(mainThread);

	assert(blockInts == 0);
}

/*
 * Start a new thread running.
 */
void
startThread (thread* tid)
{
    int i;

    /* Allocate a stack context */
    for (i = 0; i < MAXTHREADS; ++i)
		if (contexts[i].free)
			break;

    if (i == MAXTHREADS)
		panic("Too many threads");

    tid->PrivateInfo = i + 1;
    CONTEXT(tid).free = false;
    CONTEXT(tid).nextlive = liveThreads;
    liveThreads = tid;
    allocThreadStack(tid, threadStackSize);
    CONTEXT(tid).usedStackTop = CONTEXT(tid).stackBase;
    CONTEXT(tid).flags = THREAD_FLAGS_GENERAL;
    CONTEXT(tid).status = THREAD_SUSPENDED;
    CONTEXT(tid).priority = (u1)tid->priority;
    CONTEXT(tid).exceptionptr = 0;

    /* Construct the initial restore point. */
    THREADINIT((&CONTEXT(tid)), firstStartThread);

    DBG( printf("new thread %p base %p end %p\n",
				tid, CONTEXT(tid).stackBase,
				CONTEXT(tid).stackEnd); );

	talive++;
	if (tid->daemon)
		tdaemon++;

	/* Add thread into runQ */
	iresumeThread(tid);
}

/*
 * Start a daemon thread.
 */
static thread*
startDaemon(void* func, char* nm, int stackSize)
{
    thread* tid;
    int i;

    DBG( printf("startDaemon %s\n", nm); );

	tid = (thread*)builtin_new(loader_load(unicode_new_char("java/lang/Thread")));
	assert(tid != 0);

	for (i = 0; i < MAXTHREADS; ++i)
		if (contexts[i].free)
			break;
	if (i == MAXTHREADS)
		panic("Too many threads");

	tid->PrivateInfo = i + 1;
	CONTEXT(tid).free = false;
	tid->name = 0;          /* for the moment */
	tid->priority = MAX_THREAD_PRIO;
	CONTEXT(tid).priority = (u1)tid->priority;
	tid->next = 0;
	CONTEXT(tid).status = THREAD_SUSPENDED;

	allocThreadStack(tid, stackSize);
	tid->single_step = 0;
	tid->daemon = 1;
	tid->stillborn = 0;
	tid->target = 0;
	tid->interruptRequested = 0;
	tid->group = 0;

	/* Construct the initial restore point. */
	THREADINIT((&CONTEXT(tid)), func);

	talive++;
	tdaemon++;

	return tid;
}

/*
 * All threads start here.
 */
static void
firstStartThread(void)
{
    methodinfo *method;

    DBG( printf("firstStartThread %p\n", currentThread); );

	/* Every thread starts with the interrupts off */
	intsRestore();
	assert(blockInts == 0);

	/* Find the run()V method and call it */
	method = class_findmethod(currentThread->header.vftbl->class,
							  unicode_new_char("run"), unicode_new_char("()V"));
	if (method == 0)
		panic("Cannot find method \'void run ()\'");
	asm_calljavamethod(method, currentThread, NULL, NULL, NULL);

	killThread(0);
	assert("Thread returned from killThread" == 0);
}

/*
 * Resume a thread running.
 * This routine has to be called only from locations which ensure
 * run / block queue consistency. There is no check for illegal resume
 * conditions (like explicitly resuming an IO blocked thread). There also
 * is no update of any blocking queue. Both has to be done by the caller
 */
void
iresumeThread(thread* tid)
{
    DBG( printf("resumeThread %p\n", tid); );

	intsDisable();

	if (CONTEXT(tid).status != THREAD_RUNNING)
	{
		CONTEXT(tid).status = THREAD_RUNNING;

		DBG( fprintf(stderr, "prio is %d\n", CONTEXT(tid).priority); );

		/* Place thread on the end of its queue */
		if (threadQhead[CONTEXT(tid).priority] == 0) {
			threadQhead[CONTEXT(tid).priority] = tid;
			threadQtail[CONTEXT(tid).priority] = tid;
			if (CONTEXT(tid).priority
				> CONTEXT(currentThread).priority)
				needReschedule = true;
		}
		else
		{
			threadQtail[CONTEXT(tid).priority]->next = tid;
			threadQtail[CONTEXT(tid).priority] = tid;
		}
		tid->next = 0;
	}
	SDBG( else { printf("Re-resuming %p\n", tid); } );

	intsRestore();
}

/*
 * Yield process to another thread of equal priority.
 */
void
yieldThread()
{
    intsDisable();

    if (threadQhead[CONTEXT(currentThread).priority]
		!= threadQtail[CONTEXT(currentThread).priority])
    {
		/* Get the next thread and move me to the end */
		threadQhead[CONTEXT(currentThread).priority] = currentThread->next;
		threadQtail[CONTEXT(currentThread).priority]->next = currentThread;
		threadQtail[CONTEXT(currentThread).priority] = currentThread;
		currentThread->next = 0;
		needReschedule = true;
    }

    intsRestore();
}

/*
 * Explicit request by user to resume a thread
 * The definition says that it is just legal to call this after a preceeding
 * suspend (which got through). If the thread was blocked for some other
 * reason (either sleep or IO or a muxSem), we simply can't do it
 * We use a new thread flag THREAD_FLAGS_USER_SUSPEND for this purpose
 * (which is set by suspendThread(.))
 */
void
resumeThread(thread* tid)
{
    if ((CONTEXT(tid).flags & THREAD_FLAGS_USER_SUSPEND) != 0)
    {
		intsDisable();
		CONTEXT(tid).flags &= ~THREAD_FLAGS_USER_SUSPEND;
		iresumeThread(tid);
		intsRestore();
    }
}

/*
 * Suspend a thread.
 * This is an explicit user request to suspend the thread - the counterpart
 * for resumeThreadRequest(.). It is JUST called by the java method
 * Thread.suspend()
 * What makes it distinct is the fact that the suspended thread is not contained
 * in any block queue. Without a special flag (indicating the user suspend), we
 * can't check s suspended thread for this condition afterwards (which is
 * required by resumeThreadRequest()). The new thread flag
 * THREAD_FLAGS_USER_SUSPEND is used for this purpose.
 */
void
suspendThread(thread* tid)
{
    thread** ntid;

    intsDisable();

    if (CONTEXT(tid).status != THREAD_SUSPENDED)
    {
		CONTEXT(tid).status = THREAD_SUSPENDED;
		
		/*
		 * This is used to indicate the explicit suspend condition
		 * required by resumeThreadRequest()
		 */
		CONTEXT(tid).flags |= THREAD_FLAGS_USER_SUSPEND;

		for (ntid = &threadQhead[CONTEXT(tid).priority];
			 *ntid != 0;
			 ntid = &(*ntid)->next)
		{
			if (*ntid == tid)
			{
				*ntid = tid->next;
				tid->next = 0;
				if (tid == currentThread)
				{
					reschedule();
				}
				break;
			}
		}
    }
	SDBG( else { printf("Re-suspending %p\n", tid); } );

	intsRestore();
}

/*
 * Suspend a thread on a queue.
 */
void
suspendOnQThread(thread* tid, thread** queue)
{
    thread** ntid;

	DBG( printf("suspendOnQThread %p %p\n", tid, queue); );

	assert(blockInts == 1);

	if (CONTEXT(tid).status != THREAD_SUSPENDED)
	{
		CONTEXT(tid).status = THREAD_SUSPENDED;

		for (ntid = &threadQhead[CONTEXT(tid).priority];
			 *ntid != 0;
			 ntid = &(*ntid)->next)
		{
			if (*ntid == tid)
			{
				*ntid = tid->next;
				/* Insert onto head of lock wait Q */
				tid->next = *queue;
				*queue = tid;
				if (tid == currentThread)
				{
					DBG( fprintf(stderr, "suspending %p (cur=%p) with prio %d\n",
								 tid, currentThread, CONTEXT(tid).priority); );
					reschedule();
				}
				break;
			}
		}
	}
	SDBG( else { printf("Re-suspending %p on %p\n", tid, *queue); } );
}

/*
 * Kill thread.
 */
void
killThread(thread* tid)
{
    thread** ntid;

    intsDisable();

    /* A null tid means the current thread */
    if (tid == 0)
    {
		tid = currentThread;
    }

	DBG( printf("killThread %p\n", tid); );

	if (CONTEXT(tid).status != THREAD_DEAD)
	{
		/* Get thread off runq (if it needs it) */
		if (CONTEXT(tid).status == THREAD_RUNNING)
		{
			for (ntid = &threadQhead[CONTEXT(tid).priority];
				 *ntid != 0;
				 ntid = &(*ntid)->next)
			{
				if (*ntid == tid)
				{
					*ntid = tid->next;
					break;
				}
			}
		}

		CONTEXT(tid).status = THREAD_DEAD;
		talive--;
		if (tid->daemon) {
			tdaemon--;
		}

		/* If we only have daemons left, then everyone is dead. */
		if (talive == tdaemon) {
			/* Am I suppose to close things down nicely ?? */
			exit(0);
		}

		/* Notify on the object just in case anyone is waiting */
		internal_lock_mutex_for_object(&tid->header);
		internal_broadcast_cond_for_object(&tid->header);
		internal_unlock_mutex_for_object(&tid->header);

		/* Remove thread from live list to it can be garbaged */
		for (ntid = &liveThreads;
			 *ntid != 0;
			 ntid = &(CONTEXT((*ntid)).nextlive))
		{
			if (tid == (*ntid))
			{
				(*ntid) = CONTEXT(tid).nextlive;
				break;
			}
		}

		/* Free stack */
		freeThreadStack(tid);

		/* free context */
		CONTEXT(tid).free = true;

		/* Run something else */
		needReschedule = true;
	}
	intsRestore();
}

/*
 * Change thread priority.
 */
void
setPriorityThread(thread* tid, int prio)
{
    thread** ntid;

    if (tid->PrivateInfo == 0) {
		tid->priority = prio;
		return;
    }

    if (CONTEXT(tid).status == THREAD_SUSPENDED) {
		CONTEXT(tid).priority = (u8)prio;
		return;
    }

    intsDisable();

    /* Remove from current thread list */
    for (ntid = &threadQhead[CONTEXT(tid).priority]; *ntid != 0; ntid = &(*ntid)->next) {
		if (*ntid == tid) {
			*ntid = tid->next;
			break;
		}
    }

    /* Insert onto a new one */
    tid->priority = prio;
    CONTEXT(tid).priority = (u8)tid->priority;
    if (threadQhead[prio] == 0) {
		threadQhead[prio] = tid;
		threadQtail[prio] = tid;
		if (prio > CONTEXT(currentThread).priority) {
			needReschedule = true;
		}
    }
    else {
		threadQtail[prio]->next = tid;
		threadQtail[prio] = tid;
    }
    tid->next = 0;

    intsRestore();
}

/*
 * Is this thread alive?
 */
bool
aliveThread(thread* tid)
{
    if (tid->PrivateInfo != 0 && CONTEXT(tid).status != THREAD_DEAD)
		return (true);
    else
		return (false);
}

/*
 * Reschedule the thread.
 * Called whenever a change in the running thread is required.
 */
void
reschedule(void)
{
    int i;
    thread* lastThread;
    int b;
    /*    sigset_t nsig; */

    /* A reschedule in a non-blocked context is half way to hell */
    assert(blockInts > 0);
    b = blockInts;
    
    /* Check events - we may release a high priority thread */
    /* Just check IO, no need for a reschedule call by checkEvents() */
    needReschedule = false;
    checkEvents(false);

    for (;;)
    {
		for (i = MAX_THREAD_PRIO; i >= MIN_THREAD_PRIO; i--)
		{
			if (threadQhead[i] != 0)
			{
				DBG( fprintf(stderr, "found thread %p in head %d\n", threadQhead[i], i); );

				if (threadQhead[i] != currentThread)
				{
					/* USEDSTACKTOP((CONTEXT(currentThread).usedStackTop)); */

					lastThread = currentThread;
					currentThread = threadQhead[i];

					CONTEXT(currentThread).exceptionptr = exceptionptr;

					THREADSWITCH((&CONTEXT(currentThread)),
								 (&CONTEXT(lastThread)));
					blockInts = b;

					exceptionptr = CONTEXT(currentThread).exceptionptr;

					/* Alarm signal may be blocked - if so
					 * unblock it.
					 */
					/*
					  if (alarmBlocked == true) {
					  alarmBlocked = false;
					  sigemptyset(&nsig);
					  sigaddset(&nsig, SIGALRM);
					  sigprocmask(SIG_UNBLOCK, &nsig, 0);
					  }
					*/

					/* I might be dying */
					if ((CONTEXT(lastThread).flags & THREAD_FLAGS_KILLED)
						!= 0)
					{
						CONTEXT(lastThread).flags &= ~THREAD_FLAGS_KILLED;
						exceptionptr = native_new_and_init(class_java_lang_ThreadDeath);
					}
				}
				/* Now we kill the schedule and turn ints
				   back on */
				needReschedule = false;
				return;
			}
		}
		/* Nothing to run - wait for external event */
		DBG( fprintf(stderr, "nothing more to do\n"); );
		checkEvents(true);
    }
}

#endif
