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

#include "config.h"

#include <assert.h>

#include <sys/types.h>
#include <sys/mman.h>                   /* for mprotect */
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#include "thread.h"
#include "locks.h"
#include "defines.h"
#include "threads.h"

#include "tables.h"
#include "native.h"
#include "loader.h"
#include "builtin.h"
#include "asmpart.h"

#ifdef USE_BOEHM
#include "toolbox/memory.h"
#endif

static classinfo *class_java_lang_ThreadDeath;

#if defined(USE_INTERNAL_THREADS)

thread* currentThread = NULL;
thread* mainThread;
thread* threadQhead[MAX_THREAD_PRIO + 1];
thread* threadQtail[MAX_THREAD_PRIO + 1];

thread* liveThreads = NULL;
thread* sleepThreads = NULL;

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

/* Pointer to the stack of the last killed thread. The free is delayed. */
void *stack_to_be_freed = 0;

static thread* startDaemon(void* func, char* nm, int stackSize);

/*
 * Allocate the stack for a thread
 */
void
allocThreadStack (thread *tid, int size)
{
    int pageSize = getpagesize();
#ifndef USE_BOEHM
    int result;
#endif
    unsigned long pageBegin;

	assert(stack_to_be_freed == 0);

#ifdef USE_BOEHM
    CONTEXT(tid).stackMem = GCNEW(u1, size + 4 * pageSize);
#else
    CONTEXT(tid).stackMem = malloc(size + 4 * pageSize);
#endif
    assert(CONTEXT(tid).stackMem != 0);
    CONTEXT(tid).stackEnd = CONTEXT(tid).stackMem + size + 2 * pageSize;
    
    pageBegin = (unsigned long)(CONTEXT(tid).stackMem) + pageSize - 1;
    pageBegin = pageBegin - pageBegin % pageSize;

#ifndef USE_BOEHM
    result = mprotect((void*)pageBegin, pageSize, PROT_NONE);
    assert(result == 0);
#endif

    CONTEXT(tid).stackBase = (u1*)pageBegin + pageSize;
}

/*
 * Mark the stack for a thread to be freed. We cannot free the stack
 * immediately because it is still in use!
 */
void
freeThreadStack (thread *tid)
{
    if (!(CONTEXT(tid).flags & THREAD_FLAGS_NOSTACKALLOC))
    {
		int pageSize = getpagesize();
#ifndef USE_BOEHM
        int result;
#endif
		unsigned long pageBegin;

		pageBegin = (unsigned long)(CONTEXT(tid).stackMem) + pageSize - 1;
		pageBegin = pageBegin - pageBegin % pageSize;

#ifndef USE_BOEHM
		result = mprotect((void*)pageBegin, pageSize,
						  PROT_READ | PROT_WRITE | PROT_EXEC);
		assert(result == 0);
#endif

		assert(stack_to_be_freed == 0);

		stack_to_be_freed = CONTEXT(tid).stackMem;
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
	char mainname[] = "main";
	int len = strlen(mainname);

	signal(SIGPIPE, SIG_IGN);

    initLocks();

    for (i = 0; i < MAXTHREADS; ++i) {
		contexts[i].free = true;
		contexts[i].thread = NULL;
		heap_addreference((void**)&contexts[i].thread);
	}

    /* Allocate a thread to be the main thread */
    liveThreads = the_main_thread = (thread*)builtin_new(loader_load(utf_new_char("java/lang/Thread")));
    assert(the_main_thread != 0);
	/* heap_addreference((void **) &liveThreads); */
    
    the_main_thread->PrivateInfo = 1;
    CONTEXT(the_main_thread).free = false;

#if 0
    {
	/* stefan */
	methodinfo *m;
	m = class_findmethod(
			class_java_lang_String,
			utf_new_char ("toCharArray"),
			utf_new_char ("()[C")
			);
printf("DEADCODE LIVES ?????????\n");fflush(stdout);
	the_main_thread->name = asm_calljavafunction (m, javastring_new(utf_new_char("main")), 0, 0, 0);
    }
#endif
	the_main_thread->name = builtin_newarray_char(len);
	{   u2 *d = the_main_thread->name->data;
		for (i=0; i<len; i++)
			d[i] = mainname[i];
	}
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
	CONTEXT(the_main_thread).thread = the_main_thread;
	the_main_thread->single_step = 0;
	the_main_thread->daemon = 0;
	the_main_thread->stillborn = 0;
	the_main_thread->target = 0;

	the_main_thread->contextClassLoader = 0;
	the_main_thread->inheritedAccessControlContext = 0;
	the_main_thread->values = 0;

	/* Allocate and init ThreadGroup */
	the_main_thread->group = (threadGroup*)native_new_and_init(loader_load(utf_new_char("java/lang/ThreadGroup")));
	assert(the_main_thread->group != 0);

	talive++;

	/* Load exception classes */
	class_java_lang_ThreadDeath = loader_load(utf_new_char("java/lang/ThreadDeath"));

	DBG( fprintf(stderr, "finishing initThreads\n"); );

    mainThread = currentThread = the_main_thread;

	/* heap_addreference((void**)&mainThread); */

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

	assert(tid->priority >= MIN_THREAD_PRIO && tid->priority <= MAX_THREAD_PRIO);

    tid->PrivateInfo = i + 1;
    CONTEXT(tid).free = false;
	CONTEXT(tid).thread = tid;
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

	tid = (thread*)builtin_new(loader_load(utf_new_char("java/lang/Thread")));
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
	java_objectheader *local_exceptionptr = NULL;

    DBG( printf("firstStartThread %p\n", currentThread); );

	if (stack_to_be_freed != 0)
	{
#ifndef USE_BOEHM
		free(stack_to_be_freed);
#endif
		stack_to_be_freed = 0;
	}

	/* Every thread starts with the interrupts off */
	intsRestore();
	assert(blockInts == 0);

	/* Find the run()V method and call it */
	method = class_findmethod(currentThread->header.vftbl->class,
							  utf_new_char("run"), utf_new_char("()V"));
	if (method == 0)
		panic("Cannot find method \'void run ()\'");

	local_exceptionptr = asm_calljavamethod(method, currentThread, NULL, NULL, NULL);

    if (local_exceptionptr) {
        utf_display(local_exceptionptr->vftbl->class->name);
        printf("\n");
    }

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
resumeThread (thread* tid)
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

	assert(blockInts > 0);

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
			/* atexit functions get called to clean things up */
			intsRestore();
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
		if (tid != mainThread) {
			CONTEXT(tid).free = true;
			CONTEXT(tid).thread = NULL;
		}

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

	assert(prio >= MIN_THREAD_PRIO && prio <= MAX_THREAD_PRIO);

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
 * Get the current time in milliseconds since 1970-01-01.
 */
s8
currentTime (void)
{
	struct timeval tv;
	s8 time;

	gettimeofday(&tv, 0);

	time = tv.tv_sec;
	time *= 1000;
	time += tv.tv_usec / 1000;

	return time;
}

/*
 * Put a thread to sleep.
 */
void
sleepThread (s8 time)
{
    thread** tidp;

    /* Sleep for no time */
    if (time <= 0) {
		return;
    }
    
    intsDisable();

    /* Get absolute time */
    CONTEXT(currentThread).time = time + currentTime();

    /* Find place in alarm list */
    for (tidp = &sleepThreads; (*tidp) != 0; tidp = &(*tidp)->next)
	{
		if (CONTEXT(*tidp).time > CONTEXT(currentThread).time)
			break;
    }

    /* Suspend thread on it */
    suspendOnQThread(currentThread, tidp);
    
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

                    DBG( fprintf(stderr, "thread switch from: %p to: %p\n", lastThread, currentThread); );
					THREADSWITCH((&CONTEXT(currentThread)),
								 (&CONTEXT(lastThread)));
					blockInts = b;

					exceptionptr = CONTEXT(currentThread).exceptionptr;

					if (stack_to_be_freed != 0)
					{
#ifndef USE_BOEHM
						free(stack_to_be_freed);
#endif
						stack_to_be_freed = 0;
					}

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
