/* src/vm/jit/trace.h - Functions for tracing from java code.

   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
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

   $Id: trace.h 8304 2007-08-14 19:57:20Z pm $

*/

#ifndef _VM_JIT_TRACE_H
#define _VM_JIT_TRACE_H

#include <stdint.h>

#include "vmcore/method.h"

#if !defined(NDEBUG)

/* trace_java_call_enter ******************************************************
 
   Traces an entry into a java method.

   arg_regs: Array of size ARG_CNT containing all argument registers in
   the same format as in asm_vm_call_method. The array is usually allocated
   on the stack and used for restoring the argument registers later.

   stack: Pointer to first on stack argument in the same format passed to 
   asm_vm_call_method.

*******************************************************************************/

void trace_java_call_enter(methodinfo *m, uint64_t *arg_regs, uint64_t *stack);

/* trace_java_call_exit ********************************************************
 
   Traces an exit form a java method.

   return_regs: Array of size 3 containing return registers:
     [0] : REG_RESULT
	 [1] : REG_RESULT2 (if available on architecture)
	 [2] : REG_FRESULT
   The array is usually allocated on the stack and used for restoring the
   registers later. The format of the array is the same as the format of 
   register arguments passed to asm_vm_call_method.

*******************************************************************************/

void trace_java_call_exit(methodinfo *m, uint64_t *return_regs);

#endif /* !defined(NDEBUG) */

#endif

