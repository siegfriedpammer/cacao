/* jit/typecheck.c - typechecking (part of bytecode verification)

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

   Authors: Edwin Steiner

   $Id: typecheck.c 868 2004-01-10 20:12:10Z edwin $

*/

#include "global.h" /* must be here because of CACAO_TYPECHECK */

#ifdef CACAO_TYPECHECK

#include <string.h>
#include "jit.h"
#include "builtin.h"
#include "tables.h"
#include "loader.h"
#include "types.h"
#include "toolbox/loging.h"
#include "toolbox/memory.h"

#define TOUCHED_YES    0x01
#define TOUCHED_NO     0x02
#define TOUCHED_MAYBE  (TOUCHED_YES | TOUCHED_NO)

#define REACH_STD      0  /* reached by branch or fallthrough */
#define REACH_JSR      1  /* reached by JSR */
#define REACH_RET      2  /* reached by RET */ /* XXX ? */
#define REACH_THROW    3  /* reached by THROW (exception handler) */

/****************************************************************************/
/* DEBUG HELPERS                                                            */
/****************************************************************************/

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
#define LOGINFO(info)      DOLOG(do {typeinfo_print_short(get_logfile(),(info));log_plain("\n");} while(0))
#define LOGFLUSH           DOLOG(fflush(get_logfile()))
#define LOGNL              DOLOG(log_plain("\n"))
#define LOGSTR(str)        DOLOG(log_plain(str))
#define LOGSTR1(str,a)     DOLOG(dolog_plain(str,a))
#define LOGSTR2(str,a,b)   DOLOG(dolog_plain(str,a,b))
#define LOGSTR3(str,a,b,c) DOLOG(dolog_plain(str,a,b,c))
#define LOGSTRu(utf)       DOLOG(log_plain_utf(utf))
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
typeinfo_print_locals(FILE *file,u1 *vtype,typeinfo *vinfo,u1 *touched,int num)
{
    int i;

    for (i=0; i<num; ++i) {
        if (touched)
            fprintf(file," %d%s=",i,
                    (touched[i]==TOUCHED_YES) ? "*"
                    : ((touched[i]==TOUCHED_NO) ? "" : "~"));
        else
            fprintf(file," %d=",i);
        typeinfo_print_type(file,vtype[i],vinfo+i);
    }
}

static
void
typeinfo_print_stack(FILE *file,stackptr stack)
{
    while (stack) {
        typeinfo_print_type(file,stack->type,&stack->typeinfo);
        stack = stack->prev;
        if (stack) fprintf(file," ");
    }
}

static
void
typeinfo_print_block(FILE *file,stackptr instack,
                     int vnum,u1 *vtype,typeinfo *vinfo,u1 *touched)
{
    fprintf(file,"Stack: ");
    typeinfo_print_stack(file,instack);
    fprintf(file," Locals:");
    typeinfo_print_locals(file,vtype,vinfo,touched,vnum);
}

static
void
typestack_print(FILE *file,stackptr stack)
{
    while (stack) {
        typeinfo_print_stacktype(file,stack->type,&stack->typeinfo);
        stack = stack->prev;
        if (stack) fprintf(file," ");
    }
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


#if 0
static
void
typeinfo_print_blocks(FILE *file,int vnum,u1 *vtype,typeinfo *vinfo)
{
    int bi;
    /*    int j;*/

    for (bi=0; bi<block_count; ++bi) {
        fprintf(file,"%04d: (%3d) ",bi,block[bi].flags);
        typeinfo_print_block(file,block[bi].instack,
                             vnum,vtype+vnum*bi,vinfo+vnum*bi,NULL);
        fprintf(file,"\n");

/*         for (j=0; j<block[bi].icount; ++j) { */
/*             fprintf(file,"\t%s\n",icmd_names[block[bi].iinstr[j].opc]); */
/*         } */

        show_icmd_block(block+bi);
    }
}
#endif

#endif

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

static void
typestack_copy(stackptr dst,stackptr y,typevector *selected)
{
	typevector *sel;
	typeinfo_retaddr_set *sety;
	typeinfo_retaddr_set *new;
	typeinfo_retaddr_set **next;
	int k;
	
	for (;dst; dst=dst->prev, y=y->prev) {
		if (!y) panic("Stack depth mismatch");
		if (dst->type != y->type)
			panic("Stack type mismatch");
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
	if (y) panic("Stack depth mismatch");
}

static void
typestack_put_retaddr(stackptr dst,void *retaddr,typevector *loc)
{
#ifdef TYPECHECK_DEBUG
	if (dst->type != TYPE_ADDRESS)
		panic("Internal error: Storing returnAddress in non-address slot");
#endif

	
	TYPEINFO_INIT_RETURNADDRESS(dst->typeinfo,NULL);
	for (;loc; loc=loc->alt) {
		typeinfo_retaddr_set *set = DNEW(typeinfo_retaddr_set);
		set->addr = retaddr;
		set->alt = TYPESTACK_RETURNADDRESSSET(dst);
		TYPEINFO_INIT_RETURNADDRESS(dst->typeinfo,set);
	}
}

static bool
typestack_canmerge(stackptr a,stackptr b)
{
	for (; a; a = a->prev, b = b->prev) {
		if (!b) return false;
		if (a->type != b->type) return false;
		if (a->type == TYPE_ADDRESS) {
			if (((TYPEINFO_IS_PRIMITIVE(a->typeinfo)) ? 1 : 0)
				^
				((TYPEINFO_IS_PRIMITIVE(b->typeinfo)) ? 1 : 0))
				return false;
		}
	}
	if (b) return false;
	return true;
}

static void
typestack_collapse(stackptr dst)
{
	for (; dst; dst = dst->prev) {
		if (TYPESTACK_IS_RETURNADDRESS(dst))
			TYPESTACK_RETURNADDRESSSET(dst)->alt = NULL;
	}
}

/* 'dst' and 'y' are assumed to have passed typestack_canmerge! */
static bool
typestack_merge(stackptr dst,stackptr y)
{
	bool changed = false;
	for (; dst; dst = dst->prev, y=y->prev) {
		if (TYPESTACK_IS_REFERENCE(dst))
			changed |= typeinfo_merge(&(dst->typeinfo),&(y->typeinfo));
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
#ifdef TYPECHECK_DEBUG
		if (!b) panic("Internal error: typestack_separable_from: different depth");
#endif
		if (TYPESTACK_IS_RETURNADDRESS(a)) {
#ifdef TYPECHECK_DEBUG
			if (!TYPESTACK_IS_RETURNADDRESS(b))
				panic("Internal error: typestack_separable_from: unmergable stacks");
#endif
			seta = TYPESTACK_RETURNADDRESSSET(a);
			setb = TYPESTACK_RETURNADDRESSSET(b);
			RETURNADDRESSSET_SEEK(setb,kb);

			for (;seta;seta=seta->alt)
				if (seta->addr != setb->addr) return true;
		}
	}
#ifdef TYPECHECK_DEBUG
	if (b) panic("Internal error: typestack_separable_from: different depth");
#endif
	return false;
}

/* 'a' and 'b' are assumed to have passed typestack_canmerge! */
static bool
typestack_separable_from(stackptr a,int ka,stackptr b,int kb)
{
	typeinfo_retaddr_set *seta;
	typeinfo_retaddr_set *setb;
	int i;

	for (; a; a = a->prev, b = b->prev) {
#ifdef TYPECHECK_DEBUG
		if (!b) panic("Internal error: typestack_separable_from: different depth");
#endif
		if (TYPESTACK_IS_RETURNADDRESS(a)) {
#ifdef TYPECHECK_DEBUG
			if (!TYPESTACK_IS_RETURNADDRESS(b))
				panic("Internal error: typestack_separable_from: unmergable stacks");
#endif
			seta = TYPESTACK_RETURNADDRESSSET(a);
			setb = TYPESTACK_RETURNADDRESSSET(b);
			RETURNADDRESSSET_SEEK(seta,ka);
			RETURNADDRESSSET_SEEK(setb,kb);

			if (seta->addr != setb->addr) return true;
		}
	}
#ifdef TYPECHECK_DEBUG
	if (b) panic("Internal error: typestack_separable_from: different depth");
#endif
	return false;
}

/****************************************************************************/
/* TYPESTATE FUNCTIONS                                                      */
/****************************************************************************/

static bool
typestate_merge(stackptr deststack,typevector *destloc,
				stackptr ystack,typevector *yloc,
				/*				int retindex,void *retaddr, */
				int locsize)
{
	typevector *dvec,*yvec;
	int kd,ky;
	bool changed = false;
	
#ifdef TYPECHECK_DEBUG
	/* Do some sanity checks */
	/*
	  if (retindex < -1 || retindex >= locsize || (retindex >= 0 && !retaddr))
	  panic("Internal error: typestate_merge: invalid arguments");
	*/
#endif

	LOG("merge:");
	LOGSTR("dstack: "); DOLOG(typestack_print(get_logfile(),deststack)); LOGNL;
	LOGSTR("ystack: "); DOLOG(typestack_print(get_logfile(),ystack)); LOGNL;
	LOGSTR("dloc  : "); DOLOG(typevectorset_print(get_logfile(),destloc,locsize)); LOGNL;
	LOGSTR("yloc  : "); DOLOG(typevectorset_print(get_logfile(),yloc,locsize)); LOGNL;

	/* Check if the stack types of deststack and ystack match */

	/* XXX move this check into typestack_merge? */
	if (!typestack_canmerge(deststack,ystack))
		panic("Stack depth or stack type mismatch");

	/* The stack is always merged. If there are returnAddresses on
	 * the stack they are ignored in this step. */

	changed |= typestack_merge(deststack,ystack);

	for (yvec=yloc; yvec; yvec=yvec->alt) {
		ky = yvec->k;

		/* If retindex >= 0 we select only those states (ystack,yvec)
		 * with returnAddress retaddr in variable no. retindex. */

		/*
		  if (retindex >= 0) {
		  if (!TYPEDESC_IS_RETURNADDRESS(yvec->td[retindex]))
		  panic("Illegal instruction: RET on non-returnAddress");
		  if (TYPEINFO_RETURNADDRESS(yvec->td[retindex].info)
		  != retaddr)
		  continue;
		  }
		*/

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
		
	}
	
	LOG("result:");
	LOGSTR("dstack: "); DOLOG(typestack_print(get_logfile(),deststack)); LOGNL;
	LOGSTR("dloc  : "); DOLOG(typevectorset_print(get_logfile(),destloc,locsize)); LOGNL;
	
	return changed;
}

/* Globals used:
 *     block
 */
static bool
typestate_reach(void *localbuf,
				basicblock *current,
				basicblock *destblock,
				stackptr ystack,typevector *yloc,
				int locsize)
{
	typevector *yvec;
	typevector *destloc;
	int destidx;
	bool repeat = false;
	bool changed = false;
	
	destidx = destblock - block;
	destloc = MGET_TYPEVECTOR(localbuf,destidx,locsize);

	if (destblock->flags == BBTYPECHECK_UNDEF) {
		/* The destblock has never been reached before */

		LOG1("block (index %04d) reached first time",destidx);
		
		typestack_copy(destblock->instack,ystack,yloc);
		COPY_TYPEVECTORSET(yloc,destloc,locsize);
		changed = true;
	}
	else {
		/* The destblock has already been reached before */
		
		LOG1("block (index %04d) reached before",destidx);
		
		changed = typestate_merge(destblock->instack,destloc,
								  ystack,yloc,locsize);
	}

	if (changed) {
		LOG("changed!");
		destblock->flags = BBTYPECHECK_REACHED;
		if (destblock <= current) {repeat = true; LOG("REPEAT!");}
	}
	return repeat;
}

/* Globals used:
 *     see typestate_reach
 */
static bool
typestate_ret(void *localbuf,
			  basicblock *current,
			  stackptr ystack,typevector *yloc,
			  int retindex,int locsize)
{
	typevector *yvec;
	typevector *selected;
	basicblock *destblock;
	bool repeat = false;

	for (yvec=yloc; yvec; ) {
		if (!TYPEDESC_IS_RETURNADDRESS(yvec->td[retindex]))
			panic("Illegal instruction: RET on non-returnAddress");

		destblock = (basicblock*) TYPEINFO_RETURNADDRESS(yvec->td[retindex].info);

		selected = typevectorset_select(&yvec,retindex,destblock);
		
		repeat |= typestate_reach(localbuf,current,destblock,
								  ystack,selected,locsize);
	}
	return repeat;
}

static bool
typestate_jsr(void *localbuf,
			  basicblock *current,basicblock *destblock,
			  stackptr ystack,typevector *yloc,
			  int locsize)
{
	typestack_put_retaddr(ystack,current+1,yloc);
	return typestate_reach(localbuf,current,destblock,ystack,yloc,locsize);
}

/****************************************************************************/
/* HELPER FUNCTIONS                                                         */
/****************************************************************************/

/* If a field is checked, definingclass == implementingclass */
static bool
is_accessible(int flags,classinfo *definingclass,classinfo *implementingclass,
			  typeinfo *instance)
{
	/* check access rights */
	if (class != definingclass) {
		switch (flags & (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED)) {
		  case ACC_PUBLIC:
			  break;
			  
			  /* In the cases below, definingclass cannot be an interface */
			  
		  case 0:
			  /* XXX check package access */
			  break;
		  case ACC_PROTECTED:
			  /* XXX check package access and superclass access */
			  break;
		  case ACC_PRIVATE:
			  if (definingclass != class) {
				  LOG("private access");
				  return false;
			  }
			  break;
			  /* XXX check package access */
		  default:
			  panic("Invalid access flags");
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
			typeinfo tempinfo;
			
			if (((flags & ACC_PROTECTED) != 0)
				&& builtin_isanysubclass(class,implementingclass))
			{
				/* For protected access of super class members
				 * the instance must be a subclass of or the same
				 * as the current class. */

				/* XXX maybe we only are allowed to do this, if we
				 * don't have package access? */
				/* implementingclass = class; */ /* XXX does not work */
			}
			
			/* XXX use classinfo directly? */
			TYPEINFO_INIT_CLASSINFO(tempinfo,implementingclass);
			if (!typeinfo_is_assignable(instance,&tempinfo)) {
				LOG("instance not assignable");
				LOGINFO(instance);
				LOGINFO(&tempinfo);
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

/****************************************************************************/
/* STATISTICS                                                               */
/****************************************************************************/

#ifdef TYPECHECK_DEBUG
/*#define TYPECHECK_STATISTICS*/
#endif

#ifdef TYPECHECK_STATISTICS
#define TYPECHECK_COUNT(cnt)  (cnt)++
#else
#define TYPECHECK_COUNT(cnt)
#endif

/****************************************************************************/
/* INTERNAL DATA STRUCTURES                                                 */
/****************************************************************************/

typedef struct jsr_record jsr_record;

/*
 * For each basic block we store the chain of JSR instructions which
 * were used to reach the block (usually zero or one). For more
 * details on verifying JSR and RET instructions see the Java VM
 * Specification.
 *
 * CAUTION: The fields starting with sbr_ are only valid for the
 * jsr_record of the first block of the subroutine.
 */
struct jsr_record {
    basicblock  *target;      /* target of the JSR instruction (first block of subroutine) */
    jsr_record  *next;        /* for chaining in nested try ... finally */ /* XXX make it sbr_next? */
    u1          *sbr_touched; /* specifies which variables the subroutine touches */
    u1          *sbr_vtype;   /* Types of local variables after RET */
    typeinfo    *sbr_vinfo;   /* Types of local variables after RET */
	instruction *sbr_ret;     /* RET instruction of the subroutine */
    u1           touched[1];  /* touched flags for local variables */
};

/****************************************************************************/
/* MACROS USED INTERNALLY IN typecheck()                                    */
/****************************************************************************/

#define TOUCH_VARIABLE(num)										\
	do {if (jsrchain) touched[num] = TOUCHED_YES;} while (0)
#define TOUCH_TWOWORD(num)										\
	do {TOUCH_VARIABLE(num);TOUCH_VARIABLE((num)+1);} while (0)

#define INDEX_ONEWORD(num)										\
	do { if((num)<0 || (num)>=validlocals)						\
			panic("Invalid local variable index"); } while (0)
#define INDEX_TWOWORD(num)										\
	do { if((num)<0 || ((num)+1)>=validlocals)					\
			panic("Invalid local variable index"); } while (0)

#define SET_VARIABLE(num,type)									\
	do {vtype[num] = (type);									\
		if ((num)>0 && IS_2_WORD_TYPE(vtype[(num)-1])) {		\
			vtype[(num)-1] = TYPE_VOID;							\
			TOUCH_VARIABLE((num)-1);							\
		}														\
		TOUCH_VARIABLE(num);} while(0)

#define STORE_ONEWORD(num,type)									\
 	do {INDEX_ONEWORD(num);										\
		typevectorset_store(localset,num,type,NULL);} while(0)

#define STORE_TWOWORD(num,type)									\
 	do {INDEX_TWOWORD(num);										\
		typevectorset_store_twoword(localset,num,type);} while(0)

#define CHECK_ONEWORD(num,type)											\
	do {INDEX_ONEWORD(num);												\
		if (!typevectorset_checktype(localset,num,type))                \
            panic("Variable type mismatch");	                        \
		} while(0)

#define CHECK_TWOWORD(num,type)											\
	do {INDEX_TWOWORD(num);												\
		if (!typevectorset_checktype(localset,num,type))                \
            panic("Variable type mismatch");	                        \
		} while(0)

/* XXX maybe it's faster to copy always */
#define COPYTYPE(source,dest)   \
            {if ((source)->type == TYPE_ADR) \
                 TYPEINFO_COPY((source)->typeinfo,(dest)->typeinfo);}

#define ISBUILTIN(v)   (iptr->val.a == (functionptr)(v))

#define TYPECHECK_STACK(sp,tp)											\
	do { if ((sp)->type != (tp))										\
			panic("Wrong data type on stack"); } while(0)

#define TYPECHECK_ADR(sp)  TYPECHECK_STACK(sp,TYPE_ADR)
#define TYPECHECK_INT(sp)  TYPECHECK_STACK(sp,TYPE_INT)
#define TYPECHECK_LNG(sp)  TYPECHECK_STACK(sp,TYPE_LNG)
#define TYPECHECK_FLT(sp)  TYPECHECK_STACK(sp,TYPE_FLT)
#define TYPECHECK_DBL(sp)  TYPECHECK_STACK(sp,TYPE_DBL)

#define TYPECHECK_ARGS1(t1)							            \
	do {TYPECHECK_STACK(curstack,t1);} while (0)
#define TYPECHECK_ARGS2(t1,t2)						            \
	do {TYPECHECK_ARGS1(t1);									\
		TYPECHECK_STACK(curstack->prev,t2);} while (0)
#define TYPECHECK_ARGS3(t1,t2,t3)								\
	do {TYPECHECK_ARGS2(t1,t2);									\
		TYPECHECK_STACK(curstack->prev->prev,t3);} while (0)

/* TYPECHECK_COPYVARS: copy the types and typeinfos of the current local
 *     variables to the local variables of the target block.
 * Input:
 *     vtype......current local variable types
 *     vinfo......current local variable typeinfos
 *     ttype......local variable types of target block
 *     tinfo......local variable typeinfos of target block
 *     numlocals..number of local variables
 * Used:
 *     macro_i
 */
#define TYPECHECK_COPYVARS                                      \
    do {                                                        \
    LOG("TYPECHECK_COPYVARS");                                  \
    for (macro_i=0; macro_i<numlocals; ++macro_i) {             \
        if ((ttype[macro_i] = vtype[macro_i]) == TYPE_ADR)      \
            TYPEINFO_CLONE(vinfo[macro_i],tinfo[macro_i]);      \
    } } while(0)

/* TYPECHECK_MERGEVARS: merge the local variables of the target block
 *     with the current local variables.
 * Input:
 *     vtype......current local variable types
 *     vinfo......current local variable typeinfos
 *     ttype......local variable types of target block
 *     tinfo......local variable typeinfos of target block
 *     numlocals..number of local variables
 * Ouput:
 *     changed....set to true if any typeinfo has changed
 * Used:
 *     macro_i
 */
#define TYPECHECK_MERGEVARS                                             \
    do {                                                                \
    LOG("TYPECHECK_MERGEVARS");                                         \
    for (macro_i=0; macro_i<numlocals; ++macro_i) {                     \
        if ((ttype[macro_i] != TYPE_VOID) && (vtype[macro_i] != ttype[macro_i])) { \
            LOG3("var %d: type %d + type %d = void",macro_i,ttype[macro_i],vtype[macro_i]); \
            ttype[macro_i] = TYPE_VOID;                                 \
            changed = true;                                             \
        } else if (ttype[macro_i] == TYPE_ADR) {                        \
            if ( ((TYPEINFO_IS_PRIMITIVE(tinfo[macro_i])) ? 1 : 0)      \
                 ^                                                      \
                 ((TYPEINFO_IS_PRIMITIVE(vinfo[macro_i])) ? 1 : 0)) {   \
                LOG1("var %d: primitive + reference merge",macro_i);    \
                ttype[macro_i] = TYPE_VOID;                             \
                changed = true;                                         \
            }                                                           \
            else {                                                      \
                LOG1("var %d:",macro_i);                                \
                LOGINFO(tinfo+macro_i);                                 \
                LOGINFO(vinfo+macro_i);                                 \
                changed |= typeinfo_merge(tinfo+macro_i,vinfo+macro_i); \
                LOGINFO(tinfo+macro_i);                                 \
                LOGIF(changed,"vars have changed");                     \
            }                                                           \
        };                                                              \
    } } while (0)

/* TYPECHECK_MERGEJSR:
 *
 * Input:
 * Ouput:
 *     tbptr......target block
 *     changed....set to true if any typeinfo has changed
 *     numlocals..number of local variables
 *     touched....current touched flags of local variables
 * Used:
 *     macro_i, jsrtemp, jsrtemp2
 */
#define TYPECHECK_MERGEJSR                                              \
    do {                                                                \
        LOG("TYPECHECK_MERGEJSR");                                      \
    jsrtemp = jsrbuffer[tbptr-block];                                   \
    jsrtemp2 = jsrchain;                                                \
    while (jsrtemp || jsrtemp2) {                                       \
        if (!jsrtemp || !jsrtemp2)                                      \
            panic("Merging JSR subroutines of different depth");        \
        if (jsrtemp->target != jsrtemp2->target)                        \
            panic("Merging different JSR subroutines");                 \
        jsrtemp = jsrtemp->next;                                        \
        jsrtemp2 = jsrtemp2->next;                                      \
    }                                                                   \
    jsrtemp = jsrbuffer[tbptr-block];                                   \
    if (jsrtemp)                                                        \
        for (macro_i=0; macro_i<numlocals; ++macro_i) {                 \
            jsrtemp->touched[i] |= touched[i];                          \
    } } while (0)

/* TYPECHECK_COPYSTACK: copy the typeinfos of the current stack to
 *     the input stack of the target block.
 * Input:
 *     srcstack...current stack
 *     dststack...input stack of target block
 */
#define TYPECHECK_COPYSTACK                                             \
    do {                                                                \
    LOG("TYPECHECK_COPYSTACK");                                         \
    while (srcstack) {                                                  \
        LOG1("copy %d",srcstack->type);                                 \
        if (!dststack) panic("Stack depth mismatch");                   \
        if (srcstack->type != dststack->type)                           \
            panic("Type mismatch on stack");                            \
        if (srcstack->type == TYPE_ADR) {                               \
            TYPEINFO_CLONE(srcstack->typeinfo,dststack->typeinfo);      \
        }                                                               \
        dststack = dststack->prev;                                      \
        srcstack = srcstack->prev;                                      \
    }                                                                   \
    if (dststack) panic("Stack depth mismatch");                        \
    } while (0)

/* TYPECHECK_MERGESTACK: merge the input stack of the target block
 *     with the current stack.
 * Input:
 *     srcstack...current stack
 *     dststack...input stack of target block
 * Ouput:
 *     changed....set to true if any typeinfo has changed
 */
#define TYPECHECK_MERGESTACK                                            \
    do {                                                                \
    LOG("TYPECHECK_MERGESTACK");                                        \
    while (srcstack) {                                                  \
        if (!dststack) panic("Stack depth mismatch");                   \
        if (srcstack->type != dststack->type)                           \
            panic("Type mismatch on stack");                            \
        if (srcstack->type == TYPE_ADR) {                               \
            LOGINFO(&dststack->typeinfo);                               \
            LOGINFO(&srcstack->typeinfo);  LOGFLUSH;                    \
            changed |= typeinfo_merge(&dststack->typeinfo,              \
                                      &srcstack->typeinfo);             \
            LOGINFO(&dststack->typeinfo);                               \
            LOG((changed)?"CHANGED!\n":"not changed.\n");               \
        }                                                               \
        dststack = dststack->prev;                                      \
        srcstack = srcstack->prev;                                      \
    }                                                                   \
    if (dststack) panic("Stack depth mismatch");                        \
    } while(0)


/* TYPECHECK_CHECK_JSR_CHAIN: checks if the target block is reached by
 *     the same JSR targets on all control paths.
 *
 * Input:
 *     tbptr......target block
 *     jsrchain...current JSR target chain
 *     jsrbuffer..JSR target chain for each basic block
 * Output:
 *     panic if the JSR target chains don't match
 * Used:
 *     jsrtemp, jsrtemp2
 */
#define TYPECHECK_CHECK_JSR_CHAIN                                       \
    do {                                                                \
    jsrtemp = jsrbuffer[tbptr-block];                                   \
    if (!jsrtemp) panic("non-subroutine called by JSR");                \
    if (jsrtemp->target != tbptr)                                       \
        panic("Merging different JSR subroutines");                     \
    jsrtemp = jsrtemp->next;                                            \
    jsrtemp2 = jsrchain;                                                \
    while (jsrtemp || jsrtemp2) {                                       \
        if (!jsrtemp || !jsrtemp2)                                      \
            panic("Merging JSR subroutines of different depth");        \
        if (jsrtemp->target != jsrtemp2->target)                        \
            panic("Merging different JSR subroutines");                 \
        jsrtemp = jsrtemp->next;                                        \
        jsrtemp2 = jsrtemp2->next;                                      \
    } } while (0)

/* TYPECHECK_ADD_JSR: add a JSR target to the current JSR target chain
 *     and store the resulting chain in the target block.
 *
 * Input:
 *     jsrchain...current JSR target chain
 *     tbptr.....the basic block targeted by the JSR
 *     numlocals..number of local variables
 *     jsrbuffer..JSR target chain for each basic block
 * Used:
 *     jsrtemp
 */
#define TYPECHECK_ADD_JSR																\
    do {																				\
    LOG1("adding JSR to block %04d",(tbptr)-block);										\
    jsrtemp = jsrchain;																	\
    while (jsrtemp) {																	\
	    if (jsrtemp->target == tbptr)													\
		    panic("recursive JSR call");												\
        jsrtemp = jsrtemp->next;														\
    }																					\
    jsrtemp = (jsr_record *) dump_alloc(sizeof(jsr_record)+(numlocals-1)*sizeof(u1));	\
    jsrtemp->target = (tbptr);															\
    jsrtemp->next = jsrchain;															\
    jsrtemp->sbr_touched = NULL;														\
    memset(&jsrtemp->touched,TOUCHED_NO,sizeof(u1)*numlocals);							\
    jsrbuffer[tbptr-block] = jsrtemp;													\
    } while (0)

/* TYPECHECK_COPYJSR: copy the current JSR chain to the target block.
 *
 * Input:
 *     chain......current JSR target chain
 *     tbptr.....the basic block targeted by the JSR
 *     numlocals..number of local variables
 *     jsrbuffer..JSR target chain for each basic block
 *     touched....current touched flags of local variables
 * Used:
 *     jsrtemp
 */
#define TYPECHECK_COPYJSR(chain)                                        \
    do {                                                                \
        LOG("TYPECHECK_COPYJSR");                                       \
        if (chain) {                                                    \
            jsrtemp = (jsr_record *) dump_alloc(sizeof(jsr_record)+(numlocals-1)*sizeof(u1)); \
            jsrtemp->target = (chain)->target;                          \
            jsrtemp->next = (chain)->next;                              \
            jsrtemp->sbr_touched = NULL;                                \
            memcpy(&jsrtemp->touched,touched,sizeof(u1)*numlocals);     \
            jsrbuffer[tbptr-block] = jsrtemp;                           \
        }                                                               \
        else                                                            \
            jsrbuffer[tbptr-block] = NULL;                              \
    } while (0)

/* TYPECHECK_BRANCH_BACKWARDS: executed when control flow moves
 *     backwards. Checks if there are uninitialized objects on the
 *     stack or in local variables.
 * Input:
 *     dst........current output stack pointer (not needed for REACH_THROW)
 *     vtype......current local variable types
 *     vinfo......current local variable typeinfos
 * Used:
 *     srcstack, macro_i
 */
#define TYPECHECK_BRANCH_BACKWARDS                                      \
    do {                                                                \
        LOG("BACKWARDS!");                                              \
        srcstack = dst;                                                 \
        while (srcstack) {                                              \
            if (srcstack->type == TYPE_ADR &&                           \
                TYPEINFO_IS_NEWOBJECT(srcstack->typeinfo))              \
                panic("Branching backwards with uninitialized object on stack"); \
            srcstack = srcstack->prev;                                  \
        }                                                               \
        for (macro_i=0; macro_i<numlocals; ++macro_i)                   \
            if (localset->td[macro_i].type == TYPE_ADR &&               \
                TYPEINFO_IS_NEWOBJECT(localset->td[macro_i].info))      \
                panic("Branching backwards with uninitialized object in local variable"); \
    } while(0)

/* XXX convert this into a function */
/* TYPECHECK_REACH: executed, when the target block (tbptr) can be reached
 *     from the current block (bptr). The types of local variables and
 *     stack slots are propagated to the target block.
 * Input:
 *     bptr.......current block
 *     tbptr......target block
 *     dst........current output stack pointer (not needed for REACH_THROW)
 *     numlocals..number of local variables
 *     vtype......current local variable types
 *     vinfo......current local variable typeinfos
 *     jsrchain...current JSR target chain
 *     jsrbuffer..JSR target chain for each basic block
 *     way........in which way the block is reached (REACH_ constant)
 *     touched....current touched flags of local variables
 * Output:
 *     repeat.....changed to true if a block before the current
 *                block has changed
 * Used:
 *     ttype, tinfo, srcstack, dststack, changed, macro_i
 */
#define TYPECHECK_REACH(way)                                            \
    do {                                                                \
    LOG2("reaching block %04d (%d)",tbptr-block,way);                   \
    if (tbptr <= bptr)                                                  \
        TYPECHECK_BRANCH_BACKWARDS;                                     \
    repeat |= typestate_reach(localbuf,bptr,tbptr,dst,                  \
							  localset,numlocals);                      \
    LOG("done.");                                                       \
    } while (0)

/* TYPECHECK_LEAVE: executed when the method is exited non-abruptly
 * Input:
 *     class........class of the current method
 *     numlocals....number of local variables
 *     vtype........current local variable types
 *     vinfo........current local variable typeinfos
 *     initmethod...true if this is an <init> method
 */
#define TYPECHECK_LEAVE                                                 \
    do {                                                                \
        if (initmethod && class != class_java_lang_Object) {            \
            /* check the marker variable */                             \
            LOG("Checking <init> marker");                              \
            if (!typevectorset_checktype(localset,numlocals-1,TYPE_INT))\
                panic("<init> method does not initialize 'this'");      \
        }                                                               \
    } while (0)

/****************************************************************************/
/* typecheck()                                                              */
/****************************************************************************/

#define MAXPARAMS 255

/* typecheck is called directly after analyse_stack */
void
typecheck()
{
    int b_count, b_index;
    stackptr curstack;      /* input stack top for current instruction */
    stackptr srcstack;         /* source stack for copying and merging */
    stackptr dststack;         /* target stack for copying and merging */
    int opcode;                                      /* current opcode */
    int macro_i, i;                              /* temporary counters */
    int len;                        /* for counting instructions, etc. */
    bool superblockend;        /* true if no fallthrough to next block */
    bool repeat;            /* if true, blocks are iterated over again */
    bool changed;                                    /* used in macros */
    instruction *iptr;               /* pointer to current instruction */
    basicblock *bptr;                /* pointer to current basic block */
    basicblock *tbptr;                   /* temporary for target block */
    int numlocals;                        /* number of local variables */
	int validlocals;         /* number of valid local variable indices */

	void *localbuf;       /* local variable types for each block start */
	typevector *localset;        /* typevector set for local variables */
	typevector *lset;                             /* temporary pointer */
	typedescriptor *td;                           /* temporary pointer */

    typeinfo tempinfo;                                    /* temporary */

	typedescriptor returntype;        /* return type of current method */
	
    u1 *ptype;                     /* parameter types of called method */
    typeinfo *pinfo;           /* parameter typeinfos of called method */
    int rtype;                         /* return type of called method */
    typeinfo rinfo;       /* typeinfo for return type of called method */
    stackptr dst;               /* output stack of current instruction */
    basicblock **tptr;    /* pointer into target list of switch instr. */
    jsr_record **jsrbuffer;   /* JSR target chain for each basic block */
    jsr_record *jsrchain;               /* JSR chain for current block */
    jsr_record *jsrtemp,*jsrtemp2;              /* temporary variables */
    jsr_record *subroutine;    /* jsr_record of the current subroutine */
    u1 *touched;                  /* touched flags for local variables */
    xtable **handlers;                    /* active exception handlers */
    classinfo *cls;                                       /* temporary */
    bool maythrow;               /* true if this instruction may throw */
    utf *name_init;                                        /* "<init>" */
    bool initmethod;             /* true if this is an "<init>" method */
	builtin_descriptor *builtindesc;

#ifdef TYPECHECK_STATISTICS
	int count_iterations = 0;
#endif

    LOGSTR("\n==============================================================================\n");
    DOLOG(show_icmd_method());
    LOGSTR("\n==============================================================================\n");
    LOGimpSTR("Entering typecheck: ");
    LOGimpSTRu(method->name);
    LOGimpSTR("    ");
    LOGimpSTRu(method->descriptor);
    LOGimpSTR("    (class ");
    LOGimpSTRu(method->class->name);
    LOGimpSTR(")\n");

    name_init = utf_new_char("<init>");
    initmethod = (method->name == name_init);

    ptype = DMNEW(u1,MAXPARAMS);
    pinfo = DMNEW(typeinfo,MAXPARAMS);
    
    LOG("Buffer allocated.\n");

    /* reset all BBFINISHED blocks to BBTYPECHECK_UNDEF. */
    b_count = block_count;
    bptr = block;
    while (--b_count >= 0) {
#ifdef TYPECHECK_DEBUG
        if (bptr->flags != BBFINISHED && bptr->flags != BBDELETED
            && bptr->flags != BBUNDEF)
        {
            show_icmd_method();
            LOGSTR1("block flags: %d\n",bptr->flags); LOGFLUSH;
            panic("Internal error: Unexpected block flags in typecheck()");
        }
#endif
        if (bptr->flags >= BBFINISHED) {
            bptr->flags = BBTYPECHECK_UNDEF;
        }
        bptr++;
    }

    /* The first block is always reached */
    if (block_count && block[0].flags == BBTYPECHECK_UNDEF)
        block[0].flags = BBTYPECHECK_REACHED;

    LOG("Blocks reset.\n");

    /* number of local variables */
    
    /* In <init> methods we use an extra local variable to signal if
     * the 'this' reference has been initialized. */
    numlocals = maxlocals;
	validlocals = numlocals;
    if (initmethod) numlocals++;

    /* allocate the buffers for local variables */
	localbuf = DMNEW_TYPEVECTOR(block_count+1, numlocals);
	localset = MGET_TYPEVECTOR(localbuf,block_count,numlocals);
	memset(localbuf,0,(block_count+1) * TYPEVECTOR_SIZE(numlocals));

    LOG("Variable buffer allocated.\n");

    /* allocate the buffer for storing JSR target chains */
    jsrbuffer = DMNEW(jsr_record*,block_count);
    memset(jsrbuffer,0,block_count * sizeof(jsr_record*));
    jsrchain = NULL;
    
    LOG("jsrbuffer initialized.\n");

    /* allocate the buffer of active exception handlers */
    handlers = DMNEW(xtable*,method->exceptiontablelength + 1);

    /* initialize the variable types of the first block */
    /* to the types of the arguments */
	lset = MGET_TYPEVECTOR(localbuf,0,numlocals);
	td = lset->td;
	i = numlocals;

    /* if this is an instance method initialize the "this" ref type */
    if (!(method->flags & ACC_STATIC)) {
        td->type = TYPE_ADDRESS;
        if (initmethod)
            TYPEINFO_INIT_NEWOBJECT(td->info,NULL);
        else
            TYPEINFO_INIT_CLASSINFO(td->info,class);
        td++;
		i--;
    }

    LOG("'this' argument set.\n");

    /* the rest of the arguments and the return type */
    i = typedescriptors_init_from_method_args(td,method->descriptor,
											  i,
											  true, /* two word types use two slots */
											  &returntype);
	td += i;
	i = numlocals - (td - lset->td);
	while (i--) {
		td->type = TYPE_VOID;
		td++;
	}

    LOG("Arguments set.\n");

    /* initialize the input stack of exception handlers */
    for (i=0; i<method->exceptiontablelength; ++i) {
        cls = extable[i].catchtype;
        if (!cls) cls = class_java_lang_Throwable;
        LOGSTR1("handler %i: ",i); LOGSTRu(cls->name); LOGNL;
        TYPEINFO_INIT_CLASSINFO(extable[i].handler->instack->typeinfo,cls);
    }

    LOG("Exception handler stacks set.\n");

    /* loop while there are still blocks to be checked */
    do {
		TYPECHECK_COUNT(count_iterations);

        repeat = false;
        
        b_count = block_count;
        bptr = block;

        while (--b_count >= 0) {
            LOGSTR1("---- BLOCK %04d, ",bptr-block);
            LOGSTR1("blockflags: %d\n",bptr->flags);
            LOGFLUSH;
                
            if (bptr->flags == BBTYPECHECK_REACHED) {
                LOGSTR1("\n---- BLOCK %04d ------------------------------------------------\n",bptr-block);
                LOGFLUSH;
                
                superblockend = false;
                bptr->flags = BBFINISHED;
                b_index = bptr - block;

                /* init stack at the start of this block */
                curstack = bptr->instack;
					
                /* determine the active exception handlers for this block */
                /* XXX could use a faster algorithm with sorted lists or
                 * something? */
                len = 0;
                for (i=0; i<method->exceptiontablelength; ++i) {
                    if ((extable[i].start <= bptr) && (extable[i].end > bptr)) {
                        LOG1("active handler L%03d",extable[i].handler->debug_nr);
                        handlers[len++] = extable + i;
                    }
                }
                handlers[len] = NULL;
					
                /* init variable types at the start of this block */
				COPY_TYPEVECTORSET(MGET_TYPEVECTOR(localbuf,b_index,numlocals),
								   localset,numlocals);
				if (handlers[0])
					for (i=0; i<numlocals; ++i)
						if (localset->td[i].type == TYPE_ADR
							&& TYPEINFO_IS_NEWOBJECT(localset->td[i].info))
							panic("Uninitialized object in local variable inside try block");

                /* init JSR target chain */
                if ((jsrchain = jsrbuffer[b_index]) != NULL) {
#ifdef TYPECHECK_VERBOSE
                    if (typecheckverbose) {
                        LOGSTR("jsr chain:");
                        jsrtemp = jsrchain;
                        while (jsrtemp) {
                            LOGSTR1(" L%03d",jsrtemp->target->debug_nr);
                            jsrtemp = jsrtemp->next;
                        }
                        LOGNL;
                        LOGFLUSH;
                    }
#endif

                    subroutine = jsrbuffer[jsrchain->target - block];
                    memcpy(touched,jsrchain->touched,sizeof(u1)*numlocals);
                }
                else
                    subroutine = NULL;
#ifdef TYPECHECK_VERBOSE
                if (typecheckverbose) {
                    if (subroutine) {LOGSTR1("subroutine L%03d\n",subroutine->target->debug_nr);LOGFLUSH;}
                    typestate_print(get_logfile(),curstack,localset,numlocals);
                    LOGNL; LOGFLUSH;
                }
#endif

                /* loop over the instructions */
                len = bptr->icount;
                iptr = bptr->iinstr;
                while (--len >= 0)  {
                    DOLOG(show_icmd(iptr,false));
                    LOGNL;
                    LOGFLUSH;
                        
                    opcode = iptr->opc;
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
						  INDEX_ONEWORD(iptr->op1);
                          
                          /* loading a returnAddress is not allowed */
                          if (!typevectorset_checkreference(localset,iptr->op1))
                              panic("illegal instruction: ALOAD loading non-reference");

						  typevectorset_copymergedtype(localset,iptr->op1,&(dst->typeinfo));
                          break;
							
                          /****************************************/
                          /* STORING ADDRESS TO VARIABLE          */

                      case ICMD_ASTORE:
                          if (handlers[0] &&
                              TYPEINFO_IS_NEWOBJECT(curstack->typeinfo))
                              panic("Storing uninitialized object in local variable inside try block");

						  INDEX_ONEWORD(iptr->op1);
						  if (TYPESTACK_IS_RETURNADDRESS(curstack))
							  typevectorset_store_retaddr(localset,iptr->op1,&(curstack->typeinfo));
						  else
							  typevectorset_store(localset,iptr->op1,TYPE_ADDRESS,
												  &(curstack->typeinfo));
                          break;
                          
                          /****************************************/
                          /* LOADING ADDRESS FROM ARRAY           */

                      case ICMD_AALOAD:
                          if (!TYPEINFO_MAYBE_ARRAY_OF_REFS(curstack->prev->typeinfo))
                              panic("illegal instruction: AALOAD on non-reference array");

                          typeinfo_init_component(&curstack->prev->typeinfo,&dst->typeinfo);
                          maythrow = true;
                          break;
							  
                          /****************************************/
                          /* FIELD ACCESS                         */

                      case ICMD_PUTFIELD:
                          if (!TYPEINFO_IS_REFERENCE(curstack->prev->typeinfo))
                              panic("illegal instruction: PUTFIELD on non-reference");
                          if (TYPEINFO_IS_ARRAY(curstack->prev->typeinfo)) /* XXX arraystub */
                              panic("illegal instruction: PUTFIELD on array");

                          /* check if the value is assignable to the field */
						  {
							  fieldinfo *fi = (fieldinfo*) iptr[0].val.a;

							  if (TYPEINFO_IS_NEWOBJECT(curstack->prev->typeinfo)) {
								  if (initmethod
									  && !TYPEINFO_NEWOBJECT_INSTRUCTION(curstack->prev->typeinfo))
								  {
									  /* uninitialized "this" instance */
									  if (fi->class != class || (fi->flags & ACC_STATIC) != 0)
										  panic("Setting unaccessible field in uninitialized object");
								  }
								  else {
									  panic("PUTFIELD on uninitialized object");
								  }
							  }
							  else {
								  if (!is_accessible(fi->flags,fi->class,fi->class,
													 &(curstack->prev->typeinfo)))
									  panic("PUTFIELD: field is not accessible");
							  }

							  /* XXX ---> unify with ICMD_PUTSTATIC? */
							  
							  if (curstack->type != fi->type)
								  panic("PUTFIELD type mismatch");
							  if (fi->type == TYPE_ADR) {
								  TYPEINFO_INIT_FROM_FIELDINFO(rinfo,fi);
								  if (!typeinfo_is_assignable(&(curstack->typeinfo),
															  &rinfo))
									  panic("PUTFIELD reference type not assignable");
							  }
						  }
                          maythrow = true;
                          break;

                      case ICMD_PUTSTATIC:
                          /* check if the value is assignable to the field */
						  {
							  fieldinfo *fi = (fieldinfo*) iptr[0].val.a;

							  if (!is_accessible(fi->flags,fi->class,fi->class,NULL))
								  panic("PUTSTATIC: field is not accessible");

							  if (curstack->type != fi->type)
								  panic("PUTSTATIC type mismatch");
							  if (fi->type == TYPE_ADR) {
								  TYPEINFO_INIT_FROM_FIELDINFO(rinfo,fi);
								  if (!typeinfo_is_assignable(&(curstack->typeinfo),
															  &rinfo))
									  panic("PUTSTATIC reference type not assignable");
							  }
						  }
                          maythrow = true;
                          break;

                      case ICMD_GETFIELD:
                          if (!TYPEINFO_IS_REFERENCE(curstack->typeinfo))
                              panic("illegal instruction: GETFIELD on non-reference");
                          if (TYPEINFO_IS_ARRAY(curstack->typeinfo))
                              panic("illegal instruction: GETFIELD on array");
                          
                          {
                              fieldinfo *fi = (fieldinfo *)(iptr->val.a);

							  if (!is_accessible(fi->flags,fi->class,fi->class,
												 &(curstack->typeinfo)))
								  panic("GETFIELD: field is not accessible");

                              if (dst->type == TYPE_ADR) {
                                  TYPEINFO_INIT_FROM_FIELDINFO(dst->typeinfo,fi);
                              }
                              else {
                                  /* XXX check field type? */
                                  TYPEINFO_INIT_PRIMITIVE(dst->typeinfo);
                              }
                          }
                          maythrow = true;
                          break;

                      case ICMD_GETSTATIC:
                          {
                              fieldinfo *fi = (fieldinfo *)(iptr->val.a);
							  
							  if (!is_accessible(fi->flags,fi->class,fi->class,NULL))
								  panic("GETSTATIC: field is not accessible");

                              if (dst->type == TYPE_ADR) {
                                  TYPEINFO_INIT_FROM_FIELDINFO(dst->typeinfo,fi);
                              }
                              else {
                                  /* XXX check field type? */
                                  TYPEINFO_INIT_PRIMITIVE(dst->typeinfo);
                              }
                          }
                          maythrow = true;
                          break;

                          /****************************************/
                          /* PRIMITIVE ARRAY ACCESS               */

                      case ICMD_ARRAYLENGTH:
                          /* XXX should this also work on arraystubs? */
                          if (!TYPEINFO_MAYBE_ARRAY(curstack->typeinfo))
                              panic("illegal instruction: ARRAYLENGTH on non-array");
                          maythrow = true;
                          break;

						  /* XXX unify cases? */
                      case ICMD_BALOAD:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->typeinfo,ARRAYTYPE_BOOLEAN)
                              && !TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->typeinfo,ARRAYTYPE_BYTE))
                              panic("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_CALOAD:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->typeinfo,ARRAYTYPE_CHAR))
                              panic("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_DALOAD:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->typeinfo,ARRAYTYPE_DOUBLE))
                              panic("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_FALOAD:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->typeinfo,ARRAYTYPE_FLOAT))
                              panic("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_IALOAD:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->typeinfo,ARRAYTYPE_INT))
                              panic("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_SALOAD:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->typeinfo,ARRAYTYPE_SHORT))
                              panic("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_LALOAD:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->typeinfo,ARRAYTYPE_LONG))
                              panic("Array type mismatch");
                          maythrow = true;
                          break;

                      case ICMD_BASTORE:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->prev->typeinfo,ARRAYTYPE_BOOLEAN)
                              && !TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->prev->typeinfo,ARRAYTYPE_BYTE))
                              panic("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_CASTORE:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->prev->typeinfo,ARRAYTYPE_CHAR))
                              panic("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_DASTORE:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->prev->typeinfo,ARRAYTYPE_DOUBLE))
                              panic("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_FASTORE:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->prev->typeinfo,ARRAYTYPE_FLOAT))
                              panic("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_IASTORE:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->prev->typeinfo,ARRAYTYPE_INT))
                              panic("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_SASTORE:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->prev->typeinfo,ARRAYTYPE_SHORT))
                              panic("Array type mismatch");
                          maythrow = true;
                          break;
                      case ICMD_LASTORE:
                          if (!TYPEINFO_MAYBE_PRIMITIVE_ARRAY(curstack->prev->prev->typeinfo,ARRAYTYPE_LONG))
                              panic("Array type mismatch");
                          maythrow = true;
                          break;

                          /****************************************/
                          /* ADDRESS CONSTANTS                    */

                      case ICMD_ACONST:
                          if (iptr->val.a == NULL)
                              TYPEINFO_INIT_NULLTYPE(dst->typeinfo);
                          else
                              /* XXX constants for builtin functions */
                              /* string constants */
                              TYPEINFO_INIT_CLASSINFO(dst->typeinfo,class_java_lang_String);
                          break;

                          /****************************************/
                          /* CHECKCAST AND INSTANCEOF             */

                      case ICMD_CHECKCAST:
						  TYPECHECK_ADR(curstack);
                          /* returnAddress is not allowed */
                          if (!TYPEINFO_IS_REFERENCE(curstack->typeinfo))
                              panic("Illegal instruction: CHECKCAST on non-reference");

                          /* XXX check if the cast can be done statically */
                          TYPEINFO_INIT_CLASSINFO(dst->typeinfo,(classinfo *)iptr[0].val.a);
                          /* XXX */
                          maythrow = true;
                          break;

                      case ICMD_INSTANCEOF:
						  TYPECHECK_ADR(curstack);
                          /* returnAddress is not allowed */
                          if (!TYPEINFO_IS_REFERENCE(curstack->typeinfo))
                              panic("Illegal instruction: INSTANCEOF on non-reference");
                          
                          /* XXX optimize statically? */
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
                          tbptr = (basicblock *) iptr->target;

                          /* propagate stack and variables to the target block */
                          TYPECHECK_REACH(REACH_STD);
                          /* XXX */
                          break;

                          /****************************************/
                          /* SWITCHES                             */
                          
                      case ICMD_TABLESWITCH:
                          {
                              s4 *s4ptr = iptr->val.a;
                              s4ptr++;              /* skip default */
                              i = *s4ptr++;         /* low */
                              i = *s4ptr++ - i + 2; /* +1 for default target */
                          }
                          goto switch_instruction_tail;
                          
                      case ICMD_LOOKUPSWITCH:
                          {
                              s4 *s4ptr = iptr->val.a;
                              s4ptr++;              /* skip default */
                              i = *s4ptr++ + 1;     /* count +1 for default */
                          }
                    switch_instruction_tail:
                          tptr = (basicblock **)iptr->target;
                          
                          while (--i >= 0) {
                              tbptr = *tptr++;
                              LOG2("target %d is block %04d",(tptr-(basicblock **)iptr->target)-1,tbptr-block);
                              TYPECHECK_REACH(REACH_STD);
                          }
                          LOG("switch done");
                          superblockend = true;
                          break;

                          /****************************************/
                          /* RETURNS AND THROW                    */

                      case ICMD_ATHROW:
                          TYPEINFO_INIT_CLASSINFO(tempinfo,class_java_lang_Throwable);
                          if (!typeinfo_is_assignable(&curstack->typeinfo,&tempinfo))
                              panic("illegal instruction: ATHROW on non-Throwable");
                          superblockend = true;
                          maythrow = true;
                          break;

                      case ICMD_ARETURN:
                          if (!TYPEINFO_IS_REFERENCE(curstack->typeinfo))
                              panic("illegal instruction: ARETURN on non-reference");

                          if (returntype.type != TYPE_ADDRESS
                              || !typeinfo_is_assignable(&curstack->typeinfo,&(returntype.info)))
                              panic("Return type mismatch");
						  goto return_tail;
						  
                      case ICMD_IRETURN:
                          if (returntype.type != TYPE_INT) panic("Return type mismatch");
						  goto return_tail;
						  
                      case ICMD_LRETURN:
                          if (returntype.type != TYPE_LONG) panic("Return type mismatch");
						  goto return_tail;
						  
                      case ICMD_FRETURN:
                          if (returntype.type != TYPE_FLOAT) panic("Return type mismatch");
						  goto return_tail;
						  
                      case ICMD_DRETURN:
                          if (returntype.type != TYPE_DOUBLE) panic("Return type mismatch");
						  goto return_tail;
						  
                      case ICMD_RETURN:
                          if (returntype.type != TYPE_VOID) panic("Return type mismatch");
					  return_tail:
                          TYPECHECK_LEAVE;
                          superblockend = true;
                          maythrow = true;
                          break;
                                                    
                          /****************************************/
                          /* SUBROUTINE INSTRUCTIONS              */

                      case ICMD_JSR:
                          LOG("jsr");

                          /* XXX This is a dirty hack. It is needed
                           * because of the special handling of ICMD_JSR in stack.c
                           */
                          dst = (stackptr) iptr->val.a;
                          
                          tbptr = (basicblock *) iptr->target;
						  repeat |= typestate_jsr(localbuf,bptr,tbptr,dst,localset,numlocals);
#if 0
                          TYPEINFO_INIT_RETURNADDRESS(dst->typeinfo,tbptr);

                          LOG("reaching block...");
                          
                          /* add the target to the JSR target chain and */
                          /* propagate stack and variables to the target block */
                          TYPECHECK_REACH(REACH_JSR);

                          /* set dst to the stack after the subroutine execution */
                          /* XXX We assume (as in stack.c) that the
                           * subroutine returns the stack as it was
                           * before the JSR instruction. Is this
                           * correct?
                           */
                          dst = iptr->dst;

                          /* Find the jsr_record of the called subroutine */
                          jsrtemp = jsrbuffer[tbptr - block];

                          /* Check if we already calculated (at least
                           * for one RET) which variables the
                           * subroutine touches.
                           */
                          if (jsrtemp->sbr_touched) {
                              /* Calculate the local variables after the subroutine call */
                              for (i=0; i<numlocals; ++i)
                                  if (jsrtemp->sbr_touched[i] != TOUCHED_NO) {
                                      TOUCH_VARIABLE(i);
                                      if ((vtype[i] = jsrtemp->sbr_vtype[i]) == TYPE_ADR)
                                          TYPEINFO_CLONE(jsrtemp->sbr_vinfo[i],vinfo[i]);
                                  }

                              /* continue after the JSR call */
                              superblockend = false;
                          }
                          else {
                              /* We cannot proceed until the subroutine has been typechecked. */
                              /* XXX actually we would not have to check this block again */
                              bptr->flags = BBTYPECHECK_REACHED;
                              repeat = true;
                              superblockend = true;
                          }
#endif
                          /* XXX may throw? I don't think so. */
						  superblockend = true;
                          break;
                          
                      case ICMD_RET:
                          /* check returnAddress variable */
                          INDEX_ONEWORD(iptr->op1);
						  if (!typevectorset_checkretaddr(localset,iptr->op1))
                              panic("illegal instruction: RET using non-returnAddress variable");

						  repeat |= typestate_ret(localbuf,bptr,curstack,
												  localset,iptr->op1,numlocals);

#if 0
                          /* check if we are inside a subroutine */
                          if (!subroutine)
                              panic("RET outside of subroutine");

						  /* check if the right returnAddress is used (XXX is this too strict?) */
						  if ((basicblock*)TYPEINFO_RETURNADDRESS(vinfo[iptr->op1])
							  != subroutine->target)
							  panic("RET uses returnAddress of another subroutine");

                          /* determine which variables are touched by this subroutine */
                          /* and their types */
                          if (subroutine->sbr_touched) {
							  /* We have reached a RET in this subroutine before. */
							  
							  /* Check if there is more than one RET instruction
							   * returning from this subroutine. */
							  if (subroutine->sbr_ret != iptr)
								  panic("JSR subroutine has more than one RET instruction");

							  /* Merge the array of touched locals and their types */
                              for (i=0; i<numlocals; ++i)
                                  subroutine->sbr_touched[i] |= touched[i];
                              ttype = subroutine->sbr_vtype;
                              tinfo = subroutine->sbr_vinfo;
                              TYPECHECK_MERGEVARS;
							  /* XXX check if subroutine changed types? */
                          }
                          else {
							  /* This is the first time we reach a RET in this subroutine */
							  subroutine->sbr_ret = iptr;
                              subroutine->sbr_touched = DMNEW(u1,numlocals);
                              memcpy(subroutine->sbr_touched,touched,sizeof(u1)*numlocals);
                              subroutine->sbr_vtype = DMNEW(u1,numlocals);
                              memcpy(subroutine->sbr_vtype,vtype,sizeof(u1)*numlocals);
                              subroutine->sbr_vinfo = DMNEW(typeinfo,numlocals);
                              for (i=0; i<numlocals; ++i)
                                  if (vtype[i] == TYPE_ADR)
                                      TYPEINFO_CLONE(vinfo[i],subroutine->sbr_vinfo[i]);
                          }

                          LOGSTR("subroutine touches:");
                          DOLOG(typeinfo_print_locals(get_logfile(),subroutine->sbr_vtype,subroutine->sbr_vinfo,
                                                      subroutine->sbr_touched,numlocals));
                          LOGNL; LOGFLUSH;

                          /* XXX reach blocks after JSR statements */
                          for (i=0; i<block_count; ++i) {
                              tbptr = block + i;
                              LOG1("block L%03d",tbptr->debug_nr);
                              if (tbptr->iinstr[tbptr->icount - 1].opc != ICMD_JSR)
                                  continue;
                              LOG("ends with JSR");
                              if ((basicblock*) tbptr->iinstr[tbptr->icount - 1].target != subroutine->target)
                                  continue;
                              tbptr++;

                              LOG1("RET reaches block L%03d",tbptr->debug_nr);

                              /*TYPECHECK_REACH(REACH_RET);*/
                          }
#endif
                          
                          superblockend = true;
                          break;
							  
                          /****************************************/
                          /* INVOKATIONS                          */

                      case ICMD_INVOKEVIRTUAL:
                      case ICMD_INVOKESPECIAL:
                      case ICMD_INVOKESTATIC:
                      case ICMD_INVOKEINTERFACE:
                          {
                              methodinfo *mi = (methodinfo*) iptr->val.a;
							  bool specialmethod = (mi->name->text[0] == '<');
                              bool callinginit = (opcode == ICMD_INVOKESPECIAL && mi->name == name_init);
                              instruction *ins;
                              classinfo *initclass;

							  if (specialmethod && !callinginit)
								  panic("Invalid invocation of special method");

							  if (opcode == ICMD_INVOKESPECIAL) {
								  /* XXX for INVOKESPECIAL: check if the invokation is done at all */
								  
								  /* (If callinginit the class is checked later.) */
								  if (!callinginit) { 
 									  if (!builtin_isanysubclass(class,mi->class)) 
 										  panic("Illegal instruction: INVOKESPECIAL calling non-superclass method"); 
 								  } 
							  }

                              /* fetch parameter types and return type */
                              /* XXX might use dst->typeinfo directly if non void */
                              i = 0;
                              if (opcode != ICMD_INVOKESTATIC) {
                                  ptype[0] = TYPE_ADR;
                                  TYPEINFO_INIT_CLASSINFO(pinfo[0],mi->class);
                                  i++;
                              }
                              typeinfo_init_from_method_args(mi->descriptor,ptype+i,pinfo+i,
                                                             MAXPARAMS-i,false,
                                                             &rtype,&rinfo);

                              /* check parameter types */
                              srcstack = curstack;
                              i = mi->paramcount; /* number of parameters including 'this'*/
                              while (--i >= 0) {
                                  LOG1("param %d",i);
                                  if (srcstack->type != ptype[i])
                                      panic("Parameter type mismatch in method invocation");
                                  if (srcstack->type == TYPE_ADR) {
                                      LOGINFO(&(srcstack->typeinfo));
                                      LOGINFO(pinfo + i);
                                      if (i==0 && callinginit)
                                      {
										  /* typeinfo tempinfo; */
										  
                                          /* first argument to <init> method */
                                          if (!TYPEINFO_IS_NEWOBJECT(srcstack->typeinfo))
                                              panic("Calling <init> on initialized object");
                                          
                                          /* get the address of the NEW instruction */
                                          LOGINFO(&(srcstack->typeinfo));
                                          ins = (instruction*)TYPEINFO_NEWOBJECT_INSTRUCTION(srcstack->typeinfo);
                                          initclass = (ins) ? (classinfo*)ins[-1].val.a : method->class;
                                          LOGSTR("class: "); LOGSTRu(initclass->name); LOGNL;

										  /* check type */
										  /* (This is checked below.) */
/* 										  TYPEINFO_INIT_CLASSINFO(tempinfo,initclass); */
/*                                           if (!typeinfo_is_assignable(&tempinfo,pinfo+0)) */
/*                                               panic("Parameter reference type mismatch in <init> invocation"); */
                                      }
                                      else {
                                          if (!typeinfo_is_assignable(&(srcstack->typeinfo),pinfo+i))
                                              panic("Parameter reference type mismatch in method invocation");
                                      }
                                  }
                                  LOG("ok");

                                  if (i) srcstack = srcstack->prev;
                              }

							  /* XXX We should resolve the method and pass its
							   * class as implementingclass to is_accessible. */
							  if (!is_accessible(mi->flags,mi->class,NULL,
												 (opcode == ICMD_INVOKESTATIC) ? NULL
												 : &(srcstack->typeinfo)))
								  panic("Invoking unaccessible method");

							  LOG("checking return type");
                              if (rtype != TYPE_VOID) {
                                  if (rtype != dst->type)
                                      panic("Return type mismatch in method invocation");
                                  TYPEINFO_COPY(rinfo,dst->typeinfo);
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
                                          TYPEINFO_INIT_CLASSINFO(srcstack->typeinfo,initclass);
                                      }
                                      srcstack = srcstack->prev;
                                  }
                                  /* replace uninitialized object type in locals */
								  typevectorset_init_object(localset,ins,initclass,numlocals);

                                  /* initializing the 'this' reference? */
                                  if (!ins) {
#ifdef TYPECHECK_DEBUG
									  if (!initmethod)
										  panic("Internal error: calling <init> on this in non-<init> method.");
#endif
									  /* must be <init> of current class or direct superclass */
									  if (mi->class != class && mi->class != class->super)
										  panic("<init> calling <init> of the wrong class");
									  
                                      /* set our marker variable to type int */
                                      LOG("setting <init> marker");
									  typevectorset_store(localset,numlocals-1,TYPE_INT,NULL);
                                  }
								  else {
									  /* initializing an instance created with NEW */
									  /* XXX is this strictness ok? */
									  if (mi->class != initclass)
										  panic("Calling <init> method of the wrong class");
								  }
                              }
                          }
                          maythrow = true;
                          break;
                          
                      case ICMD_MULTIANEWARRAY:
						  {
							  vftbl *arrayvftbl;
							  arraydescriptor *desc;
							  
							  /* check the array lengths on the stack */
							  i = iptr[0].op1;
							  if (i<1) panic("MULTIANEWARRAY with dimensions < 1");
							  srcstack = curstack;
							  while (i--) {
								  if (!srcstack)
									  panic("MULTIANEWARRAY missing array length");
								  if (srcstack->type != TYPE_INT)
									  panic("MULTIANEWARRAY using non-int as array length");
								  srcstack = srcstack->prev;
							  }
							  
							  /* check array descriptor */
							  arrayvftbl = (vftbl*) iptr[0].val.a;
							  if (!arrayvftbl)
								  panic("MULTIANEWARRAY with unlinked class");
							  if ((desc = arrayvftbl->arraydesc) == NULL)
								  panic("MULTIANEWARRAY with non-array class");
							  if (desc->dimension < iptr[0].op1)
								  panic("MULTIANEWARRAY dimension to high");
							  
							  /* set the array type of the result */
							  TYPEINFO_INIT_CLASSINFO(dst->typeinfo,arrayvftbl->class);
						  }
                          maythrow = true;
                          break;
                          
                      case ICMD_BUILTIN3:
                          if (ISBUILTIN(BUILTIN_aastore)) {
							  TYPECHECK_ADR(curstack);
							  TYPECHECK_INT(curstack->prev);
							  TYPECHECK_ADR(curstack->prev->prev);
                              if (!TYPEINFO_MAYBE_ARRAY_OF_REFS(curstack->prev->prev->typeinfo))
                                  panic("illegal instruction: AASTORE to non-reference array");

                              /* XXX optimize */
                              /*
                                typeinfo_init_component(&curstack->prev->prev->typeinfo,&tempinfo);
                                if (!typeinfo_is_assignable(&curstack->typeinfo,&tempinfo))
                                panic("illegal instruction: AASTORE to incompatible type");
                              */
                          }
						  else {
							  /* XXX put these checks in a function */
							  builtindesc = builtin_desc;
							  while (builtindesc->opcode && builtindesc->builtin
									 != (functionptr) iptr->val.a) builtindesc++;
							  if (!builtindesc->opcode) {
								  dolog("Builtin not in table: %s",icmd_builtin_name((functionptr) iptr->val.a));
								  panic("Internal error: builtin not found in table");
							  }
							  TYPECHECK_ARGS3(builtindesc->type_s3,builtindesc->type_s2,builtindesc->type_s1);
						  }
                          maythrow = true; /* XXX better safe than sorry */
                          break;
                          
                      case ICMD_BUILTIN2:
                          if (ISBUILTIN(BUILTIN_newarray))
                          {
							  vftbl *vft;
							  TYPECHECK_INT(curstack->prev);
                              if (iptr[-1].opc != ICMD_ACONST)
                                  panic("illegal instruction: builtin_newarray without classinfo");
							  vft = (vftbl *)iptr[-1].val.a;
							  if (!vft)
								  panic("ANEWARRAY with unlinked class");
							  if (!vft->arraydesc)
								  panic("ANEWARRAY with non-array class");
                              TYPEINFO_INIT_CLASSINFO(dst->typeinfo,vft->class);
                          }
                          else if (ISBUILTIN(BUILTIN_arrayinstanceof))
                          {
							  vftbl *vft;
							  TYPECHECK_ADR(curstack->prev);
                              if (iptr[-1].opc != ICMD_ACONST)
                                  panic("illegal instruction: builtin_arrayinstanceof without classinfo");
							  vft = (vftbl *)iptr[-1].val.a;
							  if (!vft)
								  panic("INSTANCEOF with unlinked class");
							  if (!vft->arraydesc)
								  panic("internal error: builtin_arrayinstanceof with non-array class");
						  }
                          else if (ISBUILTIN(BUILTIN_checkarraycast)) {
							  vftbl *vft;
							  TYPECHECK_ADR(curstack->prev);
                              if (iptr[-1].opc != ICMD_ACONST)
                                  panic("illegal instruction: BUILTIN_checkarraycast without classinfo");
							  vft = (vftbl *)iptr[-1].val.a;
							  if (!vft)
								  panic("CHECKCAST with unlinked class");
							  if (!vft->arraydesc)
								  panic("internal error: builtin_checkarraycast with non-array class");
                              TYPEINFO_INIT_CLASSINFO(dst->typeinfo,vft->class);
                          }
						  else {
							  builtindesc = builtin_desc;
							  while (builtindesc->opcode && builtindesc->builtin
									 != (functionptr) iptr->val.a) builtindesc++;
							  if (!builtindesc->opcode) {
								  dolog("Builtin not in table: %s",icmd_builtin_name((functionptr) iptr->val.a));
								  panic("Internal error: builtin not found in table");
							  }
							  TYPECHECK_ARGS2(builtindesc->type_s2,builtindesc->type_s1);
						  }
                          maythrow = true; /* XXX better safe than sorry */
                          break;
                          
                      case ICMD_BUILTIN1:
                          if (ISBUILTIN(BUILTIN_new)) {
							  
                              if (iptr[-1].opc != ICMD_ACONST)
                                  panic("illegal instruction: builtin_new without classinfo");
							  cls = (classinfo *) iptr[-1].val.a;
							  if (!cls->linked)
								  panic("Internal error: NEW with unlinked class");
							  /* The following check also forbids array classes and interfaces: */
							  if ((cls->flags & ACC_ABSTRACT) != 0)
								  panic("Invalid instruction: NEW creating instance of abstract class");
                              TYPEINFO_INIT_NEWOBJECT(dst->typeinfo,iptr);
                          }
						  /* XXX unify the following cases? */
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
						  else {
							  builtindesc = builtin_desc;
							  while (builtindesc->opcode && builtindesc->builtin
									 != (functionptr) iptr->val.a) builtindesc++;
							  if (!builtindesc->opcode) {
								  dolog("Builtin not in table: %s",icmd_builtin_name((functionptr) iptr->val.a));
								  panic("Internal error: builtin not found in table");
							  }
							  TYPECHECK_ARGS1(builtindesc->type_s1);
						  }
                          maythrow = true; /* XXX better safe than sorry */
                          break;
                                                     
                          /****************************************/
                          /* UNCHECKED OPERATIONS                 */

                          /* These ops have no input or output to be checked */
                          /* (apart from the checks done in analyse_stack).  */
                                                /* XXX only add cases for them in debug mode? */

                      case ICMD_NOP:
                      case ICMD_POP:
                      case ICMD_POP2:
                          break;

                          /****************************************/
                          /* SIMPLE EXCEPTION THROWING TESTS      */

                      case ICMD_CHECKASIZE:
						  /* The argument to CHECKASIZE is typechecked by
						   * typechecking the array creation instructions. */

						  /* FALLTHROUGH! */
                      case ICMD_NULLCHECKPOP:
						  /* NULLCHECKPOP just requires that the stack top
						   * is an address. This is checked in stack.c */
						  
                          maythrow = true;
                          break;

                          /****************************************/
                          /* INSTRUCTIONS WHICH SHOULD HAVE BEEN  */
                          /* REPLACED BY OTHER OPCODES            */

                      case ICMD_NEW:
                      case ICMD_NEWARRAY:
                      case ICMD_ANEWARRAY:
                      case ICMD_MONITORENTER:
                      case ICMD_MONITOREXIT:
                      case ICMD_AASTORE:
                          /* XXX only check this in debug mode? */
                          LOG2("ICMD %d at %d\n", iptr->opc, (int)(iptr-instr));
						  LOG("Should have been converted to builtin function call.");
                          panic("Internal error: unexpected instruction encountered");
                          break;
                                                     
                      case ICMD_READONLY_ARG:
                      case ICMD_CLEAR_ARGREN:
                          /* XXX only check this in debug mode? */
                          LOG2("ICMD %d at %d\n", iptr->opc, (int)(iptr-instr));
						  LOG("Should have been replaced in stack.c.");
                          panic("Internal error: unexpected pseudo instruction encountered");
                          break;
						  
                          /****************************************/
                          /* ARITHMETIC AND CONVERSION            */

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
                          /* XXX only add cases for them in debug mode? */
                          
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
                      case ICMD_IREM0X10001:
                      case ICMD_LREM0X10001:
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

                          break;
                          
                          /****************************************/

                      default:
                          LOG2("ICMD %d at %d\n", iptr->opc, (int)(iptr-instr));
                          panic("Missing ICMD code during typecheck");
					}

                    /* the output of this instruction becomes the current stack */
                    curstack = dst;
                    
                    /* reach exception handlers for this instruction */
                    if (maythrow) {
                        LOG("reaching exception handlers");
                        i = 0;
                        while (handlers[i]) {
                            tbptr = handlers[i]->handler;
							repeat |= typestate_reach(localbuf,bptr,tbptr,
													  tbptr->instack,localset,numlocals);
                            i++;
                        }
                    }

                    /*
                      DOLOG(typeinfo_print_block(get_logfile(),curstack,numlocals,vtype,vinfo,(jsrchain) ? touched : NULL));
                      LOGNL; LOGFLUSH;
                    */
                    
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
                        if ((tbptr-block) >= block_count)
                            panic("Control flow falls off the last block");
#endif
                    }
                    TYPECHECK_REACH(REACH_STD);
                }
                
            } /* if block has to be checked */
            bptr++;
        } /* while blocks */
        
        LOGIF(repeat,"repeat=true");
    } while (repeat);

#ifdef TYPECHECK_STATISTICS
	dolog("Typechecker did %4d iterations",count_iterations);
#endif

#ifdef TYPECHECK_DEBUG
	for (i=0; i<block_count; ++i) {
		if (block[i].flags != BBDELETED
			&& block[i].flags != BBUNDEF
			&& block[i].flags != BBFINISHED
			&& block[i].flags != BBTYPECHECK_UNDEF) /* typecheck may never reach
													 * some exception handlers,
													 * that's ok. */
		{
			LOG2("block L%03d has invalid flags after typecheck: %d",
				 block[i].debug_nr,block[i].flags);
			panic("Invalid block flags after typecheck");
		}
	}
#endif
	
	/* Reset blocks we never reached */
	for (i=0; i<block_count; ++i) {
		if (block[i].flags == BBTYPECHECK_UNDEF)
			block[i].flags = BBFINISHED;
	}
		
    LOGimp("exiting typecheck");
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
 */
