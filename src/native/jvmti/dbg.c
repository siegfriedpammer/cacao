/* src/native/jvmti/dbg.c - jvmti os/architecture support

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

/* at the moment linux/i386 is the only plattform available */
#if defined(__LINUX__)  && defined (__I386__)

#include "dbg.h"
#include <sys/ptrace.h>
#include <linux/user.h>

#include <stdio.h>

void* getip(pid_t pid) {
    struct user_regs_struct regs;

    GETREGS(pid,regs);
    return (void*)regs.eip;
}

void setip(pid_t pid, void* ip) {
    struct user_regs_struct regs;

    GETREGS(pid,regs);
    regs.eip=(long)ip; 
    ptrace(PTRACE_SETREGS, pid, 0, &regs); 
}

void setbrk(pid_t pid, void* addr, long* orig) {
    long ins;

	*orig = GETMEM(pid,addr);

	ins = (*orig & ~0x000000FF) | TRAPINS; 

	fprintf (stderr,"pid %d set brk at %p orig: %X new: %X\n",pid,addr,*orig,ins);
	if (ptrace(PTRACE_POKEDATA, pid, (caddr_t) addr, ins)==-1) 
		perror("setbrk error ");
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
