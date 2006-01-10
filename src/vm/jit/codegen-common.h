/* src/vm/jit/codegen-common.h - architecture independent code generator stuff

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

   Changes: Christian Ullrich

   $Id: codegen-common.h 4118 2006-01-10 10:59:20Z twisti $

*/


#ifndef _CODEGEN_COMMON_H
#define _CODEGEN_COMMON_H

/* forward typedefs ***********************************************************/

typedef struct codegendata codegendata;
typedef struct branchref branchref;
typedef struct threadcritnodetemp threadcritnodetemp;


#include "config.h"
#include "vm/types.h"

#include "vm/global.h"
#include "vm/references.h"
#include "vm/method.h"
#include "vm/jit/dseg.h"
#include "vm/jit/jit.h"
#include "vm/jit/reg.h"
#include "vm/jit/inline/inline.h"


#define MCODEINITSIZE (1<<15)       /* 32 Kbyte code area initialization size */
#define DSEGINITSIZE  (1<<12)       /*  4 Kbyte data area initialization size */

#define NCODEINITSIZE (1<<15)       /* 32 Kbyte code area initialization size */


/* Register Pack/Unpack Macros ************************************************/

/* ATTENTION: Don't change the order where low and high bits are
   stored! At least mips32 relys in one case on that order. */

#define PACK_REGS(low,high) \
    ( (((high) & 0x0000ffff) << 16) | ((low) & 0x0000ffff) )

#define GET_LOW_REG(a)      ((a) & 0x0000ffff)
#define GET_HIGH_REG(a)    (((a) & 0xffff0000) >> 16)


/************************* critical sections  *********************************/

struct threadcritnodetemp {
	threadcritnodetemp *next;
	s4                  mcodebegin;
	s4                  mcodeend;
	s4                  mcoderestart;
};


struct codegendata {
	u1             *mcodebase;      /* base pointer of code area              */
	s4             *mcodeend;       /* pointer to end of code area            */
	s4              mcodesize;      /* complete size of code area (bytes)     */
	u1             *mcodeptr;       /* code generation pointer                */

#if defined(__I386__) || defined(__MIPS__) || defined(__X86_64__) || defined(ENABLE_INTRP)
	u1             *lastmcodeptr;   /* last patcher position of basic block   */
#endif

#if defined(ENABLE_INTRP)
	u1             *ncodebase;      /* base pointer of native code area       */
	s4              ncodesize;      /* complete size of native code area      */
	u1             *ncodeptr;       /* native code generation pointer         */

	u4              lastinstwithoutdispatch; /* ~0 if there was a dispatch    */

	s4              lastpatcheroffset; /* -1 if current super has no patcher  */
	s4              dynsuperm;      /* offsets of start of current dynamic ...*/
	s4              dynsupern;      /* ... superinstruction starts            */
	struct superstart *superstarts; /* list of supers without patchers        */
#endif

	u1             *dsegtop;        /* pointer to top (end) of data area      */
	s4              dsegsize;       /* complete size of data area (bytes)     */
	s4              dseglen;        /* used size of data area (bytes)         */
                                    /* data area grows from top to bottom     */

	jumpref        *jumpreferences; /* list of jumptable target addresses     */
	dataref        *datareferences; /* list of data segment references        */
	branchref      *xboundrefs;     /* list of bound check branches           */
	branchref      *xnullrefs;      /* list of null check branches            */
	branchref      *xcastrefs;      /* list of cast check branches            */
	branchref      *xstorerefs;     /* list of array store check branches     */
	branchref      *xdivrefs;       /* list of divide by zero branches        */
	branchref      *xexceptionrefs; /* list of exception branches             */
	patchref       *patchrefs;

	linenumberref  *linenumberreferences; /* list of line numbers and the     */
	                                /* program counters of their first        */
	                                /* instruction                            */
	s4              linenumbertablesizepos;
	s4              linenumbertablestartpos;
	s4              linenumbertab;

	methodinfo     *method;
	s4              exceptiontablelength; /* exceptiontable length            */
	exceptiontable *exceptiontable; /* the exceptiontable                     */

	threadcritnodetemp *threadcrit; /* List of critical code regions          */
	threadcritnodetemp threadcritcurrent;
	s4                 threadcritcount; /* Number of critical regions         */

	s4              maxstack;
	s4              maxlocals;
};


/***************** forward references in branch instructions ******************/

struct branchref {
	s4         branchpos;       /* patching position in code segment          */
	s4         reg;             /* used for ArrayIndexOutOfBounds index reg   */
	branchref *next;            /* next element in branchref list             */
};


#if defined(__I386__) || defined(__X86_64__) || defined(ENABLE_INTRP) || defined(DISABLE_GC)
typedef struct _methodtree_element methodtree_element;

struct _methodtree_element {
	u1 *startpc;
	u1 *endpc;
};
#endif


/* function prototypes ********************************************************/

void codegen_init(void);
void codegen_setup(methodinfo *m, codegendata *cd, t_inlining_globals *e);

void codegen_free(methodinfo *m, codegendata *cd);
void codegen_close(void);

s4 *codegen_increase(codegendata *cd, u1 *mcodeptr);

#if defined(ENABLE_INTRP)
u1 *codegen_ncode_increase(codegendata *cd, u1 *ncodeptr);
#endif

void codegen_addreference(codegendata *cd, basicblock *target, void *branchptr);

void codegen_addxboundrefs(codegendata *cd, void *branchptr, s4 reg);
void codegen_addxcastrefs(codegendata *cd, void *branchptr);
void codegen_addxdivrefs(codegendata *cd, void *branchptr);
void codegen_addxstorerefs(codegendata *cd, void *branchptr);
void codegen_addxnullrefs(codegendata *cd, void *branchptr);
void codegen_addxexceptionrefs(codegendata *cd, void *branchptr);

void codegen_addpatchref(codegendata *cd, voidptr branchptr,
						 functionptr patcher, voidptr ref, s4 disp);

void codegen_insertmethod(u1 *startpc, u1 *endpc);
u1 *codegen_findmethod(u1 *pc);

void codegen_finish(methodinfo *m, codegendata *cd, s4 mcodelen);

u1 *codegen_createnativestub(functionptr f, methodinfo *m);
void codegen_disassemble_nativestub(methodinfo *m, u1 *start, u1 *end);

void codegen_start_native_call(u1 *datasp, u1 *pv, u1 *sp, u1 *ra);
void codegen_finish_native_call(u1 *datasp);

u1 *createcompilerstub(methodinfo *m);
u1 *createnativestub(functionptr f, methodinfo *m, codegendata *cd,
					 registerdata *rd, methoddesc *md);

void removecompilerstub(u1 *stub);
void removenativestub(u1 *stub);

s4 reg_of_var(registerdata *rd, stackptr v, s4 tempregnum);

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void codegen_threadcritrestart(codegendata *cd, int offset);
void codegen_threadcritstart(codegendata *cd, int offset);
void codegen_threadcritstop(codegendata *cd, int offset);
#endif

/* machine dependent functions */
u1 *md_codegen_findmethod(u1 *ra);
bool codegen(methodinfo *m, codegendata *cd, registerdata *rd);

#endif /* _CODEGEN_COMMON_H */


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
