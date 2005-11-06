/* src/native/jvmti/linux-i386.h - jvmti os/architecture support

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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

   Author: Martin Platter

   Changes:             

   
   $Id: dbg.h 3588 2005-11-06 14:01:10Z motse $

*/

/* at the moment linux/i386 is the only plattform available */
#if defined(__LINUX__)  && defined (__I386__)
#include <sys/ptrace.h>

#define TRACEME ptrace(PTRACE_TRACEME, 0, 0, 0)
#define GETREGS(pid, regs) ptrace(PTRACE_GETREGS, pid, 0, &regs)
#define GETMEM(pid, addr) ptrace(PTRACE_PEEKDATA, pid, (caddr_t) addr, 0)
#define ENABLEBRK(pid,ins,addr) ptrace(PTRACE_POKEDATA, pid, (caddr_t) addr, (ins & ~0x000000FF) | 0xcc)
#define CONT(pid) ptrace(PTRACE_CONT, pid, 0, 0)
#define DISABLEBRK(pid,ins,addr) ptrace(PTRACE_POKEDATA, pid, (caddr_t) addr, ins)
#define GETIP(pid,ip) ptrace(PTRACE_GETREGS, pid, 0, &regs); \
                      ip=regs.eip;
#define SETIP(pid,ip) ip=regs.eip; \
                      ptrace(PTRACE_SETREGS, pid, 0, &regs); 
#endif

