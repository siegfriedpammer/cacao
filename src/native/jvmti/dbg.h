/* src/native/jvmti/linux-i386.h - jvmti os/architecture support

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Contact: cacao@cacaojvm.org

   Author: Martin Platter

   Changes:             

   
   $Id: dbg.h 4892 2006-05-06 18:29:55Z motse $

*/

/* at the moment linux/i386 is the only plattform available */
#if defined(__LINUX__)  && defined (__I386__)

#ifndef _DBG_H
#define _DBG_H

#include <sys/types.h>
#include <sys/ptrace.h>

#define TRACEATTACH(pid) ptrace(PTRACE_ATTACH, pid, 0, 0)
#define DETACH(pid,sig)  ptrace(PTRACE_DETACH, pid, 0, sig)
#define TRAPINS 0xcc /* opcode for brk */
#define TRAP asm("int3")
#define GETMEM(pid, addr) ptrace(PTRACE_PEEKDATA, pid, addr, 0)
#define CONT(pid,sig) if(ptrace(PTRACE_CONT, pid, 0, sig)==-1) \
                         perror("continue failed: ");
#define DISABLEBRK(pid,ins,addr) ptrace(PTRACE_POKEDATA, pid, (caddr_t) addr, ins)
#define GETREGS(pid,regs) ptrace(PTRACE_GETREGS, pid, 0, &regs)

void* getip(pid_t pid);
void setip(pid_t pid, void* ip);

void setbrk(pid_t pid, void* addr,long* orig);

#endif
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
