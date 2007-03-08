/* src/vm/jit/codegen-common.c - architecture independent code generator stuff

   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
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

   $Id: codegen-common.c 7489 2007-03-08 17:12:56Z michi $

*/


#include "config.h"

#include <assert.h>
#include <string.h>

#include "vm/types.h"

#if defined(ENABLE_JIT)
/* this is required PATCHER_CALL_SIZE */
# include "codegen.h"
#endif

#if defined(__ARM__)
/* this is required for REG_SPLIT */
# include "md-abi.h"
#endif

#include "mm/memory.h"

#include "toolbox/avl.h"
#include "toolbox/list.h"
#include "toolbox/logging.h"

#include "native/jni.h"
#include "native/native.h"

#if defined(ENABLE_THREADS)
# include "threads/native/threads.h"
#endif

#include "vm/exceptions.h"
#include "vm/stringlocal.h"

#include "vm/jit/abi.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/codegen-common.h"

#if defined(ENABLE_DISASSEMBLER)
# include "vm/jit/disass.h"
#endif

#include "vm/jit/dseg.h"
#include "vm/jit/jit.h"
#include "vm/jit/md.h"
#include "vm/jit/replace.h"
#include "vm/jit/stacktrace.h"

#if defined(ENABLE_INTRP)
#include "vm/jit/intrp/intrp.h"
#endif

#include "vmcore/method.h"
#include "vmcore/options.h"

#if defined(ENABLE_STATISTICS)
# include "vmcore/statistics.h"
#endif


/* in this tree we store all method addresses *********************************/

static avl_tree *methodtree = NULL;
static s4 methodtree_comparator(const void *pc, const void *element);


/* codegen_init ****************************************************************

   TODO

*******************************************************************************/

void codegen_init(void)
{
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
}


/* codegen_setup ***************************************************************

   Allocates and initialises code area, data area and references.

*******************************************************************************/

void codegen_setup(jitdata *jd)
{
	methodinfo  *m;
	codegendata *cd;

	/* get required compiler data */

	m  = jd->m;
	cd = jd->cd;

	cd->mcodebase = DMNEW(u1, MCODEINITSIZE);
	cd->mcodeend  = cd->mcodebase + MCODEINITSIZE;
	cd->mcodesize = MCODEINITSIZE;

	/* initialize mcode variables */

	cd->mcodeptr     = cd->mcodebase;
	cd->lastmcodeptr = cd->mcodebase;

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

	cd->dseg           = NULL;
	cd->dseglen        = 0;

	cd->jumpreferences = NULL;

#if defined(__I386__) || defined(__X86_64__) || defined(__XDSPCORE__) || defined(ENABLE_INTRP)
	cd->datareferences = NULL;
#endif

	cd->exceptionrefs  = NULL;
/* 	cd->patchrefs      = list_create_dump(OFFSET(patchref, linkage)); */
	cd->patchrefs      = NULL;

	cd->linenumberreferences = NULL;
	cd->linenumbertablesizepos = 0;
	cd->linenumbertablestartpos = 0;
	cd->linenumbertab = 0;
	
	cd->method = m;

	cd->maxstack = m->maxstack;

#if defined(ENABLE_THREADS)
	cd->threadcritcurrent.next = NULL;
	cd->threadcritcount = 0;
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

void codegen_increase(codegendata *cd)
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
	cd->mcodeend   = cd->mcodebase + cd->mcodesize;

	/* set new mcodeptr */

	cd->mcodeptr = cd->mcodebase + (cd->mcodeptr - oldmcodebase);

#if defined(__I386__) || defined(__MIPS__) || defined(__X86_64__) || defined(ENABLE_INTRP)
	/* adjust the pointer to the last patcher position */

	if (cd->lastmcodeptr != NULL)
		cd->lastmcodeptr = cd->mcodebase + (cd->lastmcodeptr - oldmcodebase);
#endif
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


/* codegen_add_branch_ref ******************************************************

   Prepends an branch to the list.

*******************************************************************************/

void codegen_add_branch_ref(codegendata *cd, basicblock *target)
{
	s4 branchmpc;

	/* calculate the mpc of the branch instruction */

	branchmpc = cd->mcodeptr - cd->mcodebase;

#if defined(ENABLE_JIT)
	/* Check if the target basicblock has already a start pc, so the
	   jump is backward and we can resolve it immediately. */

	if ((target->mpc >= 0)
# if defined(ENABLE_INTRP)
		/* The interpreter uses absolute branches, so we do branch
		   resolving after the code and data segment move. */

		&& !opt_intrp
# endif
		)
	{
		md_codegen_patch_branch(cd, branchmpc, target->mpc);
	}
	else
#endif
	{
		branchref *br = DNEW(branchref);

		br->branchpos = branchmpc;
		br->next      = target->branchrefs;

		target->branchrefs = br;
	}
}


/* codegen_resolve_branchrefs **************************************************

   Resolves and patches the branch references of a given basic block.

*******************************************************************************/

void codegen_resolve_branchrefs(codegendata *cd, basicblock *bptr)
{
	branchref *br;
	s4         branchmpc;
	s4         targetmpc;

	/* set target */

	targetmpc = bptr->mpc;

	for (br = bptr->branchrefs; br != NULL; br = br->next) {
		branchmpc = br->branchpos;

		md_codegen_patch_branch(cd, branchmpc, targetmpc);
	}
}


/* codegen_add_exception_ref ***************************************************

   Prepends an exception branch to the list.

*******************************************************************************/

static void codegen_add_exception_ref(codegendata *cd, s4 reg,
									  functionptr function)
{
	s4            branchmpc;
	exceptionref *er;

	branchmpc = cd->mcodeptr - cd->mcodebase;

	er = DNEW(exceptionref);

	er->branchpos = branchmpc;
	er->reg       = reg;
	er->function  = function;

	er->next      = cd->exceptionrefs;

	cd->exceptionrefs = er;
}


/* codegen_add_arithmeticexception_ref *****************************************

   Adds an ArithmeticException branch to the list.

*******************************************************************************/

void codegen_add_arithmeticexception_ref(codegendata *cd)
{
	codegen_add_exception_ref(cd, -1, STACKTRACE_inline_arithmeticexception);
}


/* codegen_add_arrayindexoutofboundsexception_ref ******************************

   Adds an ArrayIndexOutOfBoundsException branch to the list.

*******************************************************************************/

void codegen_add_arrayindexoutofboundsexception_ref(codegendata *cd, s4 reg)
{
	codegen_add_exception_ref(cd, reg,
							  STACKTRACE_inline_arrayindexoutofboundsexception);
}


/* codegen_add_arraystoreexception_ref *****************************************

   Adds an ArrayStoreException branch to the list.

*******************************************************************************/

void codegen_add_arraystoreexception_ref(codegendata *cd)
{
	codegen_add_exception_ref(cd, -1, STACKTRACE_inline_arraystoreexception);
}


/* codegen_add_classcastexception_ref ******************************************

   Adds an ClassCastException branch to the list.

*******************************************************************************/

void codegen_add_classcastexception_ref(codegendata *cd, s4 reg)
{
	codegen_add_exception_ref(cd, reg, STACKTRACE_inline_classcastexception);
}


/* codegen_add_nullpointerexception_ref ****************************************

   Adds an NullPointerException branch to the list.

*******************************************************************************/

void codegen_add_nullpointerexception_ref(codegendata *cd)
{
	codegen_add_exception_ref(cd, -1, STACKTRACE_inline_nullpointerexception);
}


/* codegen_add_fillinstacktrace_ref ********************************************

   Adds a fillInStackTrace branch to the list.

*******************************************************************************/

void codegen_add_fillinstacktrace_ref(codegendata *cd)
{
	codegen_add_exception_ref(cd, -1, STACKTRACE_inline_fillInStackTrace);
}


/* codegen_add_patch_ref *******************************************************

   Appends a new patcher reference to the list of patching positions.

*******************************************************************************/

void codegen_add_patch_ref(codegendata *cd, functionptr patcher, voidptr ref,
						   s4 disp)
{
	patchref *pr;
	s4        branchmpc;

	branchmpc = cd->mcodeptr - cd->mcodebase;

	pr = DNEW(patchref);

	pr->branchpos = branchmpc;
	pr->disp      = disp;
	pr->patcher   = patcher;
	pr->ref       = ref;

/* 	list_add_first(cd->patchrefs, pr); */
	pr->next      = cd->patchrefs;
	cd->patchrefs = pr;

#if defined(ENABLE_JIT) && (defined(__ALPHA__) || defined(__MIPS__) || defined(__POWERPC__) || defined(__SPARC_64__) || defined(__X86_64__) ||  defined(__S390__))
	/* Generate NOPs for opt_shownops. */

	if (opt_shownops)
		PATCHER_NOPS;
#endif

#if defined(ENABLE_JIT) && (defined(__I386__) || defined(__MIPS__) || defined(__X86_64__))
	/* On some architectures the patcher stub call instruction might
	   be longer than the actual instruction generated.  On this
	   architectures we store the last patcher call position and after
	   the basic block code generation is completed, we check the
	   range and maybe generate some nop's. */

	cd->lastmcodeptr = cd->mcodeptr + PATCHER_CALL_SIZE;
#endif
}


/* methodtree_comparator *******************************************************

   Comparator function used for the AVL tree of methods.

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

#ifdef __S390__
	/* On S390 addresses are 31 bit. Compare only 31 bits of value.
	 */
#	define ADDR_MASK(a) ((a) & 0x7FFFFFFF)
#else
#	define ADDR_MASK(a) (a)
#endif

	if (ADDR_MASK((long) mte->startpc) <= ADDR_MASK((long) mtepc->startpc) &&
		ADDR_MASK((long) mtepc->startpc) <= ADDR_MASK((long) mte->endpc) &&
		ADDR_MASK((long) mte->startpc) <= ADDR_MASK((long) mtepc->endpc) &&
		ADDR_MASK((long) mtepc->endpc) <= ADDR_MASK((long) mte->endpc)) {
		return 0;

	} else if (ADDR_MASK((long) mtepc->startpc) < ADDR_MASK((long) mte->startpc)) {
		return -1;

	} else {
		return 1;
	}

#	undef ADDR_MASK
}


/* codegen_insertmethod ********************************************************

   Insert the machine code range of a method into the AVL tree of methods.

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


/* codegen_get_pv_from_pc ******************************************************

   Find the PV for the given PC by searching in the AVL tree of
   methods.

*******************************************************************************/

u1 *codegen_get_pv_from_pc(u1 *pc)
{
	methodtree_element  mtepc;
	methodtree_element *mte;

	/* allocation of the search structure on the stack is much faster */

	mtepc.startpc = pc;
	mtepc.endpc   = pc;

	mte = avl_find(methodtree, &mtepc);

	if (mte == NULL) {
		/* No method was found.  Let's dump a stacktrace. */

		log_println("We received a SIGSEGV and tried to handle it, but we were");
		log_println("unable to find a Java method at:");
		log_println("");
#if SIZEOF_VOID_P == 8
		log_println("PC=0x%016lx", pc);
#else
		log_println("PC=0x%08x", pc);
#endif
		log_println("");
		log_println("Dumping the current stacktrace:");

		stacktrace_dump_trace(THREADOBJECT);

		vm_abort("Exiting...");
	}

	return mte->startpc;
}


/* codegen_get_pv_from_pc_nocheck **********************************************

   Find the PV for the given PC by searching in the AVL tree of
   methods.  This method does not check the return value and is used
   by the profiler.

*******************************************************************************/

u1 *codegen_get_pv_from_pc_nocheck(u1 *pc)
{
	methodtree_element  mtepc;
	methodtree_element *mte;

	/* allocation of the search structure on the stack is much faster */

	mtepc.startpc = pc;
	mtepc.endpc   = pc;

	mte = avl_find(methodtree, &mtepc);

	if (mte == NULL)
		return NULL;
	else
		return mte->startpc;
}


/* codegen_set_replacement_point_notrap ****************************************

   Record the position of a non-trappable replacement point.

*******************************************************************************/

#if defined(ENABLE_REPLACEMENT)
#if !defined(NDEBUG)
void codegen_set_replacement_point_notrap(codegendata *cd, s4 type)
#else
void codegen_set_replacement_point_notrap(codegendata *cd)
#endif
{
	assert(cd->replacementpoint);
	assert(cd->replacementpoint->type == type);
	assert(cd->replacementpoint->flags & RPLPOINT_FLAG_NOTRAP);

	cd->replacementpoint->pc = (u1*) (ptrint) (cd->mcodeptr - cd->mcodebase);

	cd->replacementpoint++;
}
#endif /* defined(ENABLE_REPLACEMENT) */


/* codegen_set_replacement_point ***********************************************

   Record the position of a trappable replacement point.

*******************************************************************************/

#if defined(ENABLE_REPLACEMENT)
#if !defined(NDEBUG)
void codegen_set_replacement_point(codegendata *cd, s4 type)
#else
void codegen_set_replacement_point(codegendata *cd)
#endif
{
	assert(cd->replacementpoint);
	assert(cd->replacementpoint->type == type);
	assert(!(cd->replacementpoint->flags & RPLPOINT_FLAG_NOTRAP));

	cd->replacementpoint->pc = (u1*) (ptrint) (cd->mcodeptr - cd->mcodebase);

	cd->replacementpoint++;

	/* XXX assert(cd->lastmcodeptr <= cd->mcodeptr); */

	cd->lastmcodeptr = cd->mcodeptr + PATCHER_CALL_SIZE;
}
#endif /* defined(ENABLE_REPLACEMENT) */


/* codegen_finish **************************************************************

   Finishes the code generation. A new memory, large enough for both
   data and code, is allocated and data and code are copied together
   to their final layout, unresolved jumps are resolved, ...

*******************************************************************************/

void codegen_finish(jitdata *jd)
{
	codeinfo    *code;
	codegendata *cd;
	s4           mcodelen;
#if defined(ENABLE_INTRP)
	s4           ncodelen;
#endif
	s4           alignedmcodelen;
	jumpref     *jr;
	u1          *epoint;
	s4           extralen;
	s4           alignedlen;

	/* get required compiler data */

	code = jd->code;
	cd   = jd->cd;

	/* prevent compiler warning */

#if defined(ENABLE_INTRP)
	ncodelen = 0;
#endif

	/* calculate the code length */

	mcodelen = (s4) (cd->mcodeptr - cd->mcodebase);

#if defined(ENABLE_THREADS)
	extralen = sizeof(critical_section_node_t) * cd->threadcritcount;
#else
	extralen = 0;
#endif

#if defined(ENABLE_STATISTICS)
	if (opt_stat) {
		count_code_len += mcodelen;
		count_data_len += cd->dseglen;
	}
#endif

	alignedmcodelen = MEMORY_ALIGN(mcodelen, MAX_ALIGN);

#if defined(ENABLE_INTRP)
	if (opt_intrp)
		ncodelen = cd->ncodeptr - cd->ncodebase;
	else {
		ncodelen = 0; /* avoid compiler warning */
	}
#endif

	cd->dseglen = MEMORY_ALIGN(cd->dseglen, MAX_ALIGN);
	alignedlen = alignedmcodelen + cd->dseglen;

#if defined(ENABLE_INTRP)
	if (opt_intrp) {
		alignedlen += ncodelen;
	}
#endif

	/* allocate new memory */

	code->mcodelength = mcodelen + cd->dseglen;
	code->mcode       = CNEW(u1, alignedlen + extralen);

	/* set the entrypoint of the method */
	
	assert(code->entrypoint == NULL);
	code->entrypoint = epoint = (code->mcode + cd->dseglen);

	/* fill the data segment (code->entrypoint must already be set!) */

	dseg_finish(jd);

	/* copy code to the new location */

	MCOPY((void *) code->entrypoint, cd->mcodebase, u1, mcodelen);

#if defined(ENABLE_INTRP)
	/* relocate native dynamic superinstruction code (if any) */

	if (opt_intrp) {
		cd->mcodebase = code->entrypoint;

		if (ncodelen > 0) {
			u1 *ncodebase = code->mcode + cd->dseglen + alignedmcodelen;

			MCOPY((void *) ncodebase, cd->ncodebase, u1, ncodelen);

			/* flush the instruction and data caches */

			md_cacheflush(ncodebase, ncodelen);

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

#if defined(ENABLE_REPLACEMENT)
	/* replacement point resolving */
	{
		int i;
		rplpoint *rp;

		code->replacementstubs += (ptrint) epoint;

		rp = code->rplpoints;
		for (i=0; i<code->rplpointcount; ++i, ++rp) {
			rp->pc = (u1*) ((ptrint) epoint + (ptrint) rp->pc);
		}
	}
#endif /* defined(ENABLE_REPLACEMENT) */

	/* add method into methodtree to find the entrypoint */

	codegen_insertmethod(code->entrypoint, code->entrypoint + mcodelen);

#if defined(__I386__) || defined(__X86_64__) || defined(__XDSPCORE__) || defined(ENABLE_INTRP)
	/* resolve data segment references */

	dseg_resolve_datareferences(jd);
#endif

#if defined(ENABLE_THREADS)
	{
		critical_section_node_t *n = (critical_section_node_t *) ((ptrint) code->mcode + alignedlen);
		s4 i;
		codegen_critical_section_t *nt = cd->threadcrit;

		for (i = 0; i < cd->threadcritcount; i++) {
			n->mcodebegin = (u1 *) (ptrint) code->mcode + nt->mcodebegin;
			n->mcodeend = (u1 *) (ptrint) code->mcode + nt->mcodeend;
			n->mcoderestart = (u1 *) (ptrint) code->mcode + nt->mcoderestart;
			critical_register_critical_section(n);
			n++;
			nt = nt->next;
		}
	}
#endif

	/* flush the instruction and data caches */

	md_cacheflush(code->mcode, code->mcodelength);
}


/* codegen_createnativestub ****************************************************

   Wrapper for createnativestub.

   Returns:
       the codeinfo representing the stub code.

*******************************************************************************/

codeinfo *codegen_createnativestub(functionptr f, methodinfo *m)
{
	jitdata     *jd;
	codeinfo    *code;
	s4           dumpsize;
	methoddesc  *md;
	methoddesc  *nmd;	
	s4           nativeparams;

	/* mark dump memory */

	dumpsize = dump_size();

	jd = DNEW(jitdata);

	jd->m     = m;
	jd->cd    = DNEW(codegendata);
	jd->rd    = DNEW(registerdata);
	jd->flags = 0;

	/* Allocate codeinfo memory from the heap as we need to keep them. */

	jd->code  = code_codeinfo_new(m); /* XXX check allocation */

	/* get required compiler data */

	code = jd->code;

	/* set the flags for the current JIT run */

#if defined(ENABLE_PROFILING)
	if (opt_prof)
		jd->flags |= JITDATA_FLAG_INSTRUMENT;
#endif

	if (opt_verbosecall)
		jd->flags |= JITDATA_FLAG_VERBOSECALL;

	/* setup code generation stuff */

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (!opt_intrp)
# endif
		reg_setup(jd);
#endif

	codegen_setup(jd);

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
		/* pre-allocate the arguments for the native ABI */

		md_param_alloc_native(nmd);
#endif

	/* generate the code */

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (opt_intrp)
		code->entrypoint = intrp_createnativestub(f, jd, nmd);
	else
# endif
		code->entrypoint = createnativestub(f, jd, nmd);
#else
	code->entrypoint = intrp_createnativestub(f, jd, nmd);
#endif

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		count_nstub_len += code->mcodelength;
#endif

#if !defined(NDEBUG)
	/* disassemble native stub */

	if (opt_shownativestub) {
#if defined(ENABLE_DISASSEMBLER)
		codegen_disassemble_nativestub(m,
									   (u1 *) (ptrint) code->entrypoint,
									   (u1 *) (ptrint) code->entrypoint + (code->mcodelength - jd->cd->dseglen));
#endif

		/* show data segment */

		if (opt_showddatasegment)
			dseg_display(jd);
	}
#endif /* !defined(NDEBUG) */

	/* release memory */

	dump_release(dumpsize);

	/* return native stub code */

	return code;
}


/* codegen_disassemble_nativestub **********************************************

   Disassembles the generated native stub.

*******************************************************************************/

#if defined(ENABLE_DISASSEMBLER)
void codegen_disassemble_nativestub(methodinfo *m, u1 *start, u1 *end)
{
	printf("Native stub: ");
	utf_fprint_printable_ascii_classname(stdout, m->class->name);
	printf(".");
	utf_fprint_printable_ascii(stdout, m->name);
	utf_fprint_printable_ascii(stdout, m->descriptor);
	printf("\n\nLength: %d\n\n", (s4) (end - start));

	DISASSEMBLE(start, end);
}
#endif


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

#if defined(ENABLE_JAVASE)
	/* add current JNI local references table to this thread */

	lrt->capacity    = LOCALREFTABLE_CAPACITY;
	lrt->used        = 0;
	lrt->localframes = 1;
	lrt->prev        = LOCALREFTABLE;

	/* clear the references array (memset is faster the a for-loop) */

	MSET(lrt->refs, 0, java_objectheader*, LOCALREFTABLE_CAPACITY);

	LOCALREFTABLE = lrt;
#endif

#if defined(ENABLE_THREADS) && defined(ENABLE_GC_CACAO)
	/* set the native world flag */

	THREADOBJECT->flags |= THREAD_FLAG_IN_NATIVE;
#endif
}


/* codegen_finish_native_call **************************************************

   Removes the stuff required for a native (JNI) function call.
   Additionally it checks for an exceptions and in case, get the
   exception object and clear the pointer.

*******************************************************************************/

java_objectheader *codegen_finish_native_call(u1 *datasp)
{
	stackframeinfo     *sfi;
	stackframeinfo    **psfi;
	localref_table     *lrt;
	localref_table     *plrt;
	s4                  localframes;
	java_objectheader  *e;

	/* get data structures from stack */

	sfi = (stackframeinfo *) (datasp - sizeof(stackframeinfo));
	lrt = (localref_table *) (datasp - sizeof(stackframeinfo) - 
							  sizeof(localref_table));

#if defined(ENABLE_THREADS) && defined(ENABLE_GC_CACAO)
	/* clear the native world flag */

	THREADOBJECT->flags &= ~THREAD_FLAG_IN_NATIVE;
#endif

	/* remove current stackframeinfo from chain */

	psfi = STACKFRAMEINFO;

	*psfi = sfi->prev;

#if defined(ENABLE_JAVASE)
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
#endif

	/* get the exception and return it */

	e = exceptions_get_and_clear_exception();

	return e;
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


/* codegen_reg_of_var **********************************************************

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

s4 codegen_reg_of_var(u2 opcode, varinfo *v, s4 tempregnum)
{

#if 0
	/* Do we have to generate a conditional move?  Yes, then always
	   return the temporary register.  The real register is identified
	   during the store. */

	if (opcode & ICMD_CONDITION_MASK)
		return tempregnum;
#endif

	if (!(v->flags & INMEMORY)) {
#if defined(__ARM__) && defined(__ARMEL__)
		if (IS_2_WORD_TYPE(v->type) && (GET_HIGH_REG(v->vv.regoff) == REG_SPLIT))
			return PACK_REGS(GET_LOW_REG(v->vv.regoff),
							 GET_HIGH_REG(tempregnum));
#endif
#if defined(__ARM__) && defined(__ARMEB__)
		if (IS_2_WORD_TYPE(v->type) && (GET_LOW_REG(v->vv.regoff) == REG_SPLIT))
			return PACK_REGS(GET_LOW_REG(tempregnum),
							 GET_HIGH_REG(v->vv.regoff));
#endif
		return v->vv.regoff;
	}

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		count_spills_read++;
#endif

	return tempregnum;
}

/* codegen_reg_of_dst **********************************************************

   This function determines a register, to which the result of an
   operation should go, when it is ultimatively intended to store the
   result in iptr->dst.var.  If dst.var is assigned to an actual
   register, this register will be returned.  Otherwise (when it is
   spilled) this function returns tempregnum.  If not already done,
   regoff and flags are set in the stack location.
       
   On ARM we have to check if a long/double variable is splitted
   across reg/stack (HIGH_REG == REG_SPLIT). We return the actual
   register of dst.var for LOW_REG and the tempregnum for HIGH_REG in such
   cases.  (michi 2005/07/24)

*******************************************************************************/

s4 codegen_reg_of_dst(jitdata *jd, instruction *iptr, s4 tempregnum)
{
	return codegen_reg_of_var(iptr->opc, VAROP(iptr->dst), tempregnum);
}


#if defined(ENABLE_THREADS)
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
	cd->threadcrit = DNEW(codegen_critical_section_t);
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
