/* src/vm/jit/compiler2/Compiler.cpp - 2nd stage Just-In-Time compiler

   Copyright (C) 2013
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

*/

#include "config.h"
#include <assert.h>

#include "vm/hook.hpp"
#include "vm/rt-timing.hpp"
#include "vm/statistics.hpp"
#include "vm/jit/cfg.hpp"
#include "vm/jit/jit.hpp"
#include "vm/jit/parse.hpp"
#include "vm/jit/show.hpp"
#include "vm/jit/stubs.hpp"

#include "vm/jit/allocator/simplereg.hpp"

#if defined(ENABLE_IFCONV)
# include "vm/jit/optimizing/ifconv.hpp"
#endif
#include "vm/jit/verify/typecheck.hpp"

#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/Compiler.hpp"

#include "vm/jit/compiler2/Debug.hpp"

#include "vm/options.hpp"

namespace {

/* debug macros ***************************************************************/

#if !defined(NDEBUG)
#define DEBUG_JIT_COMPILEVERBOSE(x)				\
    do {										\
        if (compileverbose) {					\
            log_message_method(x, m);			\
        }										\
    } while (0)
#else
#define DEBUG_JIT_COMPILEVERBOSE(x)    /* nothing */
#endif

#if !defined(NDEBUG)
# define TRACECOMPILERCALLS()								\
	do {													\
		if (opt_TraceCompilerCalls) {						\
			log_start();									\
			log_print("[JIT compiler started: method=");	\
			method_print(m);								\
			log_print("]");									\
			log_finish();									\
		}													\
	} while (0)
#else
# define TRACECOMPILERCALLS()
#endif

/* dummy function, used when there is no JavaVM code available                */

u1 *do_nothing_function(void)
{
	return NULL;
}

/* compile ***************************************************************

   Recompiles a Java method.

*******************************************************************************/

// forward declarations
u1 *jit_compile_intern(jitdata *jd);

u1 *compile_intern(methodinfo *m)
{
	u1      *r;
	jitdata *jd;
	u1       optlevel;

	/* check for max. optimization level */

	optlevel = (m->code) ? m->code->optlevel : 0;

#if 0
	if (optlevel == 1) {
/* 		log_message_method("not recompiling: ", m); */
		return NULL;
	}
#endif

	DEBUG_JIT_COMPILEVERBOSE("Recompiling start: ");

	STATISTICS(count_jit_calls++);

#if defined(ENABLE_STATISTICS)
	/* measure time */

	if (opt_getcompilingtime)
		compilingtime_start();
#endif

	// Create new dump memory area.
	DumpMemoryArea dma;

	/* create jitdata structure */

	jd = jit_jitdata_new(m);

	/* set the current optimization level to the previous one plus 1 */

	jd->code->optlevel = optlevel + 1;

	/* get the optimization flags for the current JIT run */

#if defined(ENABLE_VERIFIER)
	jd->flags |= JITDATA_FLAG_VERIFY;
#endif

	/* jd->flags |= JITDATA_FLAG_REORDER; */
	if (opt_showintermediate)
		jd->flags |= JITDATA_FLAG_SHOWINTERMEDIATE;
	if (opt_showdisassemble)
		jd->flags |= JITDATA_FLAG_SHOWDISASSEMBLE;
	if (opt_verbosecall)
		jd->flags |= JITDATA_FLAG_VERBOSECALL;

#if defined(ENABLE_INLINING)
#warning Inlining currently disabled (broken)
#if 0
	if (opt_Inline)
		jd->flags |= JITDATA_FLAG_INLINE;
#endif
#endif

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (!opt_intrp)
# endif
		/* initialize the register allocator */

		reg_setup(jd);
#endif

	/* setup the codegendata memory */

	codegen_setup(jd);

	/* now call internal compile function */

	bool bak = opt_RegallocSpillAll;
	opt_RegallocSpillAll = true;
	r = jit_compile_intern(jd);
	opt_RegallocSpillAll = bak;

	if (r == NULL) {
		/* We had an exception! Finish stuff here if necessary. */

		/* release codeinfo */

		code_codeinfo_free(jd->code);
	}

#if defined(ENABLE_STATISTICS)
	/* measure time */

	if (opt_getcompilingtime)
		compilingtime_stop();
#endif

	// Hook point just after code was generated.
	Hook::jit_generated(m, m->code);

	DEBUG_JIT_COMPILEVERBOSE("Recompiling done: ");

	/* return pointer to the methods entry point */

	return r;
}

#if defined(ENABLE_PM_HACKS)
#include "vm/jit/jit_pm_1.inc"
#endif

/* jit_compile_intern **********************************************************

   Static internal function which does the actual compilation.

*******************************************************************************/

u1 *jit_compile_intern(jitdata *jd)
{
	methodinfo  *m;
	codegendata *cd;
	codeinfo    *code;

#if defined(ENABLE_RT_TIMING)
	struct timespec time_start,time_checks,time_parse,time_stack,
					time_typecheck,time_loop,time_ifconv,time_alloc,
					time_codegen;
#endif

	RT_TIMING_GET_TIME(time_start);

	/* get required compiler data */

#if defined(ENABLE_LSRA) || defined(ENABLE_SSA)
	jd->ls = NULL;
#endif
	m    = jd->m;
	code = jd->code;
	cd   = jd->cd;

#if defined(ENABLE_DEBUG_FILTER)
	show_filters_apply(jd->m);
#endif

	// Handle native methods and create a native stub.
	if (m->flags & ACC_NATIVE) {
		NativeMethods& nm = VM::get_current()->get_nativemethods();
		void* f = nm.resolve_method(m);

		if (f == NULL)
			return NULL;

		code = NativeStub::generate(m, (functionptr) f);

		/* Native methods are never recompiled. */

		assert(!m->code);

		m->code = code;

		return code->entrypoint;
	}

	/* if there is no javacode, print error message and return empty method   */

	if (m->jcode == NULL) {
		DEBUG_JIT_COMPILEVERBOSE("No code given for: ");

		code->entrypoint = (u1 *) (ptrint) do_nothing_function;
		m->code = code;

		return code->entrypoint;        /* return empty method                */
	}

#if defined(ENABLE_STATISTICS)
	if (opt_stat) {
		count_javacodesize += m->jcodelength + 18;
		count_tryblocks    += jd->exceptiontablelength;
		count_javaexcsize  += jd->exceptiontablelength * SIZEOF_VOID_P;
	}
#endif

	RT_TIMING_GET_TIME(time_checks);

#if defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)
	/* Code for Sun's OpenJDK (see
	   hotspot/src/share/vm/classfile/verifier.cpp
	   (Verifier::is_eligible_for_verification)): Don't verify
	   dynamically-generated bytecodes. */

# if defined(ENABLE_VERIFIER)
	if (class_issubclass(m->clazz, class_sun_reflect_MagicAccessorImpl))
		jd->flags &= ~JITDATA_FLAG_VERIFY;
# endif
#endif

	/* call the compiler passes ***********************************************/

	DEBUG_JIT_COMPILEVERBOSE("Parsing: ");

	/* call parse pass */

	if (!parse(jd)) {
		DEBUG_JIT_COMPILEVERBOSE("Exception while parsing: ");

		return NULL;
	}
	RT_TIMING_GET_TIME(time_parse);

	DEBUG_JIT_COMPILEVERBOSE("Parsing done: ");

	DEBUG_JIT_COMPILEVERBOSE("Analysing: ");

	/* call stack analysis pass */

	if (!stack_analyse(jd)) {
		DEBUG_JIT_COMPILEVERBOSE("Exception while analysing: ");

		return NULL;
	}
	RT_TIMING_GET_TIME(time_stack);

	DEBUG_JIT_COMPILEVERBOSE("Analysing done: ");

#ifdef ENABLE_VERIFIER
	if (JITDATA_HAS_FLAG_VERIFY(jd)) {
		DEBUG_JIT_COMPILEVERBOSE("Typechecking: ");

		/* call typecheck pass */
		if (!typecheck(jd)) {
			DEBUG_JIT_COMPILEVERBOSE("Exception while typechecking: ");

			return NULL;
		}

		DEBUG_JIT_COMPILEVERBOSE("Typechecking done: ");
	}
#endif
	RT_TIMING_GET_TIME(time_typecheck);

#if defined(ENABLE_LOOP)
	if (opt_loops) {
		depthFirst(jd);
		analyseGraph(jd);
		optimize_loops(jd);
		jit_renumber_basicblocks(jd);
	}
#endif
	RT_TIMING_GET_TIME(time_loop);

#if defined(ENABLE_IFCONV)
	if (JITDATA_HAS_FLAG_IFCONV(jd)) {
		if (!ifconv_static(jd))
			return NULL;
		jit_renumber_basicblocks(jd);
	}
#endif
	RT_TIMING_GET_TIME(time_ifconv);

	/* inlining */

#if defined(ENABLE_INLINING) && (!defined(ENABLE_ESCAPE) || 1)
	if (JITDATA_HAS_FLAG_INLINE(jd)) {
		if (!inline_inline(jd))
			return NULL;
	}
#endif

#if defined(ENABLE_SSA)
	if (opt_lsra) {
		fix_exception_handlers(jd);
	}
#endif

	/* Build the CFG.  This has to be done after stack_analyse, as
	   there happens the JSR elimination. */

	if (!cfg_build(jd))
		return NULL;

#if defined(ENABLE_PROFILING)
	/* Basic block reordering.  I think this should be done after
	   if-conversion, as we could lose the ability to do the
	   if-conversion. */
#if 0
	if (JITDATA_HAS_FLAG_REORDER(jd)) {
		if (!reorder(jd))
			return NULL;
		jit_renumber_basicblocks(jd);
	}
#endif
#endif

#if defined(ENABLE_PM_HACKS)
#include "vm/jit/jit_pm_2.inc"
#endif
	DEBUG_JIT_COMPILEVERBOSE("Allocating registers: ");

#if defined(ENABLE_LSRA) && !defined(ENABLE_SSA)
	/* allocate registers */
	if (opt_lsra) {
		if (!lsra(jd))
			return NULL;

		STATISTICS(count_methods_allocated_by_lsra++);

	} else
# endif /* defined(ENABLE_LSRA) && !defined(ENABLE_SSA) */
#if defined(ENABLE_SSA)
	/* allocate registers */
	if (
		(opt_lsra &&
		jd->code->optlevel > 0) 
		/* strncmp(jd->m->name->text, "hottie", 6) == 0*/
		/*&& jd->exceptiontablelength == 0*/
	) {
		/*printf("=== %s ===\n", jd->m->name->text);*/
		jd->ls = (lsradata*) DumpMemory::allocate(sizeof(lsradata));
		jd->ls = NULL;
		ssa(jd);
		/*lsra(jd);*/ regalloc(jd);
		/*eliminate_subbasicblocks(jd);*/
		STATISTICS(count_methods_allocated_by_lsra++);

	} else
# endif /* defined(ENABLE_SSA) */
	{
		STATISTICS(count_locals_conflicts += (jd->maxlocals - 1) * (jd->maxlocals));

		regalloc(jd);
	}

	STATISTICS(simplereg_make_statistics(jd));

	DEBUG_JIT_COMPILEVERBOSE("Allocating registers done: ");
	RT_TIMING_GET_TIME(time_alloc);

#if defined(ENABLE_CFG_PRINTER)
#if 0
		/* FIXME: WOW that is dirty. Use filter stuff instead */
		/* Only method is need, signature and class are optional */
		#if 1
		if (opt_CFGPrinterMethod
		    && strcmp(m->name->text, opt_CFGPrinterMethod) == 0)
		if (!opt_CFGPrinterClass
		    || strcmp(m->clazz->name->text, opt_CFGPrinterClass) == 0)
		if (!opt_CFGPrinterSignature
		    || strcmp(m->descriptor->text, opt_CFGPrinterSignature) == 0)
		#endif
		{

			GraphPrinter<JitDataGraph>::print(get_filename(m,jd,"cfg_",".dot"),
			                                  JitDataGraph(*jd, opt_CFGPrinterVerbose));
		}
#endif
#endif

#if defined(ENABLE_PROFILING)
	/* Allocate memory for basic block profiling information. This
	   _must_ be done after loop optimization and register allocation,
	   since they can change the basic block count. */

	if (JITDATA_HAS_FLAG_INSTRUMENT(jd)) {
		code->basicblockcount = jd->basicblockcount;
		code->bbfrequency = MNEW(u4, jd->basicblockcount);
	}
#endif

	DEBUG_JIT_COMPILEVERBOSE("Generating code: ");

	/* now generate the machine code */

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (opt_intrp) {
#if defined(ENABLE_VERIFIER)
		if (opt_verify) {
			DEBUG_JIT_COMPILEVERBOSE("Typechecking (stackbased): ");

			if (!typecheck_stackbased(jd)) {
				DEBUG_JIT_COMPILEVERBOSE("Exception while typechecking (stackbased): ");
				return NULL;
			}
		}
#endif
		if (!intrp_codegen(jd)) {
			DEBUG_JIT_COMPILEVERBOSE("Exception while generating code: ");

			return NULL;
		}
	} else
# endif
		{
			if (!codegen_generate(jd)) {
				DEBUG_JIT_COMPILEVERBOSE("Exception while generating code: ");

				return NULL;
			}
		}
#else
	if (!intrp_codegen(jd)) {
		DEBUG_JIT_COMPILEVERBOSE("Exception while generating code: ");

		return NULL;
	}
#endif
	RT_TIMING_GET_TIME(time_codegen);

	DEBUG_JIT_COMPILEVERBOSE("Generating code done: ");

#if !defined(NDEBUG) && defined(ENABLE_REPLACEMENT)
	/* activate replacement points inside newly created code */

	if (opt_TestReplacement)
		replace_activate_replacement_points(code, false);
#endif

#if !defined(NDEBUG)
#if defined(ENABLE_DEBUG_FILTER)
	if (jd->m->filtermatches & SHOW_FILTER_FLAG_SHOW_METHOD)
#endif
	{
		/* intermediate and assembly code listings */

		if (JITDATA_HAS_FLAG_SHOWINTERMEDIATE(jd)) {
			show_method(jd, SHOW_CODE);
		}
		else if (JITDATA_HAS_FLAG_SHOWDISASSEMBLE(jd)) {
# if defined(ENABLE_DISASSEMBLER)
			DISASSEMBLE(code->entrypoint,
						code->entrypoint + (code->mcodelength - cd->dseglen));
# endif
		}

		if (opt_showddatasegment)
			dseg_display(jd);
	}
#endif

	/* switch to the newly generated code */

	assert(code);
	assert(code->entrypoint);

	/* add the current compile version to the methodinfo */

	code->prev = m->code;
	m->code = code;

	RT_TIMING_TIME_DIFF(time_start,time_checks,RT_TIMING_JIT_CHECKS);
	RT_TIMING_TIME_DIFF(time_checks,time_parse,RT_TIMING_JIT_PARSE);
	RT_TIMING_TIME_DIFF(time_parse,time_stack,RT_TIMING_JIT_STACK);
	RT_TIMING_TIME_DIFF(time_stack,time_typecheck,RT_TIMING_JIT_TYPECHECK);
	RT_TIMING_TIME_DIFF(time_typecheck,time_loop,RT_TIMING_JIT_LOOP);
	RT_TIMING_TIME_DIFF(time_loop,time_alloc,RT_TIMING_JIT_ALLOC);
	RT_TIMING_TIME_DIFF(time_alloc,time_codegen,RT_TIMING_JIT_CODEGEN);
	RT_TIMING_TIME_DIFF(time_start,time_codegen,RT_TIMING_JIT_TOTAL);

	/* return pointer to the methods entry point */

	return code->entrypoint;
}

} // end anonymous namespace

namespace cacao {
namespace jit {
namespace compiler2 {

MachineCode* compile(methodinfo* m)
{
	INFO(dbg() << BOLDWHITE << "Compiler Start: " << RESET ; method_print(m); dbg() << "\n";)
	MachineCode* mc = compile_intern(m);
	INFO(dbg() << BOLDWHITE << "Compiler End: " << RESET ; method_print(m); dbg() << "\n";)
	return mc;
}

} // end namespace cacao
} // end namespace jit
} // end namespace compiler2

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
