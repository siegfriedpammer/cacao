/* src/vm/method.h - method functions header

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

   Authors: Reinhard Grafl

   Changes: Christian Thalinger
            Edwin Steiner

   $Id: method.h 4598 2006-03-14 22:16:47Z edwin $
*/


#ifndef _METHOD_H
#define _METHOD_H

/* forward typedefs ***********************************************************/

typedef struct methodinfo methodinfo; 
typedef struct exceptiontable exceptiontable;
typedef struct lineinfo lineinfo; 
typedef struct codeinfo codeinfo;
typedef struct rplpoint rplpoint;
typedef struct executionsstate executionsstate;
typedef struct sourcestate sourcestate;

#include "config.h"
#include "vm/types.h"

#include "vm/descriptor.h"
#include "vm/global.h"
#include "vm/linker.h"
#include "vm/references.h"
#include "vm/utf8.h"
#include "vm/jit/jit.h"


/* methodinfo *****************************************************************/

struct methodinfo {                 /* method structure                       */
	java_objectheader header;       /* we need this in jit's monitorenter     */
	s4            flags;            /* ACC flags                              */
	utf          *name;             /* name of method                         */
	utf          *descriptor;       /* JavaVM descriptor string of method     */
	methoddesc   *parseddesc;       /* parsed descriptor                      */
			     
	bool          isleafmethod;     /* does method call subroutines           */ /* XXX */
			     
	classinfo    *class;            /* class, the method belongs to           */
	s4            vftblindex;       /* index of method in virtual function    */
	                                /* table (if it is a virtual method)      */
	s4            maxstack;         /* maximum stack depth of method          */
	s4            maxlocals;        /* maximum number of local variables      */
	s4            jcodelength;      /* length of JavaVM code                  */
	u1           *jcode;            /* pointer to JavaVM code                 */
			     
	s4            basicblockcount;  /* number of basic blocks                 */
	basicblock   *basicblocks;      /* points to basic block array            */
	s4           *basicblockindex;  /* a table which contains for every byte  */
	                                /* of JavaVM code a basic block index if  */
	                                /* at this byte is the start of a basic   */
	                                /* block                                  */

	s4            instructioncount; /* number of JavaVM instructions          */
	instruction  *instructions;     /* points to intermediate code instr.     */

	s4            stackcount;       /* number of stack elements               */
	stackelement *stack;            /* points to stack elements               */

	s4            exceptiontablelength; /* exceptiontable length              */
	exceptiontable *exceptiontable; /* the exceptiontable                     */

	u2            thrownexceptionscount; /* number of exceptions attribute    */
	classref_or_classinfo *thrownexceptions; /* except. a method may throw    */

	u2            linenumbercount;  /* number of linenumber attributes        */
	lineinfo     *linenumbers;      /* array of lineinfo items                */

	int       c_debug_nr;           /* a counter to number all BB with an     */
	                                /* unique value                           */

	u1           *stubroutine;      /* stub for compiling or calling natives  */
	codeinfo     *code;             /* current code of this method            */

#if defined(ENABLE_LSRA)
	s4            maxlifetimes;     /* helper for lsra                        */
#endif

	u4            frequency;        /* number of method invocations           */ /* XXX */
	u4           *bbfrequency; /* XXX */

	s8            cycles;           /* number of cpu cycles                   */ /* XXX */
};


/* exceptiontable *************************************************************/

struct exceptiontable {         /* exceptiontable entry in a method           */
	s4              startpc;    /* start pc of guarded area (inclusive)       */
	basicblock     *start;

	s4              endpc;      /* end pc of guarded area (exklusive)         */
	basicblock     *end;

	s4              handlerpc;  /* pc of exception handler                    */
	basicblock     *handler;

	classref_or_classinfo catchtype; /* catchtype of exc. (NULL == catchall)  */
	exceptiontable *next;       /* used to build a list of exception when     */
	                            /* loops are copied */
	exceptiontable *down;       /* instead of the old array, a list is used   */
};


/* lineinfo *******************************************************************/

struct lineinfo {
	u2 start_pc;
	u2 line_number;
};


/* function prototypes ********************************************************/

void method_free(methodinfo *m);
bool method_canoverwrite(methodinfo *m, methodinfo *old);

methodinfo *method_vftbl_lookup(vftbl_t *vftbl, methodinfo* m);

#if !defined(NDEBUG)
void method_printflags(methodinfo *m);
void method_print(methodinfo *m);
void method_println(methodinfo *m);
#endif

#endif /* _METHOD_H */


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
