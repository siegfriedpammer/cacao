/* src/vm/method.h - method functions header

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

   Authors: Reinhard Grafl

   Changes: Christian Thalinger

   $Id: method.h 3268 2005-09-21 20:22:18Z twisti $
*/


#ifndef _METHOD_H
#define _METHOD_H

/* forward typedefs ***********************************************************/

typedef struct methodinfo methodinfo; 
typedef struct exceptiontable exceptiontable;
typedef struct lineinfo lineinfo; 

#include "config.h"
#include "vm/types.h"

#include "vm/global.h"
#include "vm/utf8.h"
#include "vm/references.h"
#include "vm/descriptor.h"
#include "vm/jit/jit.h"
#include "vm/jit/inline/parseXTA.h"


/* methodinfo *****************************************************************/

struct methodinfo {                 /* method structure                       */
	java_objectheader header;       /* we need this in jit's monitorenter     */
	s4          flags;              /* ACC flags                              */
	utf        *name;               /* name of method                         */
	utf        *descriptor;         /* JavaVM descriptor string of method     */
	methoddesc *parseddesc;         /* parsed descriptor                      */

	s4          returntype;         /* only temporary valid, return type      */
	classinfo  *returnclass;        /* pointer to classinfo for the rtn type  */ /*XTA*/ 

	s4          paramcount;         /* only temporary valid, parameter count  */
	u1         *paramtypes;         /* only temporary valid, parameter types  */
	classinfo **paramclass;         /* pointer to classinfo for a parameter   */ /*XTA*/

	bool        isleafmethod;       /* does method call subroutines           */

	classinfo  *class;              /* class, the method belongs to           */
	s4          vftblindex;         /* index of method in virtual function    */
	                                /* table (if it is a virtual method)      */
	s4          maxstack;           /* maximum stack depth of method          */
	s4          maxlocals;          /* maximum number of local variables      */
	s4          jcodelength;        /* length of JavaVM code                  */
	u1         *jcode;              /* pointer to JavaVM code                 */

	s4          basicblockcount;    /* number of basic blocks                 */
	basicblock *basicblocks;        /* points to basic block array            */
	s4         *basicblockindex;    /* a table which contains for every byte  */
	                                /* of JavaVM code a basic block index if  */
	                                /* at this byte is the start of a basic   */
	                                /* block                                  */

	s4          instructioncount;   /* number of JavaVM instructions          */
	instruction *instructions;      /* points to intermediate code instr.     */

	s4          stackcount;         /* number of stack elements               */
	stackelement *stack;            /* points to intermediate code instr.     */

	s4          exceptiontablelength;/* exceptiontable length                 */
	exceptiontable *exceptiontable; /* the exceptiontable                     */

	u2          thrownexceptionscount;/* number of exceptions attribute       */
	classref_or_classinfo *thrownexceptions; /* except. a method may throw    */

	u2          linenumbercount;    /* number of linenumber attributes        */
	lineinfo   *linenumbers;        /* array of lineinfo items                */

	int       c_debug_nr;           /* a counter to number all BB with an     */
	                                /* unique value                           */

	functionptr stubroutine;        /* stub for compiling or calling natives  */
	s4          mcodelength;        /* legth of generated machine code        */
	functionptr mcode;              /* pointer to machine code                */
	functionptr entrypoint;         /* entry point in machine code            */

	/*rtainfo   rta;*/
	xtainfo    *xta;

	bool        methodXTAparsed;    /*  true if xta parsed */
	s4          methodUsed;         /* marked (might be used later) /not used /used */
	s4          monoPoly;           /* call is mono or poly or unknown        */ /*RT stats */
        /* should # method def'd and used be kept after static parse (will it be used?) */
	s4	        subRedefs;
	s4	        subRedefsUsed;
	s4	        nativelyoverloaded; /* used in header.c and only valid there  */
	/* helper for lsra */
#ifdef LSRA
	s4          maxlifetimes;
#endif
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

void method_display(methodinfo *m);
void method_display_w_class(methodinfo *m);

void method_descriptor2types(methodinfo *m);

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
 */
