/* src/vm/jit/verify/typecheck.c - typechecking (part of bytecode verification)

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

   Authors: Edwin Steiner

   Changes: Christian Thalinger

   $Id: typecheck.c 2982 2005-07-11 11:14:17Z twisti $

*/


#include <assert.h>
#include <string.h>

#include "vm/global.h" /* must be here because of CACAO_TYPECHECK */

#ifdef CACAO_TYPECHECK

#include "types.h"
#include "mm/memory.h"
#include "toolbox/logging.h"
#include "native/native.h"
#include "vm/builtin.h"
#include "vm/jit/patcher.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/tables.h"
#include "vm/jit/jit.h"
#include "vm/jit/stack.h"
#include "vm/access.h"
#include "vm/resolve.h"


/****************************************************************************/
/* DEBUG HELPERS                                                            */
/****************************************************************************/

#ifdef TYPECHECK_DEBUG
#define TYPECHECK_ASSERT(cond)  assert(cond)
#else
#define TYPECHECK_ASSERT(cond)
#endif

#ifdef TYPECHECK_VERBOSE_OPT
bool typecheckverbose = false;
#define DOLOG(action)  do { if (typecheckverbose) {action;} } while(0)
#else
#define DOLOG(action)
#endif

#ifdef TYPECHECK_VERBOSE
#define TYPECHECK_VERBOSE_IMPORTANT
#define LOG(str)           DOLOG(log_text(str))
#define LOG1(str,a)        DOLOG(dolog(str,a))
#define LOG2(str,a,b)      DOLOG(dolog(str,a,b))
#define LOG3(str,a,b,c)    DOLOG(dolog(str,a,b,c))
#define LOGIF(cond,str)    DOLOG(do {if (cond) log_text(str);} while(0))
#ifdef  TYPEINFO_DEBUG
#define LOGINFO(info)      DOLOG(do {typeinfo_print_short(get_logfile(),(info));log_plain("\n");} while(0))
#else
#define LOGINFO(info)
#define typevectorset_print(x,y,z)
#endif
#define LOGFLUSH           DOLOG(fflush(get_logfile()))
#define LOGNL              DOLOG(log_plain("\n"))
#define LOGSTR(str)        DOLOG(log_plain(str))
#define LOGSTR1(str,a)     DOLOG(dolog_plain(str,a))
#define LOGSTR2(str,a,b)   DOLOG(dolog_plain(str,a,b))
#define LOGSTR3(str,a,b,c) DOLOG(dolog_plain(str,a,b,c))
#define LOGSTRu(utf)       DOLOG(log_plain_utf(utf))
#define LOGNAME(c)         DOLOG(do {log_plain_utf(IS_CLASSREF(c) ? c.ref->name : c.cls->name);} while(0))
#else
#define LOG(str)
#define LOG1(str,a)
#define LOG2(str,a,b)
#define LOG3(str,a,b,c)
#define LOGIF(cond,str)
#define LOGINFO(info)
#define LOGFLUSH
#define LOGNL
#define LOGSTR(str)
#define LOGSTR1(str,a)
#define LOGSTR2(str,a,b)
#define LOGSTR3(str,a,b,c)
#define LOGSTRu(utf)
#define LOGNAME(c)
#endif

#ifdef TYPECHECK_VERBOSE_IMPORTANT
#define LOGimp(str)     DOLOG(log_text(str))
#define LOGimpSTR(str)  DOLOG(log_plain(str))
#define LOGimpSTRu(utf) DOLOG(log_plain_utf(utf))
#else
#define LOGimp(str)
#define LOGimpSTR(str)
#define LOGimpSTRu(utf)
#endif

#if defined(TYPECHECK_VERBOSE) || defined(TYPECHECK_VERBOSE_IMPORTANT)

#include <stdio.h>

static
void
typestack_print(FILE *file,stackptr stack)
{
#ifdef TYPEINFO_DEBUG
    while (stack) {
		/*fprintf(file,"<%p>",stack);*/
        typeinfo_print_stacktype(file,stack->type,&(stack->typeinfo));
        stack = stack->prev;
        if (stack) fprintf(file," ");
    }
#endif
}

static
void
typestate_print(FILE *file,stackptr instack,typevector *localset,int size)
{
    fprintf(file,"Stack: ");
    typestack_print(file,instack);
    fprintf(file," Locals:");
    typevectorset_print(file,localset,size);
}

#endif

/****************************************************************************/
/* STATISTICS                                                               */
/****************************************************************************/

#ifdef TYPECHECK_DEBUG
/*#define TYPECHECK_STATISTICS*/
#endif

#ifdef TYPECHECK_STATISTICS
#define STAT_ITERATIONS  10
#define STAT_BLOCKS      10
#define STAT_LOCALS      16

static int stat_typechecked = 0;
static int stat_typechecked_jsr = 0;
static int stat_iterations[STAT_ITERATIONS+1] = { 0 };
static int stat_reached = 0;
static int stat_copied = 0;
static int stat_merged = 0;
static int stat_merging_changed = 0;
static int stat_backwards = 0;
static int stat_blocks[STAT_BLOCKS+1] = { 0 };
static int stat_locals[STAT_LOCALS+1] = { 0 };
static int stat_ins = 0;
static int stat_ins_field = 0;
static int stat_ins_invoke = 0;
static int stat_ins_primload = 0;
static int stat_ins_aload = 0;
static int stat_ins_builtin = 0;
static int stat_ins_builtin_gen = 0;
static int stat_ins_branch = 0;
static int stat_ins_switch = 0;
static int stat_ins_unchecked = 0;
static int stat_handlers_reached = 0;
static int stat_savedstack = 0;

#define TYPECHECK_COUNT(cnt)  (cnt)++
#define TYPECHECK_COUNTIF(cond,cnt)  do{if(cond) (cnt)++;} while(0)
#define TYPECHECK_COUNT_FREQ(array,val,limit) \
	do {									  \
		if ((val) < (limit)) (array)[val]++;  \
		else (array)[limit]++;				  \
	} while (0)

static void print_freq(FILE *file,int *array,int limit)
{
	int i;
	for (i=0; i<limit; ++i)
		fprintf(file,"      %3d: %8d\n",i,array[i]);
	fprintf(file,"    >=%3d: %8d\n",limit,array[limit]);
}

void typecheck_print_statistics(FILE *file) {
	fprintf(file,"typechecked methods: %8d\n",stat_typechecked);
	fprintf(file,"methods with JSR   : %8d\n",stat_typechecked_jsr);
	fprintf(file,"reached blocks     : %8d\n",stat_reached);
	fprintf(file,"copied states      : %8d\n",stat_copied);
	fprintf(file,"merged states      : %8d\n",stat_merged);
	fprintf(file,"merging changed    : %8d\n",stat_merging_changed);
	fprintf(file,"backwards branches : %8d\n",stat_backwards);
	fprintf(file,"handlers reached   : %8d\n",stat_handlers_reached);
	fprintf(file,"saved stack (times): %8d\n",stat_savedstack);
	fprintf(file,"instructions       : %8d\n",stat_ins);
	fprintf(file,"    field access   : %8d\n",stat_ins_field);
	fprintf(file,"    invocations    : %8d\n",stat_ins_invoke);
	fprintf(file,"    load primitive : %8d\n",stat_ins_primload);
	fprintf(file,"    load address   : %8d\n",stat_ins_aload);
	fprintf(file,"    builtins       : %8d\n",stat_ins_builtin);
	fprintf(file,"        generic    : %8d\n",stat_ins_builtin_gen);
	fprintf(file,"    unchecked      : %8d\n",stat_ins_unchecked);
	fprintf(file,"    branches       : %8d\n",stat_ins_branch);
	fprintf(file,"    switches       : %8d\n",stat_ins_switch);
	fprintf(file,"iterations used:\n");
	print_freq(file,stat_iterations,STAT_ITERATIONS);
	fprintf(file,"basic blocks per method / 10:\n");
	print_freq(file,stat_blocks,STAT_BLOCKS);
	fprintf(file,"locals:\n");
	print_freq(file,stat_locals,STAT_LOCALS);
}
						   
#else
						   
#define TYPECHECK_COUNT(cnt)
#define TYPECHECK_COUNTIF(cond,cnt)
#define TYPECHECK_COUNT_FREQ(array,val,limit)
#endif


/****************************************************************************/
/* MACROS FOR STACK TYPE CHECKING                                           */
/****************************************************************************/

#define TYPECHECK_VERIFYERROR_ret(m,msg,retval) \
    do { \
        *exceptionptr = new_verifyerror((m), (msg)); \
        return (retval); \
    } while (0)

#define TYPECHECK_VERIFYERROR_main(msg)  TYPECHECK_VERIFYERROR_ret(state.m,(msg),NULL)
#define TYPECHECK_VERIFYERROR_bool(msg)  TYPECHECK_VERIFYERROR_ret(state->m,(msg),false)

#define TYPECHECK_CHECK_TYPE(sp,tp,msg) \
    do { \
		if ((sp)->type != (tp)) { \
        	*exceptionptr = new_verifyerror(state->m, (msg)); \
        	return false; \
		} \
    } while (0)

#define TYPECHECK_INT(sp)  TYPECHECK_CHECK_TYPE(sp,TYPE_INT,"Expected to find integer on stack")
#define TYPECHECK_LNG(sp)  TYPECHECK_CHECK_TYPE(sp,TYPE_LNG,"Expected to find long on stack")
#define TYPECHECK_FLT(sp)  TYPECHECK_CHECK_TYPE(sp,TYPE_FLT,"Expected to find float on stack")
#define TYPECHECK_DBL(sp)  TYPECHECK_CHECK_TYPE(sp,TYPE_DBL,"Expected to find double on stack")
#define TYPECHECK_ADR(sp)  TYPECHECK_CHECK_TYPE(sp,TYPE_ADR,"Expected to find object on stack")

/****************************************************************************/
/* VERIFIER STATE STRUCT                                                    */
/****************************************************************************/

/* verifier_state - This structure keeps the current state of the      */
/* bytecode verifier for passing it between verifier functions.        */

typedef struct verifier_state {
    stackptr curstack;      /* input stack top for current instruction */
    instruction *iptr;               /* pointer to current instruction */
    basicblock *bptr;                /* pointer to current basic block */

	methodinfo *m;                               /* the current method */
	codegendata *cd;                 /* codegendata for current method */
	registerdata *rd;               /* registerdata for current method */
	
	s4 numlocals;                         /* number of local variables */
	s4 validlocals;                /* number of Java-accessible locals */
	void *localbuf;       /* local variable types for each block start */
	typevector *localset;        /* typevector set for local variables */
	typedescriptor returntype;    /* return type of the current method */
	
	stackptr savedstackbuf;             /* buffer for saving the stack */
	stackptr savedstack;             /* saved instack of current block */
	
    exceptiontable **handlers;            /* active exception handlers */
	stackelement excstack;           /* instack for exception handlers */
	
    bool repeat;            /* if true, blocks are iterated over again */
    bool initmethod;             /* true if this is an "<init>" method */
	bool jsrencountered;                 /* true if we there was a JSR */
} verifier_state;

/****************************************************************************/
/* TYPESTACK MACROS AND FUNCTIONS                                           */
/*                                                                          */
/* These macros and functions act on the 'type stack', which is a shorthand */
/* for the types of the stackslots of the current stack. The type of a      */
/* stack slot is usually described by a TYPE_* constant and -- for TYPE_ADR */
/* -- by the typeinfo of the slot. The only thing that makes the type stack */
/* more complicated are returnAddresses of local subroutines, because a     */
/* single stack slot may contain a set of more than one possible return     */
/* address. This is handled by 'return address sets'. A return address set  */
/* is kept as a linked list dangling off the typeinfo of the stack slot.    */
/****************************************************************************/

#define TYPESTACK_IS_RETURNADDRESS(sptr) \
            TYPE_IS_RETURNADDRESS((sptr)->type,(sptr)->typeinfo)

#define TYPESTACK_IS_REFERENCE(sptr) \
            TYPE_IS_REFERENCE((sptr)->type,(sptr)->typeinfo)

#define TYPESTACK_RETURNADDRESSSET(sptr) \
            ((typeinfo_retaddr_set*)TYPEINFO_RETURNADDRESS((sptr)->typeinfo))

#define RETURNADDRESSSET_SEEK(set,pos) \
            do {int i; for (i=pos;i--;) set=set->alt;} while(0)

#define TYPESTACK_COPY(sp,copy)									\
	        do {for(; sp; sp=sp->prev, copy=copy->prev) {		\
					copy->type = sp->type;						\
					TYPEINFO_COPY(sp->typeinfo,copy->typeinfo);	\
				}} while (0)									\

/* typestack_copy **************************************************************
 
   Copy the types on the given stack to the destination stack.

   This function does a straight forward copy except for returnAddress types.
   For returnAddress slots only the return addresses corresponding to
   typevectors in the SELECTED set are copied.
   
   IN:
	   state............current verifier state
	   y................stack with types to copy
	   selected.........set of selected typevectors

   OUT:
       *dst.............the destination stack

   RETURN VALUE:
       true.............success
	   false............an exception has been thrown

*******************************************************************************/

static bool
typestack_copy(verifier_state *state,stackptr dst,stackptr y,typevector *selected)
{
	typevector *sel;
	typeinfo_retaddr_set *sety;
	typeinfo_retaddr_set *new;
	typeinfo_retaddr_set **next;
	int k;
	
	for (;dst; dst=dst->prev, y=y->prev) {
		if (!y) {
			*exceptionptr = new_verifyerror(state->m,"Stack depth mismatch");
			return false;
		}
		if (dst->type != y->type) {
			*exceptionptr = new_verifyerror(state->m,"Stack type mismatch");
			return false;
		}
		LOG3("copy %p -> %p (type %d)",y,dst,dst->type);
		if (dst->type == TYPE_ADDRESS) {
			if (TYPEINFO_IS_PRIMITIVE(y->typeinfo)) {
				/* We copy the returnAddresses from the selected
				 * states only. */

				LOG("copying returnAddress");
				sety = TYPESTACK_RETURNADDRESSSET(y);
				next = &new;
				for (k=0,sel=selected; sel; sel=sel->alt) {
					LOG1("selected k=%d",sel->k);
					while (k<sel->k) {
						sety = sety->alt;
						k++;
					}
					*next = DNEW(typeinfo_retaddr_set);
					(*next)->addr = sety->addr;
					next = &((*next)->alt);
				}
				*next = NULL;
				TYPEINFO_INIT_RETURNADDRESS(dst->typeinfo,new);
			}
			else {
				TYPEINFO_CLONE(y->typeinfo,dst->typeinfo);
			}
		}
	}
	if (y) {
		*exceptionptr = new_verifyerror(state->m,"Stack depth mismatch");
		return false;
	}
	return true;
}

/* typestack_put_retaddr *******************************************************
 
   Put a returnAddress into a stack slot.

   The stack slot receives a set of return addresses with as many members as
   there are typevectors in the local variable set.

   IN:
	   retaddr..........the returnAddress to set (a basicblock *)
	   loc..............the local variable typevector set

   OUT:
       *dst.............the destination stack slot

*******************************************************************************/

static void
typestack_put_retaddr(stackptr dst,void *retaddr,typevector *loc)
{
	TYPECHECK_ASSERT(dst->type == TYPE_ADDRESS);
	
	TYPEINFO_INIT_RETURNADDRESS(dst->typeinfo,NULL);
	for (;loc; loc=loc->alt) {
		typeinfo_retaddr_set *set = DNEW(typeinfo_retaddr_set);
		set->addr = retaddr;
		set->alt = TYPESTACK_RETURNADDRESSSET(dst);
		TYPEINFO_INIT_RETURNADDRESS(dst->typeinfo,set);
	}
}

/* typestack_collapse **********************************************************
 
   Collapse the given stack by shortening all return address sets to a single
   member.

   OUT:
       *dst.............the destination stack to collapse

*******************************************************************************/

static void
typestack_collapse(stackptr dst)
{
	for (; dst; dst = dst->prev) {
		if (TYPESTACK_IS_RETURNADDRESS(dst))
			TYPESTACK_RETURNADDRESSSET(dst)->alt = NULL;
	}
}

/* typestack_merge *************************************************************
 
   Merge the types on one stack into the destination stack.

   IN:
       state............current state of the verifier
	   dst..............the destination stack
	   y................the second stack

   OUT:
       *dst.............receives the result of the stack merge

   RETURN VALUE:
       typecheck_TRUE...*dst has been modified
	   typecheck_FALSE..*dst has not been modified
	   typecheck_FAIL...an exception has been thrown

*******************************************************************************/

static typecheck_result
typestack_merge(verifier_state *state,stackptr dst,stackptr y)
{
	typecheck_result r;
	bool changed = false;
	
	for (; dst; dst = dst->prev, y=y->prev) {
		if (!y) {
			*exceptionptr = new_verifyerror(state->m,"Stack depth mismatch");
			return typecheck_FAIL;
		}
		if (dst->type != y->type) {
			*exceptionptr = new_verifyerror(state->m,"Stack type mismatch");
			return typecheck_FAIL;
		}
		if (dst->type == TYPE_ADDRESS) {
			if (TYPEINFO_IS_PRIMITIVE(dst->typeinfo)) {
				/* dst has returnAddress type */
				if (!TYPEINFO_IS_PRIMITIVE(y->typeinfo)) {
					*exceptionptr = new_verifyerror(state->m,"Merging returnAddress with reference");
					return typecheck_FAIL;
				}
			}
			else {
				/* dst has reference type */
				if (TYPEINFO_IS_PRIMITIVE(y->typeinfo)) {
					*exceptionptr = new_verifyerror(state->m,"Merging reference with returnAddress");
					return typecheck_FAIL;
				}
				r = typeinfo_merge(state->m,&(dst->typeinfo),&(y->typeinfo));
				if (r == typecheck_FAIL)
					return r;
				changed |= r;
			}
		}
	}
	if (y) {
		*exceptionptr = new_verifyerror(state->m,"Stack depth mismatch");
		return typecheck_FAIL;
	}
	return changed;
}

/* typestack_add ***************************************************************
 
   Add the return addresses in the given stack at a given k-index to the
   corresponding return address sets in the destination stack.

   IN:
	   dst..............the destination stack
	   y................the second stack
	   ky...............the k-index which should be selected from the Y stack

   OUT:
       *dst.............receives the result of adding the addresses

*******************************************************************************/

static void
typestack_add(stackptr dst,stackptr y,int ky)
{
	typeinfo_retaddr_set *setd;
	typeinfo_retaddr_set *sety;
	
	for (; dst; dst = dst->prev, y=y->prev) {
		if (TYPESTACK_IS_RETURNADDRESS(dst)) {
			setd = TYPESTACK_RETURNADDRESSSET(dst);
			sety = TYPESTACK_RETURNADDRESSSET(y);
			RETURNADDRESSSET_SEEK(sety,ky);
			while (setd->alt)
				setd=setd->alt;
			setd->alt = DNEW(typeinfo_retaddr_set);
			setd->alt->addr = sety->addr;
			setd->alt->alt = NULL;
		}
	}
}

/* 'a' and 'b' are assumed to have passed typestack_canmerge! */
static bool
typestack_separable_with(stackptr a,stackptr b,int kb)
{
	typeinfo_retaddr_set *seta;
	typeinfo_retaddr_set *setb;
	
	for (; a; a = a->prev, b = b->prev) {
		TYPECHECK_ASSERT(b);
		if (TYPESTACK_IS_RETURNADDRESS(a)) {
			TYPECHECK_ASSERT(TYPESTACK_IS_RETURNADDRESS(b));
			seta = TYPESTACK_RETURNADDRESSSET(a);
			setb = TYPESTACK_RETURNADDRESSSET(b);
			RETURNADDRESSSET_SEEK(setb,kb);

			for (;seta;seta=seta->alt)
				if (seta->addr != setb->addr) return true;
		}
	}
	TYPECHECK_ASSERT(!b);
	return false;
}

/* 'a' and 'b' are assumed to have passed typestack_canmerge! */
static bool
typestack_separable_from(stackptr a,int ka,stackptr b,int kb)
{
	typeinfo_retaddr_set *seta;
	typeinfo_retaddr_set *setb;

	for (; a; a = a->prev, b = b->prev) {
		TYPECHECK_ASSERT(b);
		if (TYPESTACK_IS_RETURNADDRESS(a)) {
			TYPECHECK_ASSERT(TYPESTACK_IS_RETURNADDRESS(b));
			seta = TYPESTACK_RETURNADDRESSSET(a);
			setb = TYPESTACK_RETURNADDRESSSET(b);
			RETURNADDRESSSET_SEEK(seta,ka);
			RETURNADDRESSSET_SEEK(setb,kb);

			if (seta->addr != setb->addr) return true;
		}
	}
	TYPECHECK_ASSERT(!b);
	return false;
}

/****************************************************************************/
/* TYPESTATE FUNCTIONS                                                      */
/*                                                                          */
/* These functions act on the 'type state', which comprises:                */
/*     - the types of the stack slots of the current stack                  */
/*     - the set of type vectors describing the local variables             */
/****************************************************************************/

/* typestate_merge *************************************************************
 
   Merge the types of one state into the destination state.

   IN:
       state............current state of the verifier
	   deststack........the destination stack
	   destloc..........the destination set of local variable typevectors
	   ystack...........the second stack
	   yloc.............the second set of local variable typevectors

   OUT:
       *deststack.......receives the result of the stack merge
	   *destloc.........receives the result of the local variable merge

   RETURN VALUE:
       typecheck_TRUE...destination state has been modified
	   typecheck_FALSE..destination state has not been modified
	   typecheck_FAIL...an exception has been thrown

*******************************************************************************/

static typecheck_result
typestate_merge(verifier_state *state,
				stackptr deststack,typevector *destloc,
				stackptr ystack,typevector *yloc)
{
	typevector *dvec,*yvec;
	int kd,ky;
	bool changed = false;
	typecheck_result r;
	
	LOG("merge:");
	LOGSTR("dstack: "); DOLOG(typestack_print(get_logfile(),deststack)); LOGNL;
	LOGSTR("ystack: "); DOLOG(typestack_print(get_logfile(),ystack)); LOGNL;
	LOGSTR("dloc  : "); DOLOG(typevectorset_print(get_logfile(),destloc,state->numlocals)); LOGNL;
	LOGSTR("yloc  : "); DOLOG(typevectorset_print(get_logfile(),yloc,state->numlocals)); LOGNL;
	LOGFLUSH;

	/* The stack is always merged. If there are returnAddresses on
	 * the stack they are ignored in this step. */

	r = typestack_merge(state,deststack,ystack);
	if (r == typecheck_FAIL)
		return r;
	changed |= r;

	/* If there have not been any JSRs we just have a single typevector merge */
	if (!state->jsrencountered) {
		r = typevector_merge(state->m,destloc,yloc,state->numlocals);
		if (r == typecheck_FAIL)
			return r;
		return changed | r;
	}

	for (yvec=yloc; yvec; yvec=yvec->alt) {
		ky = yvec->k;

		/* Check if the typestates (deststack,destloc) will be
		 * separable when (ystack,yvec) is added. */

		if (!typestack_separable_with(deststack,ystack,ky)
			&& !typevectorset_separable_with(destloc,yvec,state->numlocals))
		{
			/* No, the resulting set won't be separable, thus we
			 * may merge all states in (deststack,destloc) and
			 * (ystack,yvec). */

			typestack_collapse(deststack);
			if (typevectorset_collapse(state->m,destloc,state->numlocals) == typecheck_FAIL)
				return typecheck_FAIL;
			if (typevector_merge(state->m,destloc,yvec,state->numlocals) == typecheck_FAIL)
				return typecheck_FAIL;
		}
		else {
			/* Yes, the resulting set will be separable. Thus we check
			 * if we may merge (ystack,yvec) with a single state in
			 * (deststack,destloc). */
		
			for (dvec=destloc,kd=0; dvec; dvec=dvec->alt, kd++) {
				if (!typestack_separable_from(ystack,ky,deststack,kd)
					&& !typevector_separable_from(yvec,dvec,state->numlocals))
				{
					/* The typestate (ystack,yvec) is not separable from
					 * (deststack,dvec) by any returnAddress. Thus we may
					 * merge the states. */
					
					r = typevector_merge(state->m,dvec,yvec,state->numlocals);
					if (r == typecheck_FAIL)
						return r;
					changed |= r;
					
					goto merged;
				}
			}

			/* The typestate (ystack,yvec) is separable from all typestates
			 * (deststack,destloc). Thus we must add this state to the
			 * result set. */

			typestack_add(deststack,ystack,ky);
			typevectorset_add(destloc,yvec,state->numlocals);
			changed = true;
		}
		   
	merged:
		;
	}
	
	LOG("result:");
	LOGSTR("dstack: "); DOLOG(typestack_print(get_logfile(),deststack)); LOGNL;
	LOGSTR("dloc  : "); DOLOG(typevectorset_print(get_logfile(),destloc,state->numlocals)); LOGNL;
	LOGFLUSH;
	
	return changed;
}

/* typestate_reach *************************************************************
 
   Reach a destination block and propagate stack and local variable types

   IN:
       state............current state of the verifier
	   destblock........destination basic block
	   ystack...........stack to propagate
	   yloc.............set of local variable typevectors to propagate

   OUT:
       state->repeat....set to true if the verifier must iterate again
	                    over the basic blocks
	   
   RETURN VALUE:
       true.............success
	   false............an exception has been thrown

*******************************************************************************/

static bool
typestate_reach(verifier_state *state,
				basicblock *destblock,
				stackptr ystack,typevector *yloc)
{
	typevector *destloc;
	int destidx;
	bool changed = false;
	typecheck_result r;

	LOG1("reaching block L%03d",destblock->debug_nr);
	TYPECHECK_COUNT(stat_reached);
	
	destidx = destblock - state->cd->method->basicblocks;
	destloc = MGET_TYPEVECTOR(state->localbuf,destidx,state->numlocals);

	/* When branching backwards we have to check for uninitialized objects */
	
	if (destblock <= state->bptr) {
		stackptr sp;
		int i;

		/* XXX FIXME FOR INLINING */

		if (!useinlining) {
			TYPECHECK_COUNT(stat_backwards);
        		LOG("BACKWARDS!");
		        for (sp = ystack; sp; sp=sp->prev)
				if (sp->type == TYPE_ADR &&
                		TYPEINFO_IS_NEWOBJECT(sp->typeinfo)) {
					/*printf("current: %d, dest: %d\n", state->bptr->debug_nr, destblock->debug_nr);*/
					*exceptionptr = new_verifyerror(state->m,"Branching backwards with uninitialized object on stack");
					return false;
				}

			for (i=0; i<state->numlocals; ++i)
				if (yloc->td[i].type == TYPE_ADR &&
					TYPEINFO_IS_NEWOBJECT(yloc->td[i].info)) {
					*exceptionptr = new_verifyerror(state->m,"Branching backwards with uninitialized object in local variable");
					return false;
				}
		}
	}
	
	if (destblock->flags == BBTYPECHECK_UNDEF) {
		/* The destblock has never been reached before */

		TYPECHECK_COUNT(stat_copied);
		LOG1("block (index %04d) reached first time",destidx);
		
		if (!typestack_copy(state,destblock->instack,ystack,yloc))
			return false;
		COPY_TYPEVECTORSET(yloc,destloc,state->numlocals);
		changed = true;
	}
	else {
		/* The destblock has already been reached before */
		
		TYPECHECK_COUNT(stat_merged);
		LOG1("block (index %04d) reached before",destidx);
		
		r = typestate_merge(state,destblock->instack,destloc,ystack,yloc);
		if (r == typecheck_FAIL)
			return false;
		changed = r;
		TYPECHECK_COUNTIF(changed,stat_merging_changed);
	}

	if (changed) {
		LOG("changed!");
		destblock->flags = BBTYPECHECK_REACHED;
		if (destblock <= state->bptr) {
			LOG("REPEAT!"); 
			state->repeat = true;
		}
	}
	return true;
}

/* typestate_ret ***************************************************************
 
   Reach the destinations of a RET instruction.

   IN:
       state............current state of the verifier
	   retindex.........index of local variable containing the returnAddress

   OUT:
       state->repeat....set to true if the verifier must iterate again
	                    over the basic blocks
	   
   RETURN VALUE:
       true.............success
	   false............an exception has been thrown

*******************************************************************************/

static bool
typestate_ret(verifier_state *state,int retindex)
{
	typevector *yvec;
	typevector *selected;
	basicblock *destblock;

	for (yvec=state->localset; yvec; ) {
		if (!TYPEDESC_IS_RETURNADDRESS(yvec->td[retindex])) {
			*exceptionptr = new_verifyerror(state->m,"Illegal instruction: RET on non-returnAddress");
			return false;
		}

		destblock = (basicblock*) TYPEINFO_RETURNADDRESS(yvec->td[retindex].info);

		selected = typevectorset_select(&yvec,retindex,destblock);
		
		if (!typestate_reach(state,destblock,state->curstack,selected))
			return false;
	}
	return true;
}

/****************************************************************************/
/* MACROS FOR LOCAL VARIABLE CHECKING                                       */
/****************************************************************************/

#define INDEX_ONEWORD(num)										\
	do { if((num)<0 || (num)>=state->validlocals)				\
			TYPECHECK_VERIFYERROR_bool("Invalid local variable index"); } while (0)
#define INDEX_TWOWORD(num)										\
	do { if((num)<0 || ((num)+1)>=state->validlocals)			\
			TYPECHECK_VERIFYERROR_bool("Invalid local variable index"); } while (0)

#define STORE_ONEWORD(num,type)									\
 	do {typevectorset_store(state->localset,num,type,NULL);} while(0)

#define STORE_TWOWORD(num,type)										\
 	do {typevectorset_store_twoword(state->localset,num,type);} while(0)


#ifdef TYPECHECK_VERBOSE
#define WORDCHECKFAULT \
  	do { \
		dolog("localset->td index: %ld\ninstruction belongs to:%s.%s, outermethod:%s.%s\n", \
		state->iptr->op1,state->iptr->method->class->name->text, \
			state->iptr->method->name->text,state->m->class->name->text,state->m->name->text); \
		show_icmd(state->iptr++, false); \
		show_icmd(state->iptr, false); \
	} while (0)
#else
#define WORDCHECKFAULT
#endif


#define CHECK_ONEWORD(num,tp)											\
	do {TYPECHECK_COUNT(stat_ins_primload);								\
		if (state->jsrencountered) {											\
			if (!typevectorset_checktype(state->localset,num,tp)) {				\
				WORDCHECKFAULT;	\
				TYPECHECK_VERIFYERROR_bool("Variable type mismatch");						\
			}	\
		}																\
		else {															\
			if (state->localset->td[num].type != tp) {							\
				TYPECHECK_VERIFYERROR_bool("Variable type mismatch");						\
				WORDCHECKFAULT;	\
			} \
		}																\
		} while(0)

#define CHECK_TWOWORD(num,type)											\
	do {TYPECHECK_COUNT(stat_ins_primload);								\
		if (!typevectorset_checktype(state->localset,num,type)) {                \
			WORDCHECKFAULT;	\
            		TYPECHECK_VERIFYERROR_bool("Variable type mismatch");	                        \
		} \
	} while(0)


/****************************************************************************/
/* MISC MACROS                                                              */
/****************************************************************************/

#define COPYTYPE(source,dest)   \
	{if ((source)->type == TYPE_ADR)								\
			TYPEINFO_COPY((source)->typeinfo,(dest)->typeinfo);}

#define ISBUILTIN(v)   (bte->fp == (functionptr) (v))

/* TYPECHECK_LEAVE: executed when the method is exited non-abruptly
 * Input:
 *     class........class of the current method
 *     state........verifier state
 */
#define TYPECHECK_LEAVE                                                 \
    do {                                                                \
        if (state->initmethod && state->m->class != class_java_lang_Object) {         \
            /* check the marker variable */                             \
            LOG("Checking <init> marker");                              \
            if (!typevectorset_checktype(state->localset,state->numlocals-1,TYPE_INT))\
                TYPECHECK_VERIFYERROR_bool("<init> method does not initialize 'this'");      \
        }                                                               \
    } while (0)

/* verify_invocation ***********************************************************
 
   Verify an ICMD_INVOKE* instruction.
  
   IN:
       state............the current state of the verifier

   RETURN VALUE:
       true.............successful verification,
	   false............an exception has been thrown.

*******************************************************************************/

static bool
verify_invocation(verifier_state *state)
{
	unresolved_method *um;      /* struct describing the called method */
	constant_FMIref *mref;           /* reference to the called method */
	methoddesc *md;                 /* descriptor of the called method */
	bool specialmethod;            /* true if a <...> method is called */
	int opcode;                                   /* invocation opcode */
	bool callinginit;                      /* true if <init> is called */
	instruction *ins;
	classref_or_classinfo initclass;
	typedesc *td;
	stackelement *stack;                    /* temporary stack pointer */
	stackelement *dst;               /* result stack of the invocation */
	int i;                                                  /* counter */
    u1 rtype;                          /* return type of called method */
	static utf *name_init = NULL;     /* cache for "<init>" utf string */

	um = (unresolved_method *) state->iptr[0].target;
	mref = um->methodref;
	md = mref->parseddesc.md;
	specialmethod = (mref->name->text[0] == '<');
	opcode = state->iptr[0].opc;
	dst = state->iptr->dst;

	if (!name_init)
		name_init = utf_new_char("<init>");
	
	/* check whether we are calling <init> */
	
	callinginit = (opcode == ICMD_INVOKESPECIAL && mref->name == name_init);
	if (specialmethod && !callinginit)
		TYPECHECK_VERIFYERROR_bool("Invalid invocation of special method");

	/* record subtype constraints for parameters */
	
	if (!constrain_unresolved_method(um,state->m->class,state->m,state->iptr,state->curstack))
		return false; /* XXX maybe wrap exception */

	/* try to resolve the method lazily */
	
	if (!resolve_method(um,resolveLazy,(methodinfo **) &(state->iptr[0].val.a)))
		return false;

#if 0
	if (opcode == ICMD_INVOKESPECIAL) {
		/* XXX for INVOKESPECIAL: check if the invokation is done at all */

		/* (If callinginit the class is checked later.) */
		if (!callinginit) { 
			/* XXX classrefs */
			if (!builtin_isanysubclass(myclass,mi->class)) 
				XXXTYPECHECK_VERIFYERROR_bool("Illegal instruction: INVOKESPECIAL calling non-superclass method"); 
		} 
	}
#endif

	/* allocate parameters if necessary */
	
	if (!md->params)
		if (!descriptor_params_from_paramtypes(md,
					(opcode == ICMD_INVOKESTATIC) ? ACC_STATIC : ACC_NONE))
			return false;

	/* check parameter types */

	stack = state->curstack;
	i = md->paramcount; /* number of parameters including 'this'*/
	while (--i >= 0) {
		LOG1("param %d",i);
		td = md->paramtypes + i;
		if (stack->type != td->type)
			TYPECHECK_VERIFYERROR_bool("Parameter type mismatch in method invocation");
		if (stack->type == TYPE_ADR) {
			LOGINFO(&(stack->typeinfo));
			if (i==0 && callinginit)
			{
				/* first argument to <init> method */
				if (!TYPEINFO_IS_NEWOBJECT(stack->typeinfo))
					TYPECHECK_VERIFYERROR_bool("Calling <init> on initialized object");

				/* get the address of the NEW instruction */
				LOGINFO(&(stack->typeinfo));
				ins = (instruction*)TYPEINFO_NEWOBJECT_INSTRUCTION(stack->typeinfo);
				if (ins)
					initclass = CLASSREF_OR_CLASSINFO(ins[-1].val.a);
				else
					initclass.cls = state->m->class;
				LOGSTR("class: "); LOGNAME(initclass); LOGNL;
			}
		}
		LOG("ok");

		if (i)
			stack = stack->prev;
	}

	LOG("checking return type");
	rtype = md->returntype.type;
	if (rtype != TYPE_VOID) {
		if (rtype != dst->type)
			TYPECHECK_VERIFYERROR_bool("Return type mismatch in method invocation");
		if (!typeinfo_init_from_typedesc(&(md->returntype),NULL,&(dst->typeinfo)))
			return false;
	}

	if (callinginit) {
		LOG("replacing uninitialized object");
		/* replace uninitialized object type on stack */
		stack = dst;
		while (stack) {
			if (stack->type == TYPE_ADR
					&& TYPEINFO_IS_NEWOBJECT(stack->typeinfo)
					&& TYPEINFO_NEWOBJECT_INSTRUCTION(stack->typeinfo) == ins)
			{
				LOG("replacing uninitialized type on stack");

				/* If this stackslot is in the instack of
				 * this basic block we must save the type(s)
				 * we are going to replace.
				 */
				if (stack <= state->bptr->instack && !state->savedstack)
				{
					stackptr sp;
					stackptr copy;
					LOG("saving input stack types");
					if (!state->savedstackbuf) {
						LOG("allocating savedstack buffer");
						state->savedstackbuf = DMNEW(stackelement, state->cd->maxstack);
						state->savedstackbuf->prev = NULL;
						for (i = 1; i < state->cd->maxstack; ++i)
							state->savedstackbuf[i].prev = state->savedstackbuf+(i-1);
					}
					sp = state->savedstack = state->bptr->instack;
					copy = state->bptr->instack = state->savedstackbuf + (state->bptr->indepth-1);
					TYPESTACK_COPY(sp,copy);
				}

				if (!typeinfo_init_class(&(stack->typeinfo),initclass))
					return false;
			}
			stack = stack->prev;
		}
		/* replace uninitialized object type in locals */
		if (!typevectorset_init_object(state->localset,ins,initclass,state->numlocals))
			return false;

		/* initializing the 'this' reference? */
		if (!ins) {
			TYPECHECK_ASSERT(state->initmethod);
			/* must be <init> of current class or direct superclass */
			/* XXX check with classrefs */
#if 0
			if (mi->class != m->class && mi->class != m->class->super.cls)
				TYPECHECK_VERIFYERROR_bool("<init> calling <init> of the wrong class");
#endif

			/* set our marker variable to type int */
			LOG("setting <init> marker");
			typevectorset_store(state->localset,state->numlocals-1,TYPE_INT,NULL);
		}
		else {
			/* initializing an instance created with NEW */
			/* XXX is this strictness ok? */
			/* XXX check with classrefs */
#if 0
			if (mi->class != initclass.cls)
				TYPECHECK_VERIFYERROR_bool("Calling <init> method of the wrong class");
#endif
		}
	}
	return true;
}

/* verify_builtin **************************************************************
 
   Verify the call of a builtin function.
  
   IN:
       state............the current state of the verifier

   RETURN VALUE:
       true.............successful verification,
	   false............an exception has been thrown.

*******************************************************************************/

static bool
verify_builtin(verifier_state *state)
{
	builtintable_entry *bte;
    classinfo *cls;
    stackptr dst;               /* output stack of current instruction */

	bte = (builtintable_entry *) state->iptr[0].val.a;
	dst = state->iptr->dst;

	if (ISBUILTIN(BUILTIN_new) || ISBUILTIN(PATCHER_builtin_new)) {
		if (state->iptr[-1].opc != ICMD_ACONST)
			TYPECHECK_VERIFYERROR_bool("illegal instruction: builtin_new without classinfo");
		cls = (classinfo *) state->iptr[-1].val.a;
#ifdef XXX
		TYPECHECK_ASSERT(!cls || cls->linked);
		/* The following check also forbids array classes and interfaces: */
		if ((cls->flags & ACC_ABSTRACT) != 0)
			TYPECHECK_VERIFYERROR_bool("Invalid instruction: NEW creating instance of abstract class");
#endif
		TYPEINFO_INIT_NEWOBJECT(dst->typeinfo,state->iptr);
	}
	else if (ISBUILTIN(BUILTIN_newarray_boolean)) {
		TYPECHECK_INT(state->curstack);
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dst->typeinfo,ARRAYTYPE_BOOLEAN);
	}
	else if (ISBUILTIN(BUILTIN_newarray_char)) {
		TYPECHECK_INT(state->curstack);
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dst->typeinfo,ARRAYTYPE_CHAR);
	}
	else if (ISBUILTIN(BUILTIN_newarray_float)) {
		TYPECHECK_INT(state->curstack);
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dst->typeinfo,ARRAYTYPE_FLOAT);
	}
	else if (ISBUILTIN(BUILTIN_newarray_double)) {
		TYPECHECK_INT(state->curstack);
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dst->typeinfo,ARRAYTYPE_DOUBLE);
	}
	else if (ISBUILTIN(BUILTIN_newarray_byte)) {
		TYPECHECK_INT(state->curstack);
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dst->typeinfo,ARRAYTYPE_BYTE);
	}
	else if (ISBUILTIN(BUILTIN_newarray_short)) {
		TYPECHECK_INT(state->curstack);
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dst->typeinfo,ARRAYTYPE_SHORT);
	}
	else if (ISBUILTIN(BUILTIN_newarray_int)) {
		TYPECHECK_INT(state->curstack);
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dst->typeinfo,ARRAYTYPE_INT);
	}
	else if (ISBUILTIN(BUILTIN_newarray_long)) {
		TYPECHECK_INT(state->curstack);
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dst->typeinfo,ARRAYTYPE_LONG);
	}
	else if (ISBUILTIN(BUILTIN_newarray))
	{
		vftbl_t *vft;
		TYPECHECK_INT(state->curstack->prev);
		if (state->iptr[-1].opc != ICMD_ACONST)
			TYPECHECK_VERIFYERROR_bool("illegal instruction: builtin_newarray without classinfo");
		vft = (vftbl_t *)state->iptr[-1].val.a;
		if (!vft)
			TYPECHECK_VERIFYERROR_bool("ANEWARRAY with unlinked class");
		if (!vft->arraydesc)
			TYPECHECK_VERIFYERROR_bool("ANEWARRAY with non-array class");
		TYPEINFO_INIT_CLASSINFO(dst->typeinfo,vft->class);
	}
	else if (ISBUILTIN(PATCHER_builtin_newarray))
	{
		TYPECHECK_INT(state->curstack->prev);
		if (state->iptr[-1].opc != ICMD_ACONST)
			TYPECHECK_VERIFYERROR_bool("illegal instruction: builtin_newarray without classinfo");
		if (!typeinfo_init_class(&(dst->typeinfo),CLASSREF_OR_CLASSINFO(state->iptr[-1].val.a)))
			return false;
	}
	else if (ISBUILTIN(BUILTIN_newarray))
	{
		vftbl_t *vft;
		TYPECHECK_INT(state->curstack->prev);
		if (state->iptr[-1].opc != ICMD_ACONST)
			TYPECHECK_VERIFYERROR_bool("illegal instruction: builtin_newarray without classinfo");
		vft = (vftbl_t *)state->iptr[-1].val.a;
		if (!vft)
			TYPECHECK_VERIFYERROR_bool("ANEWARRAY with unlinked class");
		if (!vft->arraydesc)
			TYPECHECK_VERIFYERROR_bool("ANEWARRAY with non-array class");
		TYPEINFO_INIT_CLASSINFO(dst->typeinfo,vft->class);
	}
	else if (ISBUILTIN(BUILTIN_arrayinstanceof))
	{
		vftbl_t *vft;
		TYPECHECK_ADR(state->curstack->prev);
		if (state->iptr[-1].opc != ICMD_ACONST)
			TYPECHECK_VERIFYERROR_bool("illegal instruction: builtin_arrayinstanceof without classinfo");
		vft = (vftbl_t *)state->iptr[-1].val.a;
		if (!vft)
			TYPECHECK_VERIFYERROR_bool("INSTANCEOF with unlinked class");
		if (!vft->arraydesc)
			TYPECHECK_VERIFYERROR_bool("internal error: builtin_arrayinstanceof with non-array class");
	}
#if !defined(__POWERPC__) && !defined(__X86_64__) && !defined(__I386__) && !defined(__ALPHA__)
	else if (ISBUILTIN(BUILTIN_arraycheckcast)) {
		vftbl_t *vft;
		TYPECHECK_ADR(state->curstack->prev);
		if (state->iptr[-1].opc != ICMD_ACONST)
			TYPECHECK_VERIFYERROR_bool("illegal instruction: BUILTIN_arraycheckcast without classinfo");
		vft = (vftbl_t *)state->iptr[-1].val.a;
		if (!vft)
			TYPECHECK_VERIFYERROR_bool("CHECKCAST with unlinked class");
		if (!vft->arraydesc)
			TYPECHECK_VERIFYERROR_bool("internal error: builtin_arraycheckcast with non-array class");
		TYPEINFO_INIT_CLASSINFO(dst->typeinfo,vft->class);
	}
	else if (ISBUILTIN(PATCHER_builtin_arraycheckcast)) {
		TYPECHECK_ADR(state->curstack->prev);
		if (state->iptr[-1].opc != ICMD_ACONST)
			TYPECHECK_VERIFYERROR_bool("illegal instruction: BUILTIN_arraycheckcast without classinfo");
		if (!typeinfo_init_class(&(dst->typeinfo),CLASSREF_OR_CLASSINFO(state->iptr[-1].val.a)))
			return false;
	}
#endif
#if !defined(__POWERPC__) && !defined(__X86_64__) && !defined(__I386__) && !defined(__ALPHA__)
	else if (ISBUILTIN(BUILTIN_aastore)) {
		TYPECHECK_ADR(state->curstack);
		TYPECHECK_INT(state->curstack->prev);
		TYPECHECK_ADR(state->curstack->prev->prev);
		if (!TYPEINFO_MAYBE_ARRAY_OF_REFS(state->curstack->prev->prev->typeinfo))
			TYPECHECK_VERIFYERROR_bool("illegal instruction: AASTORE to non-reference array");
	}
#endif
	else {
#if 0
		/* XXX put these checks in a function */
		TYPECHECK_COUNT(stat_ins_builtin_gen);
		builtindesc = builtin_desc;
		while (builtindesc->opcode && builtindesc->builtin
				!= state->iptr->val.fp) builtindesc++;
		if (!builtindesc->opcode) {
			dolog("Builtin not in table: %s",icmd_builtin_name(state->iptr->val.fp));
			TYPECHECK_ASSERT(false);
		}
		TYPECHECK_ARGS3(builtindesc->type_s3,builtindesc->type_s2,builtindesc->type_s1);
#endif
	}
	return true;
}

/* verify_basic_block **********************************************************
 
   Perform bytecode verification of a basic block.
  
   IN:
       state............the current state of the verifier

   RETURN VALUE:
       true.............successful verification,
	   false............an exception has been thrown.

*******************************************************************************/

static bool
verify_basic_block(verifier_state *state)
{
    stackptr srcstack;         /* source stack for copying and merging */
    int opcode;                                      /* current opcode */
    int len;                        /* for counting instructions, etc. */
    bool superblockend;        /* true if no fallthrough to next block */
    basicblock *tbptr;                   /* temporary for target block */
    stackptr dst;               /* output stack of current instruction */
    basicblock **tptr;    /* pointer into target list of switch instr. */
    classinfo *cls;                                       /* temporary */
    bool maythrow;               /* true if this instruction may throw */
    classinfo *myclass;
	unresolved_field *uf;                        /* for field accesses */
	fieldinfo **fieldinfop;                      /* for field accesses */
	s4 i;
	s4 b_index;
	typecheck_result r;

	LOGSTR1("\n---- BLOCK %04d ------------------------------------------------\n",state->bptr->debug_nr);
	LOGFLUSH;

	superblockend = false;
	state->bptr->flags = BBFINISHED;
	b_index = state->bptr - state->m->basicblocks;

	/* init stack at the start of this block */
	state->curstack = state->bptr->instack;

	/* determine the active exception handlers for this block */
	/* XXX could use a faster algorithm with sorted lists or  */
	/* something?                                             */
	len = 0;
	for (i = 0; i < state->cd->exceptiontablelength; ++i) {
		if ((state->cd->exceptiontable[i].start <= state->bptr) && (state->cd->exceptiontable[i].end > state->bptr)) {
			LOG1("active handler L%03d", state->cd->exceptiontable[i].handler->debug_nr);
			state->handlers[len++] = state->cd->exceptiontable + i;
		}
	}
	state->handlers[len] = NULL;

	/* init variable types at the start of this block */
	COPY_TYPEVECTORSET(MGET_TYPEVECTOR(state->localbuf,b_index,state->numlocals),
			state->localset,state->numlocals);

	/* XXX FIXME FOR INLINING */
	if(!useinlining) {
		if (state->handlers[0])
			for (i=0; i<state->numlocals; ++i)
				if (state->localset->td[i].type == TYPE_ADR
						&& TYPEINFO_IS_NEWOBJECT(state->localset->td[i].info)) {
					/* XXX we do not check this for the uninitialized 'this' instance in */
					/* <init> methods. Otherwise there are problems with try blocks in   */
					/* <init>. The spec seems to indicate that we should perform the test*/
					/* in all cases, but this fails with real code.                      */
					/* Example: org/eclipse/ui/internal/PerspectiveBarNewContributionItem*/
					/* of eclipse 3.0.2                                                  */
					if (TYPEINFO_NEWOBJECT_INSTRUCTION(state->localset->td[i].info) != NULL) {
						/*show_icmd_method(state->m, state->cd, state->rd);*/
						printf("Uninitialized variale: %d, block: %d\n", i, state->bptr->debug_nr);
						TYPECHECK_VERIFYERROR_bool("Uninitialized object in local variable inside try block");
					}
				}
	}
	DOLOG(typestate_print(get_logfile(),state->curstack,state->localset,state->numlocals));
	LOGNL; LOGFLUSH;

	/* loop over the instructions */
	len = state->bptr->icount;
	state->iptr = state->bptr->iinstr;
	while (--len >= 0)  {
		TYPECHECK_COUNT(stat_ins);

		DOLOG(typestate_print(get_logfile(),state->curstack,state->localset,state->numlocals));
		LOGNL; LOGFLUSH;

		DOLOG(show_icmd(state->iptr,false)); LOGNL; LOGFLUSH;

		opcode = state->iptr->opc;
		myclass = state->iptr->method->class;
		dst = state->iptr->dst;
		maythrow = false;

		switch (opcode) {

			/****************************************/
			/* STACK MANIPULATIONS                  */

			/* We just need to copy the typeinfo */
			/* for slots containing addresses.   */

			/* CAUTION: We assume that the destination stack
			 * slots were continuously allocated in
			 * memory!  (The current implementation in
			 * stack.c)
			 */

			case ICMD_DUP:
				COPYTYPE(state->curstack,dst);
				break;

			case ICMD_DUP_X1:
				COPYTYPE(state->curstack,dst);
				COPYTYPE(state->curstack,dst-2);
				COPYTYPE(state->curstack->prev,dst-1);
				break;

			case ICMD_DUP_X2:
				COPYTYPE(state->curstack,dst);
				COPYTYPE(state->curstack,dst-3);
				COPYTYPE(state->curstack->prev,dst-1);
				COPYTYPE(state->curstack->prev->prev,dst-2);
				break;

			case ICMD_DUP2:
				COPYTYPE(state->curstack,dst);
				COPYTYPE(state->curstack->prev,dst-1);
				break;

			case ICMD_DUP2_X1:
				COPYTYPE(state->curstack,dst);
				COPYTYPE(state->curstack->prev,dst-1);
				COPYTYPE(state->curstack,dst-3);
				COPYTYPE(state->curstack->prev,dst-4);
				COPYTYPE(state->curstack->prev->prev,dst-2);
				break;

			case ICMD_DUP2_X2:
				COPYTYPE(state->curstack,dst);
				COPYTYPE(state->curstack->prev,dst-1);
				COPYTYPE(state->curstack,dst-4);
				COPYTYPE(state->curstack->prev,dst-5);
				COPYTYPE(state->curstack->prev->prev,dst-2);
				COPYTYPE(state->curstack->prev->prev->prev,dst-3);
				break;

			case ICMD_SWAP:
				COPYTYPE(state->curstack,dst-1);
				COPYTYPE(state->curstack->prev,dst);
				break;

				/****************************************/
				/* PRIMITIVE VARIABLE ACCESS            */

			case ICMD_ILOAD: CHECK_ONEWORD(state->iptr->op1,TYPE_INT); break;
			case ICMD_FLOAD: CHECK_ONEWORD(state->iptr->op1,TYPE_FLOAT); break;
			case ICMD_IINC:  CHECK_ONEWORD(state->iptr->op1,TYPE_INT); break;
			case ICMD_LLOAD: CHECK_TWOWORD(state->iptr->op1,TYPE_LONG); break;
			case ICMD_DLOAD: CHECK_TWOWORD(state->iptr->op1,TYPE_DOUBLE); break;

			case ICMD_FSTORE: STORE_ONEWORD(state->iptr->op1,TYPE_FLOAT); break;
			case ICMD_ISTORE: STORE_ONEWORD(state->iptr->op1,TYPE_INT); break;
			case ICMD_LSTORE: STORE_TWOWORD(state->iptr->op1,TYPE_LONG); break;
			case ICMD_DSTORE: STORE_TWOWORD(state->iptr->op1,TYPE_DOUBLE); break;

				/****************************************/
				/* LOADING ADDRESS FROM VARIABLE        */

			case ICMD_ALOAD:
				TYPECHECK_COUNT(stat_ins_aload);

				/* loading a returnAddress is not allowed */
				if (state->jsrencountered) {
					if (!typevectorset_checkreference(state->localset,state->iptr->op1)) {
						TYPECHECK_VERIFYERROR_bool("illegal instruction: ALOAD loading non-reference");
					}
					if (typevectorset_copymergedtype(state->m,state->localset,state->iptr->op1,&(dst->typeinfo)) == -1)
						return false;
				}
				else {
					if (!TYPEDESC_IS_REFERENCE(state->localset->td[state->iptr->op1])) {
						TYPECHECK_VERIFYERROR_bool("illegal instruction: ALOAD loading non-reference");
					}
					TYPEINFO_COPY(state->localset->td[state->iptr->op1].info,dst->typeinfo);
				}
				break;

				/****************************************/
				/* STORING ADDRESS TO VARIABLE          */

			case ICMD_ASTORE:
				if (state->handlers[0] && TYPEINFO_IS_NEWOBJECT(state->curstack->typeinfo)) {
					TYPECHECK_VERIFYERROR_bool("Storing uninitialized object in local variable inside try block");
				}

				if (TYPESTACK_IS_RETURNADDRESS(state->curstack)) {
					typevectorset_store_retaddr(state->localset,state->iptr->op1,&(state->curstack->typeinfo));
				}
				else {
					typevectorset_store(state->localset,state->iptr->op1,TYPE_ADDRESS,
							&(state->curstack->typeinfo));
				}
				break;

				/****************************************/
				/* LOADING ADDRESS FROM ARRAY           */

			case ICMD_AALOAD:
				if (!TYPEINFO_MAYBE_ARRAY_OF_REFS(state->curstack->prev->typeinfo))
					TYPECHECK_VERIFYERROR_bool("illegal instruction: AALOAD on non-reference array");

				if (!typeinfo_init_component(&state->curstack->prev->typeinfo,&dst->typeinfo))
					return false;
				maythrow = true;
				break;

				/****************************************/
				/* FIELD ACCESS                         */

			case ICMD_PUTFIELDCONST:
			case ICMD_PUTSTATICCONST:
				TYPECHECK_COUNT(stat_ins_field);

				uf = INSTRUCTION_PUTCONST_FIELDREF(state->iptr);
				fieldinfop = INSTRUCTION_PUTCONST_FIELDINFO_PTR(state->iptr);

				goto fieldaccess_tail;

			case ICMD_PUTFIELD:
			case ICMD_PUTSTATIC:
				TYPECHECK_COUNT(stat_ins_field);

				uf = (unresolved_field *) state->iptr[0].target;
				fieldinfop = (fieldinfo **) &(state->iptr[0].val.a);
				
				goto fieldaccess_tail;

			case ICMD_GETFIELD:
			case ICMD_GETSTATIC:
				TYPECHECK_COUNT(stat_ins_field);

				uf = (unresolved_field *) state->iptr[0].target;
				fieldinfop = (fieldinfo **) &(state->iptr[0].val.a);

				/* the result is pushed on the stack */
				if (dst->type == TYPE_ADR) {
					if (!typeinfo_init_from_typedesc(uf->fieldref->parseddesc.fd,NULL,&(dst->typeinfo)))
						return false;
				}

fieldaccess_tail:
				/* record the subtype constraints for this field access */
				if (!constrain_unresolved_field(uf,state->m->class,state->m,state->iptr,state->curstack))
					return false; /* XXX maybe wrap exception? */

				/* try to resolve the field reference */
				if (!resolve_field(uf,resolveLazy,fieldinfop))
					return false;

				/* we need a patcher, so this is not a leafmethod */
#if defined(__MIPS__) || defined(__POWERPC__)
				if (!*fieldinfop || !(*fieldinfop)->class->initialized)
					state->cd->method->isleafmethod = false;
#endif
					
				maythrow = true;
				break;

				/****************************************/
				/* PRIMITIVE ARRAY ACCESS               */

			case ICMD_ARRAYLENGTH:
				if (!TYPEINFO_MAYBE_ARRAY(state->curstack->typeinfo)
						&& state->curstack->typeinfo.typeclass.cls != pseudo_class_Arraystub)
					TYPECHECK_VERIFYERROR_bool("illegal instruction: ARRAYLENGTH on non-array");
				maythrow = true;
				break;

			case ICMD_BALOAD:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(state->curstack->prev->typeinfo,ARRAYTYPE_BOOLEAN)
						&& !TYPEINFO_MAYBE_PRIMITIVE_ARRAY(state->curstack->prev->typeinfo,ARRAYTYPE_BYTE))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;
			case ICMD_CALOAD:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(state->curstack->prev->typeinfo,ARRAYTYPE_CHAR))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;
			case ICMD_DALOAD:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(state->curstack->prev->typeinfo,ARRAYTYPE_DOUBLE))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;
			case ICMD_FALOAD:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(state->curstack->prev->typeinfo,ARRAYTYPE_FLOAT))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;
			case ICMD_IALOAD:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(state->curstack->prev->typeinfo,ARRAYTYPE_INT))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;
			case ICMD_SALOAD:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(state->curstack->prev->typeinfo,ARRAYTYPE_SHORT))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;
			case ICMD_LALOAD:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(state->curstack->prev->typeinfo,ARRAYTYPE_LONG))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;

			case ICMD_BASTORE:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(state->curstack->prev->prev->typeinfo,ARRAYTYPE_BOOLEAN)
						&& !TYPEINFO_MAYBE_PRIMITIVE_ARRAY(state->curstack->prev->prev->typeinfo,ARRAYTYPE_BYTE))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;
			case ICMD_CASTORE:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(state->curstack->prev->prev->typeinfo,ARRAYTYPE_CHAR))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;
			case ICMD_DASTORE:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(state->curstack->prev->prev->typeinfo,ARRAYTYPE_DOUBLE))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;
			case ICMD_FASTORE:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(state->curstack->prev->prev->typeinfo,ARRAYTYPE_FLOAT))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;
			case ICMD_IASTORE:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(state->curstack->prev->prev->typeinfo,ARRAYTYPE_INT))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;
			case ICMD_SASTORE:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(state->curstack->prev->prev->typeinfo,ARRAYTYPE_SHORT))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;
			case ICMD_LASTORE:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(state->curstack->prev->prev->typeinfo,ARRAYTYPE_LONG))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;

#if defined(__POWERPC__) || defined(__X86_64__) || defined(__I386__) || defined(__ALPHA__)
			case ICMD_AASTORE:
				/* we just check the basic input types and that the           */
				/* destination is an array of references. Assignability to    */
				/* the actual array must be checked at runtime, each time the */
				/* instruction is performed. (See builtin_canstore.)          */
				TYPECHECK_ADR(state->curstack);
				TYPECHECK_INT(state->curstack->prev);
				TYPECHECK_ADR(state->curstack->prev->prev);
				if (!TYPEINFO_MAYBE_ARRAY_OF_REFS(state->curstack->prev->prev->typeinfo))
					TYPECHECK_VERIFYERROR_bool("illegal instruction: AASTORE to non-reference array");
				maythrow = true;
				break;
#endif

			case ICMD_IASTORECONST:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(state->curstack->prev->typeinfo, ARRAYTYPE_INT))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;

			case ICMD_LASTORECONST:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(state->curstack->prev->typeinfo, ARRAYTYPE_LONG))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;

			case ICMD_BASTORECONST:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(state->curstack->prev->typeinfo, ARRAYTYPE_BOOLEAN)
						&& !TYPEINFO_MAYBE_PRIMITIVE_ARRAY(state->curstack->prev->typeinfo, ARRAYTYPE_BYTE))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;

			case ICMD_CASTORECONST:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(state->curstack->prev->typeinfo, ARRAYTYPE_CHAR))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;

			case ICMD_SASTORECONST:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(state->curstack->prev->typeinfo, ARRAYTYPE_SHORT))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;

				/****************************************/
				/* ADDRESS CONSTANTS                    */

			case ICMD_ACONST:
				if (state->iptr->val.a == NULL)
					TYPEINFO_INIT_NULLTYPE(dst->typeinfo);
				else
					/* string constant (or constant for builtin function) */
					TYPEINFO_INIT_CLASSINFO(dst->typeinfo,class_java_lang_String);
				break;

				/****************************************/
				/* CHECKCAST AND INSTANCEOF             */

			case ICMD_CHECKCAST:
				TYPECHECK_ADR(state->curstack);
				/* returnAddress is not allowed */
				if (!TYPEINFO_IS_REFERENCE(state->curstack->typeinfo))
					TYPECHECK_VERIFYERROR_bool("Illegal instruction: CHECKCAST on non-reference");

				cls = (classinfo *) state->iptr[0].val.a;
				if (cls)
					TYPEINFO_INIT_CLASSINFO(dst->typeinfo,cls);
				else
					if (!typeinfo_init_class(&(dst->typeinfo),CLASSREF_OR_CLASSINFO(state->iptr[0].target)))
						return false;
				maythrow = true;
				break;

			case ICMD_ARRAYCHECKCAST:
				TYPECHECK_ADR(state->curstack);
				/* returnAddress is not allowed */
				if (!TYPEINFO_IS_REFERENCE(state->curstack->typeinfo))
					TYPECHECK_VERIFYERROR_bool("Illegal instruction: ARRAYCHECKCAST on non-reference");

				if (state->iptr[0].op1) {
					/* a resolved array class */
					cls = ((vftbl_t *)state->iptr[0].target)->class;
					TYPEINFO_INIT_CLASSINFO(dst->typeinfo,cls);
				}
				else {
					/* an unresolved array class reference */
					if (!typeinfo_init_class(&(dst->typeinfo),CLASSREF_OR_CLASSINFO(state->iptr[0].target)))
						return false;
				}
				maythrow = true;
				break;

			case ICMD_INSTANCEOF:
				TYPECHECK_ADR(state->curstack);
				/* returnAddress is not allowed */
				if (!TYPEINFO_IS_REFERENCE(state->curstack->typeinfo))
					TYPECHECK_VERIFYERROR_bool("Illegal instruction: INSTANCEOF on non-reference");
				break;

				/****************************************/
				/* BRANCH INSTRUCTIONS                  */

			case ICMD_GOTO:
				superblockend = true;
				/* FALLTHROUGH! */
			case ICMD_IFNULL:
			case ICMD_IFNONNULL:
			case ICMD_IFEQ:
			case ICMD_IFNE:
			case ICMD_IFLT:
			case ICMD_IFGE:
			case ICMD_IFGT:
			case ICMD_IFLE:
			case ICMD_IF_ICMPEQ:
			case ICMD_IF_ICMPNE:
			case ICMD_IF_ICMPLT:
			case ICMD_IF_ICMPGE:
			case ICMD_IF_ICMPGT:
			case ICMD_IF_ICMPLE:
			case ICMD_IF_ACMPEQ:
			case ICMD_IF_ACMPNE:
			case ICMD_IF_LEQ:
			case ICMD_IF_LNE:
			case ICMD_IF_LLT:
			case ICMD_IF_LGE:
			case ICMD_IF_LGT:
			case ICMD_IF_LLE:
			case ICMD_IF_LCMPEQ:
			case ICMD_IF_LCMPNE:
			case ICMD_IF_LCMPLT:
			case ICMD_IF_LCMPGE:
			case ICMD_IF_LCMPGT:
			case ICMD_IF_LCMPLE:
				TYPECHECK_COUNT(stat_ins_branch);
				tbptr = (basicblock *) state->iptr->target;

				/* propagate stack and variables to the target block */
				if (!typestate_reach(state,tbptr,dst,state->localset))
					return false;
				break;

				/****************************************/
				/* SWITCHES                             */

			case ICMD_TABLESWITCH:
				TYPECHECK_COUNT(stat_ins_switch);
				{
					s4 *s4ptr = state->iptr->val.a;
					s4ptr++;              /* skip default */
					i = *s4ptr++;         /* low */
					i = *s4ptr++ - i + 2; /* +1 for default target */
				}
				goto switch_instruction_tail;

			case ICMD_LOOKUPSWITCH:
				TYPECHECK_COUNT(stat_ins_switch);
				{
					s4 *s4ptr = state->iptr->val.a;
					s4ptr++;              /* skip default */
					i = *s4ptr++ + 1;     /* count +1 for default */
				}
switch_instruction_tail:
				tptr = (basicblock **)state->iptr->target;

				while (--i >= 0) {
					tbptr = *tptr++;
					LOG2("target %d is block %04d",(tptr-(basicblock **)state->iptr->target)-1,tbptr->debug_nr);
					if (!typestate_reach(state,tbptr,dst,state->localset))
						return false;
				}
				LOG("switch done");
				superblockend = true;
				break;

				/****************************************/
				/* ADDRESS RETURNS AND THROW            */

			case ICMD_ATHROW:
				r = typeinfo_is_assignable_to_class(&state->curstack->typeinfo,
						CLASSREF_OR_CLASSINFO(class_java_lang_Throwable));
				if (r == typecheck_FALSE)
					TYPECHECK_VERIFYERROR_bool("illegal instruction: ATHROW on non-Throwable");
				if (r == typecheck_FAIL)
					return false;
				/* XXX handle typecheck_MAYBE */
				superblockend = true;
				maythrow = true;
				break;

			case ICMD_ARETURN:
				if (!TYPEINFO_IS_REFERENCE(state->curstack->typeinfo))
					TYPECHECK_VERIFYERROR_bool("illegal instruction: ARETURN on non-reference");

				if (state->returntype.type != TYPE_ADDRESS
						|| (r = typeinfo_is_assignable(&state->curstack->typeinfo,&(state->returntype.info))) 
								== typecheck_FALSE)
					TYPECHECK_VERIFYERROR_bool("Return type mismatch");
				if (r == typecheck_FAIL)
					return false;
				/* XXX handle typecheck_MAYBE */
				goto return_tail;

				/****************************************/
				/* PRIMITIVE RETURNS                    */

			case ICMD_IRETURN:
				if (state->returntype.type != TYPE_INT) TYPECHECK_VERIFYERROR_bool("Return type mismatch");
				goto return_tail;

			case ICMD_LRETURN:
				if (state->returntype.type != TYPE_LONG) TYPECHECK_VERIFYERROR_bool("Return type mismatch");
				goto return_tail;

			case ICMD_FRETURN:
				if (state->returntype.type != TYPE_FLOAT) TYPECHECK_VERIFYERROR_bool("Return type mismatch");
				goto return_tail;

			case ICMD_DRETURN:
				if (state->returntype.type != TYPE_DOUBLE) TYPECHECK_VERIFYERROR_bool("Return type mismatch");
				goto return_tail;

			case ICMD_RETURN:
				if (state->returntype.type != TYPE_VOID) TYPECHECK_VERIFYERROR_bool("Return type mismatch");
return_tail:
				TYPECHECK_LEAVE;
				superblockend = true;
				maythrow = true;
				break;

				/****************************************/
				/* SUBROUTINE INSTRUCTIONS              */

			case ICMD_JSR:
				LOG("jsr");
				state->jsrencountered = true;

				/* This is a dirty hack. It is needed
				 * because of the special handling of
				 * ICMD_JSR in stack.c
				 */
				dst = (stackptr) state->iptr->val.a;

				tbptr = (basicblock *) state->iptr->target;
				if (state->bptr + 1 == (state->m->basicblocks + state->m->basicblockcount + 1))
					TYPECHECK_VERIFYERROR_bool("Illegal instruction: JSR at end of bytecode");
				typestack_put_retaddr(dst,state->bptr+1,state->localset);
				if (!typestate_reach(state,tbptr,dst,state->localset))
					return false;

				superblockend = true;
				break;

			case ICMD_RET:
				/* check returnAddress variable */
				if (!typevectorset_checkretaddr(state->localset,state->iptr->op1))
					TYPECHECK_VERIFYERROR_bool("illegal instruction: RET using non-returnAddress variable");

				if (!typestate_ret(state,state->iptr->op1))
					return false;

				superblockend = true;
				break;

				/****************************************/
				/* INVOKATIONS                          */

			case ICMD_INVOKEVIRTUAL:
			case ICMD_INVOKESPECIAL:
			case ICMD_INVOKESTATIC:
			case ICMD_INVOKEINTERFACE:
				TYPECHECK_COUNT(stat_ins_invoke);
				if (!verify_invocation(state))
					return false;
				maythrow = true;
				break;

				/****************************************/
				/* MULTIANEWARRAY                       */

			case ICMD_MULTIANEWARRAY:
				/* XXX make this a separate function */
				{
					vftbl_t *arrayvftbl;
					arraydescriptor *desc;

					/* check the array lengths on the stack */
					i = state->iptr[0].op1;
					if (i < 1)
						TYPECHECK_VERIFYERROR_bool("Illegal dimension argument");
					srcstack = state->curstack;
					while (i--) {
						if (!srcstack)
							TYPECHECK_VERIFYERROR_bool("Unable to pop operand off an empty stack");
						TYPECHECK_INT(srcstack);
						srcstack = srcstack->prev;
					}

					/* check array descriptor */
					if (state->iptr[0].target == NULL) {
						arrayvftbl = (vftbl_t*) state->iptr[0].val.a;
						if (!arrayvftbl)
							TYPECHECK_VERIFYERROR_bool("MULTIANEWARRAY with unlinked class");
						if ((desc = arrayvftbl->arraydesc) == NULL)
							TYPECHECK_VERIFYERROR_bool("MULTIANEWARRAY with non-array class");
						if (desc->dimension < state->iptr[0].op1)
							TYPECHECK_VERIFYERROR_bool("MULTIANEWARRAY dimension to high");

						/* set the array type of the result */
						TYPEINFO_INIT_CLASSINFO(dst->typeinfo,arrayvftbl->class);
					}
					else {
						/* XXX do checks in patcher */
						if (!typeinfo_init_class(&(dst->typeinfo),CLASSREF_OR_CLASSINFO(state->iptr[0].val.a)))
							return false;
					}
				}
				maythrow = true;
				break;

				/****************************************/
				/* BUILTINS                             */

			case ICMD_BUILTIN:
				TYPECHECK_COUNT(stat_ins_builtin);
				if (!verify_builtin(state))
					return false;
				maythrow = true;
				break;

				/****************************************/
				/* SIMPLE EXCEPTION THROWING TESTS      */

			case ICMD_CHECKASIZE:
				/* The argument to CHECKASIZE is typechecked by
				 * typechecking the array creation instructions. */

				/* FALLTHROUGH! */
			case ICMD_CHECKNULL:
				/* CHECKNULL just requires that the stack top
				 * is an address. This is checked in stack.c */
				maythrow = true;
				break;

				/****************************************/
				/* INSTRUCTIONS WHICH SHOULD HAVE BEEN  */
				/* REPLACED BY OTHER OPCODES            */

#ifdef TYPECHECK_DEBUG
			case ICMD_NEW:
			case ICMD_NEWARRAY:
			case ICMD_ANEWARRAY:
			case ICMD_MONITORENTER:
			case ICMD_MONITOREXIT:
#if !defined(__POWERPC__) && !defined(__X86_64__) && !defined(__I386__) && !defined(__ALPHA__)
			case ICMD_AASTORE:
#endif
				LOG2("ICMD %d at %d\n", state->iptr->opc, (int)(state->iptr-state->bptr->iinstr));
				LOG("Should have been converted to builtin function call.");
				TYPECHECK_ASSERT(false);
				break;

			case ICMD_READONLY_ARG:
			case ICMD_CLEAR_ARGREN:
				LOG2("ICMD %d at %d\n", state->iptr->opc, (int)(state->iptr-state->bptr->iinstr));
				LOG("Should have been replaced in stack.c.");
				TYPECHECK_ASSERT(false);
				break;
#endif

				/****************************************/
				/* UNCHECKED OPERATIONS                 */

				/*********************************************
				 * Instructions below...
				 *     *) don't operate on local variables,
				 *     *) don't operate on references,
				 *     *) don't operate on returnAddresses.
				 *
				 * (These instructions are typechecked in
				 *  analyse_stack.)
				 ********************************************/

				/* Instructions which may throw a runtime exception: */

			case ICMD_IDIV:
			case ICMD_IREM:
			case ICMD_LDIV:
			case ICMD_LREM:

				maythrow = true;
				break;

				/* Instructions which never throw a runtime exception: */
#if defined(TYPECHECK_DEBUG) || defined(TYPECHECK_STATISTICS)
			case ICMD_NOP:
			case ICMD_POP:
			case ICMD_POP2:

			case ICMD_ICONST:
			case ICMD_LCONST:
			case ICMD_FCONST:
			case ICMD_DCONST:

			case ICMD_IFEQ_ICONST:
			case ICMD_IFNE_ICONST:
			case ICMD_IFLT_ICONST:
			case ICMD_IFGE_ICONST:
			case ICMD_IFGT_ICONST:
			case ICMD_IFLE_ICONST:
			case ICMD_ELSE_ICONST:

			case ICMD_IADD:
			case ICMD_ISUB:
			case ICMD_IMUL:
			case ICMD_INEG:
			case ICMD_IAND:
			case ICMD_IOR:
			case ICMD_IXOR:
			case ICMD_ISHL:
			case ICMD_ISHR:
			case ICMD_IUSHR:
			case ICMD_LADD:
			case ICMD_LSUB:
			case ICMD_LMUL:
			case ICMD_LNEG:
			case ICMD_LAND:
			case ICMD_LOR:
			case ICMD_LXOR:
			case ICMD_LSHL:
			case ICMD_LSHR:
			case ICMD_LUSHR:
#if 0
			case ICMD_IREM0X10001:
			case ICMD_LREM0X10001:
#endif
			case ICMD_IDIVPOW2:
			case ICMD_LDIVPOW2:
			case ICMD_IADDCONST:
			case ICMD_ISUBCONST:
			case ICMD_IMULCONST:
			case ICMD_IANDCONST:
			case ICMD_IORCONST:
			case ICMD_IXORCONST:
			case ICMD_ISHLCONST:
			case ICMD_ISHRCONST:
			case ICMD_IUSHRCONST:
			case ICMD_IREMPOW2:
			case ICMD_LADDCONST:
			case ICMD_LSUBCONST:
			case ICMD_LMULCONST:
			case ICMD_LANDCONST:
			case ICMD_LORCONST:
			case ICMD_LXORCONST:
			case ICMD_LSHLCONST:
			case ICMD_LSHRCONST:
			case ICMD_LUSHRCONST:
			case ICMD_LREMPOW2:

			case ICMD_I2L:
			case ICMD_I2F:
			case ICMD_I2D:
			case ICMD_L2I:
			case ICMD_L2F:
			case ICMD_L2D:
			case ICMD_F2I:
			case ICMD_F2L:
			case ICMD_F2D:
			case ICMD_D2I:
			case ICMD_D2L:
			case ICMD_D2F:
			case ICMD_INT2BYTE:
			case ICMD_INT2CHAR:
			case ICMD_INT2SHORT:

			case ICMD_LCMP:
			case ICMD_LCMPCONST:
			case ICMD_FCMPL:
			case ICMD_FCMPG:
			case ICMD_DCMPL:
			case ICMD_DCMPG:

			case ICMD_FADD:
			case ICMD_DADD:
			case ICMD_FSUB:
			case ICMD_DSUB:
			case ICMD_FMUL:
			case ICMD_DMUL:
			case ICMD_FDIV:
			case ICMD_DDIV:
			case ICMD_FREM:
			case ICMD_DREM:
			case ICMD_FNEG:
			case ICMD_DNEG:


				/*XXX What shall we do with the following ?*/
			case ICMD_CHECKEXCEPTION:
			case ICMD_AASTORECONST:
				TYPECHECK_COUNT(stat_ins_unchecked);
				break;

				/****************************************/

			default:
				LOG2("ICMD %d at %d\n", state->iptr->opc, (int)(state->iptr-state->bptr->iinstr));
				TYPECHECK_VERIFYERROR_bool("Missing ICMD code during typecheck");
#endif
		}

		/* the output of this instruction becomes the current stack */
		state->curstack = dst;

		/* reach exception handlers for this instruction */
		if (maythrow) {
			LOG("reaching exception handlers");
			i = 0;
			while (state->handlers[i]) {
				TYPECHECK_COUNT(stat_handlers_reached);
				if (state->handlers[i]->catchtype.any)
					state->excstack.typeinfo.typeclass = state->handlers[i]->catchtype;
				else
					state->excstack.typeinfo.typeclass.cls = class_java_lang_Throwable;
				if (!typestate_reach(state,
						state->handlers[i]->handler,
						&(state->excstack),state->localset))
					return false;
				i++;
			}
		}

		LOG("next instruction");
		state->iptr++;
	} /* while instructions */

	LOG("instructions done");
	LOGSTR("RESULT=> ");
	DOLOG(typestate_print(get_logfile(),state->curstack,state->localset,state->numlocals));
	LOGNL; LOGFLUSH;

	/* propagate stack and variables to the following block */
	if (!superblockend) {
		LOG("reaching following block");
		tbptr = state->bptr + 1;
		while (tbptr->flags == BBDELETED) {
			tbptr++;
#ifdef TYPECHECK_DEBUG
			/* this must be checked in parse.c */
			if ((tbptr->debug_nr) >= state->m->basicblockcount)
				TYPECHECK_VERIFYERROR_bool("Control flow falls off the last block");
#endif
		}
		if (!typestate_reach(state,tbptr,dst,state->localset))
			return false;
	}

	/* We may have to restore the types of the instack slots. They
	 * have been saved if an <init> call inside the block has
	 * modified the instack types. (see INVOKESPECIAL) */

	if (state->savedstack) {
		stackptr sp = state->bptr->instack;
		stackptr copy = state->savedstack;
		TYPECHECK_COUNT(stat_savedstack);
		LOG("restoring saved instack");
		TYPESTACK_COPY(sp,copy);
		state->bptr->instack = state->savedstack;
		state->savedstack = NULL;
	}
	return true;
}

/* verify_init_locals **********************************************************
 
   Initialize the local variables in the verifier state.
  
   IN:
       state............the current state of the verifier

   RETURN VALUE:
       true.............success,
	   false............an exception has been thrown.

*******************************************************************************/

static bool
verify_init_locals(verifier_state *state)
{
	int i;
	typedescriptor *td;
	typevector *lset;

    /* initialize the variable types of the first block */
    /* to the types of the arguments */

	lset = MGET_TYPEVECTOR(state->localbuf,0,state->numlocals);
	lset->k = 0;
	lset->alt = NULL;
	td = lset->td;
	i = state->validlocals;

	/* allocate parameter descriptors if necessary */
	
	if (!state->m->parseddesc->params)
		if (!descriptor_params_from_paramtypes(state->m->parseddesc,state->m->flags))
			return false;

    /* if this is an instance method initialize the "this" ref type */
	
    if (!(state->m->flags & ACC_STATIC)) {
		if (!i)
			TYPECHECK_VERIFYERROR_bool("Not enough local variables for method arguments");
        td->type = TYPE_ADDRESS;
        if (state->initmethod)
            TYPEINFO_INIT_NEWOBJECT(td->info,NULL);
        else
            TYPEINFO_INIT_CLASSINFO(td->info, state->m->class);
        td++;
		i--;
    }

    LOG("'this' argument set.\n");

    /* the rest of the arguments and the return type */
	
    i = typedescriptors_init_from_methoddesc(td, state->m->parseddesc,
											  i,
											  true, /* two word types use two slots */
											  (td - lset->td), /* skip 'this' pointer */
											  &state->returntype);
	if (i == -1)
		return false;
	td += i;

	/* variables not used for arguments are initialized to TYPE_VOID */
	
	i = state->numlocals - (td - lset->td);
	while (i--) {
		td->type = TYPE_VOID;
		td++;
	}

    LOG("Arguments set.\n");
	return true;
}

/****************************************************************************/
/* typecheck()                                                              */
/* This is the main function of the bytecode verifier. It is called         */
/* directly after analyse_stack.                                            */
/*                                                                          */
/* IN:                                                                      */
/*    meth.............the method to verify                                 */
/*    cdata............codegendata for the method                           */
/*    rdata............registerdata for the method                          */
/*                                                                          */
/* RETURN VALUE:                                                            */
/*     m................successful verification                             */
/*     NULL.............an exception has been thrown                        */
/*                                                                          */
/* XXX TODO:                                                                */
/*     Bytecode verification has not been tested with inlining and          */
/*     probably does not work correctly with inlining.                      */
/****************************************************************************/

#define MAXPARAMS 255

methodinfo *typecheck(methodinfo *meth, codegendata *cdata, registerdata *rdata)
{
	verifier_state state;             /* current state of the verifier */
    int i;                                        /* temporary counter */
    static utf *name_init;            /* cache for utf string "<init>" */

	/* collect statistics */

#ifdef TYPECHECK_STATISTICS
	int count_iterations = 0;
	TYPECHECK_COUNT(stat_typechecked);
	TYPECHECK_COUNT_FREQ(stat_locals,cdata->maxlocals,STAT_LOCALS);
	TYPECHECK_COUNT_FREQ(stat_blocks,meth->basicblockcount/10,STAT_BLOCKS);
#endif

	/* some logging on entry */
	
    LOGSTR("\n==============================================================================\n");
    /*DOLOG( show_icmd_method(cdata->method,cdata,rdata));*/
    LOGSTR("\n==============================================================================\n");
    LOGimpSTR("Entering typecheck: ");
    LOGimpSTRu(cdata->method->name);
    LOGimpSTR("    ");
    LOGimpSTRu(cdata->method->descriptor);
    LOGimpSTR("    (class ");
    LOGimpSTRu(cdata->method->class->name);
    LOGimpSTR(")\n");
	LOGFLUSH;

	/* initialize the verifier state */

	state.savedstackbuf = NULL;
	state.savedstack = NULL;
	state.jsrencountered = false;
	state.m = meth;
	state.cd = cdata;
	state.rd = rdata;

	/* check if this method is an instance initializer method */

	if (!name_init)
		name_init = utf_new_char("<init>");
    state.initmethod = (state.m->name == name_init);

    /* reset all BBFINISHED blocks to BBTYPECHECK_UNDEF. */
	
    i = state.m->basicblockcount;
    state.bptr = state.m->basicblocks;
    while (--i >= 0) {
#ifdef TYPECHECK_DEBUG
        if (state.bptr->flags != BBFINISHED && state.bptr->flags != BBDELETED
            && state.bptr->flags != BBUNDEF)
        {
            /*show_icmd_method(state.cd->method,state.cd,state.rd);*/
            LOGSTR1("block flags: %d\n",state.bptr->flags); LOGFLUSH;
			TYPECHECK_ASSERT(false);
        }
#endif
        if (state.bptr->flags >= BBFINISHED) {
            state.bptr->flags = BBTYPECHECK_UNDEF;
        }
        state.bptr++;
    }

    /* the first block is always reached */
	
    if (state.m->basicblockcount && state.m->basicblocks[0].flags == BBTYPECHECK_UNDEF)
        state.m->basicblocks[0].flags = BBTYPECHECK_REACHED;

    LOG("Blocks reset.\n");

    /* number of local variables */
    
    /* In <init> methods we use an extra local variable to indicate whether */
    /* the 'this' reference has been initialized.                           */
	/*         TYPE_VOID...means 'this' has not been initialized,           */
	/*         TYPE_INT....means 'this' has been initialized.               */
    state.numlocals = state.cd->maxlocals;
	state.validlocals = state.numlocals;
    if (state.initmethod) state.numlocals++;

    /* allocate the buffers for local variables */
	
	state.localbuf = DMNEW_TYPEVECTOR(state.m->basicblockcount+1, state.numlocals);
	state.localset = MGET_TYPEVECTOR(state.localbuf,state.m->basicblockcount,state.numlocals);

    LOG("Variable buffer allocated.\n");

    /* allocate the buffer of active exception handlers */
	
    state.handlers = DMNEW(exceptiontable*, state.cd->exceptiontablelength + 1);

	/* initialized local variables of first block */

	if (!verify_init_locals(&state))
		return NULL;

    /* initialize the input stack of exception handlers */
	
	state.excstack.prev = NULL;
	state.excstack.type = TYPE_ADR;
	TYPEINFO_INIT_CLASSINFO(state.excstack.typeinfo,
							class_java_lang_Throwable); /* changed later */

    LOG("Exception handler stacks set.\n");

    /* loop while there are still blocks to be checked */
    do {
		TYPECHECK_COUNT(count_iterations);

        state.repeat = false;
        
        i = state.m->basicblockcount;
        state.bptr = state.m->basicblocks;

        while (--i >= 0) {
            LOGSTR1("---- BLOCK %04d, ",state.bptr->debug_nr);
            LOGSTR1("blockflags: %d\n",state.bptr->flags);
            LOGFLUSH;
            
		    /* verify reached block */	
            if (state.bptr->flags == BBTYPECHECK_REACHED) {
                if (!verify_basic_block(&state))
					return NULL;
            }
            state.bptr++;
        } /* while blocks */

        LOGIF(state.repeat,"state.repeat == true");
    } while (state.repeat);

	/* statistics */
	
#ifdef TYPECHECK_STATISTICS
	LOG1("Typechecker did %4d iterations",count_iterations);
	TYPECHECK_COUNT_FREQ(stat_iterations,count_iterations,STAT_ITERATIONS);
	TYPECHECK_COUNTIF(state.jsrencountered,stat_typechecked_jsr);
#endif

	/* check for invalid flags at exit */
	/* XXX make this a separate function */
	
#ifdef TYPECHECK_DEBUG
	for (i=0; i<state.m->basicblockcount; ++i) {
		if (state.m->basicblocks[i].flags != BBDELETED
			&& state.m->basicblocks[i].flags != BBUNDEF
			&& state.m->basicblocks[i].flags != BBFINISHED
			&& state.m->basicblocks[i].flags != BBTYPECHECK_UNDEF) /* typecheck may never reach
													 * some exception handlers,
													 * that's ok. */
		{
			LOG2("block L%03d has invalid flags after typecheck: %d",
				 state.m->basicblocks[i].debug_nr,state.m->basicblocks[i].flags);
			TYPECHECK_ASSERT(false);
		}
	}
#endif
	
	/* Reset blocks we never reached */
	
	for (i=0; i<state.m->basicblockcount; ++i) {
		if (state.m->basicblocks[i].flags == BBTYPECHECK_UNDEF)
			state.m->basicblocks[i].flags = BBFINISHED;
	}
		
    LOGimp("exiting typecheck");

	/* just return methodinfo* to indicate everything was ok */

	return state.m;
}

#undef COPYTYPE

#endif /* CACAO_TYPECHECK */

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
