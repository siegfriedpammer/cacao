/* jit/codegen.inc.h - code generation header

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser,
   M. Probst, S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck,
   P. Tomsich, J. Wenninger

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

   $Id: codegen.inc.h 1565 2004-11-23 15:56:37Z twisti $

*/


#ifndef _CODEGEN_INC_H
#define _CODEGEN_INC_H

/* We typedef these structures before #includes to resolve circular           */
/* dependencies.                                                              */

typedef struct codegendata codegendata;
typedef struct branchref branchref;
typedef struct jumpref jumpref;
typedef struct dataref dataref;
typedef struct clinitref clinitref;
typedef struct linenumberref linenumberref;
typedef struct threadcritnodetemp threadcritnodetemp;


#include "types.h"
#include "global.h"
#include "jit/inline.h"
#include "jit/reg.h"


#define MCODEINITSIZE (1<<15)       /* 32 Kbyte code area initialization size */
#define DSEGINITSIZE  (1<<12)       /*  4 Kbyte data area initialization size */

#if POINTERSIZE == 8
#define dseg_addaddress(cd,value)    dseg_adds8((cd), (s8) (value))
#else
#define dseg_addaddress(cd,value)    dseg_adds4((cd), (s4) (value))
#endif


/************************* critical sections  *********************************/

struct threadcritnodetemp {
	threadcritnodetemp *next;
	s4 mcodebegin;
	s4 mcodeend;
	s4 mcoderestart;
};


struct codegendata {
	u1 *mcodebase;                  /* base pointer of code area              */
	s4 *mcodeend;                   /* pointer to end of code area            */
	s4  mcodesize;                  /* complete size of code area (bytes)     */

	u1 *mcodeptr;                   /* code generation pointer                */

	u1 *dsegtop;                    /* pointer to top (end) of data area      */
	s4  dsegsize;                   /* complete size of data area (bytes)     */
	s4  dseglen;                    /* used size of data area (bytes)         */
                                    /* data area grows from top to bottom     */

	jumpref   *jumpreferences;      /* list of jumptable target addresses     */
	dataref   *datareferences;      /* list of data segment references        */
	branchref *xboundrefs;          /* list of bound check branches           */
	branchref *xcheckarefs;         /* list of array size check branches      */
	branchref *xnullrefs;           /* list of null check branches            */
	branchref *xcastrefs;           /* list of cast check branches            */
	branchref *xdivrefs;            /* list of divide by zero branches        */
	branchref *xexceptionrefs;      /* list of exception branches             */
	clinitref *clinitrefs;

	linenumberref *linenumberreferences; /* list of line numbers and the      */
	                                /* program counters of their first        */
	                                /* instruction                            */
	s4 linenumbertablesizepos;
	s4 linenumbertablestartpos;
	s4 linenumbertab;

	methodinfo *method;
        s4  exceptiontablelength;/* exceptiontable length                  */
        exceptiontable *exceptiontable; /* the exceptiontable                     */

	threadcritnodetemp *threadcrit; /* List of critical code regions          */
	threadcritnodetemp threadcritcurrent;
	s4 threadcritcount;             /* Number of critical regions             */
        int maxstack;
        int maxlocals;
};


/***************** forward references in branch instructions ******************/

struct branchref {
	s4 branchpos;               /* patching position in code segment          */
	s4 reg;                     /* used for ArrayIndexOutOfBounds index reg   */
	branchref *next;            /* next element in branchref list             */
};


/******************** forward references in tables  ***************************/

struct jumpref {
	s4 tablepos;                /* patching position in data segment          */
	struct basicblock *target;  /* target basic block                         */
	jumpref *next;              /* next element in jumpref list               */
};


struct dataref {
	u1 *pos;                    /* patching position in generated code        */
	dataref *next;              /* next element in dataref list               */
};


struct clinitref {
	s4         branchpos;
	classinfo *class;
	u4         mcode;
#if defined(__I386__) || defined(__X86_64__)
	u1         xmcode;
	u1        *mcodeptr;        /* codegendata dummy pointer to generate code */
#endif
	clinitref *next;
};


struct linenumberref {
	s4 tablepos;                /* patching position in data segment          */
	int targetmpc;              /* machine code program counter of first      */
	                            /* instruction for given line                 */
	u2 linenumber;              /* line number, used for inserting into the   */
	                            /* table and for validty checking             */
	linenumberref *next;        /* next element in linenumberref list         */
};


#if defined(__I386__) || defined(__X86_64__)
typedef struct _methodtree_element methodtree_element;

struct _methodtree_element {
	functionptr startpc;
	functionptr endpc;
};
#endif


/* function prototypes */

void codegen_init();
void codegen_setup(methodinfo *m, codegendata *cd, t_inlining_globals *e);
void codegen(methodinfo *m, codegendata *cd, registerdata *rd);
void codegen_free(methodinfo *m, codegendata *cd);
void codegen_close();
void codegen_insertmethod(functionptr startpc, functionptr endpc);

#if defined(__I386__) || defined(__X86_64__)
void codegen_addreference(codegendata *cd, struct basicblock *target, void *branchptr);
#endif

void dseg_display(methodinfo *m, codegendata *cd);

void init_exceptions();

#endif /* _CODEGEN_INC_H */


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
