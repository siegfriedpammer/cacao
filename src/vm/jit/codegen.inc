/* jit/codegen.inc - architecture independent code generator

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   Institut f. Computersprachen, TU Wien
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser, M. Probst,
   S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich,
   J. Wenninger

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
            Andreas  Krall

   Changes: Michael Gschwind
            Christian Thalinger

   All functions assume the following code area / data area layout:

   +-----------+
   |           |
   | code area | code area grows to higher addresses
   |           |
   +-----------+ <-- start of procedure
   |           |
   | data area | data area grows to lower addresses
   |           |
   +-----------+

   The functions first write into a temporary code/data area allocated by
   "codegen_init". "codegen_finish" copies the code and data area into permanent
   memory. All functions writing values into the data area return the offset
   relative the begin of the code area (start of procedure).	

   $Id: codegen.inc 1503 2004-11-14 12:58:52Z twisti $

*/


#include <string.h>
#include "exceptions.h"
#include "native.h"
#include "options.h"
#include "statistics.h"
#include "jit/codegen.inc.h"
#include "toolbox/memory.h"
#include "toolbox/logging.h"
#include "toolbox/avl.h"
#include "threads/thread.h"
#ifndef STATIC_CLASSPATH
#include <dlfcn.h>
#endif

/* in this tree we store all method addresses */

#if defined(__I386__) || defined(__X86_64__)
static struct avl_table *methodtree = NULL;
static int methodtree_comparator(const void *pc, const void *element,
								 void *param);
#endif


/* codegen_init ****************************************************************

   TODO

*******************************************************************************/

void codegen_init()
{
#if defined(__I386__) || defined(__X86_64__)
	/* this tree is global, not method specific */
	if (!methodtree) {
		methodtree_element *mte;

		methodtree = avl_create(methodtree_comparator, NULL, NULL);

		mte = NEW(methodtree_element);

		mte->startpc = (functionptr) asm_calljavafunction;
		mte->endpc = (functionptr) ((long) asm_calljavafunction2 - 1);

		avl_insert(methodtree, mte);

		mte = NEW(methodtree_element);

		mte->startpc = (functionptr) asm_calljavafunction2;
		mte->endpc = (functionptr) ((long) asm_call_jit_compiler - 1);

		avl_insert(methodtree, mte);
	}
#endif
}


/* codegen_setup **************************************************************

   allocates and initialises code area, data area and references

*******************************************************************************/

void codegen_setup(methodinfo *m, codegendata *cd, t_inlining_globals *id)
{
	cd->mcodebase = MNEW(u1, MCODEINITSIZE);
	cd->mcodesize = MCODEINITSIZE;
	
	cd->dsegtop = MNEW(u1, DSEGINITSIZE);
	cd->dsegsize = DSEGINITSIZE;
	cd->dsegtop += cd->dsegsize;
	cd->dseglen = 0;

	cd->jumpreferences = NULL;
	cd->datareferences = NULL;
	cd->xboundrefs = NULL;
	cd->xcheckarefs = NULL;
	cd->xnullrefs = NULL;
	cd->xcastrefs = NULL;
	cd->xdivrefs = NULL;
	cd->xexceptionrefs = NULL;

	cd->linenumberreferences = NULL;
	cd->linenumbertablesizepos = 0;
	cd->linenumbertablestartpos = 0;
	cd->linenumbertab = 0;
	
	cd->method = m;
	cd->exceptiontable = 0;
	cd->exceptiontablelength = 0;

	if (useinlining && id) {
		if (id->cumextablelength > 0) {
			cd->exceptiontablelength = id->cumextablelength;
			cd->exceptiontable  = DMNEW(exceptiontable, id->cumextablelength + 1);
		}

	} else if (id && (id->method->exceptiontablelength >0)) {
		cd->exceptiontablelength = m->exceptiontablelength;
		cd->exceptiontable  = DMNEW(exceptiontable, m->exceptiontablelength + 1);
	}

	if (id) {
		cd->maxstack = id->cummaxstack;
		cd->maxlocals = id->cumlocals;
	} else {
		cd->maxstack = m->maxstack;
		cd->maxlocals = m->maxlocals;
	}

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	cd->threadcritcurrent.next = NULL;
	cd->threadcritcount = 0;
#endif
}


/* codegen_free ****************************************************************

   releases temporary code and data area

*******************************************************************************/

void codegen_free(methodinfo *m, codegendata *cd)
{
	if (cd) {
#if 0
		if (cd->exceptiontablelength) {
			cd->exceptiontablelength = m->exceptiontablelength;
			MFREE(cd->exceptiontable, exceptiontable, cd->exceptiontablelength + 1);
			cd->exceptiontable = 0;
			cd->exceptiontablelength = 0;
		}
#endif

		if (cd->mcodebase) {
			MFREE(cd->mcodebase, u1, cd->mcodesize);
			cd->mcodebase = NULL;
		}

		if (cd->dsegtop) {
			MFREE(cd->dsegtop - cd->dsegsize, u1, cd->dsegsize);
			cd->dsegtop = NULL;
		}
	}
}


/* codegen_close ***************************************************************

   TODO

*******************************************************************************/

void codegen_close()
{
	/* TODO: release avl tree on i386 and x86_64 */
}


/* codegen_increase doubles code area                                         */

static s4 *codegen_increase(codegendata *cd, u1 *codeptr)
{
	long len;

	len = codeptr - cd->mcodebase;
	cd->mcodebase = MREALLOC(cd->mcodebase,
							 u1,
							 cd->mcodesize,
							 cd->mcodesize * 2);
	cd->mcodesize *= 2;
	cd->mcodeend = (s4 *) (cd->mcodebase + cd->mcodesize);

	return (s4 *) (cd->mcodebase + len);
}


/* desg_increase doubles data area                                            */

static void dseg_increase(codegendata *cd)
{
	u1 *newstorage;

	newstorage = MNEW(u1, cd->dsegsize * 2);

	memcpy(newstorage + cd->dsegsize, cd->dsegtop - cd->dsegsize, cd->dsegsize);
	MFREE(cd->dsegtop - cd->dsegsize, u1, cd->dsegsize);

	cd->dsegtop = newstorage;
	cd->dsegsize *= 2;
	cd->dsegtop += cd->dsegsize;
}


static s4 dseg_adds4_increase(codegendata *cd, s4 value)
{
	dseg_increase(cd);

	*((s4 *) (cd->dsegtop - cd->dseglen)) = value;

	return -(cd->dseglen);
}


static s4 dseg_adds4(codegendata *cd, s4 value)
{
	s4 *dataptr;

	cd->dseglen += 4;
	dataptr = (s4 *) (cd->dsegtop - cd->dseglen);

	if (cd->dseglen > cd->dsegsize)
		return dseg_adds4_increase(cd, value);

	*dataptr = value;

	return -(cd->dseglen);
}


#if !defined(__I386__)
static s4 dseg_adds8_increase(codegendata *cd, s8 value)
{
	dseg_increase(cd);

	*((s8 *) (cd->dsegtop - cd->dseglen)) = value;

	return -(cd->dseglen);
}


static s4 dseg_adds8(codegendata *cd, s8 value)
{
	s8 *dataptr;

	cd->dseglen = ALIGN(cd->dseglen + 8, 8);
	dataptr = (s8 *) (cd->dsegtop - cd->dseglen);

	if (cd->dseglen > cd->dsegsize)
		return dseg_adds8_increase(cd, value);

	*dataptr = value;

	return -(cd->dseglen);
}
#endif


static s4 dseg_addfloat_increase(codegendata *cd, float value)
{
	dseg_increase(cd);

	*((float *) (cd->dsegtop - cd->dseglen)) = value;

	return -(cd->dseglen);
}


static s4 dseg_addfloat(codegendata *cd, float value)
{
	float *dataptr;

	cd->dseglen += 4;
	dataptr = (float *) (cd->dsegtop - cd->dseglen);

	if (cd->dseglen > cd->dsegsize)
		return dseg_addfloat_increase(cd, value);

	*dataptr = value;

	return -(cd->dseglen);
}


static s4 dseg_adddouble_increase(codegendata *cd, double value)
{
	dseg_increase(cd);

	*((double *) (cd->dsegtop - cd->dseglen)) = value;

	return -(cd->dseglen);
}


static s4 dseg_adddouble(codegendata *cd, double value)
{
	double *dataptr;

	cd->dseglen = ALIGN(cd->dseglen + 8, 8);
	dataptr = (double *) (cd->dsegtop - cd->dseglen);

	if (cd->dseglen > cd->dsegsize)
		return dseg_adddouble_increase(cd, value);

	*dataptr = value;

	return -(cd->dseglen);
}


static void dseg_addtarget(codegendata *cd, basicblock *target)
{
	jumpref *jr;

	jr = DNEW(jumpref);
	jr->tablepos = dseg_addaddress(cd, NULL);
	jr->target = target;
	jr->next = cd->jumpreferences;
	cd->jumpreferences = jr;
}


static void dseg_adddata(codegendata *cd, u1 *ptr)
{
	dataref *dr;

	dr = DNEW(dataref);
	dr->pos = (u1 *) (ptr - cd->mcodebase);
	dr->next = cd->datareferences;
	cd->datareferences = dr;
}


static void dseg_addlinenumbertablesize(codegendata *cd)
{
#ifdef __ALPHA__
	dseg_adds4(cd, 0); /*PADDING*/
#endif
	cd->linenumbertablesizepos = dseg_addaddress(cd, NULL); /*it could be considered to use adds4 here, to avoid 1 double word padding on ALPHA */

	cd->linenumbertablestartpos = dseg_addaddress(cd, NULL);
#ifdef __ALPHA__
	dseg_adds4(cd, 0); /*PADDING*/
#endif
}


static void dseg_addlinenumber(codegendata *cd, u2 linenumber, u1 *ptr)
{
	linenumberref *lr;

	lr = DNEW(linenumberref);
	lr->linenumber = linenumber;
	lr->tablepos = 0;
	lr->targetmpc = (ptr - cd->mcodebase);
	lr->next = cd->linenumberreferences;
	cd->linenumberreferences = lr;
}


/* we need this function externally on i386 and x86_64, but keep the call fast
   on alpha... */

#if defined(__I386__) || defined(__X86_64__)
void codegen_addreference(codegendata *cd, basicblock *target, void *branchptr)
#else
static void codegen_addreference(codegendata *cd, basicblock *target, void *branchptr)
#endif
{
	s4 branchpos;

	branchpos = (u1 *) branchptr - cd->mcodebase;

	if (target->mpc >= 0) {
		gen_resolvebranch((u1 *) cd->mcodebase + branchpos,
						  branchpos,
						  target->mpc);

	} else {
		branchref *br = DNEW(branchref);

		br->branchpos = branchpos;
		br->next = target->branchrefs;
		target->branchrefs = br;
	}
}


static void codegen_addxboundrefs(codegendata *cd, void *branchptr, s4 reg)
{
	s4 branchpos;
	branchref *br;

	branchpos = (u1 *) branchptr - cd->mcodebase;

	br = DNEW(branchref);
	br->branchpos = branchpos;
	br->reg = reg;
	br->next = cd->xboundrefs;
	cd->xboundrefs = br;
}


static void codegen_addxcheckarefs(codegendata *cd, void *branchptr)
{
	s4 branchpos;
	branchref *br;

	branchpos = (u1 *) branchptr - cd->mcodebase;

	br = DNEW(branchref);
	br->branchpos = branchpos;
	br->next = cd->xcheckarefs;
	cd->xcheckarefs = br;
}


static void codegen_addxnullrefs(codegendata *cd, void *branchptr)
{
	s4 branchpos;
	branchref *br;

	branchpos = (u1 *) branchptr - cd->mcodebase;

	br = DNEW(branchref);
	br->branchpos = branchpos;
	br->next = cd->xnullrefs;
	cd->xnullrefs = br;
}


static void codegen_addxcastrefs(codegendata *cd, void *branchptr)
{
	s4 branchpos;
	branchref *br;

	branchpos = (u1 *) branchptr - cd->mcodebase;

	br = DNEW(branchref);
	br->branchpos = branchpos;
	br->next = cd->xcastrefs;
	cd->xcastrefs = br;
}


static void codegen_addxexceptionrefs(codegendata *cd, void *branchptr)
{
	s4 branchpos;
	branchref *br;

	branchpos = (u1 *) branchptr - cd->mcodebase;

	br = DNEW(branchref);
	br->branchpos = branchpos;
	br->next = cd->xexceptionrefs;
	cd->xexceptionrefs = br;
}


#if defined(__I386__) || defined(__X86_64__)
static void codegen_addxdivrefs(codegendata *cd, void *branchptr)
{
	s4 branchpos;
	branchref *br;

	branchpos = (u1 *) branchptr - cd->mcodebase;

	br = DNEW(branchref);
	br->branchpos = branchpos;
	br->next = cd->xdivrefs;
	cd->xdivrefs = br;
}
#endif


static void codegen_createlinenumbertable(codegendata *cd)
{
#ifdef __I386__
	linenumberref *lr;

	for (lr = cd->linenumberreferences; lr != NULL; lr = lr->next) {
		lr->tablepos = dseg_addaddress(cd, NULL);

		if (cd->linenumbertab == 0)
			cd->linenumbertab = lr->tablepos;

		dseg_addaddress(cd, lr->linenumber);
	}
#endif
}


#if defined(__I386__) || defined(__X86_64__)
static int methodtree_comparator(const void *pc, const void *element, void *param)
{
	methodtree_element *mte;
	methodtree_element *mtepc;

	mte = (methodtree_element *) element;
	mtepc = (methodtree_element *) pc;

	/* compare both startpc and endpc of pc, even if they have the same value,
	   otherwise the avl_probe sometimes thinks the element is already in the
	   tree */
	if ((long) mte->startpc <= (long) mtepc->startpc &&
		(long) mtepc->startpc <= (long) mte->endpc &&
		(long) mte->startpc <= (long) mtepc->endpc &&
		(long) mtepc->endpc <= (long) mte->endpc) {
		return 0;

	} else if ((long) mtepc->startpc < (long) mte->startpc) {
		return -1;

	} else {
		return 1;
	}
}


#if 0
void *codegen_findmethod1(void *pc)
{
	void * retVal=findmethod(pc);
	methodinfo **ma=(methodinfo**)retVal;
	methodinfo *m=ma[-1];
	if (m)
		if (m->name)
			utf_display(m->name);
		else 
			log_text("No Name");
	else log_text("No methodinfo");
	return retVal;
}
#endif


void codegen_insertmethod(functionptr startpc, functionptr endpc)
{
	methodtree_element *mte;

#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
	tables_lock();
#endif
#endif

	mte = NEW(methodtree_element);
	mte->startpc = startpc;
	mte->endpc = endpc;

	if (avl_insert(methodtree, mte)) {
#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
		tables_unlock();
#endif
#endif
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "duplicate entry");
	}

#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
	tables_unlock();
#endif
#endif
}


functionptr codegen_findmethod(functionptr pc)
{
	methodtree_element *mtepc;
	methodtree_element *mte;

#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
	tables_lock();
#endif
#endif

	mtepc = NEW(methodtree_element);
	mtepc->startpc = pc;
	mtepc->endpc = pc;

	mte = avl_find(methodtree, mtepc);

	FREE(mtepc, methodtree_element);

	if (!mte) {
#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
		tables_unlock();
#endif
#endif
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "cannot find function");
	}

#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
	tables_unlock();
#endif
#endif

	return mte->startpc;
}
#endif


static void codegen_finish(methodinfo *m, codegendata *cd, s4 mcodelen)
{
	jumpref *jr;
	functionptr epoint;
	s4 extralen;
	s4 alignedlen;

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	extralen = sizeof(threadcritnode) * cd->threadcritcount;
#else
	extralen = 0;
#endif

#if defined(STATISTICS)
	if (opt_stat) {
		count_code_len += mcodelen;
		count_data_len += cd->dseglen;
	}
#endif

	cd->dseglen = ALIGN(cd->dseglen, MAX_ALIGN);
	alignedlen = ALIGN(mcodelen, MAX_ALIGN) + cd->dseglen;

	m->mcodelength = mcodelen + cd->dseglen;
	m->mcode = (functionptr) (long) CNEW(u1, alignedlen + extralen);

	memcpy((void *) (long) m->mcode, cd->dsegtop - cd->dseglen, cd->dseglen);
	memcpy((void *) ((long) m->mcode + cd->dseglen), cd->mcodebase, mcodelen);

	m->entrypoint = epoint = (functionptr) ((long) m->mcode + cd->dseglen);

	/* jump table resolving */
	jr = cd->jumpreferences;
	while (jr != NULL) {
		*((functionptr *) ((long) epoint + jr->tablepos)) =
			(functionptr) ((long) epoint + (long) jr->target->mpc);
		jr = jr->next;
	}

#ifdef __I386__
	/* line number table resolving */
	{
		linenumberref *lr;
#if POINTERSIZE == 8
		s8 lrtlen = 0;
#else
		s4 lrtlen = 0;
#endif

		for (lr = cd->linenumberreferences; lr != NULL; lr = lr->next) {
			lrtlen++;
			*((void**) (epoint + lr->tablepos)) = epoint + lr->targetmpc;
		}
		
		*((void**) (epoint + cd->linenumbertablestartpos)) =
			epoint + cd->linenumbertab;
#if POINTERSIZE == 8
		*((s8*) (epoint + cd->linenumbertablesizepos)) = lrtlen;
#else
		*((s4*) (epoint + cd->linenumbertablesizepos)) = lrtlen;
#endif
	}
#endif

#if defined(__I386__) || defined(__X86_64__)
	{
		dataref *dr;

		/* add method into methodtree to find the entrypoint */
		codegen_insertmethod(m->entrypoint,
							 (functionptr) ((long) m->entrypoint + mcodelen));

		/* data segment references resolving */
		dr = cd->datareferences;
		while (dr != NULL) {
			*((functionptr *) ((long) epoint + (long) dr->pos - POINTERSIZE)) =
				epoint;
			dr = dr->next;
		}
	}
#endif

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	{
		threadcritnode *n = (threadcritnode *) ((long) m->mcode + alignedlen);
		s4 i;
		threadcritnodetemp *nt = cd->threadcrit;

		for (i = 0; i < cd->threadcritcount; i++) {
			n->mcodebegin = (u1 *) (long) m->mcode + nt->mcodebegin;
			n->mcodeend = (u1 *) (long) m->mcode + nt->mcodeend;
			n->mcoderestart = (u1 *) (long) m->mcode + nt->mcoderestart;
			thread_registercritical(n);
			n++;
			nt = nt->next;
		}
	}
#endif
}


void dseg_display(methodinfo *m, codegendata *cd)
{
	s4 *s4ptr;
	s4 i;
	
	s4ptr = (s4 *) (long) m->mcode;

	printf("  --- dump of datasegment\n");
	for (i = cd->dseglen; i > 0 ; i -= 4) {
		printf("-%6x: %8x\n", i, (s4) (*s4ptr++));
	}
	printf("  --- begin of data segment: %p\n", (void *) s4ptr);
}


/* reg_of_var:
    This function determines a register, to which the result of an operation
    should go, when it is ultimatively intended to store the result in
    pseudoregister v.
    If v is assigned to an actual register, this register will be returned.
    Otherwise (when v is spilled) this function returns tempregnum.
    If not already done, regoff and flags are set in the stack location.
*/        

static int reg_of_var(registerdata *rd, stackptr v, int tempregnum)
{
	varinfo *var;

	switch (v->varkind) {
	case TEMPVAR:
		if (!(v->flags & INMEMORY))
			return(v->regoff);
		break;
	case STACKVAR:
		var = &(rd->interfaces[v->varnum][v->type]);
		v->regoff = var->regoff;
		if (!(var->flags & INMEMORY))
			return(var->regoff);
		break;
	case LOCALVAR:
		var = &(rd->locals[v->varnum][v->type]);
		v->regoff = var->regoff;
		if (!(var->flags & INMEMORY))
			return(var->regoff);
		break;
	case ARGVAR:
		v->regoff = v->varnum;
		if (IS_FLT_DBL_TYPE(v->type)) {
			if (v->varnum < rd->fltreg_argnum) {
				v->regoff = rd->argfltregs[v->varnum];
				return(rd->argfltregs[v->varnum]);
			}

		} else {
#if defined(__POWERPC__)
			if (v->varnum < rd->intreg_argnum - (IS_2_WORD_TYPE(v->type) != 0)) {
#else
			if (v->varnum < rd->intreg_argnum) {
#endif
				v->regoff = rd->argintregs[v->varnum];
				return (rd->argintregs[v->varnum]);
			}
		}
#if defined(__POWERPC__)
		v->regoff += 6;
#else
		v->regoff -= rd->intreg_argnum;
#endif
		break;
	}
	v->flags |= INMEMORY;
	return tempregnum;
}


#if defined(USE_THREADS) && defined(NATIVE_THREADS)
static void codegen_threadcritrestart(codegendata *cd, int offset)
{
	cd->threadcritcurrent.mcoderestart = offset;
}


static void codegen_threadcritstart(codegendata *cd, int offset)
{
	cd->threadcritcurrent.mcodebegin = offset;
}


static void codegen_threadcritstop(codegendata *cd, int offset)
{
	cd->threadcritcurrent.next = cd->threadcrit;
	cd->threadcritcurrent.mcodeend = offset;
	cd->threadcrit = DNEW(threadcritnodetemp);
	*(cd->threadcrit) = cd->threadcritcurrent;
	cd->threadcritcount++;
}
#endif


#ifndef STATIC_CLASSPATH
static size_t codegen_overloadPartLen(utf *desc) {
        char *utf_ptr=desc->text;
        u2 c;
	size_t len=2;
        while ((c=utf_nextu2(&utf_ptr))!=')') {
                switch (c) {
                        case 'I':
                        case 'S':
                        case 'B':
                        case 'C':
                        case 'Z':
                        case 'J':
                        case 'F':
                        case 'D':
				len ++;
				break;
                        case '[':
                                len = len+2;
                                break;
                        case 'L':
                                len++;
                                while ( (c=utf_nextu2(&utf_ptr)) != ';')
                                        len++;
                                len=len+2;
                                break;
                        case '(':
                                break;
                        default: panic ("invalid method descriptor");
                }
        }
	return len;

}

static void codegen_fillInOverloadPart(char *target,utf *desc) {
        char *utf_ptr=desc->text;
        u2 c;
	char* insertpos=&target[strlen(target)];
	*(insertpos++)='_';
	*(insertpos++)='_';
        while ((c=utf_nextu2(&utf_ptr))!=')') {
                switch (c) {
                        case 'I':
                        case 'S':
                        case 'B':
                        case 'C':
                        case 'Z':
                        case 'J':
                        case 'F':
                        case 'D':
                                *(insertpos++)=c;
                                break;
                        case '[':
				*(insertpos++)='_';
				*(insertpos++)='3';
                                break;
                        case 'L':
				*(insertpos++)='L';
                                while ( (c=utf_nextu2(&utf_ptr)) != ';')
                                	if ( ((c>='a') && (c<='z'))  ||
						((c>='A') && (c<='Z')) ||
						((c>='0') && (c<='9')) )
							*(insertpos++)=c;
					else *(insertpos++)='_';
				*(insertpos++)='_';
				*(insertpos++)='2';
                                break;
                        case '(':
                                break;
                        default: panic ("invalid method descriptor");
                }
        }
	*insertpos='\0';

}

static void codegen_resolve_native(methodinfo *m,void **insertionPoint,void *jmpTarget,void **jmpPatchTarget) {
  char *nativeName, *nativeNameEscape;
  size_t nativeLen;
  size_t i;
	void *lib;
	void *sym;

  builtin_monitorenter((java_objectheader*) m);
  if ((*jmpPatchTarget)==jmpTarget) {
    builtin_monitorexit((java_objectheader*) m);
    return;
  }
  log_text("trying to resolve a native method");
  utf_display(m->class->name);
  utf_display(m->name);
  
  lib=dlopen(0,RTLD_NOW | RTLD_GLOBAL);
  if (lib) {
    int ok=0;
    /*generate the name of the native function in the form Java_package1_package2...._classname_methodname*/
    nativeLen=/*Java_*/5+strlen(m->class->name->text)+/*_*/1+strlen(m->name->text)+/*\0*/1;
    nativeName=MNEW(char,nativeLen);
    sprintf(nativeName,"Java_%s/%s",m->class->name->text,m->name->text);
	i=5;
    while (i<nativeLen) {
		if (nativeName[i]=='_') { /* escape underscore */
			nativeNameEscape = MNEW(char,nativeLen+1);
			memcpy(nativeNameEscape,nativeName,i+1);
			nativeNameEscape[i+1] = '1'; /* escape sequence for _ is _1 */
			memcpy(&nativeNameEscape[i+2],&nativeName[i+1],nativeLen-i-1);
			MFREE(nativeName,char,nativeLen);
			i++;
			nativeLen++;
			nativeNameEscape[nativeLen]=0;
			nativeName=nativeNameEscape;
		}
		if (nativeName[i]=='/') nativeName[i]='_';
		i++;
    } 

/*	printf("nativename: %s\n",nativeName); */

    sym=dlsym(lib,nativeName);
    if (sym) {
      ok=1;
      log_text("resolved");
      MFREE(nativeName,char,nativeLen);
    } else {
      size_t overloadedNativeLen=nativeLen+codegen_overloadPartLen(m->descriptor);
      char *overloadedNative=MNEW(char,overloadedNativeLen);
      sprintf(overloadedNative,"%s",nativeName);
      MFREE(nativeName,char,nativeLen);
      codegen_fillInOverloadPart(overloadedNative,m->descriptor);
      log_text("symbol not found,trying harder (overloaded member ?)");
      sym=dlsym(lib,overloadedNative);
      if (sym) {
        MFREE(overloadedNative,char,overloadedNativeLen);
        ok=1;
        log_text("resolved");
      } else { 
         MFREE(overloadedNative,char,overloadedNativeLen);
         log_text("It was not possible to find the native function implementation. Not even in overloading case");
      }
   }
    if (ok) {
      (*insertionPoint)=sym;
      (*jmpPatchTarget)=jmpTarget;
      builtin_monitorexit((java_objectheader *) m );
      return;
    }

  } else log_text("library not found");
  

  {
    char *info;
    size_t slen;
    slen=2+strlen(m->class->name->text)+strlen(m->name->text)+strlen(m->descriptor->text);
    info=(char*)MNEW(char,slen);
    sprintf(info,"%s.%s%s",m->class->name->text,m->name->text,m->descriptor->text);
    
    builtin_monitorexit((java_objectheader *) m );
    throw_cacao_exception_exit(string_java_lang_LinkageError,
                                                       info);
  }
  
}
#endif


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
