/* vm/jit/stack.h - stack analysis header

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

   $Id: stack.h 2181 2005-04-01 16:53:33Z edwin $

*/


#ifndef _STACK_H
#define _STACK_H

#include "vm/global.h"
#include "vm/exceptions.h"
#include "vm/jit/reg.h"


/**********************************************************************/
/* Macros used internally by analyse_stack                            */
/**********************************************************************/

#ifdef STATISTICS
#define COUNT(cnt) cnt++
#else
#define COUNT(cnt)
#endif
 
/* convenient abbreviations */
#define CURKIND    curstack->varkind
#define CURTYPE    curstack->type

/*--------------------------------------------------*/
/* SIGNALING ERRORS                                 */
/*--------------------------------------------------*/

#define TYPEPANIC  {panic("Stack type mismatch");}


/*--------------------------------------------------*/
/* STACK UNDERFLOW/OVERFLOW CHECKS                  */
/*--------------------------------------------------*/

/* underflow checks */

#define REQUIRE(num) \
    do { \
        if (stackdepth < (num)) { \
            *exceptionptr = \
                new_verifyerror(m, "Unable to pop operand off an empty stack"); \
            return NULL; \
        } \
    } while(0)

#define REQUIRE_1     REQUIRE(1)
#define REQUIRE_2     REQUIRE(2)
#define REQUIRE_3     REQUIRE(3)
#define REQUIRE_4     REQUIRE(4)


/* overflow check */
/* We allow ACONST instructions inserted as arguments to builtin
 * functions to exceed the maximum stack depth.  Maybe we should check
 * against maximum stack depth only at block boundaries?
 */

#define CHECKOVERFLOW \
	do { \
		if (stackdepth > m->maxstack) { \
			if (iptr[0].opc != ICMD_ACONST || iptr[0].op1 == 0) { \
                *exceptionptr = new_verifyerror(m, "Stack size too large"); \
                return NULL; \
            } \
		} \
	} while(0)


/*--------------------------------------------------*/
/* ALLOCATING STACK SLOTS                           */
/*--------------------------------------------------*/

#define NEWSTACK(s,v,n) {new->prev=curstack;new->type=s;new->flags=0;	\
                        new->varkind=v;new->varnum=n;curstack=new;new++;}
#define NEWSTACKn(s,n)  NEWSTACK(s,UNDEFVAR,n)
#define NEWSTACK0(s)    NEWSTACK(s,UNDEFVAR,0)

/* allocate the input stack for an exception handler */
#define NEWXSTACK   {NEWSTACK(TYPE_ADR,STACKVAR,0);curstack=0;}


/*--------------------------------------------------*/
/* STACK MANIPULATION                               */
/*--------------------------------------------------*/

/* resetting to an empty operand stack */
#define STACKRESET {curstack=0;stackdepth=0;}

/* set the output stack of the current instruction */
#define SETDST      {iptr->dst=curstack;}

/* The following macros do NOT check stackdepth, set stackdepth or iptr->dst */
#define POP(s)      {if(s!=curstack->type){TYPEPANIC;}										\
                     if(curstack->varkind==UNDEFVAR)curstack->varkind=TEMPVAR;\
                     curstack=curstack->prev;}
#define POPANY      {if(curstack->varkind==UNDEFVAR)curstack->varkind=TEMPVAR;	\
                     curstack=curstack->prev;}
#define COPY(s,d)   {(d)->flags=0;(d)->type=(s)->type;\
                     (d)->varkind=(s)->varkind;(d)->varnum=(s)->varnum;}


/*--------------------------------------------------*/
/* STACK OPERATIONS MODELING                        */
/*--------------------------------------------------*/

/* The following macros are used to model the stack manipulations of
 * different kinds of instructions.
 *
 * These macros check the input stackdepth and they set the output
 * stackdepth and the output stack of the instruction (iptr->dst).
 *
 * These macros do *not* check for stack overflows!
 */
   
#define PUSHCONST(s){NEWSTACKn(s,stackdepth);SETDST;stackdepth++;}
#define LOAD(s,v,n) {NEWSTACK(s,v,n);SETDST;stackdepth++;}
#define STORE(s)    {REQUIRE_1;POP(s);SETDST;stackdepth--;}
#define OP1_0(s)    {REQUIRE_1;POP(s);SETDST;stackdepth--;}
#define OP1_0ANY    {REQUIRE_1;POPANY;SETDST;stackdepth--;}
#define OP0_1(s)    {NEWSTACKn(s,stackdepth);SETDST;stackdepth++;}
#define OP1_1(s,d)  {REQUIRE_1;POP(s);NEWSTACKn(d,stackdepth-1);SETDST;}
#define OP2_0(s)    {REQUIRE_2;POP(s);POP(s);SETDST;stackdepth-=2;}
#define OPTT2_0(t,b){REQUIRE_2;POP(t);POP(b);SETDST;stackdepth-=2;}
#define OP2_1(s)    {REQUIRE_2;POP(s);POP(s);NEWSTACKn(s,stackdepth-2);SETDST;stackdepth--;}
#define OP2IAT_1(s) {REQUIRE_2;POP(TYPE_INT);POP(TYPE_ADR);NEWSTACKn(s,stackdepth-2);\
                     SETDST;stackdepth--;}
#define OP2IT_1(s)  {REQUIRE_2;POP(TYPE_INT);POP(s);NEWSTACKn(s,stackdepth-2);\
                     SETDST;stackdepth--;}
#define OPTT2_1(s,d){REQUIRE_2;POP(s);POP(s);NEWSTACKn(d,stackdepth-2);SETDST;stackdepth--;}
#define OP2_2(s)    {REQUIRE_2;POP(s);POP(s);NEWSTACKn(s,stackdepth-2);\
                     NEWSTACKn(s,stackdepth-1);SETDST;}
#define OP3TIA_0(s) {REQUIRE_3;POP(s);POP(TYPE_INT);POP(TYPE_ADR);SETDST;stackdepth-=3;}
#define OP3_0(s)    {REQUIRE_3;POP(s);POP(s);POP(s);SETDST;stackdepth-=3;}
#define POPMANY(i)  {REQUIRE(i);stackdepth-=i;while(--i>=0){POPANY;}SETDST;}
#define DUP         {REQUIRE_1;NEWSTACK(CURTYPE,CURKIND,curstack->varnum);SETDST; \
                    stackdepth++;}
#define SWAP        {REQUIRE_2;COPY(curstack,new);POPANY;COPY(curstack,new+1);POPANY;\
                    new[0].prev=curstack;new[1].prev=new;\
                    curstack=new+1;new+=2;SETDST;}
#define DUP_X1      {REQUIRE_2;COPY(curstack,new);COPY(curstack,new+2);POPANY;\
                    COPY(curstack,new+1);POPANY;new[0].prev=curstack;\
                    new[1].prev=new;new[2].prev=new+1;\
                    curstack=new+2;new+=3;SETDST;stackdepth++;}
#define DUP2_X1     {REQUIRE_3;COPY(curstack,new+1);COPY(curstack,new+4);POPANY;\
                    COPY(curstack,new);COPY(curstack,new+3);POPANY;\
                    COPY(curstack,new+2);POPANY;new[0].prev=curstack;\
                    new[1].prev=new;new[2].prev=new+1;\
                    new[3].prev=new+2;new[4].prev=new+3;\
                    curstack=new+4;new+=5;SETDST;stackdepth+=2;}
#define DUP_X2      {REQUIRE_3;COPY(curstack,new);COPY(curstack,new+3);POPANY;\
                    COPY(curstack,new+2);POPANY;COPY(curstack,new+1);POPANY;\
                    new[0].prev=curstack;new[1].prev=new;\
                    new[2].prev=new+1;new[3].prev=new+2;\
                    curstack=new+3;new+=4;SETDST;stackdepth++;}
#define DUP2_X2     {REQUIRE_4;COPY(curstack,new+1);COPY(curstack,new+5);POPANY;\
                    COPY(curstack,new);COPY(curstack,new+4);POPANY;\
                    COPY(curstack,new+3);POPANY;COPY(curstack,new+2);POPANY;\
                    new[0].prev=curstack;new[1].prev=new;\
                    new[2].prev=new+1;new[3].prev=new+2;\
                    new[4].prev=new+3;new[5].prev=new+4;\
                    curstack=new+5;new+=6;SETDST;stackdepth+=2;}


/*--------------------------------------------------*/
/* MACROS FOR HANDLING BASIC BLOCKS                 */
/*--------------------------------------------------*/

/* COPYCURSTACK makes a copy of the current operand stack (curstack)
 * and returns it in the variable copy.
 *
 * This macro is used to propagate the operand stack from one basic
 * block to another. The destination block receives the copy as its
 * input stack.
 */
#define COPYCURSTACK(copy) {\
	int d;\
	stackptr s;\
	if(curstack){\
		s=curstack;\
		new+=stackdepth;\
		d=stackdepth;\
		copy=new;\
		while(s){\
			copy--;d--;\
			copy->prev=copy-1;\
			copy->type=s->type;\
			copy->flags=0;\
			copy->varkind=STACKVAR;\
			copy->varnum=d;\
			s=s->prev;\
			}\
		copy->prev=NULL;\
		copy=new-1;\
		}\
	else\
		copy=NULL;\
}

/* BBEND is called at the end of each basic block (after the last
 * instruction of the block has been processed).
 */

#define BBEND(s,i){ \
	i = stackdepth - 1; \
	copy = s; \
	while (copy) { \
		if ((copy->varkind == STACKVAR) && (copy->varnum > i)) \
			copy->varkind = TEMPVAR; \
		else { \
			copy->varkind = STACKVAR; \
			copy->varnum = i;\
		} \
		rd->interfaces[i][copy->type].type = copy->type; \
		rd->interfaces[i][copy->type].flags |= copy->flags; \
		i--; copy = copy->prev; \
	} \
	i = bptr->indepth - 1; \
	copy = bptr->instack; \
	while (copy) { \
		rd->interfaces[i][copy->type].type = copy->type; \
		if (copy->varkind == STACKVAR) { \
			if (copy->flags & SAVEDVAR) \
				rd->interfaces[i][copy->type].flags |= SAVEDVAR; \
		} \
		i--; copy = copy->prev; \
	} \
}


/* MARKREACHED marks the destination block <b> as reached. If this
 * block has been reached before we check if stack depth and types
 * match. Otherwise the destination block receives a copy of the
 * current stack as its input stack.
 *
 * b...destination block
 * c...current stack
 */

#define MARKREACHED(b,c) \
    do { \
	    if ((b)->flags < 0) { \
		    COPYCURSTACK((c)); \
            (b)->flags = 0; \
            (b)->instack = (c); \
            (b)->indepth = stackdepth; \
        } else { \
            stackptr s = curstack; \
            stackptr t = (b)->instack; \
		    if ((b)->indepth != stackdepth) { \
			    show_icmd_method(m, cd, rd); \
                panic("Stack depth mismatch"); \
            } \
		    while (s) { \
                if (s->type != t->type) \
				    TYPEPANIC; \
			    s = s->prev; \
                t = t->prev; \
			} \
		} \
    } while (0)


/* function prototypes */

methodinfo *analyse_stack(methodinfo *m, codegendata *cd, registerdata *rd);

void icmd_print_stack(codegendata *cd, stackptr s);
char *icmd_builtin_name(functionptr bptr);
void show_icmd_method(methodinfo *m, codegendata *cd, registerdata *rd);
void show_icmd_block(methodinfo *m, codegendata *cd, basicblock *bptr);
void show_icmd(instruction *iptr, bool deadcode);

#endif /* _STACK_H */


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
