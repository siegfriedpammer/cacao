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

   $Id: codegen.inc 1343 2004-07-21 16:12:00Z twisti $

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

		mte->startpc = asm_calljavafunction;
		mte->endpc = asm_calljavafunction2 - 1;

		avl_insert(methodtree, mte);

		mte = NEW(methodtree_element);

		mte->startpc = asm_calljavafunction2;
		mte->endpc = asm_call_jit_compiler - 1;

		avl_insert(methodtree, mte);
	}
#endif
}


/* codegen_setup **************************************************************

   allocates and initialises code area, data area and references

*******************************************************************************/

void codegen_setup(methodinfo *m)
{
	codegendata *cd;

	m->codegendata = NEW(codegendata);

	/* keep code size smaller */
	cd = m->codegendata;

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

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	cd->threadcritcurrent.next = NULL;
	cd->threadcritcount = 0;
#endif
}


/* codegen_close releases temporary code and data area                        */

void codegen_close(methodinfo *m)
{
	codegendata *cd;

	cd = m->codegendata;

	if (cd) {
		if (cd->mcodebase) {
			MFREE(cd->mcodebase, u1, cd->mcodesize);
			cd->mcodebase = NULL;
		}

		if (cd->dsegtop) {
			MFREE(cd->dsegtop - cd->dsegsize, u1, cd->dsegsize);
			cd->dsegtop = NULL;
		}

		FREE(m->codegendata, codegendata);
		m->codegendata = NULL;
	}
}


/* codegen_increase doubles code area                                         */

static s4 *codegen_increase(methodinfo *m, u1 *codeptr)
{
	codegendata *cd;
	long len;

	cd = m->codegendata;

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

static void dseg_increase(methodinfo *m)
{
	codegendata *cd;
	u1 *newstorage;

	cd = m->codegendata;

	newstorage = MNEW(u1, cd->dsegsize * 2);

	memcpy(newstorage + cd->dsegsize, cd->dsegtop - cd->dsegsize, cd->dsegsize);
	MFREE(cd->dsegtop - cd->dsegsize, u1, cd->dsegsize);

	cd->dsegtop = newstorage;
	cd->dsegsize *= 2;
	cd->dsegtop += cd->dsegsize;
}


static s4 dseg_adds4_increase(methodinfo *m, s4 value)
{
	codegendata *cd;

	cd = m->codegendata;

	dseg_increase(m);

	*((s4 *) (cd->dsegtop - cd->dseglen)) = value;

	return -(cd->dseglen);
}


static s4 dseg_adds4(methodinfo *m, s4 value)
{
	codegendata *cd;
	s4 *dataptr;

	cd = m->codegendata;

	cd->dseglen += 4;
	dataptr = (s4 *) (cd->dsegtop - cd->dseglen);

	if (cd->dseglen > cd->dsegsize)
		return dseg_adds4_increase(m, value);

	*dataptr = value;

	return -(cd->dseglen);
}


#if !defined(__I386__)
static s4 dseg_adds8_increase(methodinfo *m, s8 value)
{
	codegendata *cd;

	cd = m->codegendata;

	dseg_increase(m);

	*((s8 *) (cd->dsegtop - cd->dseglen)) = value;

	return -(cd->dseglen);
}


static s4 dseg_adds8(s8 value)
{
	codegendata *cd;
	s8 *dataptr;

	cd = m->codegendata;

	cd->dseglen = ALIGN(cd->dseglen + 8, 8);
	dataptr = (s8 *) (cd->dsegtop - cd->dseglen);

	if (cd->dseglen > cd->dsegsize)
		return dseg_adds8_increase(m, value);

	*dataptr = value;

	return -(cd->dseglen);
}
#endif


static s4 dseg_addfloat_increase(methodinfo *m, float value)
{
	codegendata *cd;

	cd = m->codegendata;

	dseg_increase(m);

	*((float *) (cd->dsegtop - cd->dseglen)) = value;

	return -(cd->dseglen);
}


static s4 dseg_addfloat(methodinfo *m, float value)
{
	codegendata *cd;
	float *dataptr;

	cd = m->codegendata;

	cd->dseglen += 4;
	dataptr = (float *) (cd->dsegtop - cd->dseglen);

	if (cd->dseglen > cd->dsegsize)
		return dseg_addfloat_increase(m, value);

	*dataptr = value;

	return -(cd->dseglen);
}


static s4 dseg_adddouble_increase(methodinfo *m, double value)
{
	codegendata *cd;

	cd = m->codegendata;

	dseg_increase(m);

	*((double *) (cd->dsegtop - cd->dseglen)) = value;

	return -(cd->dseglen);
}


static s4 dseg_adddouble(methodinfo *m, double value)
{
	codegendata *cd;
	double *dataptr;

	cd = m->codegendata;

	cd->dseglen = ALIGN(cd->dseglen + 8, 8);
	dataptr = (double *) (cd->dsegtop - cd->dseglen);

	if (cd->dseglen > cd->dsegsize)
		return dseg_adddouble_increase(m, value);

	*dataptr = value;

	return -(cd->dseglen);
}


static void dseg_addtarget(methodinfo *m, basicblock *target)
{
	codegendata *cd;
	jumpref *jr;

	cd = m->codegendata;
	
	jr = DNEW(jumpref);
	jr->tablepos = dseg_addaddress(m, NULL);
	jr->target = target;
	jr->next = cd->jumpreferences;
	cd->jumpreferences = jr;
}


static void dseg_adddata(methodinfo *m, u1 *ptr)
{
	codegendata *cd;
	dataref *dr;

	cd = m->codegendata;

	dr = DNEW(dataref);
	dr->pos = (u1 *) (ptr - cd->mcodebase);
	dr->next = cd->datareferences;
	cd->datareferences = dr;
}


static void dseg_addlinenumbertablesize(methodinfo *m)
{
	codegendata *cd;

	cd = m->codegendata;

#ifdef __ALPHA__
	dseg_adds4(0); /*PADDING*/
#endif
	cd->linenumbertablesizepos = dseg_addaddress(m, NULL); /*it could be considered to use adds4 here, to avoid 1 double word padding on ALPHA */

	cd->linenumbertablestartpos = dseg_addaddress(m, NULL);
#ifdef __ALPHA__
	dseg_adds4(0); /*PADDING*/
#endif
}


static void dseg_addlinenumber(methodinfo *m, u2 linenumber, u1 *ptr)
{
	codegendata *cd;
	linenumberref *lr;

	cd = m->codegendata;

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
void codegen_addreference(methodinfo *m, basicblock *target, void *branchptr)
#else
static void codegen_addreference(methodinfo *m, basicblock *target, void *branchptr)
#endif
{
	codegendata *cd;
	s4 branchpos;

	cd = m->codegendata;

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


static void codegen_addxboundrefs(methodinfo *m, void *branchptr, s4 reg)
{
	codegendata *cd;
	s4 branchpos;
	branchref *br;

	cd = m->codegendata;

	branchpos = (u1 *) branchptr - cd->mcodebase;

	br = DNEW(branchref);
	br->branchpos = branchpos;
	br->reg = reg;
	br->next = cd->xboundrefs;
	cd->xboundrefs = br;
}


static void codegen_addxcheckarefs(methodinfo *m, void *branchptr)
{
	codegendata *cd;
	s4 branchpos;
	branchref *br;

	cd = m->codegendata;

	branchpos = (u1 *) branchptr - cd->mcodebase;

	br = DNEW(branchref);
	br->branchpos = branchpos;
	br->next = cd->xcheckarefs;
	cd->xcheckarefs = br;
}


static void codegen_addxnullrefs(methodinfo *m, void *branchptr)
{
	codegendata *cd;
	s4 branchpos;
	branchref *br;

	cd = m->codegendata;

	branchpos = (u1 *) branchptr - cd->mcodebase;

	br = DNEW(branchref);
	br->branchpos = branchpos;
	br->next = cd->xnullrefs;
	cd->xnullrefs = br;
}



static void codegen_addxcastrefs(methodinfo *m, void *branchptr)
{
	codegendata *cd;
	s4 branchpos;
	branchref *br;

	cd = m->codegendata;

	branchpos = (u1 *) branchptr - cd->mcodebase;

	br = DNEW(branchref);
	br->branchpos = branchpos;
	br->next = cd->xcastrefs;
	cd->xcastrefs = br;
}


static void codegen_addxexceptionrefs(methodinfo *m, void *branchptr)
{
	codegendata *cd;
	s4 branchpos;
	branchref *br;

	cd = m->codegendata;

	branchpos = (u1 *) branchptr - cd->mcodebase;

	br = DNEW(branchref);
	br->branchpos = branchpos;
	br->next = cd->xexceptionrefs;
	cd->xexceptionrefs = br;
}


#if defined(__I386__) || defined(__X86_64__)
static void codegen_addxdivrefs(methodinfo *m, void *branchptr)
{
	codegendata *cd;
	s4 branchpos;
	branchref *br;

	cd = m->codegendata;

	branchpos = (u1 *) branchptr - cd->mcodebase;

	br = DNEW(branchref);
	br->branchpos = branchpos;
	br->next = cd->xdivrefs;
	cd->xdivrefs = br;
}
#endif


static void codegen_createlinenumbertable(methodinfo *m)
{
#ifdef __I386__
	codegendata *cd;
	linenumberref *lr;

	cd = m->codegendata;

	for (lr = cd->linenumberreferences; lr != NULL; lr = lr->next) {
		lr->tablepos = dseg_addaddress(m, NULL);

		if (cd->linenumbertab == 0)
			cd->linenumbertab = lr->tablepos;

		dseg_addaddress(m, lr->linenumber);
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
	if (mte->startpc <= mtepc->startpc && mtepc->startpc <= mte->endpc &&
		mte->startpc <= mtepc->endpc   && mtepc->endpc   <= mte->endpc) {
		return 0;

	} else if (mtepc->startpc < mte->startpc) {
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


void codegen_insertmethod(void *startpc, void *endpc)
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


void *codegen_findmethod(void *pc)
{
	methodtree_element *mtepc;
	methodtree_element *mte;

	mtepc = NEW(methodtree_element);
	mtepc->startpc = pc;
	mtepc->endpc = pc;

	mte = avl_find(methodtree, mtepc);

	FREE(mtepc, methodtree_element);

	if (!mte)
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "cannot find function");

	return mte->startpc;
}
#endif


static void codegen_finish(methodinfo *m, s4 mcodelen)
{
	codegendata *cd;
	jumpref *jr;
	u1 *epoint;
	s4 extralen = 0;
	s4 alignedlen;

	cd = m->codegendata;

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	extralen += sizeof(threadcritnode) * cd->threadcritcount;
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
	m->mcode = CNEW(u1, alignedlen + extralen);

	memcpy(m->mcode, cd->dsegtop - cd->dseglen, cd->dseglen);
	memcpy(m->mcode + cd->dseglen, cd->mcodebase, mcodelen);

	m->entrypoint = epoint = (u1 *) (m->mcode + cd->dseglen);

	/* jump table resolving */
	jr = cd->jumpreferences;
	while (jr != NULL) {
	    *((void**) (epoint + jr->tablepos)) = epoint + jr->target->mpc;
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
		codegen_insertmethod(m->entrypoint, m->entrypoint + mcodelen);

		/* data segment references resolving */
		dr = cd->datareferences;
		while (dr != NULL) {
			*((void**) ((long) epoint + (long) dr->pos - POINTERSIZE)) = epoint;
			dr = dr->next;
		}
	}
#endif

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	{
		threadcritnode *n = (threadcritnode *) (m->mcode + alignedlen);
		s4 i;
		threadcritnodetemp *nt = cd->threadcrit;

		for (i = 0; i < cd->threadcritcount; i++) {
			n->mcodebegin = m->mcode + nt->mcodebegin;
			n->mcodeend = m->mcode + nt->mcodeend;
			n->mcoderestart = m->mcode + nt->mcoderestart;
			thread_registercritical(n);
			n++;
			nt = nt->next;
		}
	}
#endif
}


void dseg_display(methodinfo *m)
{
	s4 *s4ptr;
	s4 i;
	
	s4ptr = (s4 *) m->mcode;

	printf("  --- dump of datasegment\n");
	for (i = m->codegendata->dseglen; i > 0 ; i -= 4) {
		printf("-%6x: %8x\n", i, (s4) (*s4ptr++));
	}
	printf("  --- begin of data segment: %p\n", s4ptr);
}


/* reg_of_var:
    This function determines a register, to which the result of an operation
    should go, when it is ultimatively intended to store the result in
    pseudoregister v.
    If v is assigned to an actual register, this register will be returned.
    Otherwise (when v is spilled) this function returns tempregnum.
    If not already done, regoff and flags are set in the stack location.
*/        

static int reg_of_var(methodinfo *m, stackptr v, int tempregnum)
{
	varinfo *var;

	switch (v->varkind) {
	case TEMPVAR:
		if (!(v->flags & INMEMORY))
			return(v->regoff);
		break;
	case STACKVAR:
		var = &(m->registerdata->interfaces[v->varnum][v->type]);
		v->regoff = var->regoff;
		if (!(var->flags & INMEMORY))
			return(var->regoff);
		break;
	case LOCALVAR:
		var = &(m->registerdata->locals[v->varnum][v->type]);
		v->regoff = var->regoff;
		if (!(var->flags & INMEMORY))
			return(var->regoff);
		break;
	case ARGVAR:
		v->regoff = v->varnum;
		if (IS_FLT_DBL_TYPE(v->type)) {
			if (v->varnum < m->registerdata->fltreg_argnum) {
				v->regoff = m->registerdata->argfltregs[v->varnum];
				return(m->registerdata->argfltregs[v->varnum]);
			}
		}
		else
			if (v->varnum < m->registerdata->intreg_argnum) {
				v->regoff = m->registerdata->argintregs[v->varnum];
				return(m->registerdata->argintregs[v->varnum]);
			}
		v->regoff -= m->registerdata->intreg_argnum;
		break;
	}
	v->flags |= INMEMORY;
	return tempregnum;
}


#if defined(USE_THREADS) && defined(NATIVE_THREADS)
static void codegen_threadcritrestart(methodinfo *m, int offset)
{
	m->codegendata->threadcritcurrent.mcoderestart = offset;
}

static void codegen_threadcritstart(methodinfo *m, int offset)
{
	m->codegendata->threadcritcurrent.mcodebegin = offset;
}

static void codegen_threadcritstop(methodinfo *m, int offset)
{
	codegendata *cd;

	cd = m->codegendata;

	cd->threadcritcurrent.next = cd->threadcrit;
	cd->threadcritcurrent.mcodeend = offset;
	cd->threadcrit = DNEW(threadcritnodetemp);
	*(cd->threadcrit) = cd->threadcritcurrent;
	cd->threadcritcount++;
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
