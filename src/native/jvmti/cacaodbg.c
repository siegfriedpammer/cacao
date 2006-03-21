/* src/native/jvmti/cacaodbg.c - contains entry points for debugging support 
                                 in cacao.

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Martin Platter

   Changes: 


   $Id: cacao.c,v 3.165 2006/01/03 23:44:38 twisti Exp $

*/

#include "native/jvmti/jvmti.h"
#include "native/jvmti/cacaodbg.h"
#include "native/jvmti/cacaodbgserver.h"
#include "native/jvmti/dbg.h"
#include "vm/vm.h"
#include "vm/loader.h"
#include "vm/exceptions.h"
#include "vm/builtin.h"
#include "vm/jit/asmpart.h"
#include "vm/stringlocal.h"
#include "toolbox/logging.h"
#include "threads/native/threads.h"

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <sys/msg.h>
#include <linux/user.h>
#include <assert.h>

/* count the request for workingdatalock */
static int workingdatanum = 0;


/* msgqsendevent ***************************************************************

   sends a ptrace request to the cacaodbgserver process through the message 
   queue

*******************************************************************************/

static void msgqsendevent(ptrace_request *pt, ptrace_reply** rcvbuf, int buflen) {
	int size;

	pt->mtype = MSGQPTRACESND;

	fprintf(stderr,"msgqsendevent send new message (kind %d)\n",pt->kind);
	if (-1 == msgsnd(msgqid, pt, sizeof(ptrace_request), 0))
		perror("debugger process: msgqsendevent send error: ");

	size = sizeof(ptrace_reply)+buflen;
	*rcvbuf =(ptrace_reply*) heap_allocate(size,true,NULL);

	if (-1 == msgrcv(msgqid, *rcvbuf, size, MSGQPTRACERCV, 0))
		perror("debugger process: msgqsendevent receive error: ");
	fprintf(stderr,"msgqsendevent reply received(kind %d)\n",pt->kind);
}


/* getworkingdatalock *********************************************************

   blocks until the workingdata lock has been obtained

*******************************************************************************/

void getworkingdatalock() {
	workingdatanum++;
	if (workingdatanum==1) sem_wait(&workingdata_lock);
}

/* releaseworkingdatalock *********************************************************

   release workingdata lock

*******************************************************************************/

void releaseworkingdatalock() {
	workingdatanum--;
	assert(workingdatanum>=0);
	if (workingdatanum==0) sem_post(&workingdata_lock);
}

/* getchildproc ***************************************************************

   Get data count number of bytes from address addr for child process address 
   space. After a successfull call *ptr points to a newly created array 
   containing the requested data.

*******************************************************************************/

void getchildproc (char **ptr, void* addr, int count) {
	ptrace_request pt;
	ptrace_reply  *rcvbuf;
	
	stopdebuggee();

	pt.kind = PTPEEKDATA;
	pt.addr = addr;
	pt.data = count;

	msgqsendevent (&pt,&rcvbuf,count);
	*ptr = rcvbuf->data;

	contdebuggee(0);
}

/* threadobject2jthread *******************************************************

   Convert a cacao threadobject to jthread (java_lang_Thread)

*******************************************************************************/
jthread threadobject2jthread(threadobject* thread) {
	java_lang_Thread* t;
	
	stopdebuggee();

	fprintf (stderr,"debugger: threadobject2jthread\n");
	fflush(stderr);	

	getchildproc((char**)&t, thread->o.thread, sizeof(java_lang_Thread));
	t->header.vftbl = class_java_lang_Thread->vftbl;
	t->vmThread = thread;

	fprintf (stderr,"debugger: threadobject2jthread done (t: %p)\n",t);
	fflush(stderr);	

	contdebuggee(0);

	return (jthread)t;
}

/* allthreads *****************************************************************

   Gets an array of threadobjects of all threads

*******************************************************************************/
jvmtiError allthreads (jint * threads_count_ptr, threadobject ** threads_ptr) {
    int i = 0, cnt = 8; 
	char *data;
    threadobject *thread, *tthreads;;
	void **addr, *mainthread;

	stopdebuggee();

	fprintf (stderr,"debugger: allthreads: addr of mainthreadobj: %p sizeof(threadobject*) %d sizeof(long) %d\n",&mainthreadobj,sizeof(threadobject*),sizeof(long));
	fflush(stderr);	

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	tthreads = MNEW(jthread, (sizeof(threadobject) * cnt));

	stopdebuggee();
	/* mainthreadobj is in the same place in the child process memory because 
	   the child is a copy of this process */
	getchildproc(&data, &mainthreadobj,sizeof(threadobject*));
	addr = (void**) data;
	mainthread = *addr;

    do {
		getchildproc(&data, *addr, sizeof(threadobject));
		thread = (threadobject*) data;
		fprintf (stderr,"debugger: get all threads addr %X *addr %X thread->info.prev %p &data %p data %p\n",addr, *addr, &(thread->info.prev), &data, data);

        if(thread->o.thread != NULL) {
			fprintf (stderr,"debugger: get all threads: thread alive (i %d cnt %d tthreads %p)\n",i,cnt,tthreads);
			fflush(stderr);	

		   /* count and copy only live threads */
		   if (i>=cnt) {
			   MREALLOC(tthreads,threadobject,cnt,cnt+8);
			   cnt += 8;
		   }
		   memcpy(&tthreads[i],thread,sizeof(threadobject));
		   fprintf(stderr,"allthreads - i %d  tthreads[i].o.thread %p\n",i,tthreads[i].o.thread);
		   i++;
		}
		*addr = (void*)thread->info.prev;

		/* repeat until we got the pointer to the mainthread twice */
	} while (mainthread != *addr);

	fprintf (stderr,"debugger: get all threads: %d thread alive - going to copy\n",i);
	fflush(stderr);	

    *threads_count_ptr = i;
	
    *threads_ptr = (threadobject*) heap_allocate(sizeof(threadobject) * i,true,NULL);
	memcpy(*threads_ptr,tthreads,sizeof(threadobject)*i);
	MFREE(tthreads,threadobject,cnt);
	
	fprintf (stderr,"debugger: get all threads: done\n");
	fflush(stderr);	

	contdebuggee(0);

    return JVMTI_ERROR_NONE;
#else
	return JVMTI_ERROR_NOT_AVAILABLE;
#endif
}


/* getcurrentthread ***********************************************************

   Get jthread structure of current thread. 

*******************************************************************************/
jthread getcurrentthread() {
    /* get current thread through stacktrace. */
	threadobject *threads;
	threadobject *currthread=(threadobject*)0xffffffff; /* 32 bit max value */
	jint tcnt;
	int i;
    struct user_regs_struct *regs;
	ptrace_request pt;
	ptrace_reply* rcvbuf;

	assert (allthreads(&tcnt, &threads) == JVMTI_ERROR_NONE);
	
	pt.kind=PTGETREG;
	msgqsendevent(&pt,&rcvbuf,sizeof(struct user_regs_struct));
	regs = rcvbuf->data;
	

	/* ebp address of current thread has to have a smaller nummeric value then
	   the bottom of the stack whose address is in 
	   currentthread->info._stackframeinfo; and a stack can belong to only one 
	   thread.
	   exception: before mainthread has been started
	 */
	
	if (threads[0].info._stackframeinfo != NULL) { 
		fprintf (stderr,"debugger: get all threads: done\n");
		for (i=0;i<tcnt;i++) {
			if ((regs->ebp < (long)threads[i].info._stackframeinfo)
				&& ((long)&threads[i]> (long)currthread))
				currthread = &threads[i];
		}
	} else {
        /* if the mainthread has not been started yet return mainthread */
		fprintf (stderr,"debugger: getcurrentthread mainthreadobj - threads[0].o.thread %p\n",threads[0].o.thread);
		currthread = &threads[0];
	}

	return (jthread)threadobject2jthread(currthread);
}


/* contdebuggee ****************************************************************

   Send request to continue debuggee process through the message queue to the 
   cacaodbgserver process.

*******************************************************************************/

bool contdebuggee(int signal) {
	ptrace_request pt;
	ptrace_reply  *rcvbuf;

	/* get lock for running state */ 
	getworkingdatalock();
	cdbgshmem->hastostop--; 

	if ((!cdbgshmem->running)&&(cdbgshmem->hastostop==0)) {
		/* release lock for running state */ 
		releaseworkingdatalock();
		pt.kind=PTCONT;
		pt.data=signal;
		msgqsendevent (&pt,&rcvbuf,sizeof(bool));
		return  *((bool*)rcvbuf->data);
	} else {
		/* release lock for running state */ 
		releaseworkingdatalock();
		return false;
	}
}


/* stopdebuggee ***************************************************************

   Helper function to stop debugge process. It only sends a signal to stop if 
   the debuggee is not already stopped.

*******************************************************************************/
void stopdebuggee() {
	/* get lock for running state */ 
	getworkingdatalock();
	cdbgshmem->hastostop++;
	if (cdbgshmem->running) {
		fprintf (stderr,"debugger process: going to stop debuggee\n");
		fflush(stderr);	
		/* release lock for running state */ 
		releaseworkingdatalock();

		if (kill (debuggee, SIGUSR2)==-1) {
			perror("debugger process: stopdebuggee kill error: ");
		}
	} else 
		/* release lock for running state */ 
		releaseworkingdatalock();
}


/* brktablecreator*************************************************************

   helper function to enlarge the breakpoint table if needed

*******************************************************************************/

static void brktablecreator() {
	struct _brkpt* tmp;
	if (jvmtibrkpt.size == 0) {
		jvmtibrkpt.brk = MNEW(struct _brkpt, 16);
		memset(jvmtibrkpt.brk, 0, sizeof(struct _brkpt)*16);
		jvmtibrkpt.size = 16;
		jvmtibrkpt.num = BEGINUSERBRK;
	} else {
		jvmtibrkpt.size += 16;
		tmp = jvmtibrkpt.brk;
		jvmtibrkpt.brk = MNEW(struct _brkpt, jvmtibrkpt.size);
		memset(jvmtibrkpt.brk, 0, sizeof(struct _brkpt)*jvmtibrkpt.size);
		memcpy((void*)jvmtibrkpt.brk,(void*)tmp,jvmtibrkpt.size);
		MFREE(tmp,struct _brkpt,jvmtibrkpt.size-16);
	}	
}


/* setsysbrkpt ****************************************************************

   sets a system breakpoint int breakpoint table and calls set breakpoint

*******************************************************************************/

void setsysbrkpt(int sysbrk, void* addr) {
	ptrace_request pt;
	ptrace_reply  *rcvbuf;
	
	if (jvmtibrkpt.size == jvmtibrkpt.num)
		brktablecreator();

	assert (sysbrk < BEGINUSERBRK);
	jvmtibrkpt.brk[sysbrk].addr = addr;

	pt.kind = PTSETBRK;
	pt.addr = addr;
	msgqsendevent (&pt, &rcvbuf, sizeof(long));

	jvmtibrkpt.brk[sysbrk].orig = ((long*)rcvbuf->data)[0];

	fprintf (stderr,"setsysbrk %d %X  done\n",sysbrk, addr);
}


/* addbrkpt *******************************************************************

   adds a breakpoint to breakpoint table and calls set breakpoint

*******************************************************************************/

void addbrkpt(void* addr, jmethodID method, jlocation location) {
	ptrace_request pt;
	ptrace_reply  *rcvbuf;

	if (jvmtibrkpt.size == jvmtibrkpt.num)
		brktablecreator();

	assert (jvmtibrkpt.size > jvmtibrkpt.num);
	fprintf (stderr,"add brk add: %X\n",addr);
	jvmtibrkpt.brk[jvmtibrkpt.num].addr = addr;
	jvmtibrkpt.brk[jvmtibrkpt.num].method = method;
	jvmtibrkpt.brk[jvmtibrkpt.num].location = location;

	pt.kind = PTSETBRK;
	pt.addr = addr;
	msgqsendevent (&pt, &rcvbuf, sizeof(long));
	
	jvmtibrkpt.brk[jvmtibrkpt.num].orig = ((long*)rcvbuf->data)[0];
	jvmtibrkpt.num++;

	fprintf (stderr,"add brk done\n");
}


/* setup_jdwp_thread *****************************************************

   Helper function to start JDWP threads

*******************************************************************************/

static void setup_jdwp_thread(char* transport) {
	java_objectheader *o;
	methodinfo *m;
	java_lang_String  *s;
	classinfo *class;

	/* new gnu.classpath.jdwp.Jdwp() */
	class = load_class_from_sysloader(
            utf_new_char("gnu.classpath.jdwp.Jdwp"));
	if (!class)
		throw_main_exception_exit();

	o = builtin_new(class);

	if (!o)
		throw_main_exception_exit();
	
	m = class_resolveclassmethod(class,
                                     utf_init, 
                                     NULL,
                                     class_java_lang_Object,
                                     true);
	if (!m)
            throw_main_exception_exit();
        
	vm_call_method(m,o);
        
	/* configure(transport,NULL) */
	m = class_resolveclassmethod(
            class, utf_new_char("configure"), 
            utf_new_char("(Ljava/lang/String;)V"),
            class_java_lang_Object,
            false);

	s = javastring_new_char(&transport[1]);

	vm_call_method(m,o,s);

	if (!m)
		throw_main_exception_exit();


	/* _doInitialization */
	m = class_resolveclassmethod(class,
                                     utf_new_char("_doInitialization"), 
                                     utf_new_char("()V"),
                                     class,
                                     false);
	
	if (!m)
            throw_main_exception_exit();
        
	vm_call_method(m,o);
}

/* cacaodbglisten *************************************************************

   setup listener thread for JDWP

*******************************************************************************/

void cacaodbglisten(char* transport) {
	basic_event ev;
	genericEventData data;
	int i;

	fprintf(stderr, "jdwp/debugger process jdwp pid %d\n",getpid()); 
    fflush (stderr); 

    log_text("jdwp/debugger process - set up jdwp listening thread");

    /* setup listening thread (JDWP) */
    setup_jdwp_thread(transport);

    log_text("jdwp/debugger process - continue debuggee");
    /* start to be debugged program */
	contdebuggee(0);

	/* handle messages from cacaodbgserver */
	while (true) {
		if (-1 == msgrcv(msgqid,&ev,sizeof(basic_event),MSGQDEBUGGER,0))
			perror("debugger process: cacaodbglisten: ");
		
		switch (ev.signal) {
		case SIGTRAP:
			/* search the breakpoint that has been triggered */
			i=0;
			while ((ev.ip!=jvmtibrkpt.brk[i].addr) && (i<jvmtibrkpt.num)) i++;

			fprintf(stderr,"cacaodbglisten SIGTRAP switch after while loop i %d\n",i);

			switch (i) {
			case HEREWEGOBRK:
				/* herewego trap */
				if (!suspend) {
					fprintf(stderr,"cacaodbglisten suspend false -> continue debuggee\n");
					contdebuggee(0);
				} else {
					fprintf(stderr,"cacaodbglisten suspend true -> do no continue debuggee\n");
					getworkingdatalock();
					cdbgshmem->hastostop=1;
					releaseworkingdatalock();	
				}
				set_jvmti_phase(JVMTI_PHASE_LIVE);
				break;
			case SETTHREADOBJECTBRK:
				 /* setthreadobject */
				fprintf(stderr,"IP %X == setthreadobject\n",ev.ip);
				data.ev=JVMTI_EVENT_THREAD_START;
				fireEvent(&data);
				break;
			default:
				if ((i >= BEGINUSERBRK) && (i<jvmtibrkpt.num)) {
					log_text("todo: user defined breakpoints are not handled yet");
				} else {
					log_text("breakpoint not handled - continue anyway");
					contdebuggee(0);
				}
			}
			break;
		case SIGQUIT:
			log_text("debugger process SIGQUIT");
			data.ev=JVMTI_EVENT_VM_DEATH;
			fireEvent(&data);		
			break;
		default:
			log_text("signal not handled");
		}
	}
}

/* ipcrm ***********************************************************************

   removes messages queue 

********************************************************************************/
void ipcrm() {
	struct msqid_ds msgbuf;
	struct shmid_ds shmbuf;
	msgctl(msgqid, IPC_RMID, &msgbuf);
	shmctl(shmid, IPC_RMID, &shmbuf);
}


/* cacaodbgfork ****************************************************************

   create debugger/jdwp and debuggee process. Returns true if this is the 
   parent (debugger/jdwp) process

********************************************************************************/

bool cacaodbgfork() {
	int waitproc;

	/* todo: remove shared memory and msg queue on exit */
    /* create/initialize semaphores/shared memory/message queue */
	sem_init(&workingdata_lock, 1, 1);

	shmid = shmget(IPC_PRIVATE, sizeof(cacaodbgserver_data), IPC_CREAT | 0x180);
	if ((cdbgshmem = (cacaodbgserver_data*)shmat(shmid, NULL, 0)) == -1) {
		perror("cacaodbgfork: shared memory attach error: ");
		exit(1);
	}
	cdbgshmem->running = false;
	cdbgshmem->hastostop = 1;

	if ((msgqid =  msgget(IPC_PRIVATE, IPC_CREAT | 0x180)) == -1) {
		perror("cacaodbgfork: message queue get error");
		exit(1);
	}
	;


	/* with this function the following process structure is created:
	   
	   cacaodbgserver 
	    \_ debuggee (client/cacao vm)
		\_ debugger (jdwp/cacao vm)

	 */

	debuggee = fork();
	if (debuggee  == (-1)) {
		log_text("debuggee fork error");
		exit(1);
	} else {
		if (debuggee == 0) { 
			/* debuggee process - this is where the java client is running */
			
			/* allow helper process to trace us  */
			if (TRACEME != 0)  exit(1);
			
			fprintf(stderr, "debugee pid %d\n",getpid()); 
			fflush (stderr); 
					
			/* give parent/debugger process control */
			kill(getpid(),SIGUSR2);
			
			log_text("debuggee: continue with normal startup");
			
			/* continue with normal startup */	
			return false;
		} else {
			log_text("debugger: fork listening jdwp process");
			waitproc = fork();

			if (waitproc == (-1)) {
				log_text("waitprocess fork error");
				exit(1);
			} else {
				if (waitproc == 0) {
					log_text("jdwp process - create new jvmti environment");
					
					remotedbgjvmtienv = new_jvmtienv();

					log_text("jdwp process - init VMjdwp");
           
					if (!VMjdwpInit(remotedbgjvmtienv)) exit(1);
					
					return true;
				} else {
					/* Debugger process (parent of debugge process) 
					   Here a wait-loop is execute and all the ptrace calls are
					   done from within this process. 
					   
					   This call will never return */
					
					cacaodbgserver();
					fprintf(stderr,"cacaodbgserver returned - exit\n");
					ipcrm();
					exit(0);
				} 
			}
		}
	}
	return true; /* make compiler happy */
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
 */
