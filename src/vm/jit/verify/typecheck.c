/* src/vm/jit/verify/typecheck.c - typechecking (part of bytecode verification)

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

   Authors: Edwin Steiner

   Changes: Christian Thalinger

   $Id: typecheck.c 5514 2006-09-15 14:18:19Z edwin $

*/

/*

What's the purpose of the `typechecker`?
----------------------------------------

The typechecker analyses (the intermediate repr. of) the bytecode of
each method and ensures that for each instruction the values on the
stack and in local variables are of the correct type whenever the
instruction is executed.

type checking is a mandatory part of bytecode verification.


How does the typechecker work?
------------------------------

The JVM stack and the local variables are not statically typed, so the
typechecker has to *infer* the static types of stack slots and local
variables at each point of the method. The JVM spec imposes a lot of
restrictions on the bytecode in order to guarantee that this is always
possible.

Basically the typechecker solves the data flow equations of the method.
This is done in the usual way for a forward data flow analysis: Starting
from the entry point of the method the typechecker follows the CFG and
records the type of each stack slot and local variable at each point[1].
When two or more control flow paths merge at a point, the union of the
types for each slot/variable is taken. The algorithm continues to follow
all possible paths[2] until the recorded types do not change anymore (ie.
the equations have been solved).

If the solution has been reached and the resulting types are valid for
all instructions, then type checking terminates with success, otherwise
an exception is thrown.


Why is this code so damn complicated?
-------------------------------------

Short answer: The devil's in the details.

While the basic operation of the typechecker is no big deal, there are
many properties of Java bytecode which make type checking hard. Some of
them are not even addressed in the JVM spec. Some problems and their
solutions:

*) Finding a good representation of the union of two reference types is
difficult because of multiple inheritance of interfaces. 

	Solution: The typeinfo system can represent such "merged" types by a
	list of proper subclasses of a class. Example:

		typeclass=java.lang.Object merged={ InterfaceA, InterfaceB }
	
	represents the result of merging two interface types "InterfaceA"
	and "InterfaceB".

*) When the code of a method is verified, there may still be unresolved
references to classes/methods/fields in the code, which we may not force
to be resolved eagerly. (A similar problem arises because of the special
checks for protected members.)

	Solution: The typeinfo system knows how to deal with unresolved
	class references. Whenever a check has to be performed for an
	unresolved type, the type is annotated with constraints representing
	the check. Later, when the type is resolved, the constraints are
	checked. (See the constrain_unresolved_... and the resolve_...
	methods.)[3]

*) Checks for uninitialized object instances are hard because after the
invocation of <init> on an uninitialized object *all* slots/variables
referring to this object (and exactly those slots/variables) must be
marked as initialized.

	Solution: The JVM spec describes a solution, which has been
	implemented in this typechecker.


--- Footnotes

[1] Actually only the types of slots/variables at the start of each
basic block are remembered. Within a basic block the algorithm only keeps
the types of the slots/variables for the "current" instruction which is
being analysed. 

[2] Actually the algorithm iterates through the basic block list until
there are no more changes. Theoretically it would be wise to sort the
basic blocks topologically beforehand, but the number of average/max
iterations observed is so low, that this was not deemed necessary.

[3] This is similar to a method proposed by: Alessandro Coglio et al., A
Formal Specification of Java Class Loading, Technical Report, Kestrel
Institute April 2000, revised July 2000 
http://www.kestrel.edu/home/people/coglio/loading.pdf
An important difference is that Coglio's subtype constraints are checked
after loading, while our constraints are checked when the field/method
is accessed for the first time, so we can guarantee lexically correct
error reporting.

*/

#include <assert.h>
#include <string.h>

#include "config.h"
#include "vm/types.h"
#include "vm/global.h"

#ifdef ENABLE_VERIFIER

#include "mm/memory.h"
#include "toolbox/logging.h"
#include "native/native.h"
#include "vm/builtin.h"
#include "vm/jit/patcher.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/jit/jit.h"
#include "vm/jit/show.h"
#include "vm/access.h"
#include "vm/resolve.h"
#include "vm/exceptions.h"

/****************************************************************************/
/* DEBUG HELPERS                                                            */
/****************************************************************************/

#ifdef TYPECHECK_DEBUG
#define TYPECHECK_ASSERT(cond)  assert(cond)
#else
#define TYPECHECK_ASSERT(cond)
#endif

#ifdef TYPECHECK_VERBOSE_OPT
bool opt_typecheckverbose = false;
#define DOLOG(action)  do { if (opt_typecheckverbose) {action;} } while(0)
#else
#define DOLOG(action)
#endif

#ifdef TYPECHECK_VERBOSE
#define TYPECHECK_VERBOSE_IMPORTANT
#define LOGNL              DOLOG(puts(""))
#define LOG(str)           DOLOG(puts(str);)
#define LOG1(str,a)        DOLOG(printf(str,a); LOGNL)
#define LOG2(str,a,b)      DOLOG(printf(str,a,b); LOGNL)
#define LOG3(str,a,b,c)    DOLOG(printf(str,a,b,c); LOGNL)
#define LOGIF(cond,str)    DOLOG(do {if (cond) { puts(str); }} while(0))
#ifdef  TYPEINFO_DEBUG
#define LOGINFO(info)      DOLOG(do {typeinfo_print_short(stdout,(info)); LOGNL;} while(0))
#else
#define LOGINFO(info)
#define typevector_print(x,y,z)
#endif
#define LOGFLUSH           DOLOG(fflush(stdout))
#define LOGSTR(str)        DOLOG(printf("%s", str))
#define LOGSTR1(str,a)     DOLOG(printf(str,a))
#define LOGSTR2(str,a,b)   DOLOG(printf(str,a,b))
#define LOGSTR3(str,a,b,c) DOLOG(printf(str,a,b,c))
#define LOGNAME(c)         DOLOG(class_classref_or_classinfo_print(c))
#define LOGMETHOD(str,m)   DOLOG(printf("%s", str); method_println(m);)
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
#define LOGNAME(c)
#define LOGMETHOD(str,m)
#endif

#ifdef TYPECHECK_VERBOSE_IMPORTANT
#define LOGimp(str)     DOLOG(puts(str);LOGNL)
#define LOGimpSTR(str)  DOLOG(puts(str))
#else
#define LOGimp(str)
#define LOGimpSTR(str)
#endif

#if defined(TYPECHECK_VERBOSE) || defined(TYPECHECK_VERBOSE_IMPORTANT)

#include <stdio.h>

static void typecheck_print_var(FILE *file, jitdata *jd, s4 index)
{
	varinfo *var;

	assert(index >= 0 && index < jd->varcount);
	var = jd->var + index;
	typeinfo_print_type(file, var->type, &(var->typeinfo));
}

static void typecheck_print_vararray(FILE *file, jitdata *jd, s4 *vars, int len)
{
	s4 i;

	for (i=0; i<len; ++i) {
		if (i)
			fputc(' ', file);
		typecheck_print_var(file, jd, *vars++);
	}
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
static int stat_methods_with_handlers = 0;
static int stat_methods_maythrow = 0;
static int stat_iterations[STAT_ITERATIONS+1] = { 0 };
static int stat_reached = 0;
static int stat_copied = 0;
static int stat_merged = 0;
static int stat_merging_changed = 0;
static int stat_backwards = 0;
static int stat_blocks[STAT_BLOCKS+1] = { 0 };
static int stat_locals[STAT_LOCALS+1] = { 0 };
static int stat_ins = 0;
static int stat_ins_maythrow = 0;
static int stat_ins_stack = 0;
static int stat_ins_field = 0;
static int stat_ins_field_unresolved = 0;
static int stat_ins_field_uninitialized = 0;
static int stat_ins_invoke = 0;
static int stat_ins_invoke_unresolved = 0;
static int stat_ins_primload = 0;
static int stat_ins_aload = 0;
static int stat_ins_builtin = 0;
static int stat_ins_builtin_gen = 0;
static int stat_ins_branch = 0;
static int stat_ins_switch = 0;
static int stat_ins_primitive_return = 0;
static int stat_ins_areturn = 0;
static int stat_ins_areturn_unresolved = 0;
static int stat_ins_athrow = 0;
static int stat_ins_athrow_unresolved = 0;
static int stat_ins_unchecked = 0;
static int stat_handlers_reached = 0;
static int stat_savedstack = 0;

#define TYPECHECK_MARK(var)   ((var) = true)
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
	fprintf(file,"    with handler(s): %8d\n",stat_methods_with_handlers);
	fprintf(file,"    with throw(s)  : %8d\n",stat_methods_maythrow);
	fprintf(file,"reached blocks     : %8d\n",stat_reached);
	fprintf(file,"copied states      : %8d\n",stat_copied);
	fprintf(file,"merged states      : %8d\n",stat_merged);
	fprintf(file,"merging changed    : %8d\n",stat_merging_changed);
	fprintf(file,"backwards branches : %8d\n",stat_backwards);
	fprintf(file,"handlers reached   : %8d\n",stat_handlers_reached);
	fprintf(file,"saved stack (times): %8d\n",stat_savedstack);
	fprintf(file,"instructions       : %8d\n",stat_ins);
	fprintf(file,"    stack          : %8d\n",stat_ins_stack);
	fprintf(file,"    field access   : %8d\n",stat_ins_field);
	fprintf(file,"      (unresolved) : %8d\n",stat_ins_field_unresolved);
	fprintf(file,"      (uninit.)    : %8d\n",stat_ins_field_uninitialized);
	fprintf(file,"    invocations    : %8d\n",stat_ins_invoke);
	fprintf(file,"      (unresolved) : %8d\n",stat_ins_invoke_unresolved);
	fprintf(file,"    load primitive : (currently not counted) %8d\n",stat_ins_primload);
	fprintf(file,"    load address   : %8d\n",stat_ins_aload);
	fprintf(file,"    builtins       : %8d\n",stat_ins_builtin);
	fprintf(file,"        generic    : %8d\n",stat_ins_builtin_gen);
	fprintf(file,"    branches       : %8d\n",stat_ins_branch);
	fprintf(file,"    switches       : %8d\n",stat_ins_switch);
	fprintf(file,"    prim. return   : %8d\n",stat_ins_primitive_return);
	fprintf(file,"    areturn        : %8d\n",stat_ins_areturn);
	fprintf(file,"      (unresolved) : %8d\n",stat_ins_areturn_unresolved);
	fprintf(file,"    athrow         : %8d\n",stat_ins_athrow);
	fprintf(file,"      (unresolved) : %8d\n",stat_ins_athrow_unresolved);
	fprintf(file,"    unchecked      : %8d\n",stat_ins_unchecked);
	fprintf(file,"    maythrow       : %8d\n",stat_ins_maythrow);
	fprintf(file,"iterations used:\n");
	print_freq(file,stat_iterations,STAT_ITERATIONS);
	fprintf(file,"basic blocks per method / 10:\n");
	print_freq(file,stat_blocks,STAT_BLOCKS);
	fprintf(file,"locals:\n");
	print_freq(file,stat_locals,STAT_LOCALS);
}
						   
#else
						   
#define TYPECHECK_COUNT(cnt)
#define TYPECHECK_MARK(var)
#define TYPECHECK_COUNTIF(cond,cnt)
#define TYPECHECK_COUNT_FREQ(array,val,limit)
#endif


/****************************************************************************/
/* MACROS FOR THROWING EXCEPTIONS                                           */
/****************************************************************************/

#define TYPECHECK_VERIFYERROR_ret(m,msg,retval) \
    do { \
        exceptions_throw_verifyerror((m), (msg)); \
        return (retval); \
    } while (0)

#define TYPECHECK_VERIFYERROR_main(msg)  TYPECHECK_VERIFYERROR_ret(state.m,(msg),NULL)
#define TYPECHECK_VERIFYERROR_bool(msg)  TYPECHECK_VERIFYERROR_ret(state->m,(msg),false)


/****************************************************************************/
/* MACROS FOR VARIABLE TYPE CHECKING                                        */
/****************************************************************************/

#define TYPECHECK_CHECK_TYPE(i,tp,msg)                               \
    do {                                                             \
        if (VAR(i)->type != (tp)) {                                  \
            exceptions_throw_verifyerror(state->m, (msg));           \
            return false;                                            \
        }                                                            \
    } while (0)

#define TYPECHECK_INT(i)                                             \
    TYPECHECK_CHECK_TYPE(i,TYPE_INT,"Expected to find integer value")
#define TYPECHECK_LNG(i)                                             \
    TYPECHECK_CHECK_TYPE(i,TYPE_LNG,"Expected to find long value")
#define TYPECHECK_FLT(i)                                             \
    TYPECHECK_CHECK_TYPE(i,TYPE_FLT,"Expected to find float value")
#define TYPECHECK_DBL(i)                                             \
    TYPECHECK_CHECK_TYPE(i,TYPE_DBL,"Expected to find double value")
#define TYPECHECK_ADR(i)                                             \
    TYPECHECK_CHECK_TYPE(i,TYPE_ADR,"Expected to find object value")

#define TYPECHECK_INT_OP(o)  TYPECHECK_INT((o).varindex)
#define TYPECHECK_LNG_OP(o)  TYPECHECK_LNG((o).varindex)
#define TYPECHECK_FLT_OP(o)  TYPECHECK_FLT((o).varindex)
#define TYPECHECK_DBL_OP(o)  TYPECHECK_DBL((o).varindex)
#define TYPECHECK_ADR_OP(o)  TYPECHECK_ADR((o).varindex)


/****************************************************************************/
/* VERIFIER STATE STRUCT                                                    */
/****************************************************************************/

/* verifier_state - This structure keeps the current state of the      */
/* bytecode verifier for passing it between verifier functions.        */

typedef struct verifier_state {
    instruction *iptr;               /* pointer to current instruction */
    basicblock *bptr;                /* pointer to current basic block */

	methodinfo *m;                               /* the current method */
	jitdata *jd;                         /* jitdata for current method */
	codegendata *cd;                 /* codegendata for current method */
	registerdata *rd;               /* registerdata for current method */

	basicblock *basicblocks;
	s4 basicblockcount;
	
	s4 numlocals;                         /* number of local variables */
	s4 validlocals;                /* number of Java-accessible locals */
	typedescriptor returntype;    /* return type of the current method */
	s4 *reverselocalmap;
	
	s4 *savedindices;
	s4 *savedinvars;                            /* saved invar pointer */

	s4 exinvars;
	
    exceptiontable **handlers;            /* active exception handlers */
	
    bool repeat;            /* if true, blocks are iterated over again */
    bool initmethod;             /* true if this is an "<init>" method */

#ifdef TYPECHECK_STATISTICS
	bool stat_maythrow;          /* at least one instruction may throw */
#endif
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

/* typecheck_copy_types ********************************************************
 
   Copy the types of the source variables to the destination variables.

   IN:
	   state............current verifier state
	   srcvars..........array of variable indices to copy
	   dstvars..........array of the destination variables
	   n................number of variables to copy

   RETURN VALUE:
       true.............success
	   false............an exception has been thrown

*******************************************************************************/

static bool
typecheck_copy_types(verifier_state *state, s4 *srcvars, s4 *dstvars, s4 n)
{
	s4 i;
	varinfo *sv;
	varinfo *dv;
	jitdata *jd = state->jd;

	for (i=0; i < n; ++i, ++srcvars, ++dstvars) {
		sv = VAR(*srcvars);
		dv = VAR(*dstvars);

		dv->type = sv->type;
		if (dv->type == TYPE_ADR) {
			TYPEINFO_CLONE(sv->typeinfo,dv->typeinfo);
		}
	}
	return true;
}


/* typecheck_merge_types *******************************************************
 
   Merge the types of the source variables into the destination variables.

   IN:
       state............current state of the verifier
	   srcvars..........source variable indices
	   dstvars..........destination variable indices
	   n................number of variables

   RETURN VALUE:
       typecheck_TRUE...the destination variables have been modified
	   typecheck_FALSE..the destination variables are unchanged
	   typecheck_FAIL...an exception has been thrown

*******************************************************************************/

static typecheck_result
typecheck_merge_types(verifier_state *state,s4 *srcvars, s4 *dstvars, s4 n)
{
	s4 i;
	varinfo *sv;
	varinfo *dv;
	jitdata *jd = state->jd;
	typecheck_result r;
	bool changed = false;
	
	for (i=0; i < n; ++i, ++srcvars, ++dstvars) {
		sv = VAR(*srcvars);
		dv = VAR(*dstvars);

		if (dv->type != sv->type) {
			exceptions_throw_verifyerror(state->m,"Stack type mismatch");
			return typecheck_FAIL;
		}
		if (dv->type == TYPE_ADR) {
			if (TYPEINFO_IS_PRIMITIVE(dv->typeinfo)) {
				/* dv has returnAddress type */
				if (!TYPEINFO_IS_PRIMITIVE(sv->typeinfo)) {
					exceptions_throw_verifyerror(state->m,"Merging returnAddress with reference");
					return typecheck_FAIL;
				}
			}
			else {
				/* dv has reference type */
				if (TYPEINFO_IS_PRIMITIVE(sv->typeinfo)) {
					exceptions_throw_verifyerror(state->m,"Merging reference with returnAddress");
					return typecheck_FAIL;
				}
				r = typeinfo_merge(state->m,&(dv->typeinfo),&(sv->typeinfo));
				if (r == typecheck_FAIL)
					return r;
				changed |= r;
			}
		}
	}
	return changed;
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
	   dstvars..........indices of the destinations invars
	   dstlocals........the destinations inlocals
	   srcvars..........indices of the source's outvars
	   srclocals........the source locals
	   n................number of invars (== number of outvars)

   RETURN VALUE:
       typecheck_TRUE...destination state has been modified
	   typecheck_FALSE..destination state has not been modified
	   typecheck_FAIL...an exception has been thrown

*******************************************************************************/

static typecheck_result
typestate_merge(verifier_state *state,
				s4 *srcvars, varinfo *srclocals,
				s4 *dstvars, varinfo *dstlocals,
				s4 n)
{
	bool changed = false;
	typecheck_result r;
	
#if 0
	LOG("merge:");
	LOGSTR("dstack: "); DOLOG(typestack_print(stdout,deststack)); LOGNL;
	LOGSTR("ystack: "); DOLOG(typestack_print(stdout,ystack)); LOGNL;
	LOGSTR("dloc  : "); DOLOG(typevector_print(stdout,destloc,state->numlocals)); LOGNL;
	LOGSTR("yloc  : "); DOLOG(typevector_print(stdout,yloc,state->numlocals)); LOGNL;
	LOGFLUSH;
#endif

	/* The stack is always merged. If there are returnAddresses on
	 * the stack they are ignored in this step. */

	r = typecheck_merge_types(state, srcvars, dstvars, n);
	if (r == typecheck_FAIL)
		return r;
	changed |= r;

	/* merge the locals */

	r = typevector_merge(state->m, dstlocals, srclocals, state->numlocals);
	if (r == typecheck_FAIL)
		return r;
	return changed | r;
}


/* typestate_reach *************************************************************
 
   Reach a destination block and propagate stack and local variable types

   IN:
       state............current state of the verifier
	   destblock........destination basic block
	   srcvars..........variable indices of the outvars to propagate
	   srclocals........local variables to propagate
	   n................number of srcvars

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
				s4 *srcvars, varinfo *srclocals, s4 n)
{
	s4 i;
	varinfo *destloc;
	varinfo *sv;
	jitdata *jd = state->jd;
	bool changed = false;
	typecheck_result r;

	LOG1("reaching block L%03d",destblock->nr);
	TYPECHECK_COUNT(stat_reached);
	
	destloc = destblock->inlocals;

	if (destblock->flags == BBTYPECHECK_UNDEF) {
		/* The destblock has never been reached before */

		TYPECHECK_COUNT(stat_copied);
		LOG1("block L%03d reached first time",destblock->nr);
		
		if (!typecheck_copy_types(state, srcvars, destblock->invars, n))
			return false;
		typevector_copy_inplace(srclocals, destloc, state->numlocals);
		changed = true;
	}
	else {
		/* The destblock has already been reached before */
		
		TYPECHECK_COUNT(stat_merged);
		LOG1("block L%03d reached before", destblock->nr);
		
		r = typestate_merge(state, srcvars, srclocals, 
				destblock->invars, destblock->inlocals, n);
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


/* typestate_save_invars *******************************************************
 
   Save the invars of the current basic block in the space reserved by
   parse.

   This function must be called before an instruction modifies a variable
   that is an invar of the current block. In such cases the invars of the
   block must be saved, and restored at the end of the analysis of this
   basic block, so that the invars again reflect the *input* to this basic
   block (and do not randomly contain types that appear within the block).

   IN:
       state............current state of the verifier

*******************************************************************************/

static void
typestate_save_invars(verifier_state *state)
{
	s4 i, index;
	s4 *pindex;
	
	LOG("saving invars");

	if (!state->savedindices) {
		LOG("allocating savedindices buffer");
		pindex = DMNEW(s4, state->m->maxstack);
		state->savedindices = pindex;
		index = state->numlocals + VERIFIER_EXTRA_VARS;
		for (i=0; i<state->m->maxstack; ++i)
			*pindex++ = index++;
	}

	/* save types */

	typecheck_copy_types(state, state->bptr->invars, state->savedindices, 
			state->bptr->indepth);

	/* set the invars of the block to the saved variables */
	/* and remember the original invars                   */

	state->savedinvars = state->bptr->invars;
	state->bptr->invars = state->savedindices;
}


/* typestate_restore_invars  ***************************************************
 
   Restore the invars of the current basic block that have been previously
   saved by `typestate_save_invars`.

   IN:
       state............current state of the verifier

*******************************************************************************/

static void
typestate_restore_invars(verifier_state *state)
{
	TYPECHECK_COUNT(stat_savedstack);
	LOG("restoring saved invars");

	/* restore the invars pointer */

	state->bptr->invars = state->savedinvars;

	/* copy the types back */

	typecheck_copy_types(state, state->savedindices, state->bptr->invars,
			state->bptr->indepth);

	/* mark that there are no saved invars currently */

	state->savedinvars = NULL;
}


/****************************************************************************/
/* MISC MACROS                                                              */
/****************************************************************************/

#define COPYTYPE(source,dest)                                        \
    {if (VAROP(source)->type == TYPE_ADR)                            \
            TYPEINFO_COPY(VAROP(source)->typeinfo,VAROP(dest)->typeinfo);}

#define ISBUILTIN(v)   (bte->fp == (functionptr) (v))


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
	methodinfo *mi;                        /* resolved method (if any) */
	methoddesc *md;                 /* descriptor of the called method */
	utf *mname;                                         /* method name */
	utf *mclassname;                     /* name of the method's class */
	bool specialmethod;            /* true if a <...> method is called */
	int opcode;                                   /* invocation opcode */
	bool callinginit;                      /* true if <init> is called */
	instruction *ins;
	classref_or_classinfo initclass;
	typedesc *td;
	s4 argindex;                            /* argument variable index */
	varinfo *av;                                  /* argument variable */
	varinfo *dv;                  /* result variable of the invocation */
	int i;                                                  /* counter */
    u1 rtype;                          /* return type of called method */
	resolve_result_t result;
	jitdata *jd;

	jd = state->jd;

	/* get the FMIref and the unresolved_method struct (if any) */
	/* from the instruction                                     */

	if (INSTRUCTION_IS_UNRESOLVED(state->iptr)) {
		/* unresolved method */
		um = state->iptr->sx.s23.s3.um;
		mref = um->methodref;
	}
	else {
		/* resolved method */
		um = NULL;
		mref = state->iptr->sx.s23.s3.fmiref;
	}

	/* get method descriptor and name */

	md = mref->parseddesc.md;
	mname = mref->name;

	/* get method info (if resolved) and classname */

	if (IS_FMIREF_RESOLVED(mref)) {
		mi = mref->p.method;
		mclassname = mi->class->name;
	}
	else {
		mi = NULL;
		mclassname = mref->p.classref->name;
	}

	specialmethod = (mname->text[0] == '<');
	opcode = state->iptr[0].opc;
	dv = VAROP(state->iptr->dst);

	/* prevent compiler warnings */

	ins = NULL;

	/* check whether we are calling <init> */
	
	callinginit = (opcode == ICMD_INVOKESPECIAL && mname == utf_init);
	if (specialmethod && !callinginit)
		TYPECHECK_VERIFYERROR_bool("Invalid invocation of special method");

	/* allocate parameters if necessary */
	
	if (!md->params)
		if (!descriptor_params_from_paramtypes(md,
					(opcode == ICMD_INVOKESTATIC) ? ACC_STATIC : ACC_NONE))
			return false;

	/* check parameter types */

	i = md->paramcount; /* number of parameters including 'this'*/
	while (--i >= 0) {
		LOG1("param %d",i);
		argindex = state->iptr->sx.s23.s2.args[i];
		av = VAR(argindex);
		td = md->paramtypes + i;

		if (av->type != td->type)
			TYPECHECK_VERIFYERROR_bool("Parameter type mismatch in method invocation");

		if (av->type == TYPE_ADR) {
			LOGINFO(&(av->typeinfo));
			if (i==0 && callinginit)
			{
				/* first argument to <init> method */
				if (!TYPEINFO_IS_NEWOBJECT(av->typeinfo))
					TYPECHECK_VERIFYERROR_bool("Calling <init> on initialized object");

				/* get the address of the NEW instruction */
				LOGINFO(&(av->typeinfo));
				ins = (instruction *) TYPEINFO_NEWOBJECT_INSTRUCTION(av->typeinfo);
				if (ins)
					initclass = ins[-1].sx.val.c;
				else
					initclass.cls = state->m->class;
				LOGSTR("class: "); LOGNAME(initclass); LOGNL;
			}
		}
		else {
			/* non-adress argument. if this is the first argument and we are */
			/* invoking an instance method, this is an error.                */
			if (i==0 && opcode != ICMD_INVOKESTATIC) {
				TYPECHECK_VERIFYERROR_bool("Parameter type mismatch for 'this' argument");
			}
		}
		LOG("ok");
	}

	if (callinginit) {
		LOG("replacing uninitialized object");
		/* replace uninitialized object type on stack */

		/* for all live-in and live-through variables */ 
		for (i=0; i<state->iptr->s1.argcount; ++i) {
			argindex = state->iptr->sx.s23.s2.args[i];
			av = VAR(argindex);
			if (av->type == TYPE_ADR
					&& TYPEINFO_IS_NEWOBJECT(av->typeinfo)
					&& TYPEINFO_NEWOBJECT_INSTRUCTION(av->typeinfo) == ins)
			{
				LOG("replacing uninitialized type");

				/* If this stackslot is in the instack of
				 * this basic block we must save the type(s)
				 * we are going to replace.
				 */
				/* XXX this needs a new check */
				if (state->bptr->invars
						&& argindex >= state->bptr->invars[0] 
						&& argindex < state->bptr->varstart 
						&& !state->savedinvars)
				{
					typestate_save_invars(state);
				}

				if (!typeinfo_init_class(&(av->typeinfo),initclass))
					return false;
			}
		}

		/* replace uninitialized object type in locals */
		if (!typevector_init_object(state->jd->var, ins, initclass,
					state->numlocals))
			return false;

		/* initializing the 'this' reference? */
		if (!ins) {
			classinfo *cls;
			TYPECHECK_ASSERT(state->initmethod);
			/* { we are initializing the 'this' reference }                           */
			/* must be <init> of current class or direct superclass                   */
			/* the current class is linked, so must be its superclass. thus we can be */
			/* sure that resolving will be trivial.                                   */
			if (mi) {
				cls = mi->class;
			}
			else {
				if (!resolve_classref(state->m,mref->p.classref,resolveLazy,false,true,&cls))
					return false; /* exception */
			}

			/* if lazy resolving did not succeed, it's not one of the allowed classes */
			/* otherwise we check it directly                                         */
			if (cls == NULL || (cls != state->m->class && cls != state->m->class->super.cls)) {
				TYPECHECK_VERIFYERROR_bool("<init> calling <init> of the wrong class");
			}

			/* set our marker variable to type int */
			LOG("setting <init> marker");
			typevector_store(jd->var, state->numlocals-1, TYPE_INT, NULL);
		}
		else {
			/* { we are initializing an instance created with NEW } */
			if ((IS_CLASSREF(initclass) ? initclass.ref->name : initclass.cls->name) != mclassname) {
				TYPECHECK_VERIFYERROR_bool("wrong <init> called for uninitialized reference");
			}
		}
	}

	/* try to resolve the method lazily */

	result = new_resolve_method_lazy(jd, state->iptr, state->m);
	if (result == resolveFailed)
		return false;

	if (result != resolveSucceeded) {
		if (!um) {
			um = new_create_unresolved_method(state->m->class,
					state->m, state->iptr);

			if (!um)
				return false;
		}

		/* record subtype constraints for parameters */

		if (!new_constrain_unresolved_method(jd, um, state->m->class, 
					state->m, state->iptr))
			return false; /* XXX maybe wrap exception */

		/* store the unresolved_method pointer */

		state->iptr->sx.s23.s3.um = um;
		state->iptr->flags.bits |= INS_FLAG_UNRESOLVED;
	}
	else {
		assert(IS_FMIREF_RESOLVED(state->iptr->sx.s23.s3.fmiref));
	}

	rtype = md->returntype.type;
	if (rtype != TYPE_VOID) {
		dv->type = rtype;
		if (!typeinfo_init_from_typedesc(&(md->returntype),NULL,&(dv->typeinfo)))
			return false;
	}

	return true;
}


/* verify_generic_builtin ******************************************************
 
   Verify the call of a generic builtin method.
  
   IN:
       state............the current state of the verifier

   RETURN VALUE:
       true.............successful verification,
	   false............an exception has been thrown.

*******************************************************************************/

static bool
verify_generic_builtin(verifier_state *state)
{
	builtintable_entry *bte;
	s4 i;
	u1 rtype;
	methoddesc *md;
    varinfo *av;
	jitdata *jd = state->jd;

	TYPECHECK_COUNT(stat_ins_builtin_gen);

	bte = state->iptr->sx.s23.s3.bte;
	md = bte->md;
	i = md->paramcount;
	
	/* check the types of the arguments on the stack */

	for (i--; i >= 0; i--) {
		av = VAR(state->iptr->sx.s23.s2.args[i]);

		if (av->type != md->paramtypes[i].type) {
			TYPECHECK_VERIFYERROR_bool("parameter type mismatch for builtin method");
		}
		
#ifdef TYPECHECK_DEBUG
		/* generic builtins may only take primitive types and java.lang.Object references */
		if (av->type == TYPE_ADR && md->paramtypes[i].classref->name != utf_java_lang_Object) {
			*exceptionptr = new_internalerror("generic builtin method with non-generic reference parameter");
			return false;
		}
#endif
	}

	/* set the return type */

	rtype = md->returntype.type;
	if (rtype != TYPE_VOID) {
		varinfo *dv;

		dv = VAROP(state->iptr->dst);
		dv->type = rtype;
		if (!typeinfo_init_from_typedesc(&(md->returntype),NULL,&(dv->typeinfo)))
			return false;
	}

	return true;
}


/* verify_builtin **************************************************************
 
   Verify the call of a builtin method.
  
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
    classref_or_classinfo cls;
    varinfo *dv;               /* output variable of current instruction */
	jitdata *jd = state->jd;

	bte = state->iptr->sx.s23.s3.bte;
	dv = VAROP(state->iptr->dst);

	/* XXX this is an ugly if-chain but twisti did not want a function */
	/* pointer in builtintable_entry for this, so here you go.. ;)     */

	if (ISBUILTIN(BUILTIN_new)) {
		if (state->iptr[-1].opc != ICMD_ACONST)
			TYPECHECK_VERIFYERROR_bool("illegal instruction: builtin_new without class");
		cls = state->iptr[-1].sx.val.c;
		dv->type = TYPE_ADR;
		TYPEINFO_INIT_NEWOBJECT(dv->typeinfo,state->iptr);
	}
	else if (ISBUILTIN(BUILTIN_newarray_boolean)) {
		TYPECHECK_INT(state->iptr->sx.s23.s2.args[0]);
		dv->type = TYPE_ADR;
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dv->typeinfo,ARRAYTYPE_BOOLEAN);
	}
	else if (ISBUILTIN(BUILTIN_newarray_char)) {
		TYPECHECK_INT(state->iptr->sx.s23.s2.args[0]);
		dv->type = TYPE_ADR;
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dv->typeinfo,ARRAYTYPE_CHAR);
	}
	else if (ISBUILTIN(BUILTIN_newarray_float)) {
		TYPECHECK_INT(state->iptr->sx.s23.s2.args[0]);
		dv->type = TYPE_ADR;
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dv->typeinfo,ARRAYTYPE_FLOAT);
	}
	else if (ISBUILTIN(BUILTIN_newarray_double)) {
		TYPECHECK_INT(state->iptr->sx.s23.s2.args[0]);
		dv->type = TYPE_ADR;
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dv->typeinfo,ARRAYTYPE_DOUBLE);
	}
	else if (ISBUILTIN(BUILTIN_newarray_byte)) {
		TYPECHECK_INT(state->iptr->sx.s23.s2.args[0]);
		dv->type = TYPE_ADR;
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dv->typeinfo,ARRAYTYPE_BYTE);
	}
	else if (ISBUILTIN(BUILTIN_newarray_short)) {
		TYPECHECK_INT(state->iptr->sx.s23.s2.args[0]);
		dv->type = TYPE_ADR;
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dv->typeinfo,ARRAYTYPE_SHORT);
	}
	else if (ISBUILTIN(BUILTIN_newarray_int)) {
		TYPECHECK_INT(state->iptr->sx.s23.s2.args[0]);
		dv->type = TYPE_ADR;
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dv->typeinfo,ARRAYTYPE_INT);
	}
	else if (ISBUILTIN(BUILTIN_newarray_long)) {
		TYPECHECK_INT(state->iptr->sx.s23.s2.args[0]);
		dv->type = TYPE_ADR;
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dv->typeinfo,ARRAYTYPE_LONG);
	}
	else if (ISBUILTIN(BUILTIN_newarray))
	{
		TYPECHECK_INT(state->iptr->sx.s23.s2.args[0]);
		if (state->iptr[-1].opc != ICMD_ACONST)
			TYPECHECK_VERIFYERROR_bool("illegal instruction: builtin_newarray without class");
		/* XXX check that it is an array class(ref) */
		dv->type = TYPE_ADR;
		typeinfo_init_class(&(dv->typeinfo),state->iptr[-1].sx.val.c);
	}
	else if (ISBUILTIN(BUILTIN_arrayinstanceof))
	{
		TYPECHECK_ADR(state->iptr->sx.s23.s2.args[0]);
		if (state->iptr[-1].opc != ICMD_ACONST)
			TYPECHECK_VERIFYERROR_bool("illegal instruction: builtin_arrayinstanceof without class");
		dv->type = TYPE_INT;
		/* XXX check that it is an array class(ref) */
	}
	else {
		return verify_generic_builtin(state);
	}
	return true;
}


/* verify_multianewarray *******************************************************
 
   Verify a MULTIANEWARRAY instruction.
  
   IN:
       state............the current state of the verifier

   RETURN VALUE:
       true.............successful verification,
	   false............an exception has been thrown.

*******************************************************************************/

static bool
verify_multianewarray(verifier_state *state)
{
	classinfo *arrayclass;
	arraydescriptor *desc;
	s4 i;
	jitdata *jd = state->jd;

	/* check the array lengths on the stack */
	i = state->iptr->s1.argcount;
	if (i < 1)
		TYPECHECK_VERIFYERROR_bool("Illegal dimension argument");

	while (i--) {
		TYPECHECK_INT(state->iptr->sx.s23.s2.args[i]);
	}

	/* check array descriptor */
	if (INSTRUCTION_IS_RESOLVED(state->iptr)) {
		/* the array class reference has already been resolved */
		arrayclass = state->iptr->sx.s23.s3.c.cls;
		if (!arrayclass)
			TYPECHECK_VERIFYERROR_bool("MULTIANEWARRAY with unlinked class");
		if ((desc = arrayclass->vftbl->arraydesc) == NULL)
			TYPECHECK_VERIFYERROR_bool("MULTIANEWARRAY with non-array class");
		if (desc->dimension < state->iptr->s1.argcount)
			TYPECHECK_VERIFYERROR_bool("MULTIANEWARRAY dimension to high");

		/* set the array type of the result */
		typeinfo_init_classinfo(&(VAROP(state->iptr->dst)->typeinfo), arrayclass);
	}
	else {
		const char *p;
		constant_classref *cr;
		
		/* the array class reference is still unresolved */
		/* check that the reference indicates an array class of correct dimension */
		cr = state->iptr->sx.s23.s3.c.ref;
		i = 0;
		p = cr->name->text;
		while (p[i] == '[')
			i++;
		/* { the dimension of the array class == i } */
		if (i < 1)
			TYPECHECK_VERIFYERROR_bool("MULTIANEWARRAY with non-array class");
		if (i < state->iptr->s1.argcount)
			TYPECHECK_VERIFYERROR_bool("MULTIANEWARRAY dimension to high");

		/* set the array type of the result */
		if (!typeinfo_init_class(&(VAROP(state->iptr->dst)->typeinfo),CLASSREF_OR_CLASSINFO(cr)))
			return false;
	}

	/* set return type */

	VAROP(state->iptr->dst)->type = TYPE_ADR;

	/* everything ok */
	return true;
}


static void typecheck_invalidate_locals(verifier_state *state, s4 index, bool twoword)
{
	s4 i;
	s4 t;
	s4 mapped;
	jitdata *jd = state->jd;
	s4 *localmap = jd->local_map;
	varinfo *vars = jd->var;

	i = state->reverselocalmap[index];

	if (i > 0) {
		localmap += 5 * (i-1);
		for (t=0; t<5; ++t) {
			mapped = *localmap++;
			if (mapped >= 0 && IS_2_WORD_TYPE(vars[mapped].type)) {
				LOG1("invalidate local %d", mapped);
				vars[mapped].type = TYPE_VOID;
			}
		}
	}
	else {
		localmap += 5 * i;
	}

	for (t=0; t<5; ++t) {
		mapped = *localmap++;
		if (mapped >= 0) {
			LOG1("invalidate local %d", mapped);
			vars[mapped].type = TYPE_VOID;
		}
	}

	if (twoword) {
		for (t=0; t<5; ++t) {
			mapped = *localmap++;
			if (mapped >= 0) {
				LOG1("invalidate local %d", mapped);
				vars[mapped].type = TYPE_VOID;
			}
		}
	}
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
    int opcode;                                      /* current opcode */
    int len;                        /* for counting instructions, etc. */
    bool superblockend;        /* true if no fallthrough to next block */
	instruction *iptr;                      /* the current instruction */
    basicblock *tbptr;                   /* temporary for target block */
    bool maythrow;               /* true if this instruction may throw */
	unresolved_field *uf;                        /* for field accesses */
	constant_FMIref *fieldref;                   /* for field accesses */
	s4 i;
	typecheck_result r;
	resolve_result_t result;
	branch_target_t *table;
	lookup_target_t *lookup;
	jitdata *jd = state->jd;
	varinfo *dv;
	exceptiontable *ex;

	LOGSTR1("\n---- BLOCK %04d ------------------------------------------------\n",state->bptr->nr);
	LOGFLUSH;
	DOLOG(new_show_basicblock(jd, state->bptr, SHOW_STACK));

	superblockend = false;
	state->bptr->flags = BBFINISHED;

	/* prevent compiler warnings */

	dv = NULL;

	/* determine the active exception handlers for this block */
	/* XXX could use a faster algorithm with sorted lists or  */
	/* something?                                             */
	len = 0;
	for (ex = state->cd->exceptiontable; ex ; ex = ex->down) {
		if ((ex->start->nr <= state->bptr->nr) && (ex->end->nr > state->bptr->nr)) {
			LOG1("active handler L%03d", ex->handler->nr);
			state->handlers[len++] = ex;
		}
	}
	state->handlers[len] = NULL;

	/* init variable types at the start of this block */
	typevector_copy_inplace(state->bptr->inlocals, jd->var, state->numlocals);

	DOLOG(typecheck_print_vararray(stdout, jd, state->bptr->invars, 
				state->bptr->indepth));
	DOLOG(typevector_print(stdout, jd->var, state->numlocals));
	LOGNL; LOGFLUSH;

	/* loop over the instructions */
	len = state->bptr->icount;
	state->iptr = state->bptr->iinstr;
	while (--len >= 0)  {
		TYPECHECK_COUNT(stat_ins);

		iptr = state->iptr;

		DOLOG(typevector_print(stdout, jd->var, state->numlocals));
		LOGNL; LOGFLUSH;
		DOLOG(new_show_icmd(jd, state->iptr, false, SHOW_STACK)); LOGNL; LOGFLUSH;

		opcode = iptr->opc;
		dv = VAROP(iptr->dst);
		maythrow = false;

		switch (opcode) {

			/****************************************/
			/* STACK MANIPULATIONS                  */

			/* We just need to copy the typeinfo */
			/* for slots containing addresses.   */

			case ICMD_MOVE:
			case ICMD_COPY:
				TYPECHECK_COUNT(stat_ins_stack);
				COPYTYPE(iptr->s1, iptr->dst);
				dv->type = VAROP(iptr->s1)->type;
				break;

				/****************************************/
				/* PRIMITIVE VARIABLE ACCESS            */

			case ICMD_ILOAD: if (!typevector_checktype(jd->var,state->iptr->s1.varindex,TYPE_INT)) 
								 TYPECHECK_VERIFYERROR_bool("Local variable type mismatch");
							 dv->type = TYPE_INT;
							 break;
			case ICMD_IINC:  if (!typevector_checktype(jd->var,state->iptr->s1.varindex,TYPE_INT))
								 TYPECHECK_VERIFYERROR_bool("Local variable type mismatch");
							 dv->type = TYPE_INT;
							 break;
			case ICMD_FLOAD: if (!typevector_checktype(jd->var,state->iptr->s1.varindex,TYPE_FLT))
								 TYPECHECK_VERIFYERROR_bool("Local variable type mismatch");
							 dv->type = TYPE_FLT;
							 break;
			case ICMD_LLOAD: if (!typevector_checktype(jd->var,state->iptr->s1.varindex,TYPE_LNG))
								 TYPECHECK_VERIFYERROR_bool("Local variable type mismatch");
							 dv->type = TYPE_LNG;
							 break;
			case ICMD_DLOAD: if (!typevector_checktype(jd->var,state->iptr->s1.varindex,TYPE_DBL))
								 TYPECHECK_VERIFYERROR_bool("Local variable type mismatch");
							 dv->type = TYPE_DBL;
							 break;

			case ICMD_ISTORE: 
							 typecheck_invalidate_locals(state, state->iptr->dst.varindex, false);
							 typevector_store(jd->var,state->iptr->dst.varindex,TYPE_INT,NULL); 
							 break;
			case ICMD_FSTORE: 
							 typecheck_invalidate_locals(state, state->iptr->dst.varindex, false);
							 typevector_store(jd->var,state->iptr->dst.varindex,TYPE_FLT,NULL); 
							 break;
			case ICMD_LSTORE: 
							 typecheck_invalidate_locals(state, state->iptr->dst.varindex, true);
							 typevector_store(jd->var,state->iptr->dst.varindex,TYPE_LNG,NULL); 
							 break;
			case ICMD_DSTORE: 
							 typecheck_invalidate_locals(state, state->iptr->dst.varindex, true);
							 typevector_store(jd->var,state->iptr->dst.varindex,TYPE_DBL,NULL); 
							 break;

				/****************************************/
				/* LOADING ADDRESS FROM VARIABLE        */

			case ICMD_ALOAD:
				TYPECHECK_COUNT(stat_ins_aload);

				/* loading a returnAddress is not allowed */
				if (!TYPEDESC_IS_REFERENCE(jd->var[state->iptr->s1.varindex])) {
					TYPECHECK_VERIFYERROR_bool("illegal instruction: ALOAD loading non-reference");
				}
				TYPEINFO_COPY(jd->var[state->iptr->s1.varindex].typeinfo,dv->typeinfo);
				dv->type = TYPE_ADR;
				break;

				/****************************************/
				/* STORING ADDRESS TO VARIABLE          */

			case ICMD_ASTORE:
				typecheck_invalidate_locals(state, state->iptr->dst.varindex, false);

				if (TYPEINFO_IS_PRIMITIVE(VAROP(state->iptr->s1)->typeinfo)) {
					typevector_store_retaddr(jd->var,state->iptr->dst.varindex,&(VAROP(state->iptr->s1)->typeinfo));
				}
				else {
					typevector_store(jd->var,state->iptr->dst.varindex,TYPE_ADR,
							&(VAROP(state->iptr->s1)->typeinfo));
				}
				break;

				/****************************************/
				/* LOADING ADDRESS FROM ARRAY           */

			case ICMD_AALOAD:
				if (!TYPEINFO_MAYBE_ARRAY_OF_REFS(VAROP(state->iptr->s1)->typeinfo))
					TYPECHECK_VERIFYERROR_bool("illegal instruction: AALOAD on non-reference array");

				if (!typeinfo_init_component(&VAROP(state->iptr->s1)->typeinfo,&dv->typeinfo))
					return false;
				dv->type = TYPE_ADR;
				maythrow = true;
				break;

				/****************************************/
				/* FIELD ACCESS                         */

			case ICMD_PUTFIELD:
			case ICMD_PUTSTATIC:
			case ICMD_PUTFIELDCONST:
			case ICMD_PUTSTATICCONST:
			case ICMD_GETFIELD:
			case ICMD_GETSTATIC:
				TYPECHECK_COUNT(stat_ins_field);

				if (INSTRUCTION_IS_UNRESOLVED(state->iptr)) {
					uf = state->iptr->sx.s23.s3.uf;
					fieldref = uf->fieldref;
				}
				else {
					uf = NULL;
					fieldref = state->iptr->sx.s23.s3.fmiref;
				}

				/* try to resolve the field reference lazily */
				result = new_resolve_field_lazy(jd, state->iptr, state->m);
				if (result == resolveFailed)
					return false;

				if (result != resolveSucceeded) {
					if (!uf) {
						uf = new_create_unresolved_field(state->m->class, state->m, state->iptr);
						if (!uf)
							return false;

						state->iptr->sx.s23.s3.uf = uf;
						state->iptr->flags.bits |= INS_FLAG_UNRESOLVED;
					}

					/* record the subtype constraints for this field access */
					if (!new_constrain_unresolved_field(jd, uf,state->m->class,state->m,state->iptr))
						return false; /* XXX maybe wrap exception? */

					TYPECHECK_COUNTIF(INSTRUCTION_IS_UNRESOLVED(state->iptr),stat_ins_field_unresolved);
					TYPECHECK_COUNTIF(INSTRUCTION_IS_RESOLVED(state->iptr) && !state->iptr->sx.s23.s3.fmiref->p.field->class->initialized,stat_ins_field_uninitialized);
				}
					
				if (iptr->opc == ICMD_GETFIELD || iptr->opc == ICMD_GETSTATIC) {
					/* write the result type */
					dv->type = fieldref->parseddesc.fd->type;
					if (dv->type == TYPE_ADR) {
						if (!typeinfo_init_from_typedesc(fieldref->parseddesc.fd,NULL,&(dv->typeinfo)))
							return false;
					}
				}

				maythrow = true;
				break;

				/****************************************/
				/* PRIMITIVE ARRAY ACCESS               */

			case ICMD_ARRAYLENGTH:
				if (!TYPEINFO_MAYBE_ARRAY(VAROP(state->iptr->s1)->typeinfo)
						&& VAROP(state->iptr->s1)->typeinfo.typeclass.cls != pseudo_class_Arraystub)
					TYPECHECK_VERIFYERROR_bool("illegal instruction: ARRAYLENGTH on non-array");
				dv->type = TYPE_INT;
				maythrow = true;
				break;

			case ICMD_BALOAD:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(VAROP(state->iptr->s1)->typeinfo,ARRAYTYPE_BOOLEAN)
						&& !TYPEINFO_MAYBE_PRIMITIVE_ARRAY(VAROP(state->iptr->s1)->typeinfo,ARRAYTYPE_BYTE))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				dv->type = TYPE_INT;
				maythrow = true;
				break;
			case ICMD_CALOAD:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(VAROP(state->iptr->s1)->typeinfo,ARRAYTYPE_CHAR))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				dv->type = TYPE_INT;
				maythrow = true;
				break;
			case ICMD_DALOAD:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(VAROP(state->iptr->s1)->typeinfo,ARRAYTYPE_DOUBLE))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				dv->type = TYPE_DBL;
				maythrow = true;
				break;
			case ICMD_FALOAD:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(VAROP(state->iptr->s1)->typeinfo,ARRAYTYPE_FLOAT))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				dv->type = TYPE_FLT;
				maythrow = true;
				break;
			case ICMD_IALOAD:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(VAROP(state->iptr->s1)->typeinfo,ARRAYTYPE_INT))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				dv->type = TYPE_INT;
				maythrow = true;
				break;
			case ICMD_SALOAD:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(VAROP(state->iptr->s1)->typeinfo,ARRAYTYPE_SHORT))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				dv->type = TYPE_INT;
				maythrow = true;
				break;
			case ICMD_LALOAD:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(VAROP(state->iptr->s1)->typeinfo,ARRAYTYPE_LONG))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				dv->type = TYPE_LNG;
				maythrow = true;
				break;

			case ICMD_BASTORE:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(VAROP(state->iptr->s1)->typeinfo,ARRAYTYPE_BOOLEAN)
						&& !TYPEINFO_MAYBE_PRIMITIVE_ARRAY(VAROP(state->iptr->s1)->typeinfo,ARRAYTYPE_BYTE))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;
			case ICMD_CASTORE:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(VAROP(state->iptr->s1)->typeinfo,ARRAYTYPE_CHAR))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;
			case ICMD_DASTORE:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(VAROP(state->iptr->s1)->typeinfo,ARRAYTYPE_DOUBLE))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;
			case ICMD_FASTORE:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(VAROP(state->iptr->s1)->typeinfo,ARRAYTYPE_FLOAT))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;
			case ICMD_IASTORE:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(VAROP(state->iptr->s1)->typeinfo,ARRAYTYPE_INT))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;
			case ICMD_SASTORE:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(VAROP(state->iptr->s1)->typeinfo,ARRAYTYPE_SHORT))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;
			case ICMD_LASTORE:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(VAROP(state->iptr->s1)->typeinfo,ARRAYTYPE_LONG))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;

			case ICMD_AASTORE:
				/* we just check the basic input types and that the           */
				/* destination is an array of references. Assignability to    */
				/* the actual array must be checked at runtime, each time the */
				/* instruction is performed. (See builtin_canstore.)          */
				TYPECHECK_ADR_OP(state->iptr->sx.s23.s3);
				TYPECHECK_INT_OP(state->iptr->sx.s23.s2);
				TYPECHECK_ADR_OP(state->iptr->s1);
				if (!TYPEINFO_MAYBE_ARRAY_OF_REFS(VAROP(state->iptr->s1)->typeinfo))
					TYPECHECK_VERIFYERROR_bool("illegal instruction: AASTORE to non-reference array");
				maythrow = true;
				break;

			case ICMD_IASTORECONST:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(VAROP(state->iptr->s1)->typeinfo, ARRAYTYPE_INT))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;

			case ICMD_LASTORECONST:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(VAROP(state->iptr->s1)->typeinfo, ARRAYTYPE_LONG))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;

			case ICMD_BASTORECONST:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(VAROP(state->iptr->s1)->typeinfo, ARRAYTYPE_BOOLEAN)
						&& !TYPEINFO_MAYBE_PRIMITIVE_ARRAY(VAROP(state->iptr->s1)->typeinfo, ARRAYTYPE_BYTE))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;

			case ICMD_CASTORECONST:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(VAROP(state->iptr->s1)->typeinfo, ARRAYTYPE_CHAR))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;

			case ICMD_SASTORECONST:
				if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(VAROP(state->iptr->s1)->typeinfo, ARRAYTYPE_SHORT))
					TYPECHECK_VERIFYERROR_bool("Array type mismatch");
				maythrow = true;
				break;

				/****************************************/
				/* ADDRESS CONSTANTS                    */

			case ICMD_ACONST:
				if (state->iptr->flags.bits & INS_FLAG_CLASS) {
					/* a java.lang.Class reference */
					TYPEINFO_INIT_JAVA_LANG_CLASS(dv->typeinfo,state->iptr->sx.val.c);
				}
				else {
					if (state->iptr->sx.val.anyptr == NULL)
						TYPEINFO_INIT_NULLTYPE(dv->typeinfo);
					else {
						/* string constant (or constant for builtin function) */
						typeinfo_init_classinfo(&(dv->typeinfo),class_java_lang_String);
					}
				}
				dv->type = TYPE_ADR;
				break;

				/****************************************/
				/* CHECKCAST AND INSTANCEOF             */

			case ICMD_CHECKCAST:
				TYPECHECK_ADR_OP(state->iptr->s1);
				/* returnAddress is not allowed */
				if (!TYPEINFO_IS_REFERENCE(VAROP(state->iptr->s1)->typeinfo))
					TYPECHECK_VERIFYERROR_bool("Illegal instruction: CHECKCAST on non-reference");

				if (!typeinfo_init_class(&(dv->typeinfo),state->iptr->sx.s23.s3.c))
						return false;
				dv->type = TYPE_ADR;
				maythrow = true;
				break;

			case ICMD_INSTANCEOF:
				TYPECHECK_ADR_OP(state->iptr->s1);
				/* returnAddress is not allowed */
				if (!TYPEINFO_IS_REFERENCE(VAROP(state->iptr->s1)->typeinfo))
					TYPECHECK_VERIFYERROR_bool("Illegal instruction: INSTANCEOF on non-reference");
				dv->type = TYPE_INT;
				break;

				/****************************************/
				/* BRANCH INSTRUCTIONS                  */

			case ICMD_INLINE_GOTO:
				COPYTYPE(state->iptr->s1,state->iptr->dst);
				/* FALLTHROUGH! */
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

			case ICMD_IF_FCMPEQ:
			case ICMD_IF_FCMPNE:

			case ICMD_IF_FCMPL_LT:
			case ICMD_IF_FCMPL_GE:
			case ICMD_IF_FCMPL_GT:
			case ICMD_IF_FCMPL_LE:

			case ICMD_IF_FCMPG_LT:
			case ICMD_IF_FCMPG_GE:
			case ICMD_IF_FCMPG_GT:
			case ICMD_IF_FCMPG_LE:

			case ICMD_IF_DCMPEQ:
			case ICMD_IF_DCMPNE:

			case ICMD_IF_DCMPL_LT:
			case ICMD_IF_DCMPL_GE:
			case ICMD_IF_DCMPL_GT:
			case ICMD_IF_DCMPL_LE:

			case ICMD_IF_DCMPG_LT:
			case ICMD_IF_DCMPG_GE:
			case ICMD_IF_DCMPG_GT:
			case ICMD_IF_DCMPG_LE:
				TYPECHECK_COUNT(stat_ins_branch);

				/* propagate stack and variables to the target block */
				if (!typestate_reach(state, state->iptr->dst.block,
									 state->bptr->outvars, jd->var, 
									 state->bptr->outdepth))
					return false;
				break;

				/****************************************/
				/* SWITCHES                             */

			case ICMD_TABLESWITCH:
				TYPECHECK_COUNT(stat_ins_switch);

				table = iptr->dst.table;
				i = iptr->sx.s23.s3.tablehigh
					- iptr->sx.s23.s2.tablelow + 1 + 1; /* plus default */

				while (--i >= 0) {
					tbptr = (table++)->block;
					LOG2("target %d is block %04d",i,tbptr->nr);
					if (!typestate_reach(state, tbptr, state->bptr->outvars,
										 jd->var, state->bptr->outdepth))
						return false;
				}

				LOG("switch done");
				superblockend = true;
				break;

			case ICMD_LOOKUPSWITCH:
				TYPECHECK_COUNT(stat_ins_switch);

				lookup = iptr->dst.lookup;
				i = iptr->sx.s23.s2.lookupcount;

				if (!typestate_reach(state,iptr->sx.s23.s3.lookupdefault.block,
									 state->bptr->outvars, jd->var,
									 state->bptr->outdepth))
					return false;

				while (--i >= 0) {
					tbptr = (lookup++)->target.block;
					LOG2("target %d is block %04d",i,tbptr->nr);
					if (!typestate_reach(state, tbptr, state->bptr->outvars,
								jd->var, state->bptr->outdepth))
						return false;
				}

				LOG("switch done");
				superblockend = true;
				break;


				/****************************************/
				/* ADDRESS RETURNS AND THROW            */

			case ICMD_ATHROW:
				TYPECHECK_COUNT(stat_ins_athrow);
				r = typeinfo_is_assignable_to_class(&VAROP(state->iptr->s1)->typeinfo,
						CLASSREF_OR_CLASSINFO(class_java_lang_Throwable));
				if (r == typecheck_FALSE)
					TYPECHECK_VERIFYERROR_bool("illegal instruction: ATHROW on non-Throwable");
				if (r == typecheck_FAIL)
					return false;
				if (r == typecheck_MAYBE) {
					/* the check has to be postponed. we need a patcher */
					TYPECHECK_COUNT(stat_ins_athrow_unresolved);
					iptr->sx.s23.s2.uc = create_unresolved_class(
							state->m, 
							/* XXX make this more efficient, use class_java_lang_Throwable
							 * directly */
							class_get_classref(state->m->class,utf_java_lang_Throwable),
							&VAROP(state->iptr->s1)->typeinfo);
					iptr->flags.bits |= INS_FLAG_UNRESOLVED;
				}
				superblockend = true;
				maythrow = true;
				break;

			case ICMD_ARETURN:
				TYPECHECK_COUNT(stat_ins_areturn);
				if (!TYPEINFO_IS_REFERENCE(VAROP(state->iptr->s1)->typeinfo))
					TYPECHECK_VERIFYERROR_bool("illegal instruction: ARETURN on non-reference");

				if (state->returntype.type != TYPE_ADR
						|| (r = typeinfo_is_assignable(&VAROP(state->iptr->s1)->typeinfo,&(state->returntype.typeinfo))) 
								== typecheck_FALSE)
					TYPECHECK_VERIFYERROR_bool("Return type mismatch");
				if (r == typecheck_FAIL)
					return false;
				if (r == typecheck_MAYBE) {
					/* the check has to be postponed, we need a patcher */
					TYPECHECK_COUNT(stat_ins_areturn_unresolved);
					iptr->sx.s23.s2.uc = create_unresolved_class(
							state->m, 
							state->m->parseddesc->returntype.classref,
							&VAROP(state->iptr->s1)->typeinfo);
					iptr->flags.bits |= INS_FLAG_UNRESOLVED;
				}
				goto return_tail;

				/****************************************/
				/* PRIMITIVE RETURNS                    */

			case ICMD_IRETURN:
				if (state->returntype.type != TYPE_INT) TYPECHECK_VERIFYERROR_bool("Return type mismatch");
				goto return_tail;

			case ICMD_LRETURN:
				if (state->returntype.type != TYPE_LNG) TYPECHECK_VERIFYERROR_bool("Return type mismatch");
				goto return_tail;

			case ICMD_FRETURN:
				if (state->returntype.type != TYPE_FLT) TYPECHECK_VERIFYERROR_bool("Return type mismatch");
				goto return_tail;

			case ICMD_DRETURN:
				if (state->returntype.type != TYPE_DBL) TYPECHECK_VERIFYERROR_bool("Return type mismatch");
				goto return_tail;

			case ICMD_RETURN:
				if (state->returntype.type != TYPE_VOID) TYPECHECK_VERIFYERROR_bool("Return type mismatch");
return_tail:
				TYPECHECK_COUNT(stat_ins_primitive_return);

				if (state->initmethod && state->m->class != class_java_lang_Object) {
					/* Check if the 'this' instance has been initialized. */
					LOG("Checking <init> marker");
					if (!typevector_checktype(jd->var,state->numlocals-1,TYPE_INT))
						TYPECHECK_VERIFYERROR_bool("<init> method does not initialize 'this'");
				}

				superblockend = true;
				maythrow = true;
				break;

				/****************************************/
				/* SUBROUTINE INSTRUCTIONS              */

			case ICMD_JSR:
				LOG("jsr");

				tbptr = state->iptr->sx.s23.s3.jsrtarget.block;
				TYPEINFO_INIT_RETURNADDRESS(dv->typeinfo, state->bptr->next);
				if (!typestate_reach(state, tbptr, state->bptr->outvars, jd->var,
							state->bptr->outdepth))
					return false;

				superblockend = true;
				break;

			case ICMD_RET:
				/* check returnAddress variable */
				if (!typevector_checkretaddr(jd->var,state->iptr->s1.varindex))
					TYPECHECK_VERIFYERROR_bool("illegal instruction: RET using non-returnAddress variable");

				if (!typestate_reach(state, iptr->dst.block, state->bptr->outvars, jd->var,
							state->bptr->outdepth))
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
				TYPECHECK_COUNTIF(INSTRUCTION_IS_UNRESOLVED(iptr), stat_ins_invoke_unresolved);
				maythrow = true;
				break;

				/****************************************/
				/* MULTIANEWARRAY                       */

			case ICMD_MULTIANEWARRAY:
				if (!verify_multianewarray(state))
					return false;		
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
				LOG2("ICMD %d at %d\n", state->iptr->opc, (int)(state->iptr-state->bptr->iinstr));
				LOG("Should have been converted to builtin function call.");
				TYPECHECK_ASSERT(false);
				break;
#endif

				/****************************************/
				/* UNCHECKED OPERATIONS                 */

				/*********************************************
				 * Instructions below...
				 *     *) don't operate on local variables,
				 *     *) don't operate on references,
				 *     *) don't operate on returnAddresses,
				 *     *) don't affect control flow (except
				 *        by throwing exceptions).
				 *
				 * (These instructions are typechecked in
				 *  analyse_stack.)
				 ********************************************/

				/* Instructions which may throw a runtime exception: */

			case ICMD_IDIV:
			case ICMD_IREM:
				dv->type = TYPE_INT;
				maythrow = true;
				break;

			case ICMD_LDIV:
			case ICMD_LREM:
				dv->type = TYPE_LNG;
				maythrow = true;
				break;

				/* Instructions which never throw a runtime exception: */
			case ICMD_NOP:
			case ICMD_POP:
			case ICMD_POP2:
				break;

			case ICMD_ICONST:
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
			case ICMD_IMULPOW2:
			case ICMD_IDIVPOW2:
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
			case ICMD_INT2BYTE:
			case ICMD_INT2CHAR:
			case ICMD_INT2SHORT:
			case ICMD_L2I:
			case ICMD_F2I:
			case ICMD_D2I:
			case ICMD_LCMP:
			case ICMD_LCMPCONST:
			case ICMD_FCMPL:
			case ICMD_FCMPG:
			case ICMD_DCMPL:
			case ICMD_DCMPG:
				dv->type = TYPE_INT;
				break;

			case ICMD_LCONST:
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
			case ICMD_LMULPOW2:
			case ICMD_LDIVPOW2:
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
			case ICMD_F2L:
			case ICMD_D2L:
				dv->type = TYPE_LNG;
				break;

			case ICMD_FCONST:
			case ICMD_I2F:
			case ICMD_L2F:
			case ICMD_D2F:
			case ICMD_FADD:
			case ICMD_FSUB:
			case ICMD_FMUL:
			case ICMD_FDIV:
			case ICMD_FREM:
			case ICMD_FNEG:
				dv->type = TYPE_FLT;
				break;

			case ICMD_DCONST:
			case ICMD_I2D:
			case ICMD_L2D:
			case ICMD_F2D:
			case ICMD_DADD:
			case ICMD_DSUB:
			case ICMD_DMUL:
			case ICMD_DDIV:
			case ICMD_DREM:
			case ICMD_DNEG:
				dv->type = TYPE_DBL;
				break;

			case ICMD_INLINE_START:
			case ICMD_INLINE_END:
				break;

				/* XXX What shall we do with the following ?*/
			case ICMD_AASTORECONST:
				TYPECHECK_COUNT(stat_ins_unchecked);
				break;

				/****************************************/

			default:
				LOG2("ICMD %d at %d\n", state->iptr->opc, (int)(state->iptr-state->bptr->iinstr));
				TYPECHECK_VERIFYERROR_bool("Missing ICMD code during typecheck");
		}

		/* reach exception handlers for this instruction */

		if (maythrow) {
			TYPECHECK_COUNT(stat_ins_maythrow);
			TYPECHECK_MARK(state->stat_maythrow);
			LOG("reaching exception handlers");
			i = 0;
			while (state->handlers[i]) {
				TYPECHECK_COUNT(stat_handlers_reached);
				if (state->handlers[i]->catchtype.any)
					VAR(state->exinvars)->typeinfo.typeclass = state->handlers[i]->catchtype;
				else
					VAR(state->exinvars)->typeinfo.typeclass.cls = class_java_lang_Throwable;
				if (!typestate_reach(state,
						state->handlers[i]->handler,
						&(state->exinvars), jd->var, 1))
					return false;
				i++;
			}
		}

		LOG("\t\tnext instruction");
		state->iptr++;
	} /* while instructions */

	LOG("instructions done");
	LOGSTR("RESULT=> ");
	DOLOG(typecheck_print_vararray(stdout, jd, state->bptr->outvars,
				state->bptr->outdepth));
	DOLOG(typevector_print(stdout, jd->var, state->numlocals));
	LOGNL; LOGFLUSH;

	/* propagate stack and variables to the following block */
	if (!superblockend) {
		LOG("reaching following block");
		tbptr = state->bptr->next;
		while (tbptr->flags == BBDELETED) {
			tbptr = tbptr->next;
#ifdef TYPECHECK_DEBUG
			/* this must be checked in parse.c */
			if ((tbptr->nr) >= state->basicblockcount)
				TYPECHECK_VERIFYERROR_bool("Control flow falls off the last block");
#endif
		}
		if (!typestate_reach(state,tbptr,state->bptr->outvars, jd->var,
					state->bptr->outdepth))
			return false;
	}

	/* We may have to restore the types of the instack slots. They
	 * have been saved if an <init> call inside the block has
	 * modified the instack types. (see INVOKESPECIAL) */

	if (state->savedinvars)
		typestate_restore_invars(state);

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
	int index;
	varinfo *locals;
	varinfo *v;
	jitdata *jd = state->jd;
	int skip = 0;

	locals = state->basicblocks[0].inlocals;

	/* allocate parameter descriptors if necessary */
	
	if (!state->m->parseddesc->params)
		if (!descriptor_params_from_paramtypes(state->m->parseddesc,state->m->flags))
			return false;

	/* pre-initialize variables as TYPE_VOID */
	
	i = state->numlocals;
	v = locals;
	while (i--) {
		v->type = TYPE_VOID;
		v++;
	}

    /* if this is an instance method initialize the "this" ref type */
	
    if (!(state->m->flags & ACC_STATIC)) {
		index = jd->local_map[5*0 + TYPE_ADR];
		if (index != UNUSED) {
			if (state->validlocals < 1)
				TYPECHECK_VERIFYERROR_bool("Not enough local variables for method arguments");
			v = locals + index;
			v->type = TYPE_ADR;
			if (state->initmethod)
				TYPEINFO_INIT_NEWOBJECT(v->typeinfo, NULL);
			else
				typeinfo_init_classinfo(&(v->typeinfo), state->m->class);
		}

		skip = 1;
    }

    LOG("'this' argument set.\n");

    /* the rest of the arguments and the return type */
	
    if (!typeinfo_init_varinfos_from_methoddesc(locals, state->m->parseddesc,
											  state->validlocals,
											  skip, /* skip 'this' pointer */
											  jd->local_map,
											  &state->returntype))
		return false;

    LOG("Arguments set.\n");
	return true;
}


/* typecheck_init_flags ********************************************************
 
   Initialize the basic block flags for the following CFG traversal.
  
   IN:
       state............the current state of the verifier

*******************************************************************************/

static void
typecheck_init_flags(verifier_state *state)
{
	s4 i;
	basicblock *block;

    /* set all BBFINISHED blocks to BBTYPECHECK_UNDEF. */
	
    i = state->basicblockcount;
    for (block = state->basicblocks; block; block = block->next) {
		
#ifdef TYPECHECK_DEBUG
		/* check for invalid flags */
        if (block->flags != BBFINISHED && block->flags != BBDELETED && block->flags != BBUNDEF)
        {
            /*show_icmd_method(state->m,state->cd,state->rd);*/
            LOGSTR1("block flags: %d\n",block->flags); LOGFLUSH;
			TYPECHECK_ASSERT(false);
        }
#endif

        if (block->flags >= BBFINISHED) {
            block->flags = BBTYPECHECK_UNDEF;
        }
    }

    /* the first block is always reached */
	
    if (state->basicblockcount && state->basicblocks[0].flags == BBTYPECHECK_UNDEF)
        state->basicblocks[0].flags = BBTYPECHECK_REACHED;
}


/* typecheck_reset_flags *******************************************************
 
   Reset the flags of basic blocks we have not reached.
  
   IN:
       state............the current state of the verifier

*******************************************************************************/

static void
typecheck_reset_flags(verifier_state *state)
{
	basicblock *block;

	/* check for invalid flags at exit */
	
#ifdef TYPECHECK_DEBUG
	for (block = state->basicblocks; block; block = block->next) {
		if (block->flags != BBDELETED
			&& block->flags != BBUNDEF
			&& block->flags != BBFINISHED
			&& block->flags != BBTYPECHECK_UNDEF) /* typecheck may never reach
													 * some exception handlers,
													 * that's ok. */
		{
			LOG2("block L%03d has invalid flags after typecheck: %d",
				 block->nr,block->flags);
			TYPECHECK_ASSERT(false);
		}
	}
#endif
	
	/* Delete blocks we never reached */
	
	for (block = state->basicblocks; block; block = block->next) {
		if (block->flags == BBTYPECHECK_UNDEF)
			block->flags = BBDELETED;
	}
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
/*     true.............successful verification                             */
/*     false............an exception has been thrown                        */
/*                                                                          */
/****************************************************************************/

#define MAXPARAMS 255

bool typecheck(jitdata *jd)
{
	methodinfo     *meth;
	codegendata    *cd;
	registerdata   *rd;
	varinfo        *savedlocals;
	verifier_state  state;             /* current state of the verifier */
	s4              i;
	s4              t;

	/* collect statistics */

#ifdef TYPECHECK_STATISTICS
	int count_iterations = 0;
	TYPECHECK_COUNT(stat_typechecked);
	TYPECHECK_COUNT_FREQ(stat_locals,cdata->maxlocals,STAT_LOCALS);
	TYPECHECK_COUNT_FREQ(stat_blocks,cdata->method->basicblockcount/10,STAT_BLOCKS);
	TYPECHECK_COUNTIF(cdata->method->exceptiontablelength != 0,stat_methods_with_handlers);
	state.stat_maythrow = false;
#endif

	/* get required compiler data */

	meth = jd->m;
	cd   = jd->cd;
	rd   = jd->rd;

	/* some logging on entry */


    LOGSTR("\n==============================================================================\n");
    DOLOG( new_show_method(jd, SHOW_STACK) );
    LOGSTR("\n==============================================================================\n");
    LOGMETHOD("Entering typecheck: ",cd->method);

	/* initialize the verifier state */

	state.m = meth;
	state.jd = jd;
	state.cd = cd;
	state.rd = rd;
	state.basicblockcount = jd->new_basicblockcount;
	state.basicblocks = jd->new_basicblocks;
	state.savedindices = NULL;
	state.savedinvars = NULL;

	/* check if this method is an instance initializer method */

    state.initmethod = (state.m->name == utf_init);

	/* initialize the basic block flags for the following CFG traversal */

	typecheck_init_flags(&state);

    /* number of local variables */
    
    /* In <init> methods we use an extra local variable to indicate whether */
    /* the 'this' reference has been initialized.                           */
	/*         TYPE_VOID...means 'this' has not been initialized,           */
	/*         TYPE_INT....means 'this' has been initialized.               */

    state.numlocals = state.jd->localcount;
	state.validlocals = state.numlocals;
    if (state.initmethod) 
		state.numlocals++; /* VERIFIER_EXTRA_LOCALS */

	state.reverselocalmap = DMNEW(s4, state.validlocals);
	for (i=0; i<jd->m->maxlocals; ++i)
		for (t=0; t<5; ++t) {
			s4 mapped = jd->local_map[5*i + t];
			if (mapped >= 0)
				state.reverselocalmap[mapped] = i;
		}

    /* allocate the buffer of active exception handlers */
	
    state.handlers = DMNEW(exceptiontable*, state.cd->exceptiontablelength + 1);

	/* save local variables */

	savedlocals = DMNEW(varinfo, state.numlocals);
	MCOPY(savedlocals, jd->var, varinfo, state.numlocals);

	/* initialized local variables of first block */

	if (!verify_init_locals(&state))
		return false;

    /* initialize invars of exception handlers */
	
	state.exinvars = state.numlocals;
	VAR(state.exinvars)->type = TYPE_ADR;
	typeinfo_init_classinfo(&(VAR(state.exinvars)->typeinfo),
							class_java_lang_Throwable); /* changed later */

    LOG("Exception handler stacks set.\n");

    /* loop while there are still blocks to be checked */
    do {
		TYPECHECK_COUNT(count_iterations);

        state.repeat = false;
        
        state.bptr = state.basicblocks;

        for (; state.bptr; state.bptr = state.bptr->next) {
            LOGSTR1("---- BLOCK %04d, ",state.bptr->nr);
            LOGSTR1("blockflags: %d\n",state.bptr->flags);
            LOGFLUSH;
            
		    /* verify reached block */	
            if (state.bptr->flags == BBTYPECHECK_REACHED) {
                if (!verify_basic_block(&state))
					return false;
            }
        } /* for blocks */

        LOGIF(state.repeat,"state.repeat == true");
    } while (state.repeat);

	/* statistics */
	
#ifdef TYPECHECK_STATISTICS
	LOG1("Typechecker did %4d iterations",count_iterations);
	TYPECHECK_COUNT_FREQ(stat_iterations,count_iterations,STAT_ITERATIONS);
	TYPECHECK_COUNTIF(state.jsrencountered,stat_typechecked_jsr);
	TYPECHECK_COUNTIF(state.stat_maythrow,stat_methods_maythrow);
#endif

	/* reset the flags of blocks we haven't reached */

	typecheck_reset_flags(&state);

	/* restore locals */

	MCOPY(jd->var, savedlocals, varinfo, state.numlocals);

	/* everything's ok */

    LOGimp("exiting typecheck");
	return true;
}
#endif /* ENABLE_VERIFIER */

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
