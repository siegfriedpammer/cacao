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

   $Id: typecheck.c 2757 2005-06-20 21:14:33Z edwin $

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
	fprintf(file,"    =>%3d: %8d\n",limit,array[limit]);
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

#define TYPECHECK_VERIFYERROR(msg) \
    do { \
        *exceptionptr = new_verifyerror(m, (msg)); \
        return NULL; \
    } while (0)


#define TYPECHECK_VERIFYERROR_TYPE_INT  TYPECHECK_VERIFYERROR("Expected to find integer on stack")
#define TYPECHECK_VERIFYERROR_TYPE_LNG  TYPECHECK_VERIFYERROR("Expected to find long on stack")
#define TYPECHECK_VERIFYERROR_TYPE_FLT  TYPECHECK_VERIFYERROR("Expected to find float on stack")
#define TYPECHECK_VERIFYERROR_TYPE_DBL  TYPECHECK_VERIFYERROR("Expected to find double on stack")
#define TYPECHECK_VERIFYERROR_TYPE_ADR  TYPECHECK_VERIFYERROR("Expected to find object on stack")


#define TYPECHECK_VERIFYERROR_TYPE(t) \
    do { \
        char *type; \
        switch ((t)) { \
        case TYPE_INT: \
			type = "integer"; \
			break; \
		case TYPE_LNG: \
			type = "long"; \
			break; \
		case TYPE_FLT: \
			type = "float"; \
			break; \
		case TYPE_DBL: \
			type = "double"; \
			break; \
		case TYPE_ADR: \
			type = "object/array"; \
			break; \
		} \
        *exceptionptr = new_verifyerror(m, \
										"Expecting to find %s on stack", \
										type); \
        return NULL; \
    } while (0)


#define TYPECHECK_STACK(sp,tp) \
    do { \
        if ((sp)->type != (tp)) \
            TYPECHECK_VERIFYERROR_TYPE((tp)); \
    } while(0)


#define TYPECHECK_INT(sp) \
    do { \
        if ((sp)->type != TYPE_INT) \
            TYPECHECK_VERIFYERROR_TYPE_INT; \
    } while(0)

#define TYPECHECK_LNG(sp) \
    do { \
        if ((sp)->type != TYPE_LNG) \
            TYPECHECK_VERIFYERROR_TYPE_LNG; \
    } while(0)

#define TYPECHECK_FLT(sp) \
    do { \
        if ((sp)->type != TYPE_FLT) \
            TYPECHECK_VERIFYERROR_TYPE_FLT; \
    } while(0)

#define TYPECHECK_DBL(sp) \
    do { \
        if ((sp)->type != TYPE_DBL) \
            TYPECHECK_VERIFYERROR_TYPE_DBL; \
    } while(0)

#define TYPECHECK_ADR(sp) \
    do { \
        if ((sp)->type != TYPE_ADR) \
            TYPECHECK_VERIFYERROR_TYPE_ADR; \
    } while(0)


#define TYPECHECK_ARGS1(t1)							            \
	do {TYPECHECK_STACK(curstack,t1);} while (0)
#define TYPECHECK_ARGS2(t1,t2)						            \
	do {TYPECHECK_ARGS1(t1);									\
		TYPECHECK_STACK(curstack->prev,t2);} while (0)
#define TYPECHECK_ARGS3(t1,t2,t3)								\
	do {TYPECHECK_ARGS2(t1,t2);									\
		TYPECHECK_STACK(curstack->prev->prev,t3);} while (0)


/****************************************************************************/
/* TYPESTACK FUNCTIONS                                                      */
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

static void
typestack_copy(stackptr dst,stackptr y,typevector *selected)
{
	typevector *sel;
	typeinfo_retaddr_set *sety;
	typeinfo_retaddr_set *new;
	typeinfo_retaddr_set **next;
	int k;
	
	for (;dst; dst=dst->prev, y=y->prev) {
		if (!y) {
			log_text("Stack depth mismatch 1");
					/* XXX verify error */
			assert(0);
		}
		if (dst->type != y->type) {
			log_text("Stack type mismatch 1");
					/* XXX verify error */
			assert(0);
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
		log_text("Stack depth mismatch 2");
					/* XXX verify error */
		assert(0);
	}
}

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

static void
typestack_collapse(stackptr dst)
{
	for (; dst; dst = dst->prev) {
		if (TYPESTACK_IS_RETURNADDRESS(dst))
			TYPESTACK_RETURNADDRESSSET(dst)->alt = NULL;
	}
}

static bool
typestack_merge(stackptr dst,stackptr y)
{
	bool changed = false;
	for (; dst; dst = dst->prev, y=y->prev) {
		if (!y) {
			log_text("Stack depth mismatch 3");
					/* XXX verify error */
			assert(0);
		}
		if (dst->type != y->type) {
			log_text("Stack type mismatch 2");
					/* XXX verify error */
			assert(0);
		}
		if (dst->type == TYPE_ADDRESS) {
			if (TYPEINFO_IS_PRIMITIVE(dst->typeinfo)) {
				/* dst has returnAddress type */
				if (!TYPEINFO_IS_PRIMITIVE(y->typeinfo)) {
					log_text("Merging returnAddress with reference");
					/* XXX verify error */
					assert(0);
				}
			}
			else {
				/* dst has reference type */
				if (TYPEINFO_IS_PRIMITIVE(y->typeinfo)) {
					log_text("Merging reference with returnAddress");
					/* XXX verify error */
					assert(0);
				}
				changed |= typeinfo_merge(&(dst->typeinfo),&(y->typeinfo));
			}
		}
	}
	if (y) {
		log_text("Stack depth mismatch 4");
					/* XXX verify error */
		assert(0);
	}
	return changed;
}

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
/****************************************************************************/

static bool
typestate_merge(stackptr deststack,typevector *destloc,
				stackptr ystack,typevector *yloc,
				int locsize,bool jsrencountered)
{
	typevector *dvec,*yvec;
	int kd,ky;
	bool changed = false;
	
	LOG("merge:");
	LOGSTR("dstack: "); DOLOG(typestack_print(get_logfile(),deststack)); LOGNL;
	LOGSTR("ystack: "); DOLOG(typestack_print(get_logfile(),ystack)); LOGNL;
	LOGSTR("dloc  : "); DOLOG(typevectorset_print(get_logfile(),destloc,locsize)); LOGNL;
	LOGSTR("yloc  : "); DOLOG(typevectorset_print(get_logfile(),yloc,locsize)); LOGNL;
	LOGFLUSH;

	/* The stack is always merged. If there are returnAddresses on
	 * the stack they are ignored in this step. */

	changed |= typestack_merge(deststack,ystack);

	if (!jsrencountered)
		return typevector_merge(destloc,yloc,locsize);

	for (yvec=yloc; yvec; yvec=yvec->alt) {
		ky = yvec->k;

		/* Check if the typestates (deststack,destloc) will be
		 * separable when (ystack,yvec) is added. */

		if (!typestack_separable_with(deststack,ystack,ky)
			&& !typevectorset_separable_with(destloc,yvec,locsize))
		{
			/* No, the resulting set won't be separable, thus we
			 * may merge all states in (deststack,destloc) and
			 * (ystack,yvec). */

			typestack_collapse(deststack);
			typevectorset_collapse(destloc,locsize);
			typevector_merge(destloc,yvec,locsize);
		}
		else {
			/* Yes, the resulting set will be separable. Thus we check
			 * if we may merge (ystack,yvec) with a single state in
			 * (deststack,destloc). */
		
			for (dvec=destloc,kd=0; dvec; dvec=dvec->alt, kd++) {
				if (!typestack_separable_from(ystack,ky,deststack,kd)
					&& !typevector_separable_from(yvec,dvec,locsize))
				{
					/* The typestate (ystack,yvec) is not separable from
					 * (deststack,dvec) by any returnAddress. Thus we may
					 * merge the states. */
					
					changed |= typevector_merge(dvec,yvec,locsize);
					
					goto merged;
				}
			}

			/* The typestate (ystack,yvec) is separable from all typestates
			 * (deststack,destloc). Thus we must add this state to the
			 * result set. */

			typestack_add(deststack,ystack,ky);
			typevectorset_add(destloc,yvec,locsize);
			changed = true;
		}
		   
	merged:
		;
	}
	
	LOG("result:");
	LOGSTR("dstack: "); DOLOG(typestack_print(get_logfile(),deststack)); LOGNL;
	LOGSTR("dloc  : "); DOLOG(typevectorset_print(get_logfile(),destloc,locsize)); LOGNL;
	LOGFLUSH;
	
	return changed;
}


static bool
typestate_reach(codegendata *cd, registerdata *rd,void *localbuf,
				basicblock *current,
				basicblock *destblock,
				stackptr ystack,typevector *yloc,
				int locsize,bool jsrencountered)
{
	typevector *destloc;
	int destidx;
	bool changed = false;

	LOG1("reaching block L%03d",destblock->debug_nr);
	TYPECHECK_COUNT(stat_reached);
	
	destidx = destblock - cd->method->basicblocks;
	destloc = MGET_TYPEVECTOR(localbuf,destidx,locsize);

	/* When branching backwards we have to check for uninitialized objects */
	
	if (destblock <= current) {
		stackptr sp;
		int i;

		/* XXX FIXME FOR INLINING */

		if (!useinlining) {
			TYPECHECK_COUNT(stat_backwards);
        		LOG("BACKWARDS!");
		        for (sp = ystack; sp; sp=sp->prev)
				if (sp->type == TYPE_ADR &&
                		TYPEINFO_IS_NEWOBJECT(sp->typeinfo)) {
					/*show_icmd_method(cd->method,cd,rd);*/
					printf("current: %d, dest: %d\n", current->debug_nr, destblock->debug_nr);
					log_text("Branching backwards with uninitialized object on stack");
					/* XXX verify error */
					assert(0);
            		}

			for (i=0; i<locsize; ++i)
				if (yloc->td[i].type == TYPE_ADR &&
					TYPEINFO_IS_NEWOBJECT(yloc->td[i].info)) {
					log_text("Branching backwards with uninitialized object in local variable");
					/* XXX verify error */
					assert(0);
				}
		}
	}
	
	if (destblock->flags == BBTYPECHECK_UNDEF) {
		/* The destblock has never been reached before */

		TYPECHECK_COUNT(stat_copied);
		LOG1("block (index %04d) reached first time",destidx);
		
		typestack_copy(destblock->instack,ystack,yloc);
		COPY_TYPEVECTORSET(yloc,destloc,locsize);
		changed = true;
	}
	else {
		/* The destblock has already been reached before */
		
		TYPECHECK_COUNT(stat_merged);
		LOG1("block (index %04d) reached before",destidx);
		
		changed = typestate_merge(destblock->instack,destloc,
								  ystack,yloc,locsize,
								  jsrencountered);
		TYPECHECK_COUNTIF(changed,stat_merging_changed);
	}

	if (changed) {
		LOG("changed!");
		destblock->flags = BBTYPECHECK_REACHED;
		if (destblock <= current) {LOG("REPEAT!"); return true;}
	}
	return false;
}


static bool
typestate_ret(codegendata *cd,registerdata *rd, void *localbuf,
			  basicblock *current,
			  stackptr ystack,typevector *yloc,
			  int retindex,int locsize)
{
	typevector *yvec;
	typevector *selected;
	basicblock *destblock;
	bool repeat = false;

	for (yvec=yloc; yvec; ) {
		if (!TYPEDESC_IS_RETURNADDRESS(yvec->td[retindex])) {
			log_text("Illegal instruction: RET on non-returnAddress");
			/* XXX verify error */
			assert(0);
		}

		destblock = (basicblock*) TYPEINFO_RETURNADDRESS(yvec->td[retindex].info);

		selected = typevectorset_select(&yvec,retindex,destblock);
		
		repeat |= typestate_reach(cd, rd,  localbuf,current,destblock,
								  ystack,selected,locsize,true);
	}
	return repeat;
}

/****************************************************************************/
/* HELPER FUNCTIONS                                                         */
/****************************************************************************/

#if XXX
/* If a field is checked, definingclass == implementingclass */
static bool
is_accessible(int flags,classinfo *definingclass,classinfo *implementingclass, classinfo *methodclass,
			  typeinfo *instance)
{
	/* check access rights */
	if (methodclass != definingclass) {
		switch (flags & (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED)) {
		  case ACC_PUBLIC:
			  break;
			  
			  /* In the cases below, definingclass cannot be an interface */
			  
		  case 0:
			  if (!SAME_PACKAGE(definingclass,methodclass))
				  return false;
			  break;
		  case ACC_PROTECTED:
			  if (!SAME_PACKAGE(definingclass,methodclass)) {
				  if (!builtin_isanysubclass(methodclass,implementingclass))
					  return false;
				  
				  /* For protected access of super class members in another
				   * package the instance must be a subclass of or the same
				   * as the current class. */
				  LOG("protected access into other package");
				  implementingclass = 0; /*dont't fail, don't really get it (was: methodclass)
					This code was never triggered before, since all packagenames had been 0-pointers
					anybody an idea ?*/
			  }
			  break;
		  case ACC_PRIVATE:
			  if (definingclass != methodclass) {
				  LOG("private access");
				  return false;
			  }
			  break;
		  default:
			  log_text("Invalid access flags");
					/* XXX verify error */
			  assert(0);
		}
	}

	if (instance) {
		if ((flags & ACC_STATIC) != 0) {
			LOG("accessing STATIC member with instance");
			return false;
		}
		
		if (implementingclass
			&& !TYPEINFO_IS_NULLTYPE(*instance)
			&& !TYPEINFO_IS_NEWOBJECT(*instance))
		{
			if (!typeinfo_is_assignable_to_class(instance,
												 CLASSREF_OR_CLASSINFO(implementingclass)))
			{
				LOG("instance not assignable");
				LOGINFO(instance);
				LOGSTRu(implementingclass->name); LOGNL; LOGFLUSH;
				return false;
			}
		}
	}
	else {
		if ((flags & ACC_STATIC) == 0) {
			LOG("accessing non-STATIC member without instance");
			return false;
		}
	}
	
	return true;
}
#endif

/****************************************************************************/
/* MACROS FOR LOCAL VARIABLE CHECKING                                       */
/****************************************************************************/

#define INDEX_ONEWORD(num)										\
	do { if((num)<0 || (num)>=validlocals)						\
			TYPECHECK_VERIFYERROR("Invalid local variable index"); } while (0)
#define INDEX_TWOWORD(num)										\
	do { if((num)<0 || ((num)+1)>=validlocals)					\
			TYPECHECK_VERIFYERROR("Invalid local variable index"); } while (0)

#define STORE_ONEWORD(num,type)									\
 	do {typevectorset_store(localset,num,type,NULL);} while(0)

#define STORE_TWOWORD(num,type)										\
 	do {typevectorset_store_twoword(localset,num,type);} while(0)


#ifdef TYPECHECK_VERBOSE
#define WORDCHECKFAULT \
  	do { \
		dolog("localset->td index: %ld\ninstruction belongs to:%s.%s, outermethod:%s.%s\n", \
		iptr->op1,iptr->method->class->name->text, \
			iptr->method->name->text,m->class->name->text,m->name->text); \
		show_icmd(iptr++, false); \
		show_icmd(iptr, false); \
	} while (0)
#else
#define WORDCHECKFAULT
#endif


#define CHECK_ONEWORD(num,tp)											\
	do {TYPECHECK_COUNT(stat_ins_primload);								\
		if (jsrencountered) {											\
			if (!typevectorset_checktype(localset,num,tp)) {				\
				WORDCHECKFAULT;	\
				TYPECHECK_VERIFYERROR("Variable type mismatch");						\
			}	\
		}																\
		else {															\
			if (localset->td[num].type != tp) {							\
				TYPECHECK_VERIFYERROR("Variable type mismatch");						\
				WORDCHECKFAULT;	\
			} \
		}																\
		} while(0)

#define CHECK_TWOWORD(num,type)											\
	do {TYPECHECK_COUNT(stat_ins_primload);								\
		if (!typevectorset_checktype(localset,num,type)) {                \
			WORDCHECKFAULT;	\
            		TYPECHECK_VERIFYERROR("Variable type mismatch");	                        \
		} \
	} while(0)


/****************************************************************************/
/* MISC MACROS                                                              */
/****************************************************************************/

#define COPYTYPE(source,dest)   \
	{if ((source)->type == TYPE_ADR)								\
			TYPEINFO_COPY((source)->typeinfo,(dest)->typeinfo);}

#define ISBUILTIN(v)   (bte->fp == (functionptr) (v))

/* TYPECHECK_REACH: executed, when the target block (tbptr) can be reached
 *     from the current block (bptr). The types of local variables and
 *     stack slots are propagated to the target block.
 * Input:
 *     bptr.......current block
 *     tbptr......target block
 *     dst........current output stack pointer
 *     numlocals..number of local variables
 *     localset...current local variable vectorset
 *     localbuf...local variable vectorset buffer
 *     jsrencountered...true if a JSR has been seen
 * Output:
 *     repeat.....changed to true if a block before the current
 *                block has changed
 */
#define TYPECHECK_REACH                                                 \
    do {                                                                \
    repeat |= typestate_reach(cd,rd, localbuf,bptr,tbptr,dst,               \
							  localset,numlocals,jsrencountered);       \
    LOG("done.");                                                       \
    } while (0)

/* TYPECHECK_LEAVE: executed when the method is exited non-abruptly
 * Input:
 *     class........class of the current method
 *     numlocals....number of local variables
 *     localset.....current local variable vectorset
 *     initmethod...true if this is an <init> method
 */
#define TYPECHECK_LEAVE                                                 \
    do {                                                                \
        if (initmethod && m->class != class_java_lang_Object) {         \
            /* check the marker variable */                             \
            LOG("Checking <init> marker");                              \
            if (!typevectorset_checktype(localset,numlocals-1,TYPE_INT))\
                TYPECHECK_VERIFYERROR("<init> method does not initialize 'this'");      \
        }                                                               \
    } while (0)

/****************************************************************************/
/* typecheck()                                                              */
/****************************************************************************/

#define MAXPARAMS 255

/* typecheck is called directly after analyse_stack */

methodinfo *typecheck(methodinfo *m, codegendata *cd, registerdata *rd)
{
    int b_count, b_index;
    stackptr curstack;      /* input stack top for current instruction */
    stackptr srcstack;         /* source stack for copying and merging */
    int opcode;                                      /* current opcode */
    int i;                                        /* temporary counter */
    int len;                        /* for counting instructions, etc. */
    bool superblockend;        /* true if no fallthrough to next block */
    bool repeat;            /* if true, blocks are iterated over again */
    instruction *iptr;               /* pointer to current instruction */
    basicblock *bptr;                /* pointer to current basic block */
    basicblock *tbptr;                   /* temporary for target block */
	
    int numlocals;                        /* number of local variables */
	int validlocals;         /* number of valid local variable indices */
	void *localbuf;       /* local variable types for each block start */
	typevector *localset;        /* typevector set for local variables */
	typevector *lset;                             /* temporary pointer */
	typedescriptor *td;                           /* temporary pointer */
#ifdef XXX
	typeinfo tempti;                             /* temporary typeinfo */
#endif

	stackptr savedstackbuf = NULL;      /* buffer for saving the stack */
	stackptr savedstack = NULL;      /* saved instack of current block */

	stackelement excstack;           /* instack for exception handlers */
													  
	typedescriptor returntype;        /* return type of current method */
    u1 *ptype;                     /* parameter types of called method */
    typeinfo *pinfo;           /* parameter typeinfos of called method */
    u1 rtype;                          /* return type of called method */
#ifdef XXX
    typeinfo rinfo;       /* typeinfo for return type of called method */
#endif
	
    stackptr dst;               /* output stack of current instruction */
    basicblock **tptr;    /* pointer into target list of switch instr. */
    exceptiontable **handlers;            /* active exception handlers */
    classinfo *cls;                                       /* temporary */
    bool maythrow;               /* true if this instruction may throw */
    static utf *name_init;                                 /* "<init>" */
    bool initmethod;             /* true if this is an "<init>" method */
	bool jsrencountered = false;         /* true if we there was a JSR */

    classinfo *myclass;
	unresolved_field *uf;                        /* for field accesses */
	fieldinfo **fieldinfop;                      /* for field accesses */
	builtintable_entry *bte;


#ifdef TYPECHECK_STATISTICS
	int count_iterations = 0;
	TYPECHECK_COUNT(stat_typechecked);
	TYPECHECK_COUNT_FREQ(stat_locals,cd->maxlocals,STAT_LOCALS);
	TYPECHECK_COUNT_FREQ(stat_blocks,m->basicblockcount/10,STAT_BLOCKS);
#endif

    LOGSTR("\n==============================================================================\n");
    /*DOLOG( show_icmd_method(cd->method,cd,rd));*/
    LOGSTR("\n==============================================================================\n");
    LOGimpSTR("Entering typecheck: ");
    LOGimpSTRu(cd->method->name);
    LOGimpSTR("    ");
    LOGimpSTRu(cd->method->descriptor);
    LOGimpSTR("    (class ");
    LOGimpSTRu(cd->method->class->name);
    LOGimpSTR(")\n");
	LOGFLUSH;

	if (!name_init)
		name_init = utf_new_char("<init>");
    initmethod = (m->name == name_init);

	/* Allocate buffer for method arguments */
	
    ptype = DMNEW(u1,MAXPARAMS);
    pinfo = DMNEW(typeinfo,MAXPARAMS);
    
    LOG("Buffer allocated.\n");

    /* reset all BBFINISHED blocks to BBTYPECHECK_UNDEF. */
    b_count = m->basicblockcount;
    bptr = m->basicblocks;
    while (--b_count >= 0) {
#ifdef TYPECHECK_DEBUG
        if (bptr->flags != BBFINISHED && bptr->flags != BBDELETED
            && bptr->flags != BBUNDEF)
        {
            /*show_icmd_method(cd->method,cd,rd);*/
            LOGSTR1("block flags: %d\n",bptr->flags); LOGFLUSH;
			TYPECHECK_ASSERT(false);
        }
#endif
        if (bptr->flags >= BBFINISHED) {
            bptr->flags = BBTYPECHECK_UNDEF;
        }
        bptr++;
    }

    /* The first block is always reached */
    if (m->basicblockcount && m->basicblocks[0].flags == BBTYPECHECK_UNDEF)
        m->basicblocks[0].flags = BBTYPECHECK_REACHED;

    LOG("Blocks reset.\n");

    /* number of local variables */
    
    /* In <init> methods we use an extra local variable to signal if
     * the 'this' reference has been initialized. */
    numlocals = cd->maxlocals;
	validlocals = numlocals;
    if (initmethod) numlocals++;

    /* allocate the buffers for local variables */
	localbuf = DMNEW_TYPEVECTOR(m->basicblockcount+1, numlocals);
	localset = MGET_TYPEVECTOR(localbuf,m->basicblockcount,numlocals);

    LOG("Variable buffer allocated.\n");

    /* allocate the buffer of active exception handlers */
    handlers = DMNEW(exceptiontable*, cd->exceptiontablelength + 1);

    /* initialize the variable types of the first block */
    /* to the types of the arguments */
	lset = MGET_TYPEVECTOR(localbuf,0,numlocals);
	lset->k = 0;
	lset->alt = NULL;
	td = lset->td;
	i = validlocals;

	/* allocate parameter descriptors if necessary */
	if (!m->parseddesc->params)
		if (!descriptor_params_from_paramtypes(m->parseddesc,m->flags))
			return NULL;

    /* if this is an instance method initialize the "this" ref type */
    if (!(m->flags & ACC_STATIC)) {
		if (!i)
			TYPECHECK_VERIFYERROR("Not enough local variables for method arguments");
        td->type = TYPE_ADDRESS;
        if (initmethod)
            TYPEINFO_INIT_NEWOBJECT(td->info,NULL);
        else
            TYPEINFO_INIT_CLASSINFO(td->info, m->class);
        td++;
		i--;
    }

    LOG("'this' argument set.\n");

    /* the rest of the arguments and the return type */
    i = typedescriptors_init_from_methoddesc(td, m->parseddesc,
											  i,
											  true, /* two word types use two slots */
											  (td - lset->td), /* skip 'this' pointer */
											  &returntype);
	td += i;

	/* variables not used for arguments are initialized to TYPE_VOID */
	i = numlocals - (td - lset->td);
	while (i--) {
		td->type = TYPE_VOID;
		td++;
	}

    LOG("Arguments set.\n");

    /* initialize the input stack of exception handlers */
	excstack.prev = NULL;
	excstack.type = TYPE_ADR;
	TYPEINFO_INIT_CLASSINFO(excstack.typeinfo,
							class_java_lang_Throwable); /* changed later */

    LOG("Exception handler stacks set.\n");

    /* loop while there are still blocks to be checked */
    do {
		TYPECHECK_COUNT(count_iterations);

        repeat = false;
        
        b_count = m->basicblockcount;
        bptr = m->basicblocks;

        while (--b_count >= 0) {
            LOGSTR1("---- BLOCK %04d, ",bptr->debug_nr);
            LOGSTR1("blockflags: %d\n",bptr->flags);
            LOGFLUSH;
                
            if (bptr->flags == BBTYPECHECK_REACHED) {
                LOGSTR1("\n---- BLOCK %04d ------------------------------------------------\n",bptr->debug_nr);
                LOGFLUSH;
                
                superblockend = false;
                bptr->flags = BBFINISHED;
                b_index = bptr - m->basicblocks;

                /* init stack at the start of this block */
                curstack = bptr->instack;

                /* determine the active exception handlers for this block */
                /* XXX could use a faster algorithm with sorted lists or
                 * something? */
                len = 0;
                for (i = 0; i < cd->exceptiontablelength; ++i) {
                    if ((cd->exceptiontable[i].start <= bptr) && (cd->exceptiontable[i].end > bptr)) {
                        LOG1("active handler L%03d", cd->exceptiontable[i].handler->debug_nr);
                        handlers[len++] = cd->exceptiontable + i;
                    }
                }
                handlers[len] = NULL;
					
                /* init variable types at the start of this block */
				COPY_TYPEVECTORSET(MGET_TYPEVECTOR(localbuf,b_index,numlocals),
								   localset,numlocals);

				/* XXX FIXME FOR INLINING */
				if(!useinlining) {
					if (handlers[0])
						for (i=0; i<numlocals; ++i)
							if (localset->td[i].type == TYPE_ADR
									&& TYPEINFO_IS_NEWOBJECT(localset->td[i].info)) {
								/*show_icmd_method(m, cd, rd);*/
								printf("Uninitialized variale: %d, block: %d\n", i, bptr->debug_nr);
								TYPECHECK_VERIFYERROR("Uninitialized object in local variable inside try block");
							}
				}
				DOLOG(typestate_print(get_logfile(),curstack,localset,numlocals));
				LOGNL; LOGFLUSH;

                /* loop over the instructions */
                len = bptr->icount;
                iptr = bptr->iinstr;
                while (--len >= 0)  {
					TYPECHECK_COUNT(stat_ins);
					
					DOLOG(typestate_print(get_logfile(),curstack,localset,numlocals));
					LOGNL; LOGFLUSH;

                    DOLOG(show_icmd(iptr,false)); LOGNL; LOGFLUSH;
                        
                    opcode = iptr->opc;
					myclass = iptr->method->class;
                    dst = iptr->dst;
                    maythrow = false;
						
                    switch (opcode) {

                        /****************************************/
                        /* STACK MANIPULATIONS                  */

                        /* We just need to copy the typeinfo */
                        /* for slots containing addresses.   */

                        /* XXX We assume that the destination stack
                         * slots were continuously allocated in
                         * memory.  (The current implementation in
                         * stack.c)
                         */

                      case ICMD_DUP:
                          COPYTYPE(curstack,dst);
                          break;
							  
                      case ICMD_DUP_X1:
                          COPYTYPE(curstack,dst);
                          COPYTYPE(curstack,dst-2);
                          COPYTYPE(curstack->prev,dst-1);
                          break;
							  
                      case ICMD_DUP_X2:
                          COPYTYPE(curstack,dst);
                          COPYTYPE(curstack,dst-3);
                          COPYTYPE(curstack->prev,dst-1);
                          COPYTYPE(curstack->prev->prev,dst-2);
                          break;
							  
                      case ICMD_DUP2:
                          COPYTYPE(curstack,dst);
                          COPYTYPE(curstack->prev,dst-1);
                          break;

                      case ICMD_DUP2_X1:
                          COPYTYPE(curstack,dst);
                          COPYTYPE(curstack->prev,dst-1);
                          COPYTYPE(curstack,dst-3);
                          COPYTYPE(curstack->prev,dst-4);
                          COPYTYPE(curstack->prev->prev,dst-2);
                          break;
							  
                      case ICMD_DUP2_X2:
                          COPYTYPE(curstack,dst);
                          COPYTYPE(curstack->prev,dst-1);
                          COPYTYPE(curstack,dst-4);
                          COPYTYPE(curstack->prev,dst-5);
                          COPYTYPE(curstack->prev->prev,dst-2);
                          COPYTYPE(curstack->prev->prev->prev,dst-3);
                          break;
							  
                      case ICMD_SWAP:
                          COPYTYPE(curstack,dst-1);
                          COPYTYPE(curstack->prev,dst);
                          break;

                          /****************************************/
                          /* PRIMITIVE VARIABLE ACCESS            */

                      case ICMD_ILOAD: CHECK_ONEWORD(iptr->op1,TYPE_INT); break;
                      case ICMD_FLOAD: CHECK_ONEWORD(iptr->op1,TYPE_FLOAT); break;
                      case ICMD_IINC:  CHECK_ONEWORD(iptr->op1,TYPE_INT); break;
                      case ICMD_LLOAD: CHECK_TWOWORD(iptr->op1,TYPE_LONG); break;
                      case ICMD_DLOAD: CHECK_TWOWORD(iptr->op1,TYPE_DOUBLE); break;
                          
                      case ICMD_FSTORE: STORE_ONEWORD(iptr->op1,TYPE_FLOAT); break;
                      case ICMD_ISTORE: STORE_ONEWORD(iptr->op1,TYPE_INT); break;
                      case ICMD_LSTORE: STORE_TWOWORD(iptr->op1,TYPE_LONG); break;
                      case ICMD_DSTORE: STORE_TWOWORD(iptr->op1,TYPE_DOUBLE); break;
                          
                          /****************************************/
                          /* LOADING ADDRESS FROM VARIABLE        */

                      case ICMD_ALOAD:
						  TYPECHECK_COUNT(stat_ins_aload);
                          
                          /* loading a returnAddress is not allowed */
						  if (jsrencountered) {
							  if (!typevectorset_checkreference(localset,iptr->op1)) {
								  TYPECHECK_VERIFYERROR("illegal instruction: ALOAD loading non-reference");
							  }
							  typevectorset_copymergedtype(localset,iptr->op1,&(dst->typeinfo));
						  }
						  else {
							  if (!TYPEDESC_IS_REFERENCE(localset->td[iptr->op1])) {
								  TYPECHECK_VERIFYERROR("illegal instruction: ALOAD loading non-reference");
							  }
							  TYPEINFO_COPY(localset->td[iptr->op1].info,dst->typeinfo);
						  }
                          break;
							
                          /****************************************/
                          /* STORING ADDRESS TO VARIABLE          */

                      case ICMD_ASTORE:
                          if (handlers[0] && TYPEINFO_IS_NEWOBJECT(curstack->typeinfo)) {
                              TYPECHECK_VERIFYERROR("Storing uninitialized object in local variable inside try block");
						  }

						  if (TYPESTACK_IS_RETURNADDRESS(curstack)) {
							  typevectorset_store_retaddr(localset,iptr->op1,&(curstack->typeinfo));
						  }
						  else {
							  typevectorset_store(localset,iptr->op1,TYPE_ADDRESS,
												  &(curstack->typeinfo));
						  }
                          break;
                          
                          /****************************************/
                          /* LOADING ADDRESS FROM ARRAY           */

                      case ICMD_AALOAD:
                          if (!TYPEINFO_MAYBE_ARRAY_OF_REFS(curstack->prev->typeinfo))
                              TYPECHECK_VERIFYERROR("illegal instruction: AALOAD on non-reference array");

                          typeinfo_init_component(&curstack->prev->typeinfo,&dst->typeinfo);
                          maythrow = true;
                          break;
							  
                          /****************************************/
                          /* FIELD ACCESS                         */

                      case ICMD_PUTFIELDCONST:
					  case ICMD_PUTSTATICCONST:
						  TYPECHECK_COUNT(stat_ins_field);
						  
						  uf = INSTRUCTION_PUTCONST_FIELDREF(iptr);
						  fieldinfop = INSTRUCTION_PUTCONST_FIELDINFO_PTR(iptr);
						  
						  goto putfield_tail;
						  
                      case ICMD_PUTFIELD:
                      case ICMD_PUTSTATIC:
						  TYPECHECK_COUNT(stat_ins_field);

						  uf = (unresolved_field *) iptr[0].target;
						  fieldinfop = (fieldinfo **) &(iptr[0].val.a);
putfield_tail:
						  /* record the subtype constraints for this field access */
						  if (!constrain_unresolved_field(uf,m->class,m,iptr,curstack))
							  return NULL; /* XXX maybe wrap exception? */
							  
						  /* try to resolve the field reference */
						  if (!resolve_field(uf,resolveLazy,fieldinfop))
							  return NULL;
                          maythrow = true;
                          break;

                      case ICMD_GETFIELD:
                      case ICMD_GETSTATIC:
						  TYPECHECK_COUNT(stat_ins_field);

						  uf = (unresolved_field *) iptr[0].target;

						  /* record the subtype constraints for this field access */
						  if (!constrain_unresolved_field(uf,m->class,m,iptr,curstack))
							  return NULL; /* XXX maybe wrap exception? */

						  /* try to resolve the field reference */
						  if (!resolve_field(uf,resolveLazy,(fieldinfo **) &(iptr[0].val.a)))
							  return NULL;

						  /* the result is pushed on the stack */
						  if (dst->type == TYPE_ADR) {
							  typeinfo_init_from_typedesc(uf->fieldref->parseddesc.fd,NULL,&(dst->typeinfo));
						  }
                          maythrow = true;
                          break;

                          /****************************************/
                          /* PRIMITIVE ARRAY ACCESS               */

                      case ICMD_ARRAYLENGTH:
                          if (!TYPEINFO_MAYBE_ARRAY(curstack->typeinfo)
							  && curstack->typeinfo.typeclass.cls != pseudo_class_Arraystub)
                              TYPECHECK_VERIFYERROR("illegal instruction: ARRAYLENGTH on non-array");
                          maythrow = true;
                          break;

                      case ICMD_BALOAD:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->typeinfo,ARRAYTYPE_BOOLEAN)
                              && !TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->typeinfo,ARRAYTYPE_BYTE))
                              TYPECHECK_VERIFYERROR("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_CALOAD:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->typeinfo,ARRAYTYPE_CHAR))
                              TYPECHECK_VERIFYERROR("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_DALOAD:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->typeinfo,ARRAYTYPE_DOUBLE))
                              TYPECHECK_VERIFYERROR("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_FALOAD:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->typeinfo,ARRAYTYPE_FLOAT))
                              TYPECHECK_VERIFYERROR("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_IALOAD:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->typeinfo,ARRAYTYPE_INT))
                              TYPECHECK_VERIFYERROR("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_SALOAD:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->typeinfo,ARRAYTYPE_SHORT))
                              TYPECHECK_VERIFYERROR("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_LALOAD:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->typeinfo,ARRAYTYPE_LONG))
                              TYPECHECK_VERIFYERROR("Array type mismatch");
                          maythrow = true;
                          break;

                      case ICMD_BASTORE:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->prev->typeinfo,ARRAYTYPE_BOOLEAN)
                              && !TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->prev->typeinfo,ARRAYTYPE_BYTE))
                              TYPECHECK_VERIFYERROR("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_CASTORE:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->prev->typeinfo,ARRAYTYPE_CHAR))
                              TYPECHECK_VERIFYERROR("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_DASTORE:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->prev->typeinfo,ARRAYTYPE_DOUBLE))
                              TYPECHECK_VERIFYERROR("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_FASTORE:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->prev->typeinfo,ARRAYTYPE_FLOAT))
                              TYPECHECK_VERIFYERROR("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_IASTORE:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->prev->typeinfo,ARRAYTYPE_INT))
                              TYPECHECK_VERIFYERROR("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_SASTORE:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->prev->typeinfo,ARRAYTYPE_SHORT))
                              TYPECHECK_VERIFYERROR("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_LASTORE:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->prev->typeinfo,ARRAYTYPE_LONG))
                              TYPECHECK_VERIFYERROR("Array type mismatch");
                          maythrow = true;
                          break;

                      case ICMD_IASTORECONST:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->typeinfo, ARRAYTYPE_INT))
                              TYPECHECK_VERIFYERROR("Array type mismatch");
                          maythrow = true;
                          break;

                      case ICMD_LASTORECONST:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->typeinfo, ARRAYTYPE_LONG))
                              TYPECHECK_VERIFYERROR("Array type mismatch");
                          maythrow = true;
                          break;

                      case ICMD_BASTORECONST:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->typeinfo, ARRAYTYPE_BOOLEAN)
                              && !TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->typeinfo, ARRAYTYPE_BYTE))
                              TYPECHECK_VERIFYERROR("Array type mismatch");
                          maythrow = true;
                          break;

                      case ICMD_CASTORECONST:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->typeinfo, ARRAYTYPE_CHAR))
                              TYPECHECK_VERIFYERROR("Array type mismatch");
                          maythrow = true;
                          break;

                      case ICMD_SASTORECONST:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->typeinfo, ARRAYTYPE_SHORT))
                              TYPECHECK_VERIFYERROR("Array type mismatch");
                          maythrow = true;
                          break;


                          /****************************************/
                          /* ADDRESS CONSTANTS                    */

                      case ICMD_ACONST:
                          if (iptr->val.a == NULL)
                              TYPEINFO_INIT_NULLTYPE(dst->typeinfo);
                          else
                              /* string constant (or constant for builtin function) */
                              TYPEINFO_INIT_CLASSINFO(dst->typeinfo,class_java_lang_String);
                          break;

                          /****************************************/
                          /* CHECKCAST AND INSTANCEOF             */

                      case ICMD_CHECKCAST:
						  TYPECHECK_ADR(curstack);
                          /* returnAddress is not allowed */
                          if (!TYPEINFO_IS_REFERENCE(curstack->typeinfo))
                              TYPECHECK_VERIFYERROR("Illegal instruction: CHECKCAST on non-reference");

						  cls = (classinfo *) iptr[0].val.a;
						  if (cls)
							  TYPEINFO_INIT_CLASSINFO(dst->typeinfo,cls);
						  else
                              TYPEINFO_INIT_CLASSREF(dst->typeinfo,iptr[0].target);
                          maythrow = true;
                          break;

                      case ICMD_INSTANCEOF:
						  TYPECHECK_ADR(curstack);
                          /* returnAddress is not allowed */
                          if (!TYPEINFO_IS_REFERENCE(curstack->typeinfo))
                              TYPECHECK_VERIFYERROR("Illegal instruction: INSTANCEOF on non-reference");
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
                          tbptr = (basicblock *) iptr->target;

                          /* propagate stack and variables to the target block */
                          TYPECHECK_REACH;
                          break;

                          /****************************************/
                          /* SWITCHES                             */
                          
                      case ICMD_TABLESWITCH:
						  TYPECHECK_COUNT(stat_ins_switch);
                          {
                              s4 *s4ptr = iptr->val.a;
                              s4ptr++;              /* skip default */
                              i = *s4ptr++;         /* low */
                              i = *s4ptr++ - i + 2; /* +1 for default target */
                          }
                          goto switch_instruction_tail;
                          
                      case ICMD_LOOKUPSWITCH:
						  TYPECHECK_COUNT(stat_ins_switch);
                          {
                              s4 *s4ptr = iptr->val.a;
                              s4ptr++;              /* skip default */
                              i = *s4ptr++ + 1;     /* count +1 for default */
                          }
                    switch_instruction_tail:
                          tptr = (basicblock **)iptr->target;
                          
                          while (--i >= 0) {
                              tbptr = *tptr++;
                              LOG2("target %d is block %04d",(tptr-(basicblock **)iptr->target)-1,tbptr->debug_nr);
                              TYPECHECK_REACH;
                          }
                          LOG("switch done");
                          superblockend = true;
                          break;

                          /****************************************/
                          /* ADDRESS RETURNS AND THROW            */

                      case ICMD_ATHROW:
						  /* XXX lazy loading */
                          if (!typeinfo_is_assignable_to_class(
                                   &curstack->typeinfo,CLASSREF_OR_CLASSINFO(class_java_lang_Throwable)))
                              TYPECHECK_VERIFYERROR("illegal instruction: ATHROW on non-Throwable");
                          superblockend = true;
                          maythrow = true;
                          break;

                      case ICMD_ARETURN:
						  /* XXX lazy loading */
                          if (!TYPEINFO_IS_REFERENCE(curstack->typeinfo))
                              TYPECHECK_VERIFYERROR("illegal instruction: ARETURN on non-reference");

                          if (returntype.type != TYPE_ADDRESS
                              || !typeinfo_is_assignable(&curstack->typeinfo,&(returntype.info)))
                              TYPECHECK_VERIFYERROR("Return type mismatch");
						  goto return_tail;
						  
                          /****************************************/
                          /* PRIMITIVE RETURNS                    */

                      case ICMD_IRETURN:
                          if (returntype.type != TYPE_INT) TYPECHECK_VERIFYERROR("Return type mismatch");
						  goto return_tail;
						  
                      case ICMD_LRETURN:
                          if (returntype.type != TYPE_LONG) TYPECHECK_VERIFYERROR("Return type mismatch");
						  goto return_tail;
						  
                      case ICMD_FRETURN:
                          if (returntype.type != TYPE_FLOAT) TYPECHECK_VERIFYERROR("Return type mismatch");
						  goto return_tail;
						  
                      case ICMD_DRETURN:
                          if (returntype.type != TYPE_DOUBLE) TYPECHECK_VERIFYERROR("Return type mismatch");
						  goto return_tail;
						  
                      case ICMD_RETURN:
                          if (returntype.type != TYPE_VOID) TYPECHECK_VERIFYERROR("Return type mismatch");
					  return_tail:
                          TYPECHECK_LEAVE;
                          superblockend = true;
                          maythrow = true;
                          break;
                                                    
                          /****************************************/
                          /* SUBROUTINE INSTRUCTIONS              */

                      case ICMD_JSR:
                          LOG("jsr");
						  jsrencountered = true;

                          /* This is a dirty hack. It is needed
                           * because of the special handling of
                           * ICMD_JSR in stack.c
                           */
                          dst = (stackptr) iptr->val.a;
                          
                          tbptr = (basicblock *) iptr->target;
						  if (bptr + 1 == (m->basicblocks + m->basicblockcount + 1))
							  TYPECHECK_VERIFYERROR("Illegal instruction: JSR at end of bytecode");
						  typestack_put_retaddr(dst,bptr+1,localset);
						  repeat |= typestate_reach(cd, rd,localbuf,bptr,tbptr,dst,
													localset,numlocals,true);

						  superblockend = true;
                          break;
                          
                      case ICMD_RET:
                          /* check returnAddress variable */
						  if (!typevectorset_checkretaddr(localset,iptr->op1))
                              TYPECHECK_VERIFYERROR("illegal instruction: RET using non-returnAddress variable");

						  repeat |= typestate_ret(cd,rd, localbuf,bptr,curstack,
												  localset,iptr->op1,numlocals);

                          superblockend = true;
                          break;
							  
                          /****************************************/
                          /* INVOKATIONS                          */

                      case ICMD_INVOKEVIRTUAL:
                      case ICMD_INVOKESPECIAL:
                      case ICMD_INVOKESTATIC:
                      case ICMD_INVOKEINTERFACE:
						  TYPECHECK_COUNT(stat_ins_invoke);
                          {
							  unresolved_method *um = (unresolved_method *) iptr[0].target;
							  constant_FMIref *mref = um->methodref;
							  methoddesc *md = mref->parseddesc.md;
							  bool specialmethod = (mref->name->text[0] == '<');
                              bool callinginit = (opcode == ICMD_INVOKESPECIAL && mref->name == name_init);
                              instruction *ins;
                              classref_or_classinfo initclass;
							  typedesc *td;

							  if (specialmethod && !callinginit)
								  TYPECHECK_VERIFYERROR("Invalid invocation of special method");
							  
							  if (!constrain_unresolved_method(um,m->class,m,iptr,curstack))
								  return NULL; /* XXX maybe wrap exception */

							  if (!resolve_method(um,resolveLazy,(methodinfo **) &(iptr[0].val.a)))
								  return NULL;

#if 0
							  if (opcode == ICMD_INVOKESPECIAL) {
								  /* XXX for INVOKESPECIAL: check if the invokation is done at all */
								  
								  /* (If callinginit the class is checked later.) */
								  if (!callinginit) { 
									  /* XXX classrefs */
 									  if (!builtin_isanysubclass(myclass,mi->class)) 
 										  TYPECHECK_VERIFYERROR("Illegal instruction: INVOKESPECIAL calling non-superclass method"); 
 								  } 
							  }
#endif

							  /* allocate parameters if necessary */
							  if (!md->params)
								  if (!descriptor_params_from_paramtypes(md,
											  (opcode == ICMD_INVOKESTATIC) ? ACC_STATIC : ACC_NONE))
									  return NULL;

                              /* check parameter types */
                              srcstack = curstack;
                              i = md->paramcount; /* number of parameters including 'this'*/
                              while (--i >= 0) {
                                  LOG1("param %d",i);
								  td = md->paramtypes + i;
                                  if (srcstack->type != td->type)
                                      TYPECHECK_VERIFYERROR("Parameter type mismatch in method invocation");
                                  if (srcstack->type == TYPE_ADR) {
                                      LOGINFO(&(srcstack->typeinfo));
                                      if (i==0 && callinginit)
                                      {
                                          /* first argument to <init> method */
                                          if (!TYPEINFO_IS_NEWOBJECT(srcstack->typeinfo))
                                              TYPECHECK_VERIFYERROR("Calling <init> on initialized object");
                                          
                                          /* get the address of the NEW instruction */
                                          LOGINFO(&(srcstack->typeinfo));
                                          ins = (instruction*)TYPEINFO_NEWOBJECT_INSTRUCTION(srcstack->typeinfo);
										  if (ins)
											  initclass = CLASSREF_OR_CLASSINFO(ins[-1].val.a);
										  else
                                              initclass.cls = m->class;
                                          LOGSTR("class: "); LOGNAME(initclass); LOGNL;
                                      }
                                  }
                                  LOG("ok");

                                  if (i)
									  srcstack = srcstack->prev;
                              }

							  LOG("checking return type");
							  rtype = md->returntype.type;
                              if (rtype != TYPE_VOID) {
                                  if (rtype != dst->type)
                                      TYPECHECK_VERIFYERROR("Return type mismatch in method invocation");
								  typeinfo_init_from_typedesc(&(md->returntype),NULL,&(dst->typeinfo));
                              }

                              if (callinginit) {
								  LOG("replacing uninitialized object");
                                  /* replace uninitialized object type on stack */
                                  srcstack = dst;
                                  while (srcstack) {
                                      if (srcstack->type == TYPE_ADR
                                          && TYPEINFO_IS_NEWOBJECT(srcstack->typeinfo)
                                          && TYPEINFO_NEWOBJECT_INSTRUCTION(srcstack->typeinfo) == ins)
                                      {
                                          LOG("replacing uninitialized type on stack");

										  /* If this stackslot is in the instack of
										   * this basic block we must save the type(s)
										   * we are going to replace.
										   */
										  if (srcstack <= bptr->instack && !savedstack)
										  {
											  stackptr sp;
											  stackptr copy;
											  LOG("saving input stack types");
											  if (!savedstackbuf) {
												  LOG("allocating savedstack buffer");
												  savedstackbuf = DMNEW(stackelement, cd->maxstack);
												  savedstackbuf->prev = NULL;
												  for (i = 1; i < cd->maxstack; ++i)
													  savedstackbuf[i].prev = savedstackbuf+(i-1);
											  }
											  sp = savedstack = bptr->instack;
											  copy = bptr->instack = savedstackbuf + (bptr->indepth-1);
											  TYPESTACK_COPY(sp,copy);
										  }
										  
                                          TYPEINFO_INIT_CLASSREF_OR_CLASSINFO(srcstack->typeinfo,initclass);
                                      }
                                      srcstack = srcstack->prev;
                                  }
                                  /* replace uninitialized object type in locals */
								  typevectorset_init_object(localset,ins,initclass,numlocals);

                                  /* initializing the 'this' reference? */
                                  if (!ins) {
									  TYPECHECK_ASSERT(initmethod);
									  /* must be <init> of current class or direct superclass */
									  /* XXX check with classrefs */
#if 0
									  if (mi->class != m->class && mi->class != m->class->super.cls)
										  TYPECHECK_VERIFYERROR("<init> calling <init> of the wrong class");
#endif
									  
                                      /* set our marker variable to type int */
                                      LOG("setting <init> marker");
									  typevectorset_store(localset,numlocals-1,TYPE_INT,NULL);
                                  }
								  else {
									  /* initializing an instance created with NEW */
									  /* XXX is this strictness ok? */
									  /* XXX check with classrefs */
#if 0
									  if (mi->class != initclass.cls)
										  TYPECHECK_VERIFYERROR("Calling <init> method of the wrong class");
#endif
								  }
                              }
                          }
                          maythrow = true;
                          break;
                          
                      case ICMD_MULTIANEWARRAY:
						  {
							  vftbl_t *arrayvftbl;
							  arraydescriptor *desc;
							  
							  /* check the array lengths on the stack */
							  i = iptr[0].op1;
							  if (i < 1)
								  TYPECHECK_VERIFYERROR("Illegal dimension argument");
							  srcstack = curstack;
							  while (i--) {
								  if (!srcstack)
									  TYPECHECK_VERIFYERROR("Unable to pop operand off an empty stack");
								  TYPECHECK_INT(srcstack);
								  srcstack = srcstack->prev;
							  }
							  
							  /* check array descriptor */
							  if (iptr[0].target == NULL) {
								  arrayvftbl = (vftbl_t*) iptr[0].val.a;
								  if (!arrayvftbl)
									  TYPECHECK_VERIFYERROR("MULTIANEWARRAY with unlinked class");
								  if ((desc = arrayvftbl->arraydesc) == NULL)
									  TYPECHECK_VERIFYERROR("MULTIANEWARRAY with non-array class");
								  if (desc->dimension < iptr[0].op1)
									  TYPECHECK_VERIFYERROR("MULTIANEWARRAY dimension to high");

								  /* set the array type of the result */
								  TYPEINFO_INIT_CLASSINFO(dst->typeinfo,arrayvftbl->class);
							  }
							  else {
								  /* XXX do checks in patcher */
								  TYPEINFO_INIT_CLASSREF(dst->typeinfo,iptr[0].val.a);
							  }
						  }
                          maythrow = true;
                          break;

					  case ICMD_BUILTIN:
						  TYPECHECK_COUNT(stat_ins_builtin);
						  bte = (builtintable_entry *) iptr[0].val.a;
                          if (ISBUILTIN(BUILTIN_new) || ISBUILTIN(PATCHER_builtin_new)) {
							  
                              if (iptr[-1].opc != ICMD_ACONST)
                                  TYPECHECK_VERIFYERROR("illegal instruction: builtin_new without classinfo");
							  cls = (classinfo *) iptr[-1].val.a;
#ifdef XXX
							  TYPECHECK_ASSERT(!cls || cls->linked);
							  /* The following check also forbids array classes and interfaces: */
							  if ((cls->flags & ACC_ABSTRACT) != 0)
								  TYPECHECK_VERIFYERROR("Invalid instruction: NEW creating instance of abstract class");
#endif
                              TYPEINFO_INIT_NEWOBJECT(dst->typeinfo,iptr);
                          }
                          else if (ISBUILTIN(BUILTIN_newarray_boolean)) {
							  TYPECHECK_INT(curstack);
                              TYPEINFO_INIT_PRIMITIVE_ARRAY(dst->typeinfo,ARRAYTYPE_BOOLEAN);
                          }
                          else if (ISBUILTIN(BUILTIN_newarray_char)) {
							  TYPECHECK_INT(curstack);
                              TYPEINFO_INIT_PRIMITIVE_ARRAY(dst->typeinfo,ARRAYTYPE_CHAR);
                          }
                          else if (ISBUILTIN(BUILTIN_newarray_float)) {
							  TYPECHECK_INT(curstack);
                              TYPEINFO_INIT_PRIMITIVE_ARRAY(dst->typeinfo,ARRAYTYPE_FLOAT);
                          }
                          else if (ISBUILTIN(BUILTIN_newarray_double)) {
							  TYPECHECK_INT(curstack);
                              TYPEINFO_INIT_PRIMITIVE_ARRAY(dst->typeinfo,ARRAYTYPE_DOUBLE);
                          }
                          else if (ISBUILTIN(BUILTIN_newarray_byte)) {
							  TYPECHECK_INT(curstack);
                              TYPEINFO_INIT_PRIMITIVE_ARRAY(dst->typeinfo,ARRAYTYPE_BYTE);
                          }
                          else if (ISBUILTIN(BUILTIN_newarray_short)) {
							  TYPECHECK_INT(curstack);
                              TYPEINFO_INIT_PRIMITIVE_ARRAY(dst->typeinfo,ARRAYTYPE_SHORT);
                          }
                          else if (ISBUILTIN(BUILTIN_newarray_int)) {
							  TYPECHECK_INT(curstack);
                              TYPEINFO_INIT_PRIMITIVE_ARRAY(dst->typeinfo,ARRAYTYPE_INT);
                          }
                          else if (ISBUILTIN(BUILTIN_newarray_long)) {
							  TYPECHECK_INT(curstack);
                              TYPEINFO_INIT_PRIMITIVE_ARRAY(dst->typeinfo,ARRAYTYPE_LONG);
                          }
						  else if (ISBUILTIN(BUILTIN_newarray))
                          {
							  vftbl_t *vft;
							  TYPECHECK_INT(curstack->prev);
                              if (iptr[-1].opc != ICMD_ACONST)
                                  TYPECHECK_VERIFYERROR("illegal instruction: builtin_newarray without classinfo");
							  vft = (vftbl_t *)iptr[-1].val.a;
							  if (!vft)
								  TYPECHECK_VERIFYERROR("ANEWARRAY with unlinked class");
							  if (!vft->arraydesc)
								  TYPECHECK_VERIFYERROR("ANEWARRAY with non-array class");
                              TYPEINFO_INIT_CLASSINFO(dst->typeinfo,vft->class);
                          }
						  else if (ISBUILTIN(PATCHER_builtin_newarray))
                          {
							  TYPECHECK_INT(curstack->prev);
                              if (iptr[-1].opc != ICMD_ACONST)
                                  TYPECHECK_VERIFYERROR("illegal instruction: builtin_newarray without classinfo");
                              TYPEINFO_INIT_CLASSREF(dst->typeinfo,iptr[-1].val.a);
                          }
						  else if (ISBUILTIN(BUILTIN_newarray))
                          {
							  vftbl_t *vft;
							  TYPECHECK_INT(curstack->prev);
                              if (iptr[-1].opc != ICMD_ACONST)
                                  TYPECHECK_VERIFYERROR("illegal instruction: builtin_newarray without classinfo");
							  vft = (vftbl_t *)iptr[-1].val.a;
							  if (!vft)
								  TYPECHECK_VERIFYERROR("ANEWARRAY with unlinked class");
							  if (!vft->arraydesc)
								  TYPECHECK_VERIFYERROR("ANEWARRAY with non-array class");
                              TYPEINFO_INIT_CLASSINFO(dst->typeinfo,vft->class);
                          }
                          else if (ISBUILTIN(BUILTIN_arrayinstanceof))
                          {
							  vftbl_t *vft;
							  TYPECHECK_ADR(curstack->prev);
                              if (iptr[-1].opc != ICMD_ACONST)
                                  TYPECHECK_VERIFYERROR("illegal instruction: builtin_arrayinstanceof without classinfo");
							  vft = (vftbl_t *)iptr[-1].val.a;
							  if (!vft)
								  TYPECHECK_VERIFYERROR("INSTANCEOF with unlinked class");
							  if (!vft->arraydesc)
								  TYPECHECK_VERIFYERROR("internal error: builtin_arrayinstanceof with non-array class");
						  }
                          else if (ISBUILTIN(BUILTIN_arraycheckcast)) {
							  vftbl_t *vft;
							  TYPECHECK_ADR(curstack->prev);
                              if (iptr[-1].opc != ICMD_ACONST)
                                  TYPECHECK_VERIFYERROR("illegal instruction: BUILTIN_arraycheckcast without classinfo");
							  vft = (vftbl_t *)iptr[-1].val.a;
							  if (!vft)
								  TYPECHECK_VERIFYERROR("CHECKCAST with unlinked class");
							  if (!vft->arraydesc)
								  TYPECHECK_VERIFYERROR("internal error: builtin_arraycheckcast with non-array class");
                              TYPEINFO_INIT_CLASSINFO(dst->typeinfo,vft->class);
                          }
                          else if (ISBUILTIN(PATCHER_builtin_arraycheckcast)) {
							  TYPECHECK_ADR(curstack->prev);
                              if (iptr[-1].opc != ICMD_ACONST)
                                  TYPECHECK_VERIFYERROR("illegal instruction: BUILTIN_arraycheckcast without classinfo");
                              TYPEINFO_INIT_CLASSREF(dst->typeinfo,iptr[-1].val.a);
                          }
						  else if (ISBUILTIN(BUILTIN_aastore)) {
							  TYPECHECK_ADR(curstack);
							  TYPECHECK_INT(curstack->prev);
							  TYPECHECK_ADR(curstack->prev->prev);
                              if (!TYPEINFO_MAYBE_ARRAY_OF_REFS(curstack->prev->prev->typeinfo))
                                  TYPECHECK_VERIFYERROR("illegal instruction: AASTORE to non-reference array");
                          }
						  else {
#if 0
							  /* XXX put these checks in a function */
							  TYPECHECK_COUNT(stat_ins_builtin_gen);
							  builtindesc = builtin_desc;
							  while (builtindesc->opcode && builtindesc->builtin
									 != iptr->val.fp) builtindesc++;
							  if (!builtindesc->opcode) {
								  dolog("Builtin not in table: %s",icmd_builtin_name(iptr->val.fp));
								  TYPECHECK_ASSERT(false);
							  }
							  TYPECHECK_ARGS3(builtindesc->type_s3,builtindesc->type_s2,builtindesc->type_s1);
#endif
						  }
                          maythrow = true;
                          break;
                          
#if 0
                      case ICMD_BUILTIN2:
						  TYPECHECK_COUNT(stat_ins_builtin);
						  else {
							  TYPECHECK_COUNT(stat_ins_builtin_gen);
							  builtindesc = builtin_desc;
							  while (builtindesc->opcode && builtindesc->builtin
									 != iptr->val.fp) builtindesc++;
							  if (!builtindesc->opcode) {
								  dolog("Builtin not in table: %s",icmd_builtin_name(iptr->val.fp));
								  TYPECHECK_ASSERT(false);
							  }
							  TYPECHECK_ARGS2(builtindesc->type_s2,builtindesc->type_s1);
						  }
                          maythrow = true;
                          break;
                          
                      case ICMD_BUILTIN1:
						  TYPECHECK_COUNT(stat_ins_builtin);
						  else {
							  TYPECHECK_COUNT(stat_ins_builtin_gen);
							  builtindesc = builtin_desc;
							  while (builtindesc->opcode && builtindesc->builtin
									 != iptr->val.fp) builtindesc++;
							  if (!builtindesc->opcode) {
								  dolog("Builtin not in table: %s",icmd_builtin_name(iptr->val.fp));
								  TYPECHECK_ASSERT(false);
							  }
							  TYPECHECK_ARGS1(builtindesc->type_s1);
						  }
                          maythrow = true;
                          break;
#endif

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
                      case ICMD_AASTORE:
                          LOG2("ICMD %d at %d\n", iptr->opc, (int)(iptr-bptr->iinstr));
						  LOG("Should have been converted to builtin function call.");
						  TYPECHECK_ASSERT(false);
                          break;
                                                     
                      case ICMD_READONLY_ARG:
                      case ICMD_CLEAR_ARGREN:
                          LOG2("ICMD %d at %d\n", iptr->opc, (int)(iptr-bptr->iinstr));
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
                          LOG2("ICMD %d at %d\n", iptr->opc, (int)(iptr-bptr->iinstr));
                          TYPECHECK_VERIFYERROR("Missing ICMD code during typecheck");
#endif
					}

                    /* the output of this instruction becomes the current stack */
                    curstack = dst;
                    
                    /* reach exception handlers for this instruction */
                    if (maythrow) {
                        LOG("reaching exception handlers");
                        i = 0;
                        while (handlers[i]) {
							TYPECHECK_COUNT(stat_handlers_reached);
							if (handlers[i]->catchtype.any)
								excstack.typeinfo.typeclass = handlers[i]->catchtype;
							else
								excstack.typeinfo.typeclass.cls = class_java_lang_Throwable;
							repeat |= typestate_reach(cd,rd, localbuf,bptr,
													  handlers[i]->handler,
													  &excstack,localset,
													  numlocals,
													  jsrencountered);
                            i++;
                        }
                    }

					LOG("next instruction");
                    iptr++;
                } /* while instructions */

                LOG("instructions done");
                LOGSTR("RESULT=> ");
                DOLOG(typestate_print(get_logfile(),curstack,localset,numlocals));
                LOGNL; LOGFLUSH;
                
                /* propagate stack and variables to the following block */
                if (!superblockend) {
                    LOG("reaching following block");
                    tbptr = bptr + 1;
                    while (tbptr->flags == BBDELETED) {
                        tbptr++;
#ifdef TYPECHECK_DEBUG
                        if ((tbptr->debug_nr) >= m->basicblockcount)
                            TYPECHECK_VERIFYERROR("Control flow falls off the last block");
#endif
                    }
                    TYPECHECK_REACH;
                }

				/* We may have to restore the types of the instack slots. They
				 * have been saved if an <init> call inside the block has
				 * modified the instack types. (see INVOKESPECIAL) */
				
				if (savedstack) {
					stackptr sp = bptr->instack;
					stackptr copy = savedstack;
					TYPECHECK_COUNT(stat_savedstack);
					LOG("restoring saved instack");
					TYPESTACK_COPY(sp,copy);
					bptr->instack = savedstack;
					savedstack = NULL;
				}
                
            } /* if block has to be checked */
            bptr++;
        } /* while blocks */

        LOGIF(repeat,"repeat=true");
    } while (repeat);

#ifdef TYPECHECK_STATISTICS
	LOG1("Typechecker did %4d iterations",count_iterations);
	TYPECHECK_COUNT_FREQ(stat_iterations,count_iterations,STAT_ITERATIONS);
	TYPECHECK_COUNTIF(jsrencountered,stat_typechecked_jsr);
#endif

#ifdef TYPECHECK_DEBUG
	for (i=0; i<m->basicblockcount; ++i) {
		if (m->basicblocks[i].flags != BBDELETED
			&& m->basicblocks[i].flags != BBUNDEF
			&& m->basicblocks[i].flags != BBFINISHED
			&& m->basicblocks[i].flags != BBTYPECHECK_UNDEF) /* typecheck may never reach
													 * some exception handlers,
													 * that's ok. */
		{
			LOG2("block L%03d has invalid flags after typecheck: %d",
				 m->basicblocks[i].debug_nr,m->basicblocks[i].flags);
			TYPECHECK_ASSERT(false);
		}
	}
#endif
	
	/* Reset blocks we never reached */
	for (i=0; i<m->basicblockcount; ++i) {
		if (m->basicblocks[i].flags == BBTYPECHECK_UNDEF)
			m->basicblocks[i].flags = BBFINISHED;
	}
		
    LOGimp("exiting typecheck");

	/* just return methodinfo* to signal everything was ok */

	return m;
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
