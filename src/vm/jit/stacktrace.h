/* src/vm/jit/stacktrace.h - header file for stacktrace generation

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

   Authors: Christian Thalinger

   Changes:

   $Id: stacktrace.h 2799 2005-06-23 09:54:26Z twisti $

*/


#ifndef _STACKTRACE_H
#define _STACKTRACE_H

/* forward typedefs ***********************************************************/

typedef struct native_stackframeinfo native_stackframeinfo;
typedef struct stackTraceBuffer stackTraceBuffer;
typedef struct stacktraceelement stacktraceelement;

#include "config.h"
#include "types.h"

#include "vm/method.h"


struct native_stackframeinfo {
	void        *oldThreadspecificHeadValue;
	void       **addressOfThreadspecificHead;
	methodinfo  *method;
#if defined(__ALPHA__)
	void        *savedpv;
#endif
	void        *beginOfJavaStackframe; /*only used if != 0*/ /* on i386 and x86_64 this points to the return addres stored directly below the stackframe*/
	functionptr  returnToFromNative;

#if 0
	void *returnFromNative;
	void *addrReturnFromNative;
	methodinfo *method;
	native_stackframeinfo *next;
	native_stackframeinfo *prev;
#endif
};


struct stacktraceelement {
#if SIZEOF_VOID_P == 8
	u8          linenumber;
#else
	u4          linenumber;
#endif
	methodinfo *method;
};


struct stackTraceBuffer {
	s4                 needsFree;
	stacktraceelement *start;
	s4                 size;
	s4                 full;
};


/* function prototypes ********************************************************/

void cacao_stacktrace_NormalTrace(void **target);
java_objectarray *cacao_createClassContextArray(void);
java_objectheader *cacao_currentClassLoader(void);
methodinfo* cacao_callingMethod(void);
java_objectarray *cacao_getStackForVMAccessController(void);
void stacktrace_dump_trace(void);

#endif /* _STACKTRACE_H */


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
