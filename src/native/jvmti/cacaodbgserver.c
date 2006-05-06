/* src/native/jvmti/cacaodbgserver.c - contains the cacaodbgserver process. This
                                       process controls the debuggee/cacao vm.

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
            Samuel Vinson

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
#include <stdio.h>

pid_t debuggee;

/* getchildprocptrace *********************************************************

   Get data count number of bytes from address addr for child process address 
   space. Requested  data is stored in the array pointed to by ptr.

*******************************************************************************/
static void getchildprocptrace (char *ptr, void* addr, int cnt) {
	long i, longcnt;
	long *p = (long*) ptr;
	
	longcnt = cnt/sizeof(long);
	for (i=0; i<longcnt; i++) {
		p[i]=GETMEM(debuggee,addr);
		if (p[i]==-1)
		{
			fprintf(stderr,"cacaodbgserver process: getchildprocptrace: %ld\n",i);
			perror("cacaodbgserver process: getchildprocptrace:");
			exit(1);
		}
		addr+=sizeof(long);
	}
	i = GETMEM(debuggee,addr);
	memcpy(p+longcnt,&i,cnt%sizeof(long));
}


/* waitloop *******************************************************************

   waits and handles signals from debuggee/child process. Returns true if 
   cacaodbgserver should exit.

*******************************************************************************/

static bool waitloop(void* dbgcvm) {
    int status,retval,signal;
    void* ip;
	basic_event ev;
	cacaodbgcommunication vm;
	long data;
	struct _brkpt* brk;

	fprintf(stderr,"waitloop\n");

	retval = wait(&status);

	fprintf(stderr,"cacaodbgserver: waitloop we got something to do\n");
	if (retval == -1) {
		fprintf(stderr,"error in waitloop\n");
		perror("cacaodbgserver process: waitloop: ");
		return true;
	}
	
	if (retval != debuggee) {
		fprintf(stderr,"cacaodbgserver got signal from process other then debuggee/cacao vm\n");
		return false;
	}
	
	if (WIFSTOPPED(status)) {
		signal = WSTOPSIG(status);
		
		/* ignore SIGSEGV, SIGPWR, SIGBUS and SIGXCPU for now.
		   todo: in future this signals could be used to detect Garbage 
		   Collection Start/Finish or NullPointerException Events */
		if ((signal == SIGSEGV) || (signal == SIGPWR) || 
			(signal == SIGBUS)	|| (signal == SIGXCPU)) {
			fprintf(stderr,"cacaodbgserver: ignore internal signal (%d)\n",signal);
			CONT(debuggee,signal);
			return false;
		}
		
		if (signal == SIGABRT) {
			fprintf(stderr,"cacaodbgserver: got SIGABRT from debugee - exit\n");
			return true;
		}
		
		ip = getip(debuggee);
		ip--; /* EIP has already been incremented */
		fprintf(stderr,"got signal: %d IP %X\n",signal,ip);
		
			
		ev.signal = signal;
		ev.ip = ip;
		
		/* handle breakpoint */
		getchildprocptrace((char*)&vm,dbgcvm,sizeof(cacaodbgcommunication));
		
		if (vm.setbrkpt) {
			/* set a breakpoint */
			setbrk(debuggee, vm.brkaddr, &vm.brkorig);
			CONT(debuggee,0);
			return false;
		}

		if (signal == SIGTRAP) {
			/* Breakpoint hit. Place original instruction and notify cacao vm to
			   handle it */
			fprintf(stderr,"breakpoint hit\n");
		}

		return false;
	}
	
	if (WIFEXITED(status)) {
		fprintf(stderr,"cacaodbgserver: debuggee/cacao vm exited.\n");
		return true;
	}
	
	if (WIFSIGNALED(status)) {
		fprintf(stderr,"cacaodbgserver: child terminated by signal %d\n",WTERMSIG(status));
		return true;
	}
	
	if (WIFCONTINUED(status)) {
		fprintf(stderr,"cacaodbgserver: continued\n");
		return false;
	}
	
	
	fprintf(stderr,"wait not handled(child not exited or stopped)\n");
	fprintf(stderr,"retval: %d status: %d\n",retval,status);
	CONT(debuggee,0);		
	return false;
}

/* main (cacaodbgserver) ******************************************************

   main function for cacaodbgserver process.

*******************************************************************************/

int main(int argc, char **argv) {
	bool running = true;
	void *dbgcvm;
	int status;
	
	if (argc != 2) {
		fprintf(stderr,"cacaodbgserver: not enough arguments\n");
		fprintf(stderr, "cacaodbgserver cacaodbgcommunicationaddress\n");

		fprintf(stderr,"argc %d argv[0] %s\n",argc,argv[0]);
		exit(1);
	}

	dbgcvm=(cacaodbgcommunication*)strtol(argv[1],NULL,16);

	fprintf(stderr,"cacaodbgserver started pid %d ppid %d\n",getpid(), getppid());

	debuggee = getppid();

	if (TRACEATTACH(debuggee) == -1) perror("cacaodbgserver: ");

	fprintf(stderr,"cacaovm attached\n");

	if (wait(&status) == -1) {
		fprintf(stderr,"error initial wait\n");
		perror("cacaodbgserver: ");
		exit(1);
	}

	if (WIFSTOPPED(status)) 
		if (WSTOPSIG(status) == SIGSTOP)
			CONT(debuggee,0);
	
	while(running) {
		running = !waitloop(dbgcvm);
	}
	fprintf(stderr,"cacaodbgserver exit\n");
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
