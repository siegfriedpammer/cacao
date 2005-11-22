/* src/vm/jit/codegen.inc - architecture independent code generator

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

   Authors: Reinhard Grafl
            Andreas  Krall

   Changes: Christian Thalinger
            Joseph Wenninger

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

   $Id: codegen.inc 3739 2005-11-22 23:05:51Z twisti $

*/


#include <assert.h>
#include <string.h>

#include "config.h"
#include "vm/types.h"

#include "mm/memory.h"
#include "toolbox/avl.h"
#include "toolbox/logging.h"
#include "native/jni.h"
#include "native/native.h"

#if defined(USE_THREADS)
# if defined(NATIVE_THREADS)
#  include "threads/native/threads.h"
# else
#  include "threads/green/threads.h"
# endif
#endif

#include "vm/exceptions.h"
#include "vm/method.h"
#include "vm/options.h"
#include "vm/statistics.h"
#include "vm/jit/codegen.inc.h"
#include "vm/jit/disass.h"
#include "vm/jit/jit.h"
#include "vm/jit/stacktrace.h"


/* in this tree we store all method addresses *********************************/

#if defined(__I386__) || defined(__X86_64__) || defined(ENABLE_INTRP) || defined(DISABLE_GC)
static struct avl_table *methodtree = NULL;
static int methodtree_comparator(const void *pc, const void *element,
								 void *param);
#endif


/* codegen_init ****************************************************************

   TODO

*******************************************************************************/

void codegen_init(void)
{
#if defined(__I386__) || defined(__X86_64__) || defined(ENABLE_INTRP) || defined(DISABLE_GC)
	/* this tree is global, not method specific */

	if (!methodtree) {
#if !defined(ENABLE_INTRP)
		methodtree_element *mte;
#endif

		methodtree = avl_create(methodtree_comparator, NULL, NULL);

#if !defined(ENABLE_INTRP)
		/* insert asm_calljavafunction */

		mte = NEW(methodtree_element);

		mte->startpc = (functionptr) asm_calljavafunction;
		mte->endpc = (functionptr) ((ptrint) asm_calljavafunction2 - 1);

		avl_insert(methodtree, mte);

		/* insert asm_calljavafunction2 */

		mte = NEW(methodtree_element);

		mte->startpc = (functionptr) asm_calljavafunction2;
		mte->endpc = (functionptr) ((ptrint) asm_call_jit_compiler - 1);

		avl_insert(methodtree, mte);
#endif /* !defined(ENABLE_INTRP) */
	}
#endif
}


/* codegen_setup **************************************************************

   allocates and initialises code area, data area and references

*******************************************************************************/

void codegen_setup(methodinfo *m, codegendata *cd, t_inlining_globals *id)
{
	cd->mcodebase = DMNEW(u1, MCODEINITSIZE);
	cd->mcodesize = MCODEINITSIZE;
	
	cd->dsegtop = DMNEW(u1, DSEGINITSIZE);
	cd->dsegsize = DSEGINITSIZE;
	cd->dsegtop += cd->dsegsize;
	cd->dseglen = 0;

	cd->jumpreferences = NULL;
	cd->datareferences = NULL;
	cd->xboundrefs = NULL;
	cd->xnullrefs = NULL;
	cd->xcastrefs = NULL;
	cd->xstorerefs = NULL;
	cd->xdivrefs = NULL;
	cd->xexceptionrefs = NULL;
	cd->patchrefs = NULL;

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
			cd->exceptiontable =
				DMNEW(exceptiontable, id->cumextablelength + 1);
		}

	} else if (id && (id->method->exceptiontablelength > 0)) {
		cd->exceptiontablelength = m->exceptiontablelength;
		cd->exceptiontable = DMNEW(exceptiontable, m->exceptiontablelength + 1);
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

   Releases temporary code and data area.

*******************************************************************************/

void codegen_free(methodinfo *m, codegendata *cd)
{
#if 0
	if (cd) {
		if (cd->mcodebase) {
			MFREE(cd->mcodebase, u1, cd->mcodesize);
			cd->mcodebase = NULL;
		}

		if (cd->dsegtop) {
			MFREE(cd->dsegtop - cd->dsegsize, u1, cd->dsegsize);
			cd->dsegtop = NULL;
		}
	}
#endif
}


/* codegen_close ***************************************************************

   TODO

*******************************************************************************/

void codegen_close(void)
{
	/* TODO: release avl tree on i386 and x86_64 */
}


/* codegen_increase ************************************************************

   Doubles code area.

*******************************************************************************/

static s4 *codegen_increase(codegendata *cd, u1 *mcodeptr)
{
	u1 *oldmcodebase;

	/* save old mcodebase pointer */

	oldmcodebase = cd->mcodebase;

	/* reallocate to new, doubled memory */

	cd->mcodebase = DMREALLOC(cd->mcodebase,
							  u1,
							  cd->mcodesize,
							  cd->mcodesize * 2);
	cd->mcodesize *= 2;
	cd->mcodeend = (s4 *) (cd->mcodebase + cd->mcodesize);

#if defined(__I386__) || defined(__MIPS__) || defined(__X86_64__)
	/* adjust the pointer to the last patcher position */

	cd->lastmcodeptr = cd->mcodebase + (cd->lastmcodeptr - oldmcodebase);
#endif

	/* return the new mcodeptr */

	return (s4 *) (cd->mcodebase + (mcodeptr - oldmcodebase));
}


/* desg_increase ***************************************************************

   Doubles data area.

*******************************************************************************/

static void dseg_increase(codegendata *cd)
{
	u1 *newstorage;

	newstorage = DMNEW(u1, cd->dsegsize * 2);

	MCOPY(newstorage + cd->dsegsize, cd->dsegtop - cd->dsegsize, u1,
		  cd->dsegsize);

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


#if !defined(__I386__) && !defined(__POWERPC__)
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


#if !defined(__XDSPCORE__)
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
#endif /* !defined(__XDSPCORE__) */


static void dseg_addtarget(codegendata *cd, basicblock *target)
{
	jumpref *jr;

	jr = DNEW(jumpref);
	jr->tablepos = dseg_addaddress(cd, NULL);
	jr->target = target;
	jr->next = cd->jumpreferences;
	cd->jumpreferences = jr;
}


#if defined(__I386__) || defined(__X86_64__)
static void dseg_adddata(codegendata *cd, u1 *ptr)
{
	dataref *dr;

	dr = DNEW(dataref);
	dr->pos = (u1 *) (ptr - cd->mcodebase);
	dr->next = cd->datareferences;
	cd->datareferences = dr;
}
#endif


/* dseg_addlinenumbertablesize *************************************************

   XXX

*******************************************************************************/

static void dseg_addlinenumbertablesize(codegendata *cd)
{
#if SIZEOF_VOID_P == 8
	/* 4-byte ALIGNMENT PADDING */

	dseg_adds4(cd, 0);
#endif

	cd->linenumbertablesizepos = dseg_addaddress(cd, NULL);
	cd->linenumbertablestartpos = dseg_addaddress(cd, NULL);

#if SIZEOF_VOID_P == 8
	/* 4-byte ALIGNMENT PADDING */

	dseg_adds4(cd, 0);
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

#if !defined(ENABLE_INTRP)
	/* The interpreter uses absolute branches, so we do branch resolving */
	/* after the code and data segment move. */

	/* Check if the target basicblock has already a start pc, so the jump is  */
	/* backward and we can resolve it immediately.                            */

	if (target->mpc >= 0) {
		gen_resolvebranch((u1 *) cd->mcodebase + branchpos,
						  branchpos,
						  target->mpc);

	} else
#endif
	{
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


/* codegen_addxstorerefs *******************************************************

   Adds an ArrayStoreException branch to the list.

*******************************************************************************/

static void codegen_addxstorerefs(codegendata *cd, void *branchptr)
{
	s4         branchpos;
	branchref *br;

	branchpos = (u1 *) branchptr - cd->mcodebase;

	br = DNEW(branchref);
	br->branchpos = branchpos;
	br->next = cd->xstorerefs;
	cd->xstorerefs = br;
}


/* codegen_addxdivrefs *********************************************************

   Adds an ArithmeticException branch to the list.

*******************************************************************************/

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


/* codegen_addpatchref *********************************************************

   Adds a new patcher reference to the list of patching positions.

*******************************************************************************/

static void codegen_addpatchref(codegendata *cd, voidptr branchptr,
								functionptr patcher, voidptr ref, s4 disp)
{
	patchref *pr;
	s4        branchpos;

	branchpos = (u1 *) branchptr - cd->mcodebase;

	pr = DNEW(patchref);

	pr->branchpos = branchpos;
	pr->patcher   = patcher;
	pr->ref       = ref;
	pr->disp      = disp;

	pr->next      = cd->patchrefs;
	cd->patchrefs = pr;

#if defined(__I386__) || defined(__MIPS__) || defined(__X86_64__)
	/* On some architectures the patcher stub call instruction might
	be longer than the actual instruction generated.  On this
	architectures we store the last patcher call position and after
	the basic block code generation is completed, we check the range
	and maybe generate some nop's. */

	cd->lastmcodeptr = ((u1 *) branchptr) + PATCHER_CALL_SIZE;
#endif
}


/* codegen_createlinenumbertable ***********************************************

   Creates a line number table in the data segment from the created
   entries in linenumberreferences.

*******************************************************************************/

static void codegen_createlinenumbertable(codegendata *cd)
{
	linenumberref *lr;

	for (lr = cd->linenumberreferences; lr != NULL; lr = lr->next) {
		lr->tablepos = dseg_addaddress(cd, NULL);

		if (cd->linenumbertab == 0)
			cd->linenumbertab = lr->tablepos;

		dseg_addaddress(cd, lr->linenumber);
	}
}


#if defined(__I386__) || defined(__X86_64__) || defined(ENABLE_INTRP) || defined(DISABLE_GC)
/* methodtree_comparator *******************************************************

   XXX

*******************************************************************************/

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


/* codegen_insertmethod ********************************************************

   XXX

*******************************************************************************/

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
		assert(0);
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "duplicate entry");
	}

#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
	tables_unlock();
#endif
#endif
}


/* codegen_findmethod **********************************************************

   XXX

*******************************************************************************/

functionptr codegen_findmethod(functionptr pc)
{
	methodtree_element  mtepc;
	methodtree_element *mte;

#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
	tables_lock();
#endif
#endif

	mtepc.startpc = pc;
	mtepc.endpc = pc;

	mte = avl_find(methodtree, &mtepc);

	if (!mte) {
#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
		tables_unlock();
#endif
#endif
		printf("Cannot find Java function at %p\n", (void *) (ptrint) pc);
		assert(0);
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Cannot find Java function at %p", pc);
	}

#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
	tables_unlock();
#endif
#endif

	return mte->startpc;
}
#endif /* defined(__I386__) || defined(__X86_64__) || defined(ENABLE_INTRP) || defined(DISABLE_GC) */


/* codegen_finish **************************************************************

   Finishes the code generation. A new memory, large enough for both
   data and code, is allocated and data and code are copied together
   to their final layout, unresolved jumps are resolved, ...

*******************************************************************************/

static void codegen_finish(methodinfo *m, codegendata *cd, s4 mcodelen)
{
	jumpref     *jr;
	functionptr  epoint;
	s4           extralen;
	s4           alignedlen;

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

	/* allocate new memory */

	m->mcodelength = mcodelen + cd->dseglen;
	m->mcode = (functionptr) (ptrint) CNEW(u1, alignedlen + extralen);

	/* copy data and code to their new location */

	MCOPY((void *) (ptrint) m->mcode, cd->dsegtop - cd->dseglen, u1,
		  cd->dseglen);
	MCOPY((void *) ((ptrint) m->mcode + cd->dseglen), cd->mcodebase, u1,
		  mcodelen);

	m->entrypoint = epoint = (functionptr) ((ptrint) m->mcode + cd->dseglen);

	/* jump table resolving */

	jr = cd->jumpreferences;
	while (jr != NULL) {
		*((functionptr *) ((ptrint) epoint + jr->tablepos)) =
			(functionptr) ((ptrint) epoint + (ptrint) jr->target->mpc);
		jr = jr->next;
	}

	/* line number table resolving */
	{
		linenumberref *lr;
		ptrint lrtlen = 0;

		for (lr = cd->linenumberreferences; lr != NULL; lr = lr->next) {
			lrtlen++;
			*((functionptr *) ((ptrint) epoint + (ptrint) lr->tablepos)) =
				(functionptr) ((ptrint) epoint + (ptrint) lr->targetmpc);
		}
		
		*((functionptr *) ((ptrint) epoint + cd->linenumbertablestartpos)) =
			(functionptr) ((ptrint) epoint + cd->linenumbertab);

		*((ptrint *) ((ptrint) epoint + cd->linenumbertablesizepos)) = lrtlen;
	}

#if defined(__I386__) || defined(__X86_64__) || defined(ENABLE_INTRP) || defined(DISABLE_GC)
	/* add method into methodtree to find the entrypoint */

	codegen_insertmethod(m->entrypoint,
						 (functionptr) ((ptrint) m->entrypoint + mcodelen));
#endif

#if defined(__I386__) || defined(__X86_64__) || defined(__XDSPCORE__)
	{
		dataref *dr;

		/* data segment references resolving */

		dr = cd->datareferences;
		while (dr != NULL) {
			*((functionptr *) ((ptrint) epoint + (ptrint) dr->pos -
							   SIZEOF_VOID_P)) = epoint;
			dr = dr->next;
		}
	}
#endif

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	{
		threadcritnode *n = (threadcritnode *) ((ptrint) m->mcode + alignedlen);
		s4 i;
		threadcritnodetemp *nt = cd->threadcrit;

		for (i = 0; i < cd->threadcritcount; i++) {
			n->mcodebegin = (u1 *) (ptrint) m->mcode + nt->mcodebegin;
			n->mcodeend = (u1 *) (ptrint) m->mcode + nt->mcodeend;
			n->mcoderestart = (u1 *) (ptrint) m->mcode + nt->mcoderestart;
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
	
	s4ptr = (s4 *) (ptrint) m->mcode;

	printf("  --- dump of datasegment\n");
	for (i = cd->dseglen; i > 0 ; i -= 4) {
#if SIZEOF_VOID_P == 8
		printf("0x%016lx:   -%6x (%6d): %8x\n",
			   (ptrint) s4ptr, i, i, (s4) *s4ptr);
#else
		printf("0x%08x:   -%6x (%6d): %8x\n",
			   (ptrint) s4ptr, i, i, (s4) *s4ptr);
#endif
		s4ptr++;
	}

	printf("  --- begin of data segment: %p\n", (void *) s4ptr);
}



/* codegen_createnativestub ****************************************************

   Wrapper for createnativestub.

*******************************************************************************/

functionptr codegen_createnativestub(functionptr f, methodinfo *m)
{
	codegendata        *cd;
	registerdata       *rd;
	t_inlining_globals *id;
	s4                  dumpsize;
	methoddesc         *md;
	methoddesc         *nmd;	
	s4                  nativeparams;

	/* mark dump memory */

	dumpsize = dump_size();

	cd = DNEW(codegendata);
	rd = DNEW(registerdata);
	id = DNEW(t_inlining_globals);

	/* setup code generation stuff */

	inlining_setup(m, id);
#if !defined(ENABLE_INTRP)
	reg_setup(m, rd, id);
#endif
	codegen_setup(m, cd, id);
						
	/* create new method descriptor with additional native parameters */

	md = m->parseddesc;
	nativeparams = (m->flags & ACC_STATIC) ? 2 : 1;
	
	nmd = (methoddesc *) DMNEW(u1, sizeof(methoddesc) - sizeof(typedesc) +
							   md->paramcount * sizeof(typedesc) +
							   nativeparams * sizeof(typedesc));

	nmd->paramcount = md->paramcount + nativeparams;

	nmd->params = DMNEW(paramdesc, nmd->paramcount);

	nmd->paramtypes[0].type = TYPE_ADR; /* add environment pointer            */

	if (m->flags & ACC_STATIC)
		nmd->paramtypes[1].type = TYPE_ADR; /* add class pointer              */

	MCOPY(nmd->paramtypes + nativeparams, md->paramtypes, typedesc,
		  md->paramcount);

	md_param_alloc(nmd);

	/* generate the code */

	m->entrypoint = createnativestub(f, m, cd, rd, nmd);

#if defined(STATISTICS)
	if (opt_stat)
		count_nstub_len += m->mcodelength;
#endif

	/* disassemble native stub */

	if (opt_shownativestub) {
		codegen_disassemble_nativestub(m,
									   (u1 *) (ptrint) m->entrypoint,
									   (u1 *) (ptrint) m->entrypoint + (m->mcodelength - cd->dseglen));

		/* show data segment */

		if (opt_showddatasegment)
			dseg_display(m, cd);
	}

	/* release memory */

	dump_release(dumpsize);

	/* return native stub entry point */

	return m->entrypoint;
}


/* codegen_disassemble_nativestub **********************************************

   Disassembles the generated native stub.

*******************************************************************************/

void codegen_disassemble_nativestub(methodinfo *m, u1 *start, u1 *end)
{
	printf("Native stub: ");
	utf_fprint_classname(stdout, m->class->name);
	printf(".");
	utf_fprint(stdout, m->name);
	utf_fprint(stdout, m->descriptor);
	printf("\n\nLength: %d\n\n", (s4) (end - start));

	disassemble(start, end);
}


/* codegen_start_native_call ***************************************************

   Prepares the stuff required for a native (JNI) function call:

   - adds a stackframe info structure to the chain, for stacktraces
   - prepares the local references table on the stack

   The layout of the native stub stackframe should look like this:

   +---------------------------+ <- SP (of parent Java function)
   | return address            |
   +---------------------------+
   |                           |
   | stackframe info structure |
   |                           |
   +---------------------------+
   |                           |
   | local references table    |
   |                           |
   +---------------------------+
   |                           |
   | arguments (if any)        |
   |                           |
   +---------------------------+ <- SP (native stub)

*******************************************************************************/

void codegen_start_native_call(u1 *datasp, u1 *pv, u1 *sp, u1 *ra)
{
	stackframeinfo *sfi;
	localref_table *lrt;

	/* get data structures from stack */

	sfi = (stackframeinfo *) (datasp - sizeof(stackframeinfo));
	lrt = (localref_table *) (datasp - sizeof(stackframeinfo) - 
							  sizeof(localref_table));

	/* add a stackframeinfo to the chain */

	stacktrace_create_native_stackframeinfo(sfi, pv, sp, ra);

	/* add current JNI local references table to this thread */

	lrt->capacity    = LOCALREFTABLE_CAPACITY;
	lrt->used        = 0;
	lrt->localframes = 1;
	lrt->prev        = LOCALREFTABLE;

	/* clear the references array (memset is faster the a for-loop) */

	MSET(lrt->refs, 0, java_objectheader*, LOCALREFTABLE_CAPACITY);

	LOCALREFTABLE = lrt;
}


/* codegen_finish_native_call **************************************************

   Removes the stuff required for a native (JNI) function call.

*******************************************************************************/

void codegen_finish_native_call(u1 *datasp)
{
	stackframeinfo  *sfi;
	stackframeinfo **psfi;
	localref_table  *lrt;
	s4               localframes;

	/* get data structures from stack */

	sfi = (stackframeinfo *) (datasp - sizeof(stackframeinfo));
	lrt = (localref_table *) (datasp - sizeof(stackframeinfo) - 
							  sizeof(localref_table));

	/* remove current stackframeinfo from chain */

	psfi = STACKFRAMEINFO;

	*psfi = sfi->prev;

	/* release JNI local references tables for this thread */

	lrt = LOCALREFTABLE;

	/* got through all current local frames */

	for (localframes = lrt->localframes; localframes >= 1; localframes--) {
		lrt = lrt->prev;
	}

	/* now store the previous local frames in the thread structure */

	LOCALREFTABLE = lrt;
}


/* removecompilerstub **********************************************************

   Deletes a compilerstub from memory (simply by freeing it).

*******************************************************************************/

void removecompilerstub(functionptr stub)
{
	/* pass size 1 to keep the intern function happy */

	CFREE((void *) (ptrint) stub, 1);
}


/* removenativestub ************************************************************

   Removes a previously created native-stub from memory.
    
*******************************************************************************/

void removenativestub(functionptr stub)
{
	/* pass size 1 to keep the intern function happy */

	CFREE((void *) (ptrint) stub, 1);
}


/* reg_of_var:
    This function determines a register, to which the result of an operation
    should go, when it is ultimatively intended to store the result in
    pseudoregister v.
    If v is assigned to an actual register, this register will be returned.
    Otherwise (when v is spilled) this function returns tempregnum.
    If not already done, regoff and flags are set in the stack location.
*/        
/*  On ARM we have to check if a long/double variable is splitted
    across reg/stack (HIGH_REG == REG_SPLIT). We return the actual register
    of v for LOW_REG and the tempregnum for HIGH_REG in such cases.
    (michi 2005/07/24)
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
		if (!(var->flags & INMEMORY)) {
#if defined(__ARM__) && defined(__ARMEL__)
			if (IS_2_WORD_TYPE(v->type) && (GET_HIGH_REG(var->regoff) == REG_SPLIT))
				return(PACK_REGS(GET_LOW_REG(var->regoff), GET_HIGH_REG(tempregnum)));
#endif
#if defined(__ARM__) && defined(__ARMEB__)
			if (IS_2_WORD_TYPE(v->type) && (GET_LOW_REG(var->regoff) == REG_SPLIT))
				return(PACK_REGS(GET_LOW_REG(tempregnum), GET_HIGH_REG(var->regoff)));
#endif
			return(var->regoff);
		}
		break;

	case ARGVAR:
		if (!(v->flags & INMEMORY)) {
#if defined(__ARM__) && defined(__ARMEL__)
			if (IS_2_WORD_TYPE(v->type) && (GET_HIGH_REG(v->regoff) == REG_SPLIT))
				return(PACK_REGS(GET_LOW_REG(v->regoff), GET_HIGH_REG(tempregnum)));
#endif
#if defined(__ARM__) && defined(__ARMEB__)
			if (IS_2_WORD_TYPE(v->type) && (GET_LOW_REG(v->regoff) == REG_SPLIT))
				return(PACK_REGS(GET_LOW_REG(tempregnum), GET_HIGH_REG(v->regoff)));
#endif
			return(v->regoff);
		}
		break;
	}
#ifdef STATISTICS
	if (opt_stat)
		count_spills_read++;
#endif
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
