/* src/vm/jit/dseg.c - data segment handling stuff

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
            Andreas  Krall

   Changes: Christian Thalinger
            Joseph Wenninger

   $Id: dseg.h 4826 2006-04-24 16:06:16Z twisti $

*/


#ifndef _DSEG_H
#define _DSEG_H

/* forward typedefs ***********************************************************/

typedef struct jumpref jumpref;
typedef struct dataref dataref;
typedef struct exceptionref exceptionref;
typedef struct patchref patchref;
typedef struct linenumberref linenumberref;


#include "config.h"

#include "vm/types.h"

#include "vm/jit/jit.h"
#include "vm/jit/codegen-common.h"


/* global macros **************************************************************/

#if SIZEOF_VOID_P == 8
# define dseg_addaddress(cd,value)    dseg_adds8((cd), (s8) (value))
#else
# define dseg_addaddress(cd,value)    dseg_adds4((cd), (s4) (value))
#endif


/* jumpref ********************************************************************/

struct jumpref {
	s4          tablepos;       /* patching position in data segment          */
	basicblock *target;         /* target basic block                         */
	jumpref    *next;           /* next element in jumpref list               */
};


/* dataref ********************************************************************/

struct dataref {
	s4       datapos;           /* patching position in generated code        */
	dataref *next;              /* next element in dataref list               */
};


/* exceptionref ***************************************************************/

struct exceptionref {
	s4            branchpos;    /* patching position in code segment          */
	s4            reg;          /* used for ArrayIndexOutOfBounds index reg   */
	functionptr   function;     /* function pointer to generate exception     */
	exceptionref *next;         /* next element in exceptionref list          */
};


/* patchref *******************************************************************/

struct patchref {
	s4           branchpos;
	functionptr  patcher;
	voidptr      ref;
	patchref    *next;
	s4           disp;
};


/* linenumberref **************************************************************/

struct linenumberref {
	s4             tablepos;    /* patching position in data segment          */
	s4             linenumber;  /* line number, used for inserting into the   */
	                            /* table and for validity checking            */
	                            /* -1......start of inlined body              */
	                            /* -2......end of inlined body                */
	                            /* <= -3...special entry with methodinfo *    */
								/* (see doc/inlining_stacktrace.txt)          */
	ptrint         targetmpc;   /* machine code program counter of first      */
	                            /* instruction for given line                 */
								/* NOTE: for linenumber <= -3 this is a the   */
	                            /* (methodinfo *) of the inlined method       */
	linenumberref *next;        /* next element in linenumberref list         */
};


/* function prototypes ********************************************************/

s4 dseg_adds4(codegendata *cd, s4 value);
s4 dseg_adds8(codegendata *cd, s8 value);
s4 dseg_addfloat(codegendata *cd, float value);
s4 dseg_adddouble(codegendata *cd, double value);

void dseg_addtarget(codegendata *cd, basicblock *target);

void dseg_addlinenumbertablesize(codegendata *cd);
void dseg_addlinenumber(codegendata *cd, u2 linenumber);
void dseg_addlinenumber_inline_start(codegendata *cd, instruction *iptr);
void dseg_addlinenumber_inline_end(codegendata *cd, instruction *iptr);

void dseg_createlinenumbertable(codegendata *cd);

#if defined(__I386__) || defined(__X86_64__) || defined(__XDSPCORE__) || defined(ENABLE_INTRP)
void dseg_adddata(codegendata *cd);
void dseg_resolve_datareferences(jitdata *jd);
#endif

#if !defined(NDEBUG)
void dseg_display(jitdata *jd);
#endif

#endif /* _DSEG_H */


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
