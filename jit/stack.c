/* jit/stack.c - stack analysis

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

   Authors: Andreas Krall

   Changes: Edwin Steiner

   $Id: stack.c 1203 2004-06-22 23:14:55Z twisti $

*/


#include <stdio.h>
#include <string.h>
#include "global.h"
#include "main.h"
#include "native.h"
#include "builtin.h"
#include "disass.h"
#include "jit/jit.h"
#include "jit/stack.h"
#include "jit/reg.h"
#include "tables.h"
#include "types.h"
#include "toolbox/logging.h"
#include "toolbox/memory.h"


/* from codegen.inc */
extern int dseglen;

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
            sprintf(msg, "(class: "); \
            utf_sprint(msg + strlen(msg), m->class->name); \
            sprintf(msg + strlen(msg), ", method: "); \
            utf_sprint(msg + strlen(msg), m->name); \
            sprintf(msg + strlen(msg), ", signature: "); \
            utf_sprint(msg + strlen(msg), m->descriptor); \
            sprintf(msg + strlen(msg), ") Unable to pop operand off an empty stack"); \
            *exceptionptr = \
                new_exception_message(string_java_lang_VerifyError, msg); \
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
			if (iptr[0].opc != ICMD_ACONST \
                || iptr[0].op1 == 0) { \
                sprintf(msg, "(class: "); \
                utf_sprint_classname(msg + strlen(msg), m->class->name); \
                sprintf(msg + strlen(msg), ", method: "); \
                utf_sprint(msg + strlen(msg), m->name); \
                sprintf(msg + strlen(msg), ", signature: "); \
                utf_sprint(msg + strlen(msg), m->descriptor); \
                sprintf(msg + strlen(msg), ") Stack size too large"); \
                *exceptionptr = \
                    new_exception_message(string_java_lang_VerifyError, msg); \
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
		interfaces[i][copy->type].type = copy->type; \
		interfaces[i][copy->type].flags |= copy->flags; \
		i--; copy = copy->prev; \
	} \
	i = bptr->indepth - 1; \
	copy = bptr->instack; \
	while (copy) { \
		interfaces[i][copy->type].type = copy->type; \
		if (copy->varkind == STACKVAR) { \
			if (copy->flags & SAVEDVAR) \
				interfaces[i][copy->type].flags |= SAVEDVAR; \
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
#define MARKREACHED(b,c) {												\
	if(b->flags<0)														\
		{COPYCURSTACK(c);b->flags=0;b->instack=c;b->indepth=stackdepth;} \
	else {stackptr s=curstack;stackptr t=b->instack;					\
		if(b->indepth!=stackdepth)										\
			{show_icmd_method(m);panic("Stack depth mismatch");}			\
		while(s){if (s->type!=t->type)									\
				TYPEPANIC												\
			s=s->prev;t=t->prev;										\
			}															\
		}																\
}


/**********************************************************************/
/* analyse_stack                                                      */
/**********************************************************************/

/* analyse_stack uses the intermediate code created by parse.c to
 * build a model of the JVM operand stack for the current method.
 *
 * The following checks are performed:
 *   - check for operand stack underflow (before each instruction)
 *   - check for operand stack overflow (after[1] each instruction)
 *   - check for matching stack depth at merging points
 *   - check for matching basic types[2] at merging points
 *   - check basic types for instruction input (except for BUILTIN*
 *         opcodes, INVOKE* opcodes and MULTIANEWARRAY)
 *
 * [1]) Checking this after the instruction should be ok. parse.c
 * counts the number of required stack slots in such a way that it is
 * only vital that we don't exceed `maxstack` at basic block
 * boundaries.
 *
 * [2]) 'basic types' means the distinction between INT, LONG, FLOAT,
 * DOUBLE and ADDRESS types. Subtypes of INT and different ADDRESS
 * types are not discerned.
 */

methodinfo *analyse_stack(methodinfo *m)
{
	int b_count;
	int b_index;
	int stackdepth;
	stackptr curstack;
	stackptr new;
	stackptr copy;
	int opcode, i, len, loops;
	int superblockend, repeat, deadcode;
	instruction *iptr;
	basicblock *bptr;
	basicblock *tbptr;
	s4 *s4ptr;
	void* *tptr;
	int *argren;
	char msg[MAXLOGTEXT];               /* maybe we get an exception          */

	argren = DMNEW(int, m->maxlocals);
	/*int *argren = (int *)alloca(m->maxlocals * sizeof(int));*/ /* table for argument renaming */
	for (i = 0; i < m->maxlocals; i++)
		argren[i] = i;
	
	arguments_num = 0;
	new = m->stack;
	loops = 0;
	m->basicblocks[0].flags = BBREACHED;
	m->basicblocks[0].instack = 0;
	m->basicblocks[0].indepth = 0;

	for (i = 0; i < m->exceptiontablelength; i++) {
		bptr = &m->basicblocks[m->basicblockindex[m->exceptiontable[i].handlerpc]];
		bptr->flags = BBREACHED;
		bptr->type = BBTYPE_EXH;
		bptr->instack = new;
		bptr->indepth = 1;
		bptr->pre_count = 10000;
		STACKRESET;
		NEWXSTACK;
	}

#ifdef CONDITIONAL_LOADCONST
	b_count = m->basicblockcount;
	bptr = m->basicblocks;
	while (--b_count >= 0) {
		if (bptr->icount != 0) {
			iptr = bptr->iinstr + bptr->icount - 1;
			switch (iptr->opc) {
			case ICMD_RET:
			case ICMD_RETURN:
			case ICMD_IRETURN:
			case ICMD_LRETURN:
			case ICMD_FRETURN:
			case ICMD_DRETURN:
			case ICMD_ARETURN:
			case ICMD_ATHROW:
				break;

			case ICMD_IFEQ:
			case ICMD_IFNE:
			case ICMD_IFLT:
			case ICMD_IFGE:
			case ICMD_IFGT:
			case ICMD_IFLE:

			case ICMD_IFNULL:
			case ICMD_IFNONNULL:

			case ICMD_IF_ICMPEQ:
			case ICMD_IF_ICMPNE:
			case ICMD_IF_ICMPLT:
			case ICMD_IF_ICMPGE:
			case ICMD_IF_ICMPGT:
			case ICMD_IF_ICMPLE:

			case ICMD_IF_ACMPEQ:
			case ICMD_IF_ACMPNE:
				bptr[1].pre_count++;
			case ICMD_GOTO:
				m->basicblocks[m->basicblockindex[iptr->op1]].pre_count++;
				break;

			case ICMD_TABLESWITCH:
				s4ptr = iptr->val.a;
				m->basicblocks[m->basicblockindex[*s4ptr++]].pre_count++;
				i = *s4ptr++;                               /* low     */
				i = *s4ptr++ - i + 1;                       /* high    */
				while (--i >= 0) {
					m->basicblocks[m->basicblockindex[*s4ptr++]].pre_count++;
				}
				break;
					
			case ICMD_LOOKUPSWITCH:
				s4ptr = iptr->val.a;
				m->basicblocks[m->basicblockindex[*s4ptr++]].pre_count++;
				i = *s4ptr++;                               /* count   */
				while (--i >= 0) {
					m->basicblocks[m->basicblockindex[s4ptr[1]]].pre_count++;
					s4ptr += 2;
				}
				break;
			default:
				bptr[1].pre_count++;
				break;
			}
		}
		bptr++;
	}
#endif


	do {
		loops++;
		b_count = m->basicblockcount;
		bptr = m->basicblocks;
		superblockend = true;
		repeat = false;
		STACKRESET;
		deadcode = true;
		while (--b_count >= 0) {
			if (bptr->flags == BBDELETED) {
				/* do nothing */
			}
			else if (superblockend && (bptr->flags < BBREACHED))
				repeat = true;
			else if (bptr->flags <= BBREACHED) {
				if (superblockend)
					stackdepth = bptr->indepth;
				else if (bptr->flags < BBREACHED) {
					COPYCURSTACK(copy);
					bptr->instack = copy;
					bptr->indepth = stackdepth;
				}
				else if (bptr->indepth != stackdepth) {
					show_icmd_method(m);
					panic("Stack depth mismatch");
					
				}
				curstack = bptr->instack;
				deadcode = false;
				superblockend = false;
				bptr->flags = BBFINISHED;
				len = bptr->icount;
				iptr = bptr->iinstr;
				b_index = bptr - m->basicblocks;
				while (--len >= 0)  {
					opcode = iptr->opc;
					iptr->target = NULL;

/*  					dolog("p: %04d op: %s stack: %p", iptr - instr, icmd_names[opcode], curstack); */

#ifdef USEBUILTINTABLE
					{
#if 0
						stdopdescriptor *breplace;
						breplace = find_builtin(opcode);

						if (breplace && opcode == breplace->opcode) {
							iptr[0].opc = breplace->icmd;
							iptr[0].op1 = breplace->type_d;
							iptr[0].val.a = breplace->builtin;
							m->isleafmethod = false;
							switch (breplace->icmd) {
							case ICMD_BUILTIN1:
								goto builtin1;
							case ICMD_BUILTIN2:
								goto builtin2;
							}
						}
#endif
						builtin_descriptor *breplace;
						breplace = find_builtin(opcode);

						if (breplace && opcode == breplace->opcode) {
							iptr[0].opc = breplace->icmd;
							iptr[0].op1 = breplace->type_d;
							iptr[0].val.a = breplace->builtin;
							m->isleafmethod = false;
							switch (breplace->icmd) {
							case ICMD_BUILTIN1:
								goto builtin1;
							case ICMD_BUILTIN2:
								goto builtin2;
							}
						}
					}
#endif
					
					switch (opcode) {

						/* pop 0 push 0 */

					case ICMD_NOP:
					case ICMD_CHECKASIZE:
					case ICMD_CHECKEXCEPTION:

					case ICMD_IFEQ_ICONST:
					case ICMD_IFNE_ICONST:
					case ICMD_IFLT_ICONST:
					case ICMD_IFGE_ICONST:
					case ICMD_IFGT_ICONST:
					case ICMD_IFLE_ICONST:
					case ICMD_ELSE_ICONST:
						SETDST;
						break;

					case ICMD_RET:
						locals[iptr->op1][TYPE_ADR].type = TYPE_ADR;
					case ICMD_RETURN:
						COUNT(count_pcmd_return);
						SETDST;
						superblockend = true;
						break;

						/* pop 0 push 1 const */
						
					case ICMD_ICONST:
						COUNT(count_pcmd_load);
						if (len > 0) {
							switch (iptr[1].opc) {
							case ICMD_IADD:
								iptr[0].opc = ICMD_IADDCONST;
							icmd_iconst_tail:
								iptr[1].opc = ICMD_NOP;
								OP1_1(TYPE_INT,TYPE_INT);
								COUNT(count_pcmd_op);
								break;
							case ICMD_ISUB:
								iptr[0].opc = ICMD_ISUBCONST;
								goto icmd_iconst_tail;
							case ICMD_IMUL:
								iptr[0].opc = ICMD_IMULCONST;
								goto icmd_iconst_tail;
							case ICMD_IDIV:
								if (iptr[0].val.i == 0x00000002)
									iptr[0].val.i = 1;
								else if (iptr[0].val.i == 0x00000004)
									iptr[0].val.i = 2;
								else if (iptr[0].val.i == 0x00000008)
									iptr[0].val.i = 3;
								else if (iptr[0].val.i == 0x00000010)
									iptr[0].val.i = 4;
								else if (iptr[0].val.i == 0x00000020)
									iptr[0].val.i = 5;
								else if (iptr[0].val.i == 0x00000040)
									iptr[0].val.i = 6;
								else if (iptr[0].val.i == 0x00000080)
									iptr[0].val.i = 7;
								else if (iptr[0].val.i == 0x00000100)
									iptr[0].val.i = 8;
								else if (iptr[0].val.i == 0x00000200)
									iptr[0].val.i = 9;
								else if (iptr[0].val.i == 0x00000400)
									iptr[0].val.i = 10;
								else if (iptr[0].val.i == 0x00000800)
									iptr[0].val.i = 11;
								else if (iptr[0].val.i == 0x00001000)
									iptr[0].val.i = 12;
								else if (iptr[0].val.i == 0x00002000)
									iptr[0].val.i = 13;
								else if (iptr[0].val.i == 0x00004000)
									iptr[0].val.i = 14;
								else if (iptr[0].val.i == 0x00008000)
									iptr[0].val.i = 15;
								else if (iptr[0].val.i == 0x00010000)
									iptr[0].val.i = 16;
								else if (iptr[0].val.i == 0x00020000)
									iptr[0].val.i = 17;
								else if (iptr[0].val.i == 0x00040000)
									iptr[0].val.i = 18;
								else if (iptr[0].val.i == 0x00080000)
									iptr[0].val.i = 19;
								else if (iptr[0].val.i == 0x00100000)
									iptr[0].val.i = 20;
								else if (iptr[0].val.i == 0x00200000)
									iptr[0].val.i = 21;
								else if (iptr[0].val.i == 0x00400000)
									iptr[0].val.i = 22;
								else if (iptr[0].val.i == 0x00800000)
									iptr[0].val.i = 23;
								else if (iptr[0].val.i == 0x01000000)
									iptr[0].val.i = 24;
								else if (iptr[0].val.i == 0x02000000)
									iptr[0].val.i = 25;
								else if (iptr[0].val.i == 0x04000000)
									iptr[0].val.i = 26;
								else if (iptr[0].val.i == 0x08000000)
									iptr[0].val.i = 27;
								else if (iptr[0].val.i == 0x10000000)
									iptr[0].val.i = 28;
								else if (iptr[0].val.i == 0x20000000)
									iptr[0].val.i = 29;
								else if (iptr[0].val.i == 0x40000000)
									iptr[0].val.i = 30;
								else if (iptr[0].val.i == 0x80000000)
									iptr[0].val.i = 31;
								else {
									PUSHCONST(TYPE_INT);
									break;
								}
								iptr[0].opc = ICMD_IDIVPOW2;
								goto icmd_iconst_tail;
							case ICMD_IREM:
#if !defined(NO_DIV_OPT)
								if (iptr[0].val.i == 0x10001) {
									iptr[0].opc = ICMD_IREM0X10001;
									goto icmd_iconst_tail;
								}
#endif
								if ((iptr[0].val.i == 0x00000002) ||
									(iptr[0].val.i == 0x00000004) ||
									(iptr[0].val.i == 0x00000008) ||
									(iptr[0].val.i == 0x00000010) ||
									(iptr[0].val.i == 0x00000020) ||
									(iptr[0].val.i == 0x00000040) ||
									(iptr[0].val.i == 0x00000080) ||
									(iptr[0].val.i == 0x00000100) ||
									(iptr[0].val.i == 0x00000200) ||
									(iptr[0].val.i == 0x00000400) ||
									(iptr[0].val.i == 0x00000800) ||
									(iptr[0].val.i == 0x00001000) ||
									(iptr[0].val.i == 0x00002000) ||
									(iptr[0].val.i == 0x00004000) ||
									(iptr[0].val.i == 0x00008000) ||
									(iptr[0].val.i == 0x00010000) ||
									(iptr[0].val.i == 0x00020000) ||
									(iptr[0].val.i == 0x00040000) ||
									(iptr[0].val.i == 0x00080000) ||
									(iptr[0].val.i == 0x00100000) ||
									(iptr[0].val.i == 0x00200000) ||
									(iptr[0].val.i == 0x00400000) ||
									(iptr[0].val.i == 0x00800000) ||
									(iptr[0].val.i == 0x01000000) ||
									(iptr[0].val.i == 0x02000000) ||
									(iptr[0].val.i == 0x04000000) ||
									(iptr[0].val.i == 0x08000000) ||
									(iptr[0].val.i == 0x10000000) ||
									(iptr[0].val.i == 0x20000000) ||
									(iptr[0].val.i == 0x40000000) ||
									(iptr[0].val.i == 0x80000000)) {
									iptr[0].opc = ICMD_IREMPOW2;
									iptr[0].val.i -= 1;
#if defined(__I386__)
									method_uses_ecx = true;
#endif
									goto icmd_iconst_tail;
								}
								PUSHCONST(TYPE_INT);
								break;
							case ICMD_IAND:
								iptr[0].opc = ICMD_IANDCONST;
								goto icmd_iconst_tail;
							case ICMD_IOR:
								iptr[0].opc = ICMD_IORCONST;
								goto icmd_iconst_tail;
							case ICMD_IXOR:
								iptr[0].opc = ICMD_IXORCONST;
								goto icmd_iconst_tail;
							case ICMD_ISHL:
								iptr[0].opc = ICMD_ISHLCONST;
								goto icmd_iconst_tail;
							case ICMD_ISHR:
								iptr[0].opc = ICMD_ISHRCONST;
								goto icmd_iconst_tail;
							case ICMD_IUSHR:
								iptr[0].opc = ICMD_IUSHRCONST;
								goto icmd_iconst_tail;
#if SUPPORT_LONG_SHIFT
							case ICMD_LSHL:
								iptr[0].opc = ICMD_LSHLCONST;
#if defined(__I386__)
								method_uses_ecx = true;
#endif
								goto icmd_lconst_tail;
							case ICMD_LSHR:
								iptr[0].opc = ICMD_LSHRCONST;
#if defined(__I386__)
								method_uses_ecx = true;
#endif
								goto icmd_lconst_tail;
							case ICMD_LUSHR:
								iptr[0].opc = ICMD_LUSHRCONST;
#if defined(__I386__)
								method_uses_ecx = true;
#endif
								goto icmd_lconst_tail;
#endif
							case ICMD_IF_ICMPEQ:
								iptr[0].opc = ICMD_IFEQ;
							icmd_if_icmp_tail:
								iptr[0].op1 = iptr[1].op1;
								bptr->icount--;
								len--;
								/* iptr[1].opc = ICMD_NOP; */
								OP1_0(TYPE_INT);
								tbptr = m->basicblocks + m->basicblockindex[iptr->op1];

								iptr[0].target = (void *) tbptr;

								MARKREACHED(tbptr, copy);
								COUNT(count_pcmd_bra);
								break;
							case ICMD_IF_ICMPLT:
								iptr[0].opc = ICMD_IFLT;
								goto icmd_if_icmp_tail;
							case ICMD_IF_ICMPLE:
								iptr[0].opc = ICMD_IFLE;
								goto icmd_if_icmp_tail;
							case ICMD_IF_ICMPNE:
								iptr[0].opc = ICMD_IFNE;
								goto icmd_if_icmp_tail;
							case ICMD_IF_ICMPGT:
								iptr[0].opc = ICMD_IFGT;
								goto icmd_if_icmp_tail;
							case ICMD_IF_ICMPGE:
								iptr[0].opc = ICMD_IFGE;
								goto icmd_if_icmp_tail;
							default:
								PUSHCONST(TYPE_INT);
							}
						}
						else
							PUSHCONST(TYPE_INT);
						break;
					case ICMD_LCONST:
						COUNT(count_pcmd_load);
						if (len > 0) {
							switch (iptr[1].opc) {
#if SUPPORT_LONG_ADD
							case ICMD_LADD:
								iptr[0].opc = ICMD_LADDCONST;
							icmd_lconst_tail:
								iptr[1].opc = ICMD_NOP;
								OP1_1(TYPE_LNG,TYPE_LNG);
								COUNT(count_pcmd_op);
								break;
							case ICMD_LSUB:
								iptr[0].opc = ICMD_LSUBCONST;
								goto icmd_lconst_tail;
#endif
#if SUPPORT_LONG_MUL
							case ICMD_LMUL:
								iptr[0].opc = ICMD_LMULCONST;
#if defined(__I386__)
								method_uses_ecx = true;
								method_uses_edx = true;
#endif
								goto icmd_lconst_tail;
#endif
#if SUPPORT_LONG_DIV
							case ICMD_LDIV:
								if (iptr[0].val.l == 0x00000002)
									iptr[0].val.i = 1;
								else if (iptr[0].val.l == 0x00000004)
									iptr[0].val.i = 2;
								else if (iptr[0].val.l == 0x00000008)
									iptr[0].val.i = 3;
								else if (iptr[0].val.l == 0x00000010)
									iptr[0].val.i = 4;
								else if (iptr[0].val.l == 0x00000020)
									iptr[0].val.i = 5;
								else if (iptr[0].val.l == 0x00000040)
									iptr[0].val.i = 6;
								else if (iptr[0].val.l == 0x00000080)
									iptr[0].val.i = 7;
								else if (iptr[0].val.l == 0x00000100)
									iptr[0].val.i = 8;
								else if (iptr[0].val.l == 0x00000200)
									iptr[0].val.i = 9;
								else if (iptr[0].val.l == 0x00000400)
									iptr[0].val.i = 10;
								else if (iptr[0].val.l == 0x00000800)
									iptr[0].val.i = 11;
								else if (iptr[0].val.l == 0x00001000)
									iptr[0].val.i = 12;
								else if (iptr[0].val.l == 0x00002000)
									iptr[0].val.i = 13;
								else if (iptr[0].val.l == 0x00004000)
									iptr[0].val.i = 14;
								else if (iptr[0].val.l == 0x00008000)
									iptr[0].val.i = 15;
								else if (iptr[0].val.l == 0x00010000)
									iptr[0].val.i = 16;
								else if (iptr[0].val.l == 0x00020000)
									iptr[0].val.i = 17;
								else if (iptr[0].val.l == 0x00040000)
									iptr[0].val.i = 18;
								else if (iptr[0].val.l == 0x00080000)
									iptr[0].val.i = 19;
								else if (iptr[0].val.l == 0x00100000)
									iptr[0].val.i = 20;
								else if (iptr[0].val.l == 0x00200000)
									iptr[0].val.i = 21;
								else if (iptr[0].val.l == 0x00400000)
									iptr[0].val.i = 22;
								else if (iptr[0].val.l == 0x00800000)
									iptr[0].val.i = 23;
								else if (iptr[0].val.l == 0x01000000)
									iptr[0].val.i = 24;
								else if (iptr[0].val.l == 0x02000000)
									iptr[0].val.i = 25;
								else if (iptr[0].val.l == 0x04000000)
									iptr[0].val.i = 26;
								else if (iptr[0].val.l == 0x08000000)
									iptr[0].val.i = 27;
								else if (iptr[0].val.l == 0x10000000)
									iptr[0].val.i = 28;
								else if (iptr[0].val.l == 0x20000000)
									iptr[0].val.i = 29;
								else if (iptr[0].val.l == 0x40000000)
									iptr[0].val.i = 30;
								else if (iptr[0].val.l == 0x80000000)
									iptr[0].val.i = 31;
								else {
									PUSHCONST(TYPE_LNG);
									break;
								}
								iptr[0].opc = ICMD_LDIVPOW2;
#if defined(__I386__)
								method_uses_ecx = true;
#endif
								goto icmd_lconst_tail;
							case ICMD_LREM:
#if !defined(NO_DIV_OPT)
								if (iptr[0].val.l == 0x10001) {
									iptr[0].opc = ICMD_LREM0X10001;
									goto icmd_lconst_tail;
								}
#endif
								if ((iptr[0].val.l == 0x00000002) ||
									(iptr[0].val.l == 0x00000004) ||
									(iptr[0].val.l == 0x00000008) ||
									(iptr[0].val.l == 0x00000010) ||
									(iptr[0].val.l == 0x00000020) ||
									(iptr[0].val.l == 0x00000040) ||
									(iptr[0].val.l == 0x00000080) ||
									(iptr[0].val.l == 0x00000100) ||
									(iptr[0].val.l == 0x00000200) ||
									(iptr[0].val.l == 0x00000400) ||
									(iptr[0].val.l == 0x00000800) ||
									(iptr[0].val.l == 0x00001000) ||
									(iptr[0].val.l == 0x00002000) ||
									(iptr[0].val.l == 0x00004000) ||
									(iptr[0].val.l == 0x00008000) ||
									(iptr[0].val.l == 0x00010000) ||
									(iptr[0].val.l == 0x00020000) ||
									(iptr[0].val.l == 0x00040000) ||
									(iptr[0].val.l == 0x00080000) ||
									(iptr[0].val.l == 0x00100000) ||
									(iptr[0].val.l == 0x00200000) ||
									(iptr[0].val.l == 0x00400000) ||
									(iptr[0].val.l == 0x00800000) ||
									(iptr[0].val.l == 0x01000000) ||
									(iptr[0].val.l == 0x02000000) ||
									(iptr[0].val.l == 0x04000000) ||
									(iptr[0].val.l == 0x08000000) ||
									(iptr[0].val.l == 0x10000000) ||
									(iptr[0].val.l == 0x20000000) ||
									(iptr[0].val.l == 0x40000000) ||
									(iptr[0].val.l == 0x80000000)) {
									iptr[0].opc = ICMD_LREMPOW2;
									iptr[0].val.l -= 1;
#if defined(__I386__)
									method_uses_ecx = true;
#endif
									goto icmd_lconst_tail;
								}
								PUSHCONST(TYPE_LNG);
								break;
#endif
#if SUPPORT_LONG_LOG
							case ICMD_LAND:
								iptr[0].opc = ICMD_LANDCONST;
								goto icmd_lconst_tail;
							case ICMD_LOR:
								iptr[0].opc = ICMD_LORCONST;
								goto icmd_lconst_tail;
							case ICMD_LXOR:
								iptr[0].opc = ICMD_LXORCONST;
								goto icmd_lconst_tail;
#endif
#if !defined(NOLONG_CONDITIONAL)
							case ICMD_LCMP:
								if ((len > 1) && (iptr[2].val.i == 0)) {
									switch (iptr[2].opc) {
									case ICMD_IFEQ:
										iptr[0].opc = ICMD_IF_LEQ;
#if defined(__I386__)
										method_uses_ecx = true;
#endif
									icmd_lconst_lcmp_tail:
										iptr[0].op1 = iptr[2].op1;
										bptr->icount -= 2;
										len -= 2;
										/* iptr[1].opc = ICMD_NOP;
										   iptr[2].opc = ICMD_NOP; */
										OP1_0(TYPE_LNG);
										tbptr = m->basicblocks + m->basicblockindex[iptr->op1];

										iptr[0].target = (void *) tbptr;

										MARKREACHED(tbptr, copy);
										COUNT(count_pcmd_bra);
										COUNT(count_pcmd_op);
										break;
									case ICMD_IFNE:
										iptr[0].opc = ICMD_IF_LNE;
#if defined(__I386__)
										method_uses_ecx = true;
#endif
										goto icmd_lconst_lcmp_tail;
									case ICMD_IFLT:
										iptr[0].opc = ICMD_IF_LLT;
										goto icmd_lconst_lcmp_tail;
									case ICMD_IFGT:
										iptr[0].opc = ICMD_IF_LGT;
										goto icmd_lconst_lcmp_tail;
									case ICMD_IFLE:
										iptr[0].opc = ICMD_IF_LLE;
										goto icmd_lconst_lcmp_tail;
									case ICMD_IFGE:
										iptr[0].opc = ICMD_IF_LGE;
										goto icmd_lconst_lcmp_tail;
									default:
										PUSHCONST(TYPE_LNG);
									} /* switch (iptr[2].opc) */
								} /* if (iptr[2].val.i == 0) */
								else
									PUSHCONST(TYPE_LNG);
								break;
#endif
							default:
								PUSHCONST(TYPE_LNG);
							}
						}
						else
							PUSHCONST(TYPE_LNG);
						break;
					case ICMD_FCONST:
						COUNT(count_pcmd_load);
						PUSHCONST(TYPE_FLT);
						break;
					case ICMD_DCONST:
						COUNT(count_pcmd_load);
						PUSHCONST(TYPE_DBL);
						break;
					case ICMD_ACONST:
						COUNT(count_pcmd_load);
						PUSHCONST(TYPE_ADR);
						break;

						/* pop 0 push 1 load */
						
					case ICMD_ILOAD:
					case ICMD_LLOAD:
					case ICMD_FLOAD:
					case ICMD_DLOAD:
					case ICMD_ALOAD:
						COUNT(count_load_instruction);
						i = opcode-ICMD_ILOAD;
						iptr->op1 = argren[iptr->op1];
						locals[iptr->op1][i].type = i;
						LOAD(i, LOCALVAR, iptr->op1);
						break;

						/* pop 2 push 1 */

					case ICMD_LALOAD:
#if defined(__I386__)
						method_uses_ecx = true;
						method_uses_edx = true;
#endif
					case ICMD_IALOAD:
					case ICMD_FALOAD:
					case ICMD_DALOAD:
					case ICMD_AALOAD:
						COUNT(count_check_null);
						COUNT(count_check_bound);
						COUNT(count_pcmd_mem);
						OP2IAT_1(opcode-ICMD_IALOAD);
#if defined(__I386__)
						method_uses_ecx = true;
#endif
						break;

					case ICMD_BALOAD:
					case ICMD_CALOAD:
					case ICMD_SALOAD:
						COUNT(count_check_null);
						COUNT(count_check_bound);
						COUNT(count_pcmd_mem);
						OP2IAT_1(TYPE_INT);
#if defined(__I386__)
						method_uses_ecx = true;
#endif
						break;

						/* pop 0 push 0 iinc */

					case ICMD_IINC:
#ifdef STATISTICS
						i = stackdepth;
						if (i >= 10)
							count_store_depth[10]++;
						else
							count_store_depth[i]++;
#endif
						copy = curstack;
						i = stackdepth - 1;
						while (copy) {
							if ((copy->varkind == LOCALVAR) &&
								(copy->varnum == iptr->op1)) {
								copy->varkind = TEMPVAR;
								copy->varnum = i;
							}
							i--;
							copy = copy->prev;
						}
						SETDST;
						break;

						/* pop 1 push 0 store */

					case ICMD_ISTORE:
					case ICMD_LSTORE:
					case ICMD_FSTORE:
					case ICMD_DSTORE:
					case ICMD_ASTORE:
					icmd_store:
						REQUIRE_1;

					i = opcode - ICMD_ISTORE;
					locals[iptr->op1][i].type = i;
#ifdef STATISTICS
					count_pcmd_store++;
					i = new - curstack;
					if (i >= 20)
						count_store_length[20]++;
					else
						count_store_length[i]++;
					i = stackdepth - 1;
					if (i >= 10)
						count_store_depth[10]++;
					else
						count_store_depth[i]++;
#endif
					copy = curstack->prev;
					i = stackdepth - 2;
					while (copy) {
						if ((copy->varkind == LOCALVAR) &&
							(copy->varnum == iptr->op1)) {
							copy->varkind = TEMPVAR;
							copy->varnum = i;
						}
						i--;
						copy = copy->prev;
					}
					if ((new - curstack) == 1) {
						curstack->varkind = LOCALVAR;
						curstack->varnum = iptr->op1;
					};
					STORE(opcode-ICMD_ISTORE);
					break;

					/* pop 3 push 0 */

					case ICMD_IASTORE:
					case ICMD_AASTORE:
					case ICMD_LASTORE:
#if defined(__I386__)
						method_uses_ecx = true;
						method_uses_edx = true;
#endif
					case ICMD_FASTORE:
					case ICMD_DASTORE:
						COUNT(count_check_null);
						COUNT(count_check_bound);
						COUNT(count_pcmd_mem);
						OP3TIA_0(opcode-ICMD_IASTORE);
						break;

					case ICMD_BASTORE:
					case ICMD_CASTORE:
					case ICMD_SASTORE:
						COUNT(count_check_null);
						COUNT(count_check_bound);
						COUNT(count_pcmd_mem);
						OP3TIA_0(TYPE_INT);
#if defined(__I386__)
						method_uses_ecx = true;
						method_uses_edx = true;
#endif
						break;

						/* pop 1 push 0 */

					case ICMD_POP:
#ifdef TYPECHECK_STACK_COMPCAT
						if (opt_verify) {
							REQUIRE_1;
							if (IS_2_WORD_TYPE(curstack->type))
								panic("Illegal instruction: POP on category 2 type");
						}
#endif
						OP1_0ANY;
						break;

					case ICMD_IRETURN:
					case ICMD_LRETURN:
					case ICMD_FRETURN:
					case ICMD_DRETURN:
					case ICMD_ARETURN:
						COUNT(count_pcmd_return);
						OP1_0(opcode-ICMD_IRETURN);
						superblockend = true;
						break;

					case ICMD_ATHROW:
						COUNT(count_check_null);
						OP1_0(TYPE_ADR);
						STACKRESET;
						SETDST;
						superblockend = true;
						break;

					case ICMD_PUTSTATIC:
						COUNT(count_pcmd_mem);
						OP1_0(iptr->op1);
#if defined(__I386__)
						method_uses_ecx = true;
#endif
						break;

						/* pop 1 push 0 branch */

					case ICMD_IFNULL:
					case ICMD_IFNONNULL:
						COUNT(count_pcmd_bra);
						OP1_0(TYPE_ADR);
						tbptr = m->basicblocks + m->basicblockindex[iptr->op1];

						iptr[0].target = (void *) tbptr;

						MARKREACHED(tbptr, copy);
						break;

					case ICMD_IFEQ:
					case ICMD_IFNE:
					case ICMD_IFLT:
					case ICMD_IFGE:
					case ICMD_IFGT:
					case ICMD_IFLE:
						COUNT(count_pcmd_bra);
#ifdef CONDITIONAL_LOADCONST
						{
							tbptr = m->basicblocks + b_index;
							if ((b_count >= 3) &&
							    ((b_index + 2) == m->basicblockindex[iptr[0].op1]) &&
							    (tbptr[1].pre_count == 1) &&
							    (iptr[1].opc == ICMD_ICONST) &&
							    (iptr[2].opc == ICMD_GOTO)   &&
							    ((b_index + 3) == m->basicblockindex[iptr[2].op1]) &&
							    (tbptr[2].pre_count == 1) &&
							    (iptr[3].opc == ICMD_ICONST)) {
								OP1_1(TYPE_INT, TYPE_INT);
								switch (iptr[0].opc) {
								case ICMD_IFEQ:
									iptr[0].opc = ICMD_IFNE_ICONST;
									break;
								case ICMD_IFNE:
									iptr[0].opc = ICMD_IFEQ_ICONST;
									break;
								case ICMD_IFLT:
									iptr[0].opc = ICMD_IFGE_ICONST;
									break;
								case ICMD_IFGE:
									iptr[0].opc = ICMD_IFLT_ICONST;
									break;
								case ICMD_IFGT:
									iptr[0].opc = ICMD_IFLE_ICONST;
									break;
								case ICMD_IFLE:
									iptr[0].opc = ICMD_IFGT_ICONST;
									break;
								}
								iptr[0].val.i = iptr[1].val.i;
								iptr[1].opc = ICMD_ELSE_ICONST;
								iptr[1].val.i = iptr[3].val.i;
								iptr[2].opc = ICMD_NOP;
								iptr[3].opc = ICMD_NOP;
								tbptr[1].flags = BBDELETED;
								tbptr[2].flags = BBDELETED;
								tbptr[1].icount = 0;
								tbptr[2].icount = 0;
								if (tbptr[3].pre_count == 2) {
									len += tbptr[3].icount + 3;
									bptr->icount += tbptr[3].icount + 3;
									tbptr[3].flags = BBDELETED;
									tbptr[3].icount = 0;
									b_index++;
								}
								else {
									bptr->icount++;
									len ++;
								}
								b_index += 2;
								break;
							}
						}
#endif
						OP1_0(TYPE_INT);
						tbptr = m->basicblocks + m->basicblockindex[iptr->op1];

						iptr[0].target = (void *) tbptr;

						MARKREACHED(tbptr, copy);
						break;

						/* pop 0 push 0 branch */

					case ICMD_GOTO:
						COUNT(count_pcmd_bra);
						tbptr = m->basicblocks + m->basicblockindex[iptr->op1];

						iptr[0].target = (void *) tbptr;

						MARKREACHED(tbptr, copy);
						SETDST;
						superblockend = true;
						break;

						/* pop 1 push 0 table branch */

					case ICMD_TABLESWITCH:
						COUNT(count_pcmd_table);
						OP1_0(TYPE_INT);
						s4ptr = iptr->val.a;
						tbptr = m->basicblocks + m->basicblockindex[*s4ptr++];
						MARKREACHED(tbptr, copy);
						i = *s4ptr++;                          /* low     */
						i = *s4ptr++ - i + 1;                  /* high    */

						tptr = DMNEW(void*, i+1);
						iptr->target = (void *) tptr;

						tptr[0] = (void *) tbptr;
						tptr++;

						while (--i >= 0) {
							tbptr = m->basicblocks + m->basicblockindex[*s4ptr++];

							tptr[0] = (void *) tbptr;
							tptr++;

							MARKREACHED(tbptr, copy);
						}
						SETDST;
						superblockend = true;
#if defined(__I386__)
						method_uses_ecx = true;
#endif
						break;
							
						/* pop 1 push 0 table branch */

					case ICMD_LOOKUPSWITCH:
						COUNT(count_pcmd_table);
						OP1_0(TYPE_INT);
						s4ptr = iptr->val.a;
						tbptr = m->basicblocks + m->basicblockindex[*s4ptr++];
						MARKREACHED(tbptr, copy);
						i = *s4ptr++;                          /* count   */

						tptr = DMNEW(void*, i+1);
						iptr->target = (void *) tptr;

						tptr[0] = (void *) tbptr;
						tptr++;

						while (--i >= 0) {
							tbptr = m->basicblocks + m->basicblockindex[s4ptr[1]];

							tptr[0] = (void *) tbptr;
							tptr++;
								
							MARKREACHED(tbptr, copy);
							s4ptr += 2;
						}
						SETDST;
						superblockend = true;
						break;

					case ICMD_NULLCHECKPOP:
					case ICMD_MONITORENTER:
						COUNT(count_check_null);
					case ICMD_MONITOREXIT:
						OP1_0(TYPE_ADR);
						break;

						/* pop 2 push 0 branch */

					case ICMD_IF_ICMPEQ:
					case ICMD_IF_ICMPNE:
					case ICMD_IF_ICMPLT:
					case ICMD_IF_ICMPGE:
					case ICMD_IF_ICMPGT:
					case ICMD_IF_ICMPLE:
						COUNT(count_pcmd_bra);
						OP2_0(TYPE_INT);
						tbptr = m->basicblocks + m->basicblockindex[iptr->op1];
							
						iptr[0].target = (void *) tbptr;

						MARKREACHED(tbptr, copy);
						break;

					case ICMD_IF_ACMPEQ:
					case ICMD_IF_ACMPNE:
						COUNT(count_pcmd_bra);
						OP2_0(TYPE_ADR);
						tbptr = m->basicblocks + m->basicblockindex[iptr->op1];

						iptr[0].target = (void *) tbptr;

						MARKREACHED(tbptr, copy);
						break;

						/* pop 2 push 0 */

					case ICMD_PUTFIELD:
						COUNT(count_check_null);
						COUNT(count_pcmd_mem);
						OPTT2_0(iptr->op1,TYPE_ADR);
#if defined(__I386__)
						method_uses_ecx = true;
#endif
						break;

					case ICMD_POP2:
						REQUIRE_1;
						if (! IS_2_WORD_TYPE(curstack->type)) {
							/* ..., cat1 */
#ifdef TYPECHECK_STACK_COMPCAT
							if (opt_verify) {
								REQUIRE_2;
								if (IS_2_WORD_TYPE(curstack->prev->type))
									panic("Illegal instruction: POP2 on cat2, cat1 types");
							}
#endif
							OP1_0ANY;                /* second pop */
						}
						else
							iptr->opc = ICMD_POP;
						OP1_0ANY;
						break;

						/* pop 0 push 1 dup */
						
					case ICMD_DUP:
#ifdef TYPECHECK_STACK_COMPCAT
						if (opt_verify) {
							REQUIRE_1;
							if (IS_2_WORD_TYPE(curstack->type))
								panic("Illegal instruction: DUP on category 2 type");
						}
#endif
						COUNT(count_dup_instruction);
						DUP;
						break;

					case ICMD_DUP2:
						REQUIRE_1;
						if (IS_2_WORD_TYPE(curstack->type)) {
							/* ..., cat2 */
							iptr->opc = ICMD_DUP;
							DUP;
						}
						else {
							REQUIRE_2;
							/* ..., ????, cat1 */
#ifdef TYPECHECK_STACK_COMPCAT
							if (opt_verify) {
								if (IS_2_WORD_TYPE(curstack->prev->type))
									panic("Illegal instruction: DUP2 on cat2, cat1 types");
							}
#endif
							copy = curstack;
							NEWSTACK(copy->prev->type, copy->prev->varkind,
									 copy->prev->varnum);
							NEWSTACK(copy->type, copy->varkind,
									 copy->varnum);
							SETDST;
							stackdepth+=2;
						}
						break;

						/* pop 2 push 3 dup */
						
					case ICMD_DUP_X1:
#ifdef TYPECHECK_STACK_COMPCAT
						if (opt_verify) {
							REQUIRE_2;
							if (IS_2_WORD_TYPE(curstack->type) ||
								IS_2_WORD_TYPE(curstack->prev->type))
								panic("Illegal instruction: DUP_X1 on cat 2 type");
						}
#endif
						DUP_X1;
						break;

					case ICMD_DUP2_X1:
						REQUIRE_2;
						if (IS_2_WORD_TYPE(curstack->type)) {
							/* ..., ????, cat2 */
#ifdef TYPECHECK_STACK_COMPCAT
							if (opt_verify) {
								if (IS_2_WORD_TYPE(curstack->prev->type))
									panic("Illegal instruction: DUP2_X1 on cat2, cat2 types");
							}
#endif
							iptr->opc = ICMD_DUP_X1;
							DUP_X1;
						}
						else {
							/* ..., ????, cat1 */
#ifdef TYPECHECK_STACK_COMPCAT
							if (opt_verify) {
								REQUIRE_3;
								if (IS_2_WORD_TYPE(curstack->prev->type)
									|| IS_2_WORD_TYPE(curstack->prev->prev->type))
									panic("Illegal instruction: DUP2_X1 on invalid types");
							}
#endif
							DUP2_X1;
						}
						break;

						/* pop 3 push 4 dup */
						
					case ICMD_DUP_X2:
						REQUIRE_2;
						if (IS_2_WORD_TYPE(curstack->prev->type)) {
							/* ..., cat2, ???? */
#ifdef TYPECHECK_STACK_COMPCAT
							if (opt_verify) {
								if (IS_2_WORD_TYPE(curstack->type))
									panic("Illegal instruction: DUP_X2 on cat2, cat2 types");
							}
#endif
							iptr->opc = ICMD_DUP_X1;
							DUP_X1;
						}
						else {
							/* ..., cat1, ???? */
#ifdef TYPECHECK_STACK_COMPCAT
							if (opt_verify) {
								REQUIRE_3;
								if (IS_2_WORD_TYPE(curstack->type)
									|| IS_2_WORD_TYPE(curstack->prev->prev->type))
									panic("Illegal instruction: DUP_X2 on invalid types");
							}
#endif
							DUP_X2;
						}
						break;

					case ICMD_DUP2_X2:
						REQUIRE_2;
						if (IS_2_WORD_TYPE(curstack->type)) {
							/* ..., ????, cat2 */
							if (IS_2_WORD_TYPE(curstack->prev->type)) {
								/* ..., cat2, cat2 */
								iptr->opc = ICMD_DUP_X1;
								DUP_X1;
							}
							else {
								/* ..., cat1, cat2 */
#ifdef TYPECHECK_STACK_COMPCAT
								if (opt_verify) {
									REQUIRE_3;
									if (IS_2_WORD_TYPE(curstack->prev->prev->type))
										panic("Illegal instruction: DUP2_X2 on invalid types");
								}
#endif
								iptr->opc = ICMD_DUP_X2;
								DUP_X2;
							}
						}
						else {
							REQUIRE_3;
							/* ..., ????, ????, cat1 */
							if (IS_2_WORD_TYPE(curstack->prev->prev->type)) {
								/* ..., cat2, ????, cat1 */
#ifdef TYPECHECK_STACK_COMPCAT
								if (opt_verify) {
									if (IS_2_WORD_TYPE(curstack->prev->type))
										panic("Illegal instruction: DUP2_X2 on invalid types");
								}
#endif
								iptr->opc = ICMD_DUP2_X1;
								DUP2_X1;
							}
							else {
								/* ..., cat1, ????, cat1 */
#ifdef TYPECHECK_STACK_COMPCAT
								if (opt_verify) {
									REQUIRE_4;
									if (IS_2_WORD_TYPE(curstack->prev->type)
										|| IS_2_WORD_TYPE(curstack->prev->prev->prev->type))
										panic("Illegal instruction: DUP2_X2 on invalid types");
								}
#endif
								DUP2_X2;
							}
						}
						break;

						/* pop 2 push 2 swap */
						
					case ICMD_SWAP:
#ifdef TYPECHECK_STACK_COMPCAT
						if (opt_verify) {
							REQUIRE_2;
							if (IS_2_WORD_TYPE(curstack->type)
								|| IS_2_WORD_TYPE(curstack->prev->type))
								panic("Illegal instruction: SWAP on category 2 type");
						}
#endif
						SWAP;
						break;

						/* pop 2 push 1 */
						
					case ICMD_IDIV:
#if !SUPPORT_DIVISION
						iptr[0].opc = ICMD_BUILTIN2;
						iptr[0].op1 = TYPE_INT;
						iptr[0].val.a = BUILTIN_idiv;
						m->isleafmethod = false;
						goto builtin2;
#endif

					case ICMD_IREM:
#if !SUPPORT_DIVISION
						iptr[0].opc = ICMD_BUILTIN2;
						iptr[0].op1 = TYPE_INT;
						iptr[0].val.a = BUILTIN_irem;
						m->isleafmethod = false;
						goto builtin2;
#endif
#if defined(__I386__)
						method_uses_ecx = true;
						method_uses_edx = true;
#endif

					case ICMD_ISHL:
					case ICMD_ISHR:
					case ICMD_IUSHR:
#if defined(__I386__)
						method_uses_ecx = true;
#endif
					case ICMD_IADD:
					case ICMD_ISUB:
					case ICMD_IMUL:
					case ICMD_IAND:
					case ICMD_IOR:
					case ICMD_IXOR:
						COUNT(count_pcmd_op);
						OP2_1(TYPE_INT);
						break;

					case ICMD_LDIV:
#if !(SUPPORT_DIVISION && SUPPORT_LONG && SUPPORT_LONG_DIV)
						iptr[0].opc = ICMD_BUILTIN2;
						iptr[0].op1 = TYPE_LNG;
						iptr[0].val.a = BUILTIN_ldiv;
						m->isleafmethod = false;
						goto builtin2;
#endif

					case ICMD_LREM:
#if !(SUPPORT_DIVISION && SUPPORT_LONG && SUPPORT_LONG_DIV)
						iptr[0].opc = ICMD_BUILTIN2;
						iptr[0].op1 = TYPE_LNG;
						iptr[0].val.a = BUILTIN_lrem;
						m->isleafmethod = false;
						goto builtin2;
#endif

					case ICMD_LMUL:
#if defined(__I386__)
						method_uses_ecx = true;
						method_uses_edx = true;
#endif
					case ICMD_LADD:
					case ICMD_LSUB:
					case ICMD_LOR:
					case ICMD_LAND:
					case ICMD_LXOR:
						/* DEBUG */ /*dolog("OP2_1(TYPE_LNG)"); */
						COUNT(count_pcmd_op);
						OP2_1(TYPE_LNG);
						break;

					case ICMD_LSHL:
					case ICMD_LSHR:
					case ICMD_LUSHR:
						COUNT(count_pcmd_op);
						OP2IT_1(TYPE_LNG);
#if defined(__I386__)
						method_uses_ecx = true;
						method_uses_edx = true;
#endif
						break;

					case ICMD_FADD:
					case ICMD_FSUB:
					case ICMD_FMUL:
					case ICMD_FDIV:
					case ICMD_FREM:
						COUNT(count_pcmd_op);
						OP2_1(TYPE_FLT);
						break;

					case ICMD_DADD:
					case ICMD_DSUB:
					case ICMD_DMUL:
					case ICMD_DDIV:
					case ICMD_DREM:
						COUNT(count_pcmd_op);
						OP2_1(TYPE_DBL);
						break;

					case ICMD_LCMP:
						COUNT(count_pcmd_op);
#if !defined(NOLONG_CONDITIONAL)
						if ((len > 0) && (iptr[1].val.i == 0)) {
							switch (iptr[1].opc) {
							case ICMD_IFEQ:
								iptr[0].opc = ICMD_IF_LCMPEQ;
#if defined(__I386__)
								method_uses_ecx = true;
#endif
							icmd_lcmp_if_tail:
								iptr[0].op1 = iptr[1].op1;
								len--;
								bptr->icount--;
								/* iptr[1].opc = ICMD_NOP; */
								OP2_0(TYPE_LNG);
								tbptr = m->basicblocks + m->basicblockindex[iptr->op1];
			
								iptr[0].target = (void *) tbptr;

								MARKREACHED(tbptr, copy);
								COUNT(count_pcmd_bra);
								break;
							case ICMD_IFNE:
								iptr[0].opc = ICMD_IF_LCMPNE;
#if defined(__I386__)
								method_uses_ecx = true;
#endif
								goto icmd_lcmp_if_tail;
							case ICMD_IFLT:
								iptr[0].opc = ICMD_IF_LCMPLT;
								goto icmd_lcmp_if_tail;
							case ICMD_IFGT:
								iptr[0].opc = ICMD_IF_LCMPGT;
								goto icmd_lcmp_if_tail;
							case ICMD_IFLE:
								iptr[0].opc = ICMD_IF_LCMPLE;
								goto icmd_lcmp_if_tail;
							case ICMD_IFGE:
								iptr[0].opc = ICMD_IF_LCMPGE;
								goto icmd_lcmp_if_tail;
							default:
								OPTT2_1(TYPE_LNG, TYPE_INT);
							}
						}
						else
#endif
							OPTT2_1(TYPE_LNG, TYPE_INT);
						break;
					case ICMD_FCMPL:
					case ICMD_FCMPG:
						COUNT(count_pcmd_op);
						OPTT2_1(TYPE_FLT, TYPE_INT);
						break;
					case ICMD_DCMPL:
					case ICMD_DCMPG:
						COUNT(count_pcmd_op);
						OPTT2_1(TYPE_DBL, TYPE_INT);
						break;

						/* pop 1 push 1 */
						
					case ICMD_INEG:
					case ICMD_INT2BYTE:
					case ICMD_INT2CHAR:
					case ICMD_INT2SHORT:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_INT, TYPE_INT);
						break;
					case ICMD_LNEG:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_LNG, TYPE_LNG);
						break;
					case ICMD_FNEG:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_FLT, TYPE_FLT);
						break;
					case ICMD_DNEG:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_DBL, TYPE_DBL);
						break;

					case ICMD_I2L:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_INT, TYPE_LNG);
#if defined(__I386__)
						method_uses_edx = true;
#endif
						break;
					case ICMD_I2F:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_INT, TYPE_FLT);
						break;
					case ICMD_I2D:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_INT, TYPE_DBL);
						break;
					case ICMD_L2I:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_LNG, TYPE_INT);
						break;
					case ICMD_L2F:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_LNG, TYPE_FLT);
						break;
					case ICMD_L2D:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_LNG, TYPE_DBL);
						break;
					case ICMD_F2I:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_FLT, TYPE_INT);
						break;
					case ICMD_F2L:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_FLT, TYPE_LNG);
#if defined(__I386__)
						method_uses_edx = true;
#endif
						break;
					case ICMD_F2D:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_FLT, TYPE_DBL);
						break;
					case ICMD_D2I:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_DBL, TYPE_INT);
						break;
					case ICMD_D2L:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_DBL, TYPE_LNG);
#if defined(__I386__)
						method_uses_edx = true;
#endif
						break;
					case ICMD_D2F:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_DBL, TYPE_FLT);
						break;

					case ICMD_CHECKCAST:
						OP1_1(TYPE_ADR, TYPE_ADR);
#if defined(__I386__)
						method_uses_ecx = true;
						method_uses_edx = true;
#endif
						break;

					case ICMD_INSTANCEOF:
#if defined(__I386__)
						method_uses_ecx = true;
						method_uses_edx = true;
#endif
					case ICMD_ARRAYLENGTH:
						OP1_1(TYPE_ADR, TYPE_INT);
						break;

					case ICMD_NEWARRAY:
					case ICMD_ANEWARRAY:
						OP1_1(TYPE_INT, TYPE_ADR);
						break;

					case ICMD_GETFIELD:
						COUNT(count_check_null);
						COUNT(count_pcmd_mem);
						OP1_1(TYPE_ADR, iptr->op1);
#if defined(__I386__)
						method_uses_ecx = true;
#endif
						break;

						/* pop 0 push 1 */
						
					case ICMD_GETSTATIC:
						COUNT(count_pcmd_mem);
						OP0_1(iptr->op1);
#if defined(__I386__)
						method_uses_ecx = true;
#endif
						break;

					case ICMD_NEW:
						OP0_1(TYPE_ADR);
						break;

					case ICMD_JSR:
						OP0_1(TYPE_ADR);
						tbptr = m->basicblocks + m->basicblockindex[iptr->op1];

						iptr[0].target = (void *) tbptr;

						/* This is a dirty hack. The typechecker
						 * needs it because the OP1_0ANY below
						 * overwrites iptr->dst.
						 */
						iptr->val.a = (void *) iptr->dst;

						tbptr->type = BBTYPE_SBR;

						/* We need to check for overflow right here because
						 * the pushed value is poped after MARKREACHED. */
						CHECKOVERFLOW;
						MARKREACHED(tbptr, copy);
						OP1_0ANY;
						break;

						/* pop many push any */
						
					case ICMD_INVOKEVIRTUAL:
					case ICMD_INVOKESPECIAL:
					case ICMD_INVOKEINTERFACE:
					case ICMD_INVOKESTATIC:
						COUNT(count_pcmd_met);
#if defined(__I386__)
						method_uses_ecx = true;
#endif
						{
							methodinfo *m = iptr->val.a;
							if (m->flags & ACC_STATIC)
								{COUNT(count_check_null);}
							i = iptr->op1;
							if (i > arguments_num)
								arguments_num = i;
							REQUIRE(i);
#if defined(__X86_64__)
							{
								int iarg = 0;
								int farg = 0;
								int stackargs = 0;

								copy = curstack;
								while (--i >= 0) {
									(IS_FLT_DBL_TYPE(copy->type)) ? farg++ : iarg++;
									copy = copy->prev;
								}

								stackargs += (iarg < intreg_argnum) ? 0 : (iarg - intreg_argnum);
								stackargs += (farg < fltreg_argnum) ? 0 : (farg - fltreg_argnum);

								i = iptr->op1;
								copy = curstack;
								while (--i >= 0) {
									if (!(copy->flags & SAVEDVAR)) {
										copy->varkind = ARGVAR;
										if (IS_FLT_DBL_TYPE(copy->type)) {
											if (--farg < fltreg_argnum) {
												copy->varnum = farg;
											} else {
												copy->varnum = --stackargs + intreg_argnum;
											}
										} else {
											if (--iarg < intreg_argnum) {
												copy->varnum = iarg;
											} else {
												copy->varnum = --stackargs + intreg_argnum;
											}
										}
									} else {
										(IS_FLT_DBL_TYPE(copy->type)) ? --farg : --iarg;
									}
									copy = copy->prev;
								}
							}
#else
							copy = curstack;
							while (--i >= 0) {
								if (! (copy->flags & SAVEDVAR)) {
									copy->varkind = ARGVAR;
									copy->varnum = i;
								}
								copy = copy->prev;
							}
#endif
							while (copy) {
								copy->flags |= SAVEDVAR;
								copy = copy->prev;
							}
							i = iptr->op1;
							POPMANY(i);
							if (m->returntype != TYPE_VOID) {
								OP0_1(m->returntype);
							}
							break;
						}

					case ICMD_BUILTIN3:
						/* DEBUG */ /*dolog("builtin3");*/
						REQUIRE_3;
						if (! (curstack->flags & SAVEDVAR)) {
							curstack->varkind = ARGVAR;
							curstack->varnum = 2;
						}
						if (3 > arguments_num) {
							arguments_num = 3;
						}
						OP1_0ANY;

					case ICMD_BUILTIN2:
					builtin2:
						REQUIRE_2;
						/* DEBUG */ /*dolog("builtin2");*/
					if (!(curstack->flags & SAVEDVAR)) {
						curstack->varkind = ARGVAR;
						curstack->varnum = 1;
					}
					if (2 > arguments_num) {
						arguments_num = 2;
					}
					OP1_0ANY;

					case ICMD_BUILTIN1:
					builtin1:
						REQUIRE_1;
						/* DEBUG */ /*dolog("builtin1");*/
					if (!(curstack->flags & SAVEDVAR)) {
						curstack->varkind = ARGVAR;
						curstack->varnum = 0;
					}
					if (1 > arguments_num) {
						arguments_num = 1;
					}
					OP1_0ANY;
					copy = curstack;
					while (copy) {
						copy->flags |= SAVEDVAR;
						copy = copy->prev;
					}
					if (iptr->op1 != TYPE_VOID)
						OP0_1(iptr->op1);
					break;

					case ICMD_MULTIANEWARRAY:
						i = iptr->op1;
						REQUIRE(i);
						if ((i + intreg_argnum) > arguments_num)
							arguments_num = i + intreg_argnum;
						copy = curstack;
						while (--i >= 0) {
							/* check INT type here? Currently typecheck does this. */
							if (! (copy->flags & SAVEDVAR)) {
								copy->varkind = ARGVAR;
								copy->varnum = i + intreg_argnum;
							}
							copy = copy->prev;
						}
						while (copy) {
							copy->flags |= SAVEDVAR;
							copy = copy->prev;
						}
						i = iptr->op1;
						POPMANY(i);
						OP0_1(TYPE_ADR);
						break;

					case ICMD_CLEAR_ARGREN:
						for (i = iptr->op1; i<m->maxlocals; i++) 
							argren[i] = i;
						iptr->opc = opcode = ICMD_NOP;
						SETDST;
						break;
						
					case ICMD_READONLY_ARG:
					case ICMD_READONLY_ARG+1:
					case ICMD_READONLY_ARG+2:
					case ICMD_READONLY_ARG+3:
					case ICMD_READONLY_ARG+4:

						REQUIRE_1;
						if (curstack->varkind == LOCALVAR) {
							i = curstack->varnum;
							argren[iptr->op1] = i;
							iptr->op1 = i;
						}
						opcode = iptr->opc = opcode - ICMD_READONLY_ARG + ICMD_ISTORE;
						goto icmd_store;

						break;

					default:
						printf("ICMD %d at %d\n", iptr->opc, (s4) (iptr - m->instructions));
						panic("Missing ICMD code during stack analysis");
					} /* switch */

					CHECKOVERFLOW;
					
					/* DEBUG */ /*dolog("iptr++");*/
					iptr++;
				} /* while instructions */
				bptr->outstack = curstack;
				bptr->outdepth = stackdepth;
				BBEND(curstack, i);
			} /* if */
			else
				superblockend = true;
			bptr++;
		} /* while blocks */
	} while (repeat && !deadcode);

#ifdef STATISTICS
	if (m->basicblockcount > count_max_basic_blocks)
		count_max_basic_blocks = m->basicblockcount;
	count_basic_blocks += m->basicblockcount;
	if (m->instructioncount > count_max_javainstr)
		count_max_javainstr = m->instructioncount;
	count_javainstr += m->instructioncount;
	if (m->stackcount > count_upper_bound_new_stack)
		count_upper_bound_new_stack = m->stackcount;
	if ((new - m->stack) > count_max_new_stack)
		count_max_new_stack = (new - m->stack);

	b_count = m->basicblockcount;
	bptr = m->basicblocks;
	while (--b_count >= 0) {
		if (bptr->flags > BBREACHED) {
			if (bptr->indepth >= 10)
				count_block_stack[10]++;
			else
				count_block_stack[bptr->indepth]++;
			len = bptr->icount;
			if (len < 10) 
				count_block_size_distribution[len]++;
			else if (len <= 12)
				count_block_size_distribution[10]++;
			else if (len <= 14)
				count_block_size_distribution[11]++;
			else if (len <= 16)
				count_block_size_distribution[12]++;
			else if (len <= 18)
				count_block_size_distribution[13]++;
			else if (len <= 20)
				count_block_size_distribution[14]++;
			else if (len <= 25)
				count_block_size_distribution[15]++;
			else if (len <= 30)
				count_block_size_distribution[16]++;
			else
				count_block_size_distribution[17]++;
		}
		bptr++;
	}

	if (loops == 1)
		count_analyse_iterations[0]++;
	else if (loops == 2)
		count_analyse_iterations[1]++;
	else if (loops == 3)
		count_analyse_iterations[2]++;
	else if (loops == 4)
		count_analyse_iterations[3]++;
	else
		count_analyse_iterations[4]++;

	if (m->basicblockcount <= 5)
		count_method_bb_distribution[0]++;
	else if (m->basicblockcount <= 10)
		count_method_bb_distribution[1]++;
	else if (m->basicblockcount <= 15)
		count_method_bb_distribution[2]++;
	else if (m->basicblockcount <= 20)
		count_method_bb_distribution[3]++;
	else if (m->basicblockcount <= 30)
		count_method_bb_distribution[4]++;
	else if (m->basicblockcount <= 40)
		count_method_bb_distribution[5]++;
	else if (m->basicblockcount <= 50)
		count_method_bb_distribution[6]++;
	else if (m->basicblockcount <= 75)
		count_method_bb_distribution[7]++;
	else
		count_method_bb_distribution[8]++;
#endif

	/* just return methodinfo* to signal everything was ok */

	return m;
}


/**********************************************************************/
/* DEBUGGING HELPERS                                                  */
/**********************************************************************/

void icmd_print_stack(methodinfo *m, stackptr s)
{
	int i, j;
	stackptr t;

	i = m->maxstack;
	t = s;
	
	while (t) {
		i--;
		t = t->prev;
	}
	j = m->maxstack - i;
	while (--i >= 0)
		printf("    ");
	while (s) {
		j--;
		/* DEBUG */ /*printf("(%d,%d,%d,%d)",s->varkind,s->flags,s->regoff,s->varnum); fflush(stdout);*/
		if (s->flags & SAVEDVAR)
			switch (s->varkind) {
			case TEMPVAR:
				if (s->flags & INMEMORY)
					printf(" M%02d", s->regoff);
				else if ((s->type == TYPE_FLT) || (s->type == TYPE_DBL))
					printf(" F%02d", s->regoff);
				else {
					printf(" %3s", regs[s->regoff]);
				}
				break;
			case STACKVAR:
				printf(" I%02d", s->varnum);
				break;
			case LOCALVAR:
				printf(" L%02d", s->varnum);
				break;
			case ARGVAR:
				printf(" A%02d", s->varnum);
				break;
			default:
				printf(" !%02d", j);
			}
		else
			switch (s->varkind) {
			case TEMPVAR:
				if (s->flags & INMEMORY)
					printf(" m%02d", s->regoff);
				else if ((s->type == TYPE_FLT) || (s->type == TYPE_DBL))
					printf(" f%02d", s->regoff);
				else {
					printf(" %3s", regs[s->regoff]);
				}
				break;
			case STACKVAR:
				printf(" i%02d", s->varnum);
				break;
			case LOCALVAR:
				printf(" l%02d", s->varnum);
				break;
			case ARGVAR:
				printf(" a%02d", s->varnum);
				break;
			default:
				printf(" ?%02d", j);
			}
		s = s->prev;
	}
}


#if 0
static void print_reg(stackptr s) {
	if (s) {
		if (s->flags & SAVEDVAR)
			switch (s->varkind) {
			case TEMPVAR:
				if (s->flags & INMEMORY)
					printf(" tm%02d", s->regoff);
				else
					printf(" tr%02d", s->regoff);
				break;
			case STACKVAR:
				printf(" s %02d", s->varnum);
				break;
			case LOCALVAR:
				printf(" l %02d", s->varnum);
				break;
			case ARGVAR:
				printf(" a %02d", s->varnum);
				break;
			default:
				printf(" ! %02d", s->varnum);
			}
		else
			switch (s->varkind) {
			case TEMPVAR:
				if (s->flags & INMEMORY)
					printf(" Tm%02d", s->regoff);
				else
					printf(" Tr%02d", s->regoff);
				break;
			case STACKVAR:
				printf(" S %02d", s->varnum);
				break;
			case LOCALVAR:
				printf(" L %02d", s->varnum);
				break;
			case ARGVAR:
				printf(" A %02d", s->varnum);
				break;
			default:
				printf(" ? %02d", s->varnum);
			}
	}
	else
		printf("     ");
		
}
#endif


char *icmd_builtin_name(functionptr bptr)
{
	builtin_descriptor *bdesc = builtin_desc;
	while ((bdesc->opcode != 0) && (bdesc->builtin != bptr))
		bdesc++;
	return (bdesc->opcode) ? bdesc->name : "<NOT IN TABLE>";
}


static char *jit_type[] = {
	"int",
	"lng",
	"flt",
	"dbl",
	"adr"
};


void show_icmd_method(methodinfo *m)
{
	int i, j;
	basicblock *bptr;
	exceptiontable *ex;

	printf("\n");
	utf_fprint_classname(stdout, m->class->name);
	printf(".");
	utf_fprint(stdout, m->name);
	utf_fprint_classname(stdout, m->descriptor);
	printf ("\n\nMax locals: %d\n", (int) m->maxlocals);
	printf ("Max stack:  %d\n", (int) m->maxstack);

	printf ("Line number table length: %d\n", m->linenumbercount);

	printf ("Exceptions (Number: %d):\n", m->exceptiontablelength);
	for (ex = m->exceptiontable; ex != NULL; ex = ex->down) {
		printf("    L%03d ... ", ex->start->debug_nr );
		printf("L%03d  = ", ex->end->debug_nr);
		printf("L%03d\n", ex->handler->debug_nr);
	}
	
	printf ("Local Table:\n");
	for (i = 0; i < m->maxlocals; i++) {
		printf("   %3d: ", i);
		for (j = TYPE_INT; j <= TYPE_ADR; j++)
			if (locals[i][j].type >= 0) {
				printf("   (%s) ", jit_type[j]);
				if (locals[i][j].flags & INMEMORY)
					printf("m%2d", locals[i][j].regoff);
				else if ((j == TYPE_FLT) || (j == TYPE_DBL))
					printf("f%02d", locals[i][j].regoff);
				else {
					printf("%3s", regs[locals[i][j].regoff]);
				}
			}
		printf("\n");
	}
	printf("\n");

	printf ("Interface Table:\n");
	for (i = 0; i < m->maxstack; i++) {
		if ((interfaces[i][0].type >= 0) || (interfaces[i][1].type >= 0) ||
		    (interfaces[i][2].type >= 0) || (interfaces[i][3].type >= 0) ||
		    (interfaces[i][4].type >= 0)) {
			printf("   %3d: ", i);
			for (j = TYPE_INT; j <= TYPE_ADR; j++)
				if (interfaces[i][j].type >= 0) {
					printf("   (%s) ", jit_type[j]);
					if (interfaces[i][j].flags & SAVEDVAR) {
						if (interfaces[i][j].flags & INMEMORY)
							printf("M%2d", interfaces[i][j].regoff);
						else if ((j == TYPE_FLT) || (j == TYPE_DBL))
							printf("F%02d", interfaces[i][j].regoff);
						else {
							printf("%3s", regs[interfaces[i][j].regoff]);
						}
					}
					else {
						if (interfaces[i][j].flags & INMEMORY)
							printf("m%2d", interfaces[i][j].regoff);
						else if ((j == TYPE_FLT) || (j == TYPE_DBL))
							printf("f%02d", interfaces[i][j].regoff);
						else {
							printf("%3s", regs[interfaces[i][j].regoff]);
						}
					}
				}
			printf("\n");
		}
	}
	printf("\n");

	if (showdisassemble) {
#if defined(__I386__) || defined(__X86_64__)
		u1 *u1ptr;
		int a;

		u1ptr = m->mcode + dseglen;
		for (i = 0; i < m->basicblocks[0].mpc; i++, u1ptr++) {
			a = disassinstr(u1ptr, i);
			i += a;
			u1ptr += a;
		}
		printf("\n");
#else
		s4 *s4ptr;

		s4ptr = (s4 *) (m->mcode + dseglen);
		for (i = 0; i < m->basicblocks[0].mpc; i += 4, s4ptr++) {
			disassinstr(s4ptr, i); 
		}
		printf("\n");
#endif
	}
	
	for (bptr = m->basicblocks; bptr != NULL; bptr = bptr->next) {
		show_icmd_block(m, bptr);
	}
}


void show_icmd_block(methodinfo *m, basicblock *bptr)
{
	int i, j;
	int deadcode;
	instruction *iptr;

	if (bptr->flags != BBDELETED) {
		deadcode = bptr->flags <= BBREACHED;
		printf("[");
		if (deadcode)
			for (j = m->maxstack; j > 0; j--)
				printf(" ?  ");
		else
			icmd_print_stack(m, bptr->instack);
		printf("] L%03d(%d - %d) flags=%d:\n", bptr->debug_nr, bptr->icount, bptr->pre_count,bptr->flags);
		iptr = bptr->iinstr;

		for (i=0; i < bptr->icount; i++, iptr++) {
			printf("[");
			if (deadcode) {
				for (j = m->maxstack; j > 0; j--)
					printf(" ?  ");
			}
			else
				icmd_print_stack(m, iptr->dst);
			printf("]     %4d  ", i);
			/* DEBUG */ /*fflush(stdout);*/
			show_icmd(iptr,deadcode);
			printf("\n");
		}

		if (showdisassemble && (!deadcode)) {
#if defined(__I386__) || defined(__X86_64__)
			u1 *u1ptr;
			int a;

			printf("\n");
			i = bptr->mpc;
			u1ptr = m->mcode + dseglen + i;

			if (bptr->next != NULL) {
				for (; i < bptr->next->mpc; i++, u1ptr++) {
					a = disassinstr(u1ptr, i);
					i += a;
					u1ptr += a;
				}
				printf("\n");

			} else {
				for (; u1ptr < (u1 *) (m->mcode + m->mcodelength); i++, u1ptr++) {
					a = disassinstr(u1ptr, i); 
					i += a;
					u1ptr += a;
				}
				printf("\n");
			}
#else
			s4 *s4ptr;

			printf("\n");
			i = bptr->mpc;
			s4ptr = (s4 *) (m->mcode + dseglen + i);

			if (bptr->next != NULL) {
				for (; i < bptr->next->mpc; i += 4, s4ptr++) {
					disassinstr(s4ptr, i); 
				}
				printf("\n");

			} else {
				for (; s4ptr < (s4 *) (m->mcode + m->mcodelength); i += 4, s4ptr++) {
					disassinstr(s4ptr, i); 
				}
				printf("\n");
			}
#endif
		}
	}
}


void show_icmd(instruction *iptr, bool deadcode)
{
	int j;
	s4  *s4ptr;
	void **tptr = NULL;
	
	printf("%s", icmd_names[iptr->opc]);

	switch ((int) iptr->opc) {
	case ICMD_IADDCONST:
	case ICMD_ISUBCONST:
	case ICMD_IMULCONST:
	case ICMD_IDIVPOW2:
	case ICMD_IREMPOW2:
	case ICMD_IREM0X10001:
	case ICMD_IANDCONST:
	case ICMD_IORCONST:
	case ICMD_IXORCONST:
	case ICMD_ISHLCONST:
	case ICMD_ISHRCONST:
	case ICMD_IUSHRCONST:
	case ICMD_LSHLCONST:
	case ICMD_LSHRCONST:
	case ICMD_LUSHRCONST:
	case ICMD_ICONST:
	case ICMD_ELSE_ICONST:
	case ICMD_IFEQ_ICONST:
	case ICMD_IFNE_ICONST:
	case ICMD_IFLT_ICONST:
	case ICMD_IFGE_ICONST:
	case ICMD_IFGT_ICONST:
	case ICMD_IFLE_ICONST:
		printf(" %d", iptr->val.i);
		break;

	case ICMD_LADDCONST:
	case ICMD_LSUBCONST:
	case ICMD_LMULCONST:
	case ICMD_LDIVPOW2:
	case ICMD_LREMPOW2:
	case ICMD_LANDCONST:
	case ICMD_LORCONST:
	case ICMD_LXORCONST:
	case ICMD_LCONST:
#if defined(__I386__)
		printf(" %lld", iptr->val.l);
#else
		printf(" %ld", iptr->val.l);
#endif
		break;

	case ICMD_FCONST:
		printf(" %f", iptr->val.f);
		break;

	case ICMD_DCONST:
		printf(" %f", iptr->val.d);
		break;

	case ICMD_ACONST:
		printf(" %p", iptr->val.a);
		break;

	case ICMD_GETFIELD:
	case ICMD_PUTFIELD:
		printf(" %d,", ((fieldinfo *) iptr->val.a)->offset);
	case ICMD_PUTSTATIC:
	case ICMD_GETSTATIC:
		printf(" ");
		utf_fprint(stdout, ((fieldinfo *) iptr->val.a)->class->name);
		printf(".");
		utf_fprint(stdout, ((fieldinfo *) iptr->val.a)->name);
		printf(" (type ");
		utf_fprint(stdout, ((fieldinfo *) iptr->val.a)->descriptor);
		printf(")");
		break;

	case ICMD_IINC:
		printf(" %d + %d", iptr->op1, iptr->val.i);
		break;

	case ICMD_IASTORE:
	case ICMD_SASTORE:
	case ICMD_BASTORE:
	case ICMD_CASTORE:
	case ICMD_LASTORE:
	case ICMD_DASTORE:
	case ICMD_FASTORE:
	case ICMD_AASTORE:

	case ICMD_IALOAD:
	case ICMD_SALOAD:
	case ICMD_BALOAD:
	case ICMD_CALOAD:
	case ICMD_LALOAD:
	case ICMD_DALOAD:
	case ICMD_FALOAD:
	case ICMD_AALOAD:
		if (iptr->op1 != 0)
			printf("(opt.)");
		break;

	case ICMD_RET:
	case ICMD_ILOAD:
	case ICMD_LLOAD:
	case ICMD_FLOAD:
	case ICMD_DLOAD:
	case ICMD_ALOAD:
	case ICMD_ISTORE:
	case ICMD_LSTORE:
	case ICMD_FSTORE:
	case ICMD_DSTORE:
	case ICMD_ASTORE:
		printf(" %d", iptr->op1);
		break;

	case ICMD_NEW:
		printf(" ");
		utf_fprint(stdout,
				   ((classinfo *) iptr->val.a)->name);
		break;

	case ICMD_NEWARRAY:
		switch (iptr->op1) {
		case 4:
			printf(" boolean");
			break;
		case 5:
			printf(" char");
			break;
		case 6:
			printf(" float");
			break;
		case 7:
			printf(" double");
			break;
		case 8:
			printf(" byte");
			break;
		case 9:
			printf(" short");
			break;
		case 10:
			printf(" int");
			break;
		case 11:
			printf(" long");
			break;
		}
		break;

	case ICMD_ANEWARRAY:
		if (iptr->op1) {
			printf(" ");
			utf_fprint(stdout,
					   ((classinfo *) iptr->val.a)->name);
		}
		break;

	case ICMD_MULTIANEWARRAY:
		{
			vftbl *vft;
			printf(" %d ",iptr->op1);
			vft = (vftbl *)iptr->val.a;
			if (vft)
				utf_fprint(stdout,vft->class->name);
			else
				printf("<null>");
		}
		break;

	case ICMD_CHECKCAST:
	case ICMD_INSTANCEOF:
		if (iptr->op1) {
			classinfo *c = iptr->val.a;
			if (c->flags & ACC_INTERFACE)
				printf(" (INTERFACE) ");
			else
				printf(" (CLASS,%3d) ", c->vftbl->diffval);
			utf_fprint(stdout, c->name);
		}
		break;

	case ICMD_BUILTIN3:
	case ICMD_BUILTIN2:
	case ICMD_BUILTIN1:
		printf(" %s", icmd_builtin_name((functionptr) iptr->val.a));
		break;

	case ICMD_INVOKEVIRTUAL:
	case ICMD_INVOKESPECIAL:
	case ICMD_INVOKESTATIC:
	case ICMD_INVOKEINTERFACE:
		printf(" ");
		utf_fprint(stdout,
				   ((methodinfo *) iptr->val.a)->class->name);
		printf(".");
		utf_fprint(stdout,
				   ((methodinfo *) iptr->val.a)->name);
		break;

	case ICMD_IFEQ:
	case ICMD_IFNE:
	case ICMD_IFLT:
	case ICMD_IFGE:
	case ICMD_IFGT:
	case ICMD_IFLE:
		if (deadcode || !iptr->target)
			printf("(%d) op1=%d", iptr->val.i, iptr->op1);
		else
			printf("(%d) L%03d", iptr->val.i, ((basicblock *) iptr->target)->debug_nr);
		break;

	case ICMD_IF_LEQ:
	case ICMD_IF_LNE:
	case ICMD_IF_LLT:
	case ICMD_IF_LGE:
	case ICMD_IF_LGT:
	case ICMD_IF_LLE:
		if (deadcode || !iptr->target)
			printf("(%lld) op1=%d", iptr->val.l, iptr->op1);
		else
			printf("(%lld) L%03d", iptr->val.l, ((basicblock *) iptr->target)->debug_nr);
		break;

	case ICMD_JSR:
	case ICMD_GOTO:
	case ICMD_IFNULL:
	case ICMD_IFNONNULL:
	case ICMD_IF_ICMPEQ:
	case ICMD_IF_ICMPNE:
	case ICMD_IF_ICMPLT:
	case ICMD_IF_ICMPGE:
	case ICMD_IF_ICMPGT:
	case ICMD_IF_ICMPLE:
	case ICMD_IF_LCMPEQ:
	case ICMD_IF_LCMPNE:
	case ICMD_IF_LCMPLT:
	case ICMD_IF_LCMPGE:
	case ICMD_IF_LCMPGT:
	case ICMD_IF_LCMPLE:
	case ICMD_IF_ACMPEQ:
	case ICMD_IF_ACMPNE:
		if (deadcode || !iptr->target)
			printf(" op1=%d", iptr->op1);
		else
			printf(" L%03d", ((basicblock *) iptr->target)->debug_nr);
		break;

	case ICMD_TABLESWITCH:
		s4ptr = (s4*)iptr->val.a;

		if (deadcode || !iptr->target) {
			printf(" %d;", *s4ptr);
		}
		else {
			tptr = (void **) iptr->target;
			printf(" L%03d;", ((basicblock *) *tptr)->debug_nr); 
			tptr++;
		}

		s4ptr++;         /* skip default */
		j = *s4ptr++;                               /* low     */
		j = *s4ptr++ - j;                           /* high    */
		while (j >= 0) {
			if (deadcode || !*tptr)
				printf(" %d", *s4ptr++);
			else {
				printf(" L%03d", ((basicblock *) *tptr)->debug_nr);
				tptr++;
			}
			j--;
		}
		break;

	case ICMD_LOOKUPSWITCH:
		s4ptr = (s4*)iptr->val.a;

		if (deadcode || !iptr->target) {
			printf(" %d;", *s4ptr);
		}
		else {
			tptr = (void **) iptr->target;
			printf(" L%03d;", ((basicblock *) *tptr)->debug_nr);
			tptr++;
		}
		s4ptr++;                                         /* default */
		j = *s4ptr++;                                    /* count   */

		while (--j >= 0) {
			if (deadcode || !*tptr) {
				s4ptr++; /* skip value */
				printf(" %d",*s4ptr++);
			}
			else {
				printf(" L%03d", ((basicblock *) *tptr)->debug_nr);
				tptr++;
			}
		}
		break;
	}
	printf(" Line number: %d, method:",iptr->line);
	utf_display(iptr->method->class->name);
	printf(".");
	utf_display(iptr->method->name);
}


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
