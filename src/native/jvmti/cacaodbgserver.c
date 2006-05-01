/* src/native/jvmti/cacaodbgserver.c - contains the cacaodbgserver process. This
                                       process controls the debuggee.

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

   Changes: Edwin Steiner


   $Id: cacao.c,v 3.165 2006/01/03 23:44:38 twisti Exp $

*/

#include "native/jvmti/cacaodbgserver.h"
#include "native/jvmti/cacaodbg.h"
#include "native/jvmti/dbg.h"
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <sys/msg.h>
#include <linux/user.h>


/* getchildprocptrace *********************************************************

   Get data count number of bytes from address addr for child process address 
   space. Requested  data is stored in the array pointed to by ptr.

*******************************************************************************/
static void getchildprocptrace (char *ptr, void* addr, int cnt) {
	int i, longcnt;
	long *p = (long*) ptr;
	
	longcnt = cnt/sizeof(long);
	for (i=0; i<longcnt; i++) {
		p[i]=GETMEM(debuggee,addr);
		if (p[i]==-1) 
			perror("cacaodbgserver process: getchildprocptrace: ");
		addr+=sizeof(long);
	}
	longcnt = GETMEM(debuggee,addr);
	memcpy(ptr,&longcnt,cnt%sizeof(long));
}

/* contchild ******************************************************************

   Helper function to continue child process. 

*******************************************************************************/
static bool contchild(int signal) {
	/* get lock for running state */ 
	threads_sem_wait(&workingdata_lock);
	fprintf(stderr,"cacaodbgserver: contchild called (hastostop: %d)\n",cdbgshmem->hastostop);
	if(cdbgshmem->hastostop < 1) {
		fprintf(stderr,"cacaodbgserver: going to continue child\n");
		CONT(debuggee,signal);
		cdbgshmem->hastostop = 0;
		cdbgshmem->running=true;
		/* release lock for running state */ 
		threads_sem_post(&workingdata_lock);
		return true;
	} else {
		threads_sem_post(&workingdata_lock);
		return false;		
	}
}

/* msgqsendevent *******************************************************************

   sends an event notification to the jdwp/debugger process through the 
   message queue

*******************************************************************************/
static void msgqsendevent(basic_event *ev) {
	ev->mtype = MSGQDEBUGGER;
	
	if (-1 == msgsnd(msgqid, ev, sizeof(basic_event), 0)) {
		perror("cacaodbgserver process: cacaodbglisten send error: ");
		exit(1);
	}
}

/* waitloop *******************************************************************

   waits and handles signals from debuggee/child process

*******************************************************************************/

static void waitloop() {
    int status,retval,signal;
    void* ip;
	basic_event ev;

	fprintf(stderr,"waitloop\n");
    fflush (stderr); 

    while (true) {
		retval = wait(&status);

		fprintf(stderr,"cacaodbgserver: waitloop we got something to do\n");
		if (retval == -1) {
			fprintf(stderr,"error in waitloop\n");
			perror("cacaodbgserver process: waitloop: ");
			exit(1);
		}

		if (retval != debuggee) {
			fprintf(stderr,"cacaodbgserver got signal from process other then debuggee\n");
			exit(1);
		}
		
		if (WIFEXITED(status)) {
			/* generate event VMDeath */
			ev.signal = SIGQUIT;
			ev.ip = NULL;
			msgqsendevent(&ev);
			return;
		}
		
		if (WIFSTOPPED(status)) {
			signal = WSTOPSIG(status);

			/* ignore SIGSEGV, SIGPWR, SIGBUS and SIGXCPU for now.
			   todo: in future this signals can be used to detect Garbage 
			   Collection Start/Finish or NullPointerException Events */
			if ((signal == SIGSEGV) || (signal == SIGPWR) || 
				(signal == SIGBUS)	|| (signal == SIGXCPU)) {
				fprintf(stderr,"cacaodbgserver: ignore internal signal (%d)\n",signal);
				contchild(signal);
				continue;
			}

			if (signal == SIGABRT) {
				fprintf(stderr,"cacaodbgserver: got SIGABRT from debugee - exit\n");
				exit(1);
			}

			ip = getip(debuggee);
			ip--; /* EIP has already been incremented */
			fprintf(stderr,"got signal: %d IP %X\n",signal,ip);
			
			threads_sem_wait(&workingdata_lock);
			cdbgshmem->running = false;
			cdbgshmem->hastostop = 1;
			threads_sem_post(&workingdata_lock);

			if (signal==SIGUSR2) {
				fprintf(stderr,"SIGUSR2 - debuggee has stopped by jdwp process\n");
				return;
			}
			
			ev.signal = signal;
			ev.ip = ip;
			msgqsendevent(&ev);
			return;
		}
	
		fprintf(stderr,"wait not handled(child not exited or stopped)\n");
		fprintf(stderr,"retval: %d status: %d\n",retval,status);
	}
}

/* ptraceloop *****************************************************************

   this function handles the ptrace request from the jdwp/debugger process.

*******************************************************************************/

void ptraceloop() {
	bool contdebuggee=false;
	ptrace_request pt;
	ptrace_reply *buffer;
	int size;
    struct user_regs_struct *regs;

	fprintf(stderr,"ptraceloop\n");
    fflush (stderr); 
	while (!contdebuggee) {
		if (-1 == msgrcv(msgqid, &pt, sizeof(ptrace_request), MSGQPTRACESND, 0))
			perror("cacaodbgserver process: cacaodbglisten receive error: ");

		switch(pt.kind){
		case PTCONT:
			/* continue debuggee process */
			size= sizeof(ptrace_reply);
			buffer =(ptrace_reply*) MNEW(char,size);

			contdebuggee = contchild(pt.data);

			buffer->mtype = MSGQPTRACERCV;
			buffer->successful=true;
			buffer->datasize=0;
			break;
		case PTPEEKDATA:
			/* get memory content from the debuggee process */
			size= sizeof(ptrace_reply)+pt.data;
			buffer =(ptrace_reply*) MNEW(char,size);

			buffer->mtype = MSGQPTRACERCV;
			buffer->datasize = size-sizeof(ptrace_reply);

			fprintf(stderr,"getchildprocptrace: pid %d get %p - %p cnt: %d (buffer %p buffer->data %p)\n",
					debuggee, pt.addr,pt.addr+pt.data, buffer->datasize,buffer, buffer->data);
			fflush(stderr);

			getchildprocptrace(buffer->data,pt.addr,buffer->datasize);
			break;
		case PTSETBRK:
			size= sizeof(ptrace_reply)+sizeof(long);
			buffer =(ptrace_reply*) MNEW(char,size);

			/* set new breakpoint */
			buffer->mtype = MSGQPTRACERCV;
			buffer->successful=true;
			buffer->datasize=sizeof(long);
			
			setbrk(debuggee,pt.addr, (long*)(buffer->data));
			break;
		case PTDELBRK:
			/* delete breakpoint */
			size= sizeof(ptrace_reply);
			buffer =(ptrace_reply*) MNEW(char,size);
			
			DISABLEBRK(debuggee,pt.ldata,pt.addr);
			
			buffer->mtype = MSGQPTRACERCV;
			buffer->successful=true;
			buffer->datasize=0;
			break;
		case PTGETREG:
			/* get registers */
			size= sizeof(ptrace_reply)+sizeof(struct user_regs_struct);
			buffer =(ptrace_reply*) MNEW(char,size);
			regs=buffer->data;

			GETREGS(debuggee,*regs);
			
			buffer->mtype = MSGQPTRACERCV;
			buffer->successful=true;
			buffer->datasize=sizeof(struct user_regs_struct);
			break;
		default:
			fprintf(stderr,"unkown ptrace request %d\n",pt.kind);
			exit(1);
		}

		if (-1 == msgsnd(msgqid, buffer, size, 0)) {
			perror("cacaodbgserver process: cacaodbglisten send error: ");
			exit(1);
		}
		MFREE(buffer,char,size);
	}
}

/* cacaodbgserver *************************************************************

   waits for eventes from and issues ptrace calls to debuggee/child process

*******************************************************************************/

void cacaodbgserver() {
	fprintf(stderr,"cacaodbgserver started\n");
	fflush(stderr);
	while(true) {
		/* wait until debuggee process gets stopped 
		   and inform debugger process */
		waitloop();
		/* give the debugger process the opportunity to issue ptrace calls */
		ptraceloop();
		/* ptraceloop returns after a PTRACE_CONT call has been issued     */
	}
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
