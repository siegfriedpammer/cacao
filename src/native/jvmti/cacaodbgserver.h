/* src/native/jvmti/cacaodbgserver.h - contains cacao specifics for 
                                       debugging support

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

#ifndef _CACAODBGSERVER_H
#define _CACAODBGSERVER_H

#include "vm/global.h"
#include <semaphore.h>

/* supported message types */
#define MSGQCACAODBGSRV  1  /* messages for cacaodbgserver process  */
#define MSGQDEBUGGER     2  /* messages for debugger process        */
#define MSGQPTRACESND    3  /* messages for ptrace request          */
#define MSGQPTRACERCV    4  /* messages for ptrace return value     */

/* supported ptrace loop calls */
#define PTCONT           1  /* ptrace continue                      */
#define PTPEEKDATA       2  /* ptrace peek data                     */
#define PTSETBRK         3  /* set breakpiont                       */
#define PTDELBRK         4  /* delete breakpiont                    */
#define PTGETREG         5  /* get registers                        */


typedef struct {
	long mtype;
	int kind; 
	void* addr;
	int data;
	long ldata;
} ptrace_request;

typedef struct {
	long mtype;
	bool successful;
	int  datasize;
	char data[1];
} ptrace_reply;


typedef struct {
	long mtype;
	int signal; 
	void *ip;
} basic_event;

typedef struct {
	bool running;           /* true if debuggee process is running             */
	int hastostop;          /* if greater then zero the debugger needs the 
							   debuggee to be stopped                          */
} cacaodbgserver_data;


int  msgqid;                    /* message queue for the communication between 
							       debugger/jdwp and cacaodbgserver process    */
int shmid;                      /* shared memory for saving running state      */
sem_t workingdata_lock;         /* semaphore for cacaodbgserver_data shared  
							       memory structure                            */
cacaodbgserver_data *cdbgshmem; /* cacaodbgserver shared memory pointer        */



void cacaodbgserver();          /* entry point function to cacaodbgserver proc */

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
