/* src/vm/jit/codegen-common.cpp - architecture independent code generator stuff

   Copyright (C) 1996-2005, 2006, 2007, 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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

*/


#include "config.h"

#include <assert.h>
#include <string.h>

#include "vm/types.h"

#include "codegen.h"
#include "md.h"
#include "md-abi.h"

#include "mm/memory.h"

#include "toolbox/avl.h"
#include "toolbox/list.hpp"
#include "toolbox/logging.h"

#include "native/llni.h"
#include "native/localref.h"
#include "native/native.hpp"

#include "threads/thread.hpp"

#include "vm/jit/builtin.hpp"
#include "vm/exceptions.hpp"
#include "vm/method.h"
#include "vm/options.h"
#include "vm/string.hpp"

# include "vm/statistics.h"


#include "vm/jit/abi.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/code.hpp"
#include "vm/jit/codegen-common.hpp"

#if defined(ENABLE_DISASSEMBLER)
# include "vm/jit/disass.h"
#endif

#include "vm/jit/dseg.h"
#include "vm/jit/emit-common.hpp"
#include "vm/jit/jit.hpp"
#include "vm/jit/linenumbertable.hpp"
#include "vm/jit/methodheader.h"
#include "vm/jit/methodtree.h"
#include "vm/jit/patcher-common.hpp"
#include "vm/jit/replace.hpp"
#if defined(ENABLE_SSA)
# include "vm/jit/optimizing/lsra.h"
# include "vm/jit/optimizing/ssa.h"
#endif
#include "vm/jit/stacktrace.hpp"
#include "vm/jit/trace.hpp"

#if defined(ENABLE_INTRP)
#include "vm/jit/intrp/intrp.h"
#endif

#if defined(ENABLE_VMLOG)
#include <vmlog_cacao.h>
#endif

#include "show.hpp"


/* codegen_init ****************************************************************

   TODO

*******************************************************************************/

void codegen_init(void)
{
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

	/* initialize members */

	cd->flags        = 0;

	cd->mcodebase    = (u1*) DumpMemory::allocate(MCODEINITSIZE);
	cd->mcodeend     = cd->mcodebase + MCODEINITSIZE;
	cd->mcodesize    = MCODEINITSIZE;

	/* initialize mcode variables */

	cd->mcodeptr     = cd->mcodebase;
	cd->lastmcodeptr = cd->mcodebase;

#if defined(ENABLE_INTRP)
	/* native dynamic superinstructions variables */

	if (opt_intrp) {
		cd->ncodebase = (u1*) DumpMemory::allocate(NCODEINITSIZE);
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

#if defined(__I386__) || defined(__X86_64__) || defined(__XDSPCORE__) || defined(__M68K__) || defined(ENABLE_INTRP)
	cd->datareferences = NULL;
#endif

	cd->brancheslabel  = new DumpList<branch_label_ref_t*>();
	cd->linenumbers    = new DumpList<Linenumber>();
}


/* codegen_reset ***************************************************************

   Resets the codegen data structure so we can recompile the method.

*******************************************************************************/

static void codegen_reset(jitdata *jd)
{
	codeinfo    *code;
	codegendata *cd;
	basicblock  *bptr;

	/* get required compiler data */

	code = jd->code;
	cd   = jd->cd;

	/* reset error flag */

	cd->flags          &= ~CODEGENDATA_FLAG_ERROR;

	/* reset some members, we reuse the code memory already allocated
	   as this should have almost the correct size */

	cd->mcodeptr        = cd->mcodebase;
	cd->lastmcodeptr    = cd->mcodebase;

	cd->dseg            = NULL;
	cd->dseglen         = 0;

	cd->jumpreferences  = NULL;

#if defined(__I386__) || defined(__X86_64__) || defined(__XDSPCORE__) || defined(__M68K__) || defined(ENABLE_INTRP)
	cd->datareferences  = NULL;
#endif

	cd->brancheslabel   = new DumpList<branch_label_ref_t*>();
	cd->linenumbers     = new DumpList<Linenumber>();
	
	/* We need to clear the mpc and the branch references from all
	   basic blocks as they will definitely change. */

	for (bptr = jd->basicblocks; bptr != NULL; bptr = bptr->next) {
		bptr->mpc        = -1;
		bptr->branchrefs = NULL;
	}

	/* We need to clear all the patcher references from the codeinfo
	   since they all will be regenerated */

	patcher_list_reset(code);

#if defined(ENABLE_REPLACEMENT)
	code->rplpoints     = NULL;
	code->rplpointcount = 0;
	code->regalloc      = NULL;
	code->regalloccount = 0;
	code->globalcount   = 0;
#endif
}


/* codegen_generate ************************************************************

   Generates the code for the currently compiled method.

*******************************************************************************/

bool codegen_generate(jitdata *jd)
{
	codegendata *cd;

	/* get required compiler data */

	cd = jd->cd;

	/* call the machine-dependent code generation function */

	if (!codegen_emit(jd))
		return false;

	/* check for an error */

	if (CODEGENDATA_HAS_FLAG_ERROR(cd)) {
		/* check for long-branches flag, if it is set we recompile the
		   method */

#if !defined(NDEBUG)
        if (compileverbose)
            log_message_method("Re-generating code: ", jd->m);
#endif

		/* XXX maybe we should tag long-branches-methods for recompilation */

		if (CODEGENDATA_HAS_FLAG_LONGBRANCHES(cd)) {
			/* we have to reset the codegendata structure first */

			codegen_reset(jd);

			/* and restart the compiler run */

			if (!codegen_emit(jd))
				return false;
		}
		else {
			vm_abort("codegen_generate: unknown error occurred during codegen_emit: flags=%x\n", cd->flags);
		}

#if !defined(NDEBUG)
        if (compileverbose)
            log_message_method("Re-generating code done: ", jd->m);
#endif
	}

	/* reallocate the memory and finish the code generation */

	codegen_finish(jd);

	/* everything's ok */

	return true;
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

	cd->mcodebase = (u1*) DumpMemory::reallocate(cd->mcodebase,
												 cd->mcodesize,
												 cd->mcodesize * 2);
	cd->mcodesize *= 2;
	cd->mcodeend   = cd->mcodebase + cd->mcodesize;

	/* set new mcodeptr */

	cd->mcodeptr = cd->mcodebase + (cd->mcodeptr - oldmcodebase);

#if defined(__I386__) || defined(__MIPS__) || defined(__X86_64__) || defined(__M68K__) || defined(ENABLE_INTRP) \
 || defined(__SPARC_64__)
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

void codegen_add_branch_ref(codegendata *cd, basicblock *target, s4 condition, s4 reg, u4 options)
{
	branchref *br;
	s4         branchmpc;

	STATISTICS(count_branches_unresolved++);

	/* calculate the mpc of the branch instruction */

	branchmpc = cd->mcodeptr - cd->mcodebase;

	br = (branchref*) DumpMemory::allocate(sizeof(branchref));

	br->branchmpc = branchmpc;
	br->condition = condition;
	br->reg       = reg;
	br->options   = options;
	br->next      = target->branchrefs;

	target->branchrefs = br;
}


/* codegen_resolve_branchrefs **************************************************

   Resolves and patches the branch references of a given basic block.

*******************************************************************************/

void codegen_resolve_branchrefs(codegendata *cd, basicblock *bptr)
{
	branchref *br;
	u1        *mcodeptr;

	/* Save the mcodeptr because in the branch emitting functions
	   we generate code somewhere inside already generated code,
	   but we're still in the actual code generation phase. */

	mcodeptr = cd->mcodeptr;

	/* just to make sure */

	assert(bptr->mpc >= 0);

	for (br = bptr->branchrefs; br != NULL; br = br->next) {
		/* temporary set the mcodeptr */

		cd->mcodeptr = cd->mcodebase + br->branchmpc;

		/* emit_bccz and emit_branch emit the correct code, even if we
		   pass condition == BRANCH_UNCONDITIONAL or reg == -1. */

		emit_bccz(cd, bptr, br->condition, br->reg, br->options);
	}

	/* restore mcodeptr */

	cd->mcodeptr = mcodeptr;
}


/* codegen_branch_label_add ****************************************************

   Append an branch to the label-branch list.

*******************************************************************************/

void codegen_branch_label_add(codegendata *cd, s4 label, s4 condition, s4 reg, u4 options)
{
	// Calculate the current mpc.
	int32_t mpc = cd->mcodeptr - cd->mcodebase;

	branch_label_ref_t* br = (branch_label_ref_t*) DumpMemory::allocate(sizeof(branch_label_ref_t));

	br->mpc       = mpc;
	br->label     = label;
	br->condition = condition;
	br->reg       = reg;
	br->options   = options;

	// Add the branch to the list.
	cd->brancheslabel->push_back(br);
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

#if !defined(NDEBUG)
	/* XXX actually we should use an own REPLACEMENT_NOPS here! */
	if (opt_TestReplacement)
		PATCHER_NOPS;
#endif

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
	code->mcode       = CNEW(u1, alignedlen);

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

	/* Create the exception table. */

	exceptiontable_create(jd);

	/* Create the linenumber table. */

	code->linenumbertable = new LinenumberTable(jd);

	/* jump table resolving */

	for (jr = cd->jumpreferences; jr != NULL; jr = jr->next)
		*((functionptr *) ((ptrint) epoint + jr->tablepos)) =
			(functionptr) ((ptrint) epoint + (ptrint) jr->target->mpc);

	/* patcher resolving */

	patcher_resolve(jd);

#if defined(ENABLE_REPLACEMENT)
	/* replacement point resolving */
	{
		int i;
		rplpoint *rp;

		rp = code->rplpoints;
		for (i=0; i<code->rplpointcount; ++i, ++rp) {
			rp->pc = (u1*) ((ptrint) epoint + (ptrint) rp->pc);
		}
	}
#endif /* defined(ENABLE_REPLACEMENT) */

	/* Insert method into methodtree to find the entrypoint. */

	methodtree_insert(code->entrypoint, code->entrypoint + mcodelen);

#if defined(__I386__) || defined(__X86_64__) || defined(__XDSPCORE__) || defined(__M68K__) || defined(ENABLE_INTRP)
	/* resolve data segment references */

	dseg_resolve_datareferences(jd);
#endif

	/* flush the instruction and data caches */

	md_cacheflush(code->mcode, code->mcodelength);
}


/* codegen_start_native_call ***************************************************

   Prepares the stuff required for a native (JNI) function call:

   - adds a stackframe info structure to the chain, for stacktraces
   - prepares the local references table on the stack

   The layout of the native stub stackframe should look like this:

   +---------------------------+ <- java SP (of parent Java function)
   | return address            |
   +---------------------------+ <- data SP
   |                           |
   | stackframe info structure |
   |                           |
   +---------------------------+
   |                           |
   | local references table    |
   |                           |
   +---------------------------+
   |                           |
   | saved registers (if any)  |
   |                           |
   +---------------------------+
   |                           |
   | arguments (if any)        |
   |                           |
   +---------------------------+ <- current SP (native stub)

*******************************************************************************/

java_handle_t *codegen_start_native_call(u1 *sp, u1 *pv)
{
	stackframeinfo_t *sfi;
	localref_table   *lrt;
	methodinfo       *m;
	int32_t           framesize;

	uint8_t  *datasp;
	uint8_t  *javasp;
	uint64_t *arg_regs;
	uint64_t *arg_stack;

	STATISTICS(count_calls_java_to_native++);

	/* Get the methodinfo. */

	m = code_get_methodinfo_for_pv(pv);

	assert(m);

	framesize = *((int32_t *) (pv + FrameSize));

	assert(framesize >= (int32_t) (sizeof(stackframeinfo_t) + sizeof(localref_table)));

	/* calculate needed values */

#if defined(__ALPHA__) || defined(__ARM__)
	datasp    = sp + framesize - SIZEOF_VOID_P;
	javasp    = sp + framesize;
	arg_regs  = (uint64_t *) sp;
	arg_stack = (uint64_t *) javasp;
#elif defined(__MIPS__)
	/* MIPS always uses 8 bytes to store the RA */
	datasp    = sp + framesize - 8;
	javasp    = sp + framesize;
#elif defined(__S390__)
	datasp    = sp + framesize - 8;
	javasp    = sp + framesize;
	arg_regs  = (uint64_t *) (sp + 96);
	arg_stack = (uint64_t *) javasp;
#elif defined(__I386__) || defined(__M68K__) || defined(__X86_64__)
	datasp    = sp + framesize;
	javasp    = sp + framesize + SIZEOF_VOID_P;
	arg_regs  = (uint64_t *) sp;
	arg_stack = (uint64_t *) javasp;
#elif defined(__POWERPC__)
	datasp    = sp + framesize;
	javasp    = sp + framesize;
	arg_regs  = (uint64_t *) (sp + LA_SIZE + 4 * SIZEOF_VOID_P);
	arg_stack = (uint64_t *) javasp;
#elif defined(__POWERPC64__)
	datasp    = sp + framesize;
	javasp    = sp + framesize;
	arg_regs  = (uint64_t *) (sp + PA_SIZE + LA_SIZE + 4 * SIZEOF_VOID_P);
	arg_stack = (uint64_t *) javasp;
#else
	/* XXX is was unable to do this port for SPARC64, sorry. (-michi) */
	/* XXX maybe we need to pass the RA as argument there */
	vm_abort("codegen_start_native_call: unsupported architecture");
#endif

	/* get data structures from stack */

	sfi = (stackframeinfo_t *) (datasp - sizeof(stackframeinfo_t));
	lrt = (localref_table *)   (datasp - sizeof(stackframeinfo_t) - 
								sizeof(localref_table));

#if defined(ENABLE_JNI)
	/* add current JNI local references table to this thread */

	localref_table_add(lrt);
#endif

#if !defined(NDEBUG)
# if defined(__ALPHA__) || defined(__I386__) || defined(__M68K__) || defined(__POWERPC__) || defined(__POWERPC64__) || defined(__S390__) || defined(__X86_64__)
	/* print the call-trace if necesarry */
	/* BEFORE: filling the local reference table */

	if (opt_TraceJavaCalls)
		trace_java_call_enter(m, arg_regs, arg_stack);
# endif
#endif

#if defined(ENABLE_HANDLES)
	/* place all references into the local reference table */
	/* BEFORE: creating stackframeinfo */

	localref_native_enter(m, arg_regs, arg_stack);
#endif

	/* Add a stackframeinfo for this native method.  We don't have RA
	   and XPC here.  These are determined in
	   stacktrace_stackframeinfo_add. */

	stacktrace_stackframeinfo_add(sfi, pv, sp, NULL, NULL);

	/* Return a wrapped classinfo for static methods. */

	if (m->flags & ACC_STATIC)
		return (java_handle_t *) LLNI_classinfo_wrap(m->clazz);
	else
		return NULL;
}


/* codegen_finish_native_call **************************************************

   Removes the stuff required for a native (JNI) function call.
   Additionally it checks for an exceptions and in case, get the
   exception object and clear the pointer.

*******************************************************************************/

java_object_t *codegen_finish_native_call(u1 *sp, u1 *pv)
{
	stackframeinfo_t *sfi;
	java_handle_t    *e;
	java_object_t    *o;
	codeinfo         *code;
	methodinfo       *m;
	int32_t           framesize;

	uint8_t  *datasp;
	uint64_t *ret_regs;

	/* get information from method header */

	code = code_get_codeinfo_for_pv(pv);

	framesize = *((int32_t *) (pv + FrameSize));

	assert(code);

	/* get the methodinfo */

	m = code->m;
	assert(m);

	/* calculate needed values */

#if defined(__ALPHA__) || defined(__ARM__)
	datasp   = sp + framesize - SIZEOF_VOID_P;
	ret_regs = (uint64_t *) sp;
#elif defined(__MIPS__)
	/* MIPS always uses 8 bytes to store the RA */
	datasp   = sp + framesize - 8;
#elif defined(__S390__)
	datasp   = sp + framesize - 8;
	ret_regs = (uint64_t *) (sp + 96);
#elif defined(__I386__)
	datasp   = sp + framesize;
	ret_regs = (uint64_t *) (sp + 2 * SIZEOF_VOID_P);
#elif defined(__M68K__)
	datasp   = sp + framesize;
	ret_regs = (uint64_t *) (sp + 2 * 8);
#elif defined(__X86_64__)
	datasp   = sp + framesize;
	ret_regs = (uint64_t *) sp;
#elif defined(__POWERPC__)
	datasp   = sp + framesize;
	ret_regs = (uint64_t *) (sp + LA_SIZE + 2 * SIZEOF_VOID_P);
#elif defined(__POWERPC64__)
	datasp   = sp + framesize;
	ret_regs = (uint64_t *) (sp + PA_SIZE + LA_SIZE + 2 * SIZEOF_VOID_P);
#else
	vm_abort("codegen_finish_native_call: unsupported architecture");
#endif

	/* get data structures from stack */

	sfi = (stackframeinfo_t *) (datasp - sizeof(stackframeinfo_t));

	/* Remove current stackframeinfo from chain. */

	stacktrace_stackframeinfo_remove(sfi);

#if defined(ENABLE_HANDLES)
	/* unwrap the return value from the local reference table */
	/* AFTER: removing the stackframeinfo */
	/* BEFORE: releasing the local reference table */

	localref_native_exit(m, ret_regs);
#endif

	/* get and unwrap the exception */
	/* AFTER: removing the stackframe info */
	/* BEFORE: releasing the local reference table */

	e = exceptions_get_and_clear_exception();
	o = LLNI_UNWRAP(e);

#if defined(ENABLE_JNI)
	/* release JNI local references table for this thread */

	localref_frame_pop_all();
	localref_table_remove();
#endif

#if !defined(NDEBUG)
# if defined(__ALPHA__) || defined(__I386__) || defined(__M68K__) || defined(__POWERPC__) || defined(__POWERPC64__) || defined(__S390__) || defined(__X86_64__)
	/* print the call-trace if necesarry */
	/* AFTER: unwrapping the return value */

	if (opt_TraceJavaCalls)
		trace_java_call_exit(m, ret_regs);
# endif
#endif

	return o;
}


/* codegen_reg_of_var **********************************************************

   This function determines a register, to which the result of an
   operation should go, when it is ultimatively intended to store the
   result in pseudoregister v.  If v is assigned to an actual
   register, this register will be returned.  Otherwise (when v is
   spilled) this function returns tempregnum.  If not already done,
   regoff and flags are set in the stack location.
       
*******************************************************************************/

s4 codegen_reg_of_var(u2 opcode, varinfo *v, s4 tempregnum)
{
	if (!(v->flags & INMEMORY))
		return v->vv.regoff;

	return tempregnum;
}


/* codegen_reg_of_dst **********************************************************

   This function determines a register, to which the result of an
   operation should go, when it is ultimatively intended to store the
   result in iptr->dst.var.  If dst.var is assigned to an actual
   register, this register will be returned.  Otherwise (when it is
   spilled) this function returns tempregnum.  If not already done,
   regoff and flags are set in the stack location.
       
*******************************************************************************/

s4 codegen_reg_of_dst(jitdata *jd, instruction *iptr, s4 tempregnum)
{
	return codegen_reg_of_var(iptr->opc, VAROP(iptr->dst), tempregnum);
}


/* codegen_emit_phi_moves ****************************************************

   Emits phi moves at the end of the basicblock.

*******************************************************************************/

#if defined(ENABLE_SSA)
void codegen_emit_phi_moves(jitdata *jd, basicblock *bptr)
{
	int lt_d,lt_s,i;
	lsradata *ls;
	codegendata *cd;
	varinfo *s, *d;
	instruction tmp_i;

	cd = jd->cd;
	ls = jd->ls;

	MCODECHECK(512);

	/* Moves from phi functions with highest indices have to be */
	/* inserted first, since this is the order as is used for   */
	/* conflict resolution */

	for(i = ls->num_phi_moves[bptr->nr] - 1; i >= 0 ; i--) {
		lt_d = ls->phi_moves[bptr->nr][i][0];
		lt_s = ls->phi_moves[bptr->nr][i][1];
#if defined(SSA_DEBUG_VERBOSE)
		if (compileverbose)
			printf("BB %3i Move %3i <- %3i ", bptr->nr, lt_d, lt_s);
#endif
		if (lt_s == UNUSED) {
#if defined(SSA_DEBUG_VERBOSE)
		if (compileverbose)
			printf(" ... not processed \n");
#endif
			continue;
		}
			
		d = VAR(ls->lifetime[lt_d].v_index);
		s = VAR(ls->lifetime[lt_s].v_index);
		

		if (d->type == -1) {
#if defined(SSA_DEBUG_VERBOSE)
			if (compileverbose)
				printf("...returning - phi lifetimes where joined\n");
#endif
			continue;
		}

		if (s->type == -1) {
#if defined(SSA_DEBUG_VERBOSE)
			if (compileverbose)
				printf("...returning - phi lifetimes where joined\n");
#endif
			continue;
		}

		tmp_i.opc = 0;
		tmp_i.s1.varindex = ls->lifetime[lt_s].v_index;
		tmp_i.dst.varindex = ls->lifetime[lt_d].v_index;
		emit_copy(jd, &tmp_i);

#if defined(SSA_DEBUG_VERBOSE)
		if (compileverbose) {
			if (IS_INMEMORY(d->flags) && IS_INMEMORY(s->flags)) {
				/* mem -> mem */
				printf("M%3i <- M%3i",d->vv.regoff,s->vv.regoff);
			}
			else if (IS_INMEMORY(s->flags)) {
				/* mem -> reg */
				printf("R%3i <- M%3i",d->vv.regoff,s->vv.regoff);
			}
			else if (IS_INMEMORY(d->flags)) {
				/* reg -> mem */
				printf("M%3i <- R%3i",d->vv.regoff,s->vv.regoff);
			}
			else {
				/* reg -> reg */
				printf("R%3i <- R%3i",d->vv.regoff,s->vv.regoff);
			}
			printf("\n");
		}
#endif /* defined(SSA_DEBUG_VERBOSE) */
	}
}
#endif /* defined(ENABLE_SSA) */


/* REMOVEME When we have exception handling in C. */

void *md_asm_codegen_get_pv_from_pc(void *ra)
{
	return md_codegen_get_pv_from_pc(ra);
}


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
