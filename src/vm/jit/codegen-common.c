/* src/vm/jit/codegen-common.c - architecture independent code generator stuff

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

   Authors: Reinhard Grafl
            Andreas  Krall

   Changes: Christian Thalinger
            Joseph Wenninger
			Edwin Steiner

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

   $Id: codegen-common.c 4598 2006-03-14 22:16:47Z edwin $

*/


#include "config.h"

#include <assert.h>
#include <string.h>

#include "vm/types.h"

#if defined(ENABLE_JIT)
/* this is required for gen_resolvebranch and PATCHER_CALL_SIZE */
# include "codegen.h"
#endif

#if defined(__ARM__)
/* this is required for REG_SPLIT */
# include "md-abi.h"
#endif

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
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/codegen-common.h"
#include "vm/jit/disass.h"
#include "vm/jit/dseg.h"
#include "vm/jit/jit.h"
#include "vm/jit/stacktrace.h"


/* in this tree we store all method addresses *********************************/

#if defined(__I386__) || defined(__X86_64__) || defined(ENABLE_INTRP) || defined(DISABLE_GC)
static avl_tree *methodtree = NULL;
static s4 methodtree_comparator(const void *pc, const void *element);
#endif


/* codegen_init ****************************************************************

   TODO

*******************************************************************************/

void codegen_init(void)
{
#if defined(__I386__) || defined(__X86_64__) || defined(ENABLE_INTRP) || defined(DISABLE_GC)
	/* this tree is global, not method specific */

	if (!methodtree) {
#if defined(ENABLE_JIT)
		methodtree_element *mte;
#endif

		methodtree = avl_create(&methodtree_comparator);

#if defined(ENABLE_JIT)
		/* insert asm_vm_call_method */

		mte = NEW(methodtree_element);

		mte->startpc = (u1 *) (ptrint) asm_vm_call_method;
		mte->endpc   = (u1 *) ((ptrint) asm_call_jit_compiler - 1);

		avl_insert(methodtree, mte);
#endif /* defined(ENABLE_JIT) */
	}
#endif /* defined(__I386__) || defined(__X86_64__) || defined(ENABLE_INTRP) || defined(DISABLE_GC) */
}


/* codegen_setup **************************************************************

   allocates and initialises code area, data area and references

*******************************************************************************/

void codegen_setup(methodinfo *m, codegendata *cd)
{
	cd->mcodebase = DMNEW(u1, MCODEINITSIZE);
	cd->mcodesize = MCODEINITSIZE;

	/* initialize mcode variables */
	
	cd->mcodeptr = cd->mcodebase;
	cd->mcodeend = (s4 *) (cd->mcodebase + MCODEINITSIZE);

#if defined(ENABLE_INTRP)
	/* native dynamic superinstructions variables */

	if (opt_intrp) {
		cd->ncodebase = DMNEW(u1, NCODEINITSIZE);
		cd->ncodesize = NCODEINITSIZE;

		/* initialize ncode variables */
	
		cd->ncodeptr = cd->ncodebase;

		cd->lastinstwithoutdispatch = ~0; /* no inst without dispatch */
		cd->superstarts = NULL;
	}
#endif
	
	cd->dsegtop = DMNEW(u1, DSEGINITSIZE);
	cd->dsegsize = DSEGINITSIZE;
	cd->dsegtop += cd->dsegsize;
	cd->dseglen = 0;

	cd->jumpreferences = NULL;

#if defined(__I386__) || defined(__X86_64__) || defined(__XDSPCORE__) || defined(ENABLE_INTRP)
	cd->datareferences = NULL;
#endif

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
	cd->code = code_codeinfo_new(m); /* XXX check allocation */
	cd->exceptiontable = 0;
	cd->exceptiontablelength = 0;

	if (m->exceptiontablelength > 0) {
		cd->exceptiontablelength = m->exceptiontablelength;
		cd->exceptiontable = DMNEW(exceptiontable, m->exceptiontablelength + 1);
	}

	cd->maxstack = m->maxstack;
	cd->maxlocals = m->maxlocals;

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

s4 *codegen_increase(codegendata *cd, u1 *mcodeptr)
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
	cd->mcodeend   = (s4 *) (cd->mcodebase + cd->mcodesize);

#if defined(__I386__) || defined(__MIPS__) || defined(__X86_64__) || defined(ENABLE_INTRP)
	/* adjust the pointer to the last patcher position */

	if (cd->lastmcodeptr != NULL)
		cd->lastmcodeptr = cd->mcodebase + (cd->lastmcodeptr - oldmcodebase);
#endif

	/* return the new mcodeptr */

	return (s4 *) (cd->mcodebase + (mcodeptr - oldmcodebase));
}


/* codegen_ncode_increase ******************************************************

   Doubles code area.

*******************************************************************************/

#if defined(ENABLE_INTRP)
u1 *codegen_ncode_increase(codegendata *cd, u1 *ncodeptr)
{
	u1 *oldncodebase;

	/* save old ncodebase pointer */

	oldncodebase = cd->ncodebase;

	/* reallocate to new, doubled memory */

	cd->ncodebase = DMREALLOC(cd->ncodebase,
							  u1,
							  cd->ncodesize,
							  cd->ncodesize * 2);
	cd->ncodesize *= 2;

	/* return the new ncodeptr */

	return (cd->ncodebase + (ncodeptr - oldncodebase));
}
#endif


void codegen_addreference(codegendata *cd, basicblock *target, void *branchptr)
{
	s4 branchpos;

	branchpos = (u1 *) branchptr - cd->mcodebase;

#if defined(ENABLE_JIT)
	/* Check if the target basicblock has already a start pc, so the
	   jump is backward and we can resolve it immediately. */

	/* The interpreter uses absolute branches, so we do branch
	   resolving after the code and data segment move. */

	if (target->mpc >= 0
# if defined(ENABLE_INTRP)
		&& !opt_intrp
# endif
		)
	{
		gen_resolvebranch((u1 *) cd->mcodebase + branchpos,
						  branchpos,
						  target->mpc);

	} else
#endif
	{
		branchref *br = DNEW(branchref);

		br->branchpos = branchpos;
		br->next      = target->branchrefs;

		target->branchrefs = br;
	}
}


/* codegen_addxboundrefs *******************************************************

   Adds an ArrayIndexOutOfBoundsException branch to the list.

*******************************************************************************/

void codegen_addxboundrefs(codegendata *cd, void *branchptr, s4 reg)
{
	s4 branchpos;
	branchref *br;

	branchpos = (u1 *) branchptr - cd->mcodebase;

	br = DNEW(branchref);

	br->branchpos = branchpos;
	br->reg       = reg;
	br->next      = cd->xboundrefs;

	cd->xboundrefs = br;
}


/* codegen_addxcastrefs ********************************************************

   Adds an ClassCastException branch to the list.

*******************************************************************************/

void codegen_addxcastrefs(codegendata *cd, void *branchptr)
{
	s4         branchpos;
	branchref *br;

	branchpos = (u1 *) branchptr - cd->mcodebase;

	br = DNEW(branchref);

	br->branchpos = branchpos;
	br->next      = cd->xcastrefs;

	cd->xcastrefs = br;
}


/* codegen_addxdivrefs *********************************************************

   Adds an ArithmeticException branch to the list.

*******************************************************************************/

void codegen_addxdivrefs(codegendata *cd, void *branchptr)
{
	s4         branchpos;
	branchref *br;

	branchpos = (u1 *) branchptr - cd->mcodebase;

	br = DNEW(branchref);

	br->branchpos = branchpos;
	br->next      = cd->xdivrefs;

	cd->xdivrefs = br;
}


/* codegen_addxstorerefs *******************************************************

   Adds an ArrayStoreException branch to the list.

*******************************************************************************/

void codegen_addxstorerefs(codegendata *cd, void *branchptr)
{
	s4         branchpos;
	branchref *br;

	branchpos = (u1 *) branchptr - cd->mcodebase;

	br = DNEW(branchref);

	br->branchpos = branchpos;
	br->next      = cd->xstorerefs;

	cd->xstorerefs = br;
}


/* codegen_addxnullrefs ********************************************************

   Adds an NullPointerException branch to the list.

*******************************************************************************/

void codegen_addxnullrefs(codegendata *cd, void *branchptr)
{
	s4         branchpos;
	branchref *br;

	branchpos = (u1 *) branchptr - cd->mcodebase;

	br = DNEW(branchref);

	br->branchpos = branchpos;
	br->next      = cd->xnullrefs;

	cd->xnullrefs = br;
}


/* codegen_addxexceptionsrefs **************************************************

   Adds an common exception branch to the list.

*******************************************************************************/

void codegen_addxexceptionrefs(codegendata *cd, void *branchptr)
{
	s4 branchpos;
	branchref *br;

	branchpos = (u1 *) branchptr - cd->mcodebase;

	br = DNEW(branchref);

	br->branchpos = branchpos;
	br->next      = cd->xexceptionrefs;

	cd->xexceptionrefs = br;
}


/* codegen_addpatchref *********************************************************

   Adds a new patcher reference to the list of patching positions.

*******************************************************************************/

void codegen_addpatchref(codegendata *cd, voidptr branchptr,
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

#if defined(ENABLE_JIT) && (defined(__I386__) || defined(__MIPS__) || defined(__X86_64__))
	/* On some architectures the patcher stub call instruction might
	   be longer than the actual instruction generated.  On this
	   architectures we store the last patcher call position and after
	   the basic block code generation is completed, we check the
	   range and maybe generate some nop's. */

	cd->lastmcodeptr = ((u1 *) branchptr) + PATCHER_CALL_SIZE;
#endif
}


#if defined(__I386__) || defined(__X86_64__) || defined(ENABLE_INTRP) || defined(DISABLE_GC)
/* methodtree_comparator *******************************************************

   XXX

*******************************************************************************/

static s4 methodtree_comparator(const void *pc, const void *element)
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

void codegen_insertmethod(u1 *startpc, u1 *endpc)
{
	methodtree_element *mte;

	/* allocate new method entry */

	mte = NEW(methodtree_element);

	mte->startpc = startpc;
	mte->endpc   = endpc;

	/* this function does not return an error, but asserts for
	   duplicate entries */

	avl_insert(methodtree, mte);
}


/* codegen_findmethod **********************************************************

   XXX

*******************************************************************************/

u1 *codegen_findmethod(u1 *pc)
{
	methodtree_element  mtepc;
	methodtree_element *mte;

	/* allocation of the search structure on the stack is much faster */

	mtepc.startpc = pc;
	mtepc.endpc   = pc;

	mte = avl_find(methodtree, &mtepc);

	if (!mte) {
		printf("Cannot find Java function at %p\n", (void *) (ptrint) pc);
		assert(0);

		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Cannot find Java function at %p", pc);
	}

	return mte->startpc;
}
#endif /* defined(__I386__) || defined(__X86_64__) || defined(ENABLE_INTRP) || defined(DISABLE_GC) */


/* codegen_finish **************************************************************

   Finishes the code generation. A new memory, large enough for both
   data and code, is allocated and data and code are copied together
   to their final layout, unresolved jumps are resolved, ...

*******************************************************************************/

void codegen_finish(methodinfo *m, codegendata *cd, s4 mcodelen)
{
#if 0
	s4       mcodelen;
#endif
#if defined(ENABLE_INTRP)
	s4       ncodelen;
#endif
	s4       alignedmcodelen;
	jumpref *jr;
	u1      *epoint;
	s4       extralen;
	s4       alignedlen;
	codeinfo *code;

	code = cd->code;

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	extralen = sizeof(threadcritnode) * cd->threadcritcount;
#else
	extralen = 0;
#endif

#if defined(ENABLE_STATISTICS)
	if (opt_stat) {
		count_code_len += mcodelen;
		count_data_len += cd->dseglen;
	}
#endif

#if 0
	mcodelen = cd->mcodeptr - cd->mcodebase;
#endif
	alignedmcodelen = ALIGN(mcodelen, MAX_ALIGN);

#if defined(ENABLE_INTRP)
	if (opt_intrp) {
		ncodelen = cd->ncodeptr - cd->ncodebase;
	}
#endif

	cd->dseglen = ALIGN(cd->dseglen, MAX_ALIGN);
	alignedlen = alignedmcodelen + cd->dseglen;

#if defined(ENABLE_INTRP)
	if (opt_intrp) {
		alignedlen += ncodelen;
	}
#endif

	/* allocate new memory */

	code->mcodelength = mcodelen + cd->dseglen;
	code->mcode = CNEW(u1, alignedlen + extralen);

	/* copy data and code to their new location */

	MCOPY((void *) code->mcode, cd->dsegtop - cd->dseglen, u1, cd->dseglen);
	MCOPY((void *) (code->mcode + cd->dseglen), cd->mcodebase, u1, mcodelen);

	/* set the entrypoint of the method */
	
	assert(code->entrypoint == NULL);
	code->entrypoint = epoint = (code->mcode + cd->dseglen);

#if defined(ENABLE_INTRP)
	/* relocate native dynamic superinstruction code (if any) */

	if (opt_intrp) {
		cd->mcodebase = code->entrypoint;

		if (ncodelen > 0) {
			u1 *ncodebase = code->mcode + cd->dseglen + alignedmcodelen;
			MCOPY((void *)ncodebase, cd->ncodebase, u1, ncodelen);
			/* XXX cacheflush((void *)ncodebase, ncodelen); */
			/* set some cd variables for dynamic_super_rerwite */

			cd->ncodebase = ncodebase;

		} else {
			cd->ncodebase = NULL;
		}

		dynamic_super_rewrite(cd);
	}
#endif

	/* jump table resolving */

	for (jr = cd->jumpreferences; jr != NULL; jr = jr->next)
		*((functionptr *) ((ptrint) epoint + jr->tablepos)) =
			(functionptr) ((ptrint) epoint + (ptrint) jr->target->mpc);

	/* line number table resolving */
	{
		linenumberref *lr;
		ptrint lrtlen = 0;
		ptrint target;

		for (lr = cd->linenumberreferences; lr != NULL; lr = lr->next) {
			lrtlen++;
			target = lr->targetmpc;
			/* if the entry contains an mcode pointer (normal case), resolve it */
			/* (see doc/inlining_stacktrace.txt for details)                    */
			if (lr->linenumber >= -2) {
			    target += (ptrint) epoint;
			}
			*((functionptr *) ((ptrint) epoint + (ptrint) lr->tablepos)) = 
				(functionptr) target;
		}
		
		*((functionptr *) ((ptrint) epoint + cd->linenumbertablestartpos)) =
			(functionptr) ((ptrint) epoint + cd->linenumbertab);

		*((ptrint *) ((ptrint) epoint + cd->linenumbertablesizepos)) = lrtlen;
	}

#if defined(__I386__) || defined(__X86_64__) || defined(ENABLE_INTRP) || defined(DISABLE_GC)
	/* add method into methodtree to find the entrypoint */

	codegen_insertmethod(code->entrypoint, code->entrypoint + mcodelen);
#endif


#if defined(__I386__) || defined(__X86_64__) || defined(__XDSPCORE__) || defined(ENABLE_INTRP)
	/* resolve data segment references */

	dseg_resolve_datareferences(cd, m);
#endif


#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	{
		threadcritnode *n = (threadcritnode *) ((ptrint) code->mcode + alignedlen);
		s4 i;
		threadcritnodetemp *nt = cd->threadcrit;

		for (i = 0; i < cd->threadcritcount; i++) {
			n->mcodebegin = (u1 *) (ptrint) code->mcode + nt->mcodebegin;
			n->mcodeend = (u1 *) (ptrint) code->mcode + nt->mcodeend;
			n->mcoderestart = (u1 *) (ptrint) code->mcode + nt->mcoderestart;
			thread_registercritical(n);
			n++;
			nt = nt->next;
		}
	}
#endif
}


/* codegen_createnativestub ****************************************************

   Wrapper for createnativestub.

   Returns:
       the codeinfo representing the stub code.

*******************************************************************************/

codeinfo *codegen_createnativestub(functionptr f, methodinfo *m)
{
	codegendata        *cd;
	registerdata       *rd;
	s4                  dumpsize;
	methoddesc         *md;
	methoddesc         *nmd;	
	s4                  nativeparams;
	codeinfo           *code;

	/* mark dump memory */

	dumpsize = dump_size();

	cd = DNEW(codegendata);
	rd = DNEW(registerdata);

	/* setup code generation stuff */

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (!opt_intrp)
# endif
		reg_setup(m, rd);
#endif

	codegen_setup(m, cd);

	code = cd->code;
						
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

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (!opt_intrp)
# endif
		md_param_alloc(nmd);
#endif

	/* generate the code */

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (opt_intrp)
		code->entrypoint = intrp_createnativestub(f, m, cd, rd, nmd);
	else
# endif
		code->entrypoint = createnativestub(f, m, cd, rd, nmd);
#else
	code->entrypoint = intrp_createnativestub(f, m, cd, rd, nmd);
#endif

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		count_nstub_len += code->mcodelength;
#endif

	/* disassemble native stub */

	if (opt_shownativestub) {
		codegen_disassemble_nativestub(m,
									   (u1 *) (ptrint) code->entrypoint,
									   (u1 *) (ptrint) code->entrypoint + (code->mcodelength - cd->dseglen));

		/* show data segment */

		if (opt_showddatasegment)
			dseg_display(m, cd);
	}

	/* release memory */

	dump_release(dumpsize);

	/* return native stub code */

	return code;
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

	DISASSEMBLE(start, end);
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
	localref_table  *plrt;
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

	/* release all current local frames */

	for (localframes = lrt->localframes; localframes >= 1; localframes--) {
		/* get previous frame */

		plrt = lrt->prev;

		/* Clear all reference entries (only for tables allocated on
		   the Java heap). */

		if (localframes > 1)
			MSET(&lrt->refs[0], 0, java_objectheader*, lrt->capacity);

		lrt->prev = NULL;

		/* set new local references table */

		lrt = plrt;
	}

	/* now store the previous local frames in the thread structure */

	LOCALREFTABLE = lrt;
}


/* removecompilerstub **********************************************************

   Deletes a compilerstub from memory (simply by freeing it).

*******************************************************************************/

void removecompilerstub(u1 *stub)
{
	/* pass size 1 to keep the intern function happy */

	CFREE((void *) stub, 1);
}


/* removenativestub ************************************************************

   Removes a previously created native-stub from memory.
    
*******************************************************************************/

void removenativestub(u1 *stub)
{
	/* pass size 1 to keep the intern function happy */

	CFREE((void *) stub, 1);
}


/* reg_of_var ******************************************************************

   This function determines a register, to which the result of an
   operation should go, when it is ultimatively intended to store the
   result in pseudoregister v.  If v is assigned to an actual
   register, this register will be returned.  Otherwise (when v is
   spilled) this function returns tempregnum.  If not already done,
   regoff and flags are set in the stack location.
       
   On ARM we have to check if a long/double variable is splitted
   across reg/stack (HIGH_REG == REG_SPLIT). We return the actual
   register of v for LOW_REG and the tempregnum for HIGH_REG in such
   cases.  (michi 2005/07/24)

*******************************************************************************/

s4 reg_of_var(registerdata *rd, stackptr v, s4 tempregnum)
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

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		count_spills_read++;
#endif

	v->flags |= INMEMORY;

	return tempregnum;
}


#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void codegen_threadcritrestart(codegendata *cd, int offset)
{
	cd->threadcritcurrent.mcoderestart = offset;
}


void codegen_threadcritstart(codegendata *cd, int offset)
{
	cd->threadcritcurrent.mcodebegin = offset;
}


void codegen_threadcritstop(codegendata *cd, int offset)
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
 * vim:noexpandtab:sw=4:ts=4:
 */
