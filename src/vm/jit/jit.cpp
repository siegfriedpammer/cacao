/* src/vm/jit/jit.cpp - Just-In-Time compiler

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

*/


#include "config.h"

#include <assert.h>
#include <stdint.h>

#include "vm/types.h"

#include "md.h"

#include "mm/memory.h"

#include "native/native.hpp"

#include "toolbox/logging.h"

#include "threads/mutex.hpp"

#include "vm/class.hpp"
#include "vm/global.h"
#include "vm/globals.hpp"
#include "vm/initialize.hpp"
#include "vm/loader.hpp"
#include "vm/method.h"
#include "vm/options.h"
#include "vm/rt-timing.h"
#include "vm/statistics.h"

#include "vm/jit/asmpart.h"

#include "vm/jit/cfg.h"

#include "vm/jit/codegen-common.hpp"
#include "vm/jit/disass.h"
#include "vm/jit/dseg.h"
#include "vm/jit/jit.hpp"
#include "vm/jit/parse.hpp"
#include "vm/jit/reg.h"

#include "vm/jit/show.hpp"
#include "vm/jit/stack.h"
#include "vm/jit/stubs.hpp"

#if defined(ENABLE_JITCACHE)
# include "vm/jit/jitcache.hpp"
#endif

#if defined(ENABLE_OPAGENT)
#include "vm/jit/oprofile-agent.hpp"
#endif

#include "vm/jit/allocator/simplereg.h"
#if defined(ENABLE_LSRA) && !defined(ENABLE_SSA)
# include "vm/jit/allocator/lsra.h"
#endif

#if defined(ENABLE_SSA)
# include "vm/jit/optimizing/lsra.h"
# include "vm/jit/optimizing/ssa.h"
#endif

#if defined(ENABLE_INLINING)
# include "vm/jit/inline/inline.h"
#endif

#include "vm/jit/ir/bytecode.h"

#include "vm/jit/loop/analyze.h"
#include "vm/jit/loop/graph.h"
#include "vm/jit/loop/loop.h"

#if defined(ENABLE_IFCONV)
# include "vm/jit/optimizing/ifconv.h"
#endif

#include "vm/jit/optimizing/reorder.h"

#if defined(ENABLE_PYTHON)
# include "vm/jit/python.h"
#endif

#include "vm/jit/verify/typecheck.h"


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


/* jit_init ********************************************************************

   Initializes the JIT subsystem.

*******************************************************************************/

void jit_init(void)
{
	TRACESUBSYSTEMINITIALIZATION("jit_init");

#if defined(ENABLE_JIT)
	/* initialize stack analysis subsystem */

	(void) stack_init();
#endif

	/* initialize show subsystem */

#if !defined(NDEBUG)
	(void) show_init();
#endif

	/* initialize codegen subsystem */

	codegen_init();

	/* initialize code subsystem */

	(void) code_init();

	/* Machine dependent initialization. */

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (opt_intrp)
		intrp_md_init();
	else
# endif
		md_init();
#else
	intrp_md_init();
#endif

#if defined(ENABLE_OPAGENT)
	if (opt_EnableOpagent)
		OprofileAgent::initialize();
#endif
}


/* jit_close *******************************************************************

   Close the JIT subsystem.

*******************************************************************************/

void jit_close(void)
{
#if defined(ENABLE_OPAGENT)
	if (opt_EnableOpagent)
		OprofileAgent::close();
#endif
}


/* dummy function, used when there is no JavaVM code available                */

static u1 *do_nothing_function(void)
{
	return NULL;
}


/* jit_jitdata_new *************************************************************

   Allocates and initalizes a new jitdata structure.

*******************************************************************************/

jitdata *jit_jitdata_new(methodinfo *m)
{
	jitdata  *jd;
	codeinfo *code;

	/* allocate jitdata structure and fill it */

	jd = (jitdata*) DumpMemory::allocate(sizeof(jitdata));

	jd->m     = m;
	jd->cd    = (codegendata*) DumpMemory::allocate(sizeof(codegendata));
	jd->rd    = (registerdata*) DumpMemory::allocate(sizeof(registerdata));
#if defined(ENABLE_LOOP)
	jd->ld    = (loopdata*) DumpMemory::allocate(sizeof(loopdata));
#endif

	/* Allocate codeinfo memory from the heap as we need to keep them. */

	code = code_codeinfo_new(m);

	/* Set codeinfo flags. */

#if defined(ENABLE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		code_flag_synchronized(code);

	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		code_unflag_leafmethod(code);
	else
#endif
		code_flag_leafmethod(code);

	/* initialize variables */

	jd->code                 = code;
	jd->flags                = 0;
	jd->exceptiontable       = NULL;
	jd->exceptiontablelength = 0;
	jd->returncount          = 0;
	jd->branchtoentry        = false;
	jd->branchtoend          = false;
	jd->returncount          = 0;
	jd->returnblock          = NULL;
	jd->maxlocals            = m->maxlocals;

	return jd;
}


/* jit_compile *****************************************************************

   Translates one method to machine code.

*******************************************************************************/

static u1 *jit_compile_intern(jitdata *jd);

u1 *jit_compile(methodinfo *m)
{
	u1      *r;
	jitdata *jd;

	STATISTICS(count_jit_calls++);

	/* Initialize the static function's class. */

	/* ATTENTION: This MUST be done before the method lock is aquired,
	   otherwise we could run into a deadlock with <clinit>'s that
	   call static methods of it's own class. */

  	if ((m->flags & ACC_STATIC) && !(m->clazz->state & CLASS_INITIALIZED)) {
#if !defined(NDEBUG)
		if (initverbose)
			log_message_class("Initialize class ", m->clazz);
#endif

		if (!initialize_class(m->clazz))
			return NULL;

		/* check if the method has been compiled during initialization */

		if ((m->code != NULL) && (m->code->entrypoint != NULL))
			return m->code->entrypoint;
	}

	/* enter a monitor on the method */

	m->mutex->lock();

	/* if method has been already compiled return immediately */

	if (m->code != NULL) {
		m->mutex->unlock();

		assert(m->code->entrypoint);
		return m->code->entrypoint;
	}

	TRACECOMPILERCALLS();

	STATISTICS(count_methods++);

#if defined (ENABLE_JITCACHE)

	if (jitcache_load (m))
	{
		m->mutex->unlock();

		return m->code->entrypoint;
	}

#endif

#if defined(ENABLE_STATISTICS)
	/* measure time */

	if (opt_getcompilingtime)
		compilingtime_start();
#endif

	// Create new dump memory area.
	DumpMemoryArea dma;

	/* create jitdata structure */

	jd = jit_jitdata_new(m);

	/* set the flags for the current JIT run */

	jd->flags = JITDATA_FLAG_PARSE;

#if defined(ENABLE_VERIFIER)
	if (opt_verify)
		jd->flags |= JITDATA_FLAG_VERIFY;
#endif

#if defined(ENABLE_PROFILING)
	if (opt_prof)
		jd->flags |= JITDATA_FLAG_INSTRUMENT;
#endif

#if defined(ENABLE_IFCONV)
	if (opt_ifconv)
		jd->flags |= JITDATA_FLAG_IFCONV;
#endif

#if defined(ENABLE_INLINING) && defined(ENABLE_INLINING_DEBUG)
	if (opt_Inline && opt_InlineAll)
		jd->flags |= JITDATA_FLAG_INLINE;
#endif

	if (opt_showintermediate)
		jd->flags |= JITDATA_FLAG_SHOWINTERMEDIATE;

	if (opt_showdisassemble)
		jd->flags |= JITDATA_FLAG_SHOWDISASSEMBLE;

	if (opt_verbosecall)
		jd->flags |= JITDATA_FLAG_VERBOSECALL;

#if defined(ENABLE_REPLACEMENT) && defined(ENABLE_INLINING)
	if (opt_Inline && (jd->m->hitcountdown > 0) && (jd->code->optlevel == 0)) {
		jd->flags |= JITDATA_FLAG_COUNTDOWN;
	}
#endif

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (!opt_intrp)
# endif
		/* initialize the register allocator */
	{
		reg_setup(jd);
	}
#endif

	/* setup the codegendata memory */

	codegen_setup(jd);

	/* now call internal compile function */

	r = jit_compile_intern(jd);

	if (r == NULL) {
		/* We had an exception! Finish stuff here if necessary. */

		/* release codeinfo */

		code_codeinfo_free(jd->code);

#if defined(ENABLE_PROFILING)
		/* Release memory for basic block profiling information. */

		if (JITDATA_HAS_FLAG_INSTRUMENT(jd))
			if (jd->code->bbfrequency != NULL)
				MFREE(jd->code->bbfrequency, u4, jd->code->basicblockcount);
#endif
	}
	else {
		DEBUG_JIT_COMPILEVERBOSE("Running: ");
	}

#if defined (ENABLE_JITCACHE)
	jitcache_store(m);
#endif

#if defined(ENABLE_STATISTICS)
	/* measure time */

	if (opt_getcompilingtime)
		compilingtime_stop();
#endif

#if defined(ENABLE_OPAGENT)
	if (opt_EnableOpagent)
		OprofileAgent::newmethod(m);
#endif

	/* leave the monitor */

	m->mutex->unlock();

	/* return pointer to the methods entry point */

	return r;
}


/* jit_recompile ***************************************************************

   Recompiles a Java method.

*******************************************************************************/

u1 *jit_recompile(methodinfo *m)
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
	if (opt_Inline)
		jd->flags |= JITDATA_FLAG_INLINE;
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

	r = jit_compile_intern(jd);

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

#if defined(ENABLE_OPAGENT)
	if (opt_EnableOpagent)
		OprofileAgent::newmethod(m);
#endif

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

static u1 *jit_compile_intern(jitdata *jd)
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
	
#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (!opt_intrp) {
# endif
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

		if (JITDATA_HAS_FLAG_REORDER(jd)) {
			if (!reorder(jd))
				return NULL;
			jit_renumber_basicblocks(jd);
		}
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
# if defined(ENABLE_INTRP)
	}
# endif
#endif /* defined(ENABLE_JIT) */
	RT_TIMING_GET_TIME(time_alloc);

#if defined(ENABLE_PROFILING)
	/* Allocate memory for basic block profiling information. This
	   _must_ be done after loop optimization and register allocation,
	   since they can change the basic block count. */

	if (JITDATA_HAS_FLAG_INSTRUMENT(jd))
		code->bbfrequency = MNEW(u4, jd->basicblockcount);
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


/* jit_invalidate_code *********************************************************

   Mark the compiled code of the given method as invalid and take care that
   it is replaced if necessary.

   XXX Not fully implemented, yet.

*******************************************************************************/

void jit_invalidate_code(methodinfo *m)
{
	codeinfo *code;

	code = m->code;

	if (code == NULL || code_is_invalid(code))
		return;

	code_flag_invalid(code);

	/* activate mappable replacement points */

#if defined(ENABLE_REPLACEMENT)
	replace_activate_replacement_points(code, true);
#else
	vm_abort("invalidating code only works with ENABLE_REPLACEMENT");
#endif
}


/* jit_request_optimization ****************************************************

   Request optimization of the given method. If the code of the method is
   unoptimized, it will be invalidated, so the next jit_get_current_code(m)
   triggers an optimized recompilation.
   If the method is already optimized, this function does nothing.

   IN:
       m................the method

*******************************************************************************/

void jit_request_optimization(methodinfo *m)
{
	codeinfo *code;

	code = m->code;

	if (code && code->optlevel == 0)
		jit_invalidate_code(m);
}


/* jit_get_current_code ********************************************************

   Get the currently valid code for the given method. If there is no valid
   code, (re)compile the method.

   IN:
       m................the method

   RETURN VALUE:
       the codeinfo* for the current code, or
	   NULL if an exception has been thrown during recompilation.

*******************************************************************************/

codeinfo *jit_get_current_code(methodinfo *m)
{
	assert(m);

	/* if we have valid code, return it */

	if (m->code && !code_is_invalid(m->code))
		return m->code;

	/* otherwise: recompile */

	if (!jit_recompile(m))
		return NULL;

	assert(m->code);

	return m->code;
}


/* jit_asm_compile *************************************************************

   This method is called from asm_vm_call_method and does:

     - create stackframe info for exceptions
     - compile the method
     - patch the entrypoint of the method into the calculated address in
       the JIT code
     - flushes the instruction cache.

*******************************************************************************/

#if defined(ENABLE_JIT)
#if !defined(JIT_COMPILER_VIA_SIGNAL)
extern "C" {
void* jit_asm_compile(methodinfo *m, void* mptr, void* sp, void* ra)
{
	stackframeinfo_t  sfi;
	void             *entrypoint;
	void             *pa;
	uintptr_t        *p;

	/* create the stackframeinfo (subtract 1 from RA as it points to the */
	/* instruction after the call)                                       */

	stacktrace_stackframeinfo_add(&sfi, NULL, sp, ra, ((uint8_t*) ra) - 1);

	/* actually compile the method */

	entrypoint = jit_compile(m);

	/* remove the stackframeinfo */

	stacktrace_stackframeinfo_remove(&sfi);

	/* there was a problem during compilation */

	if (entrypoint == NULL)
		return NULL;

	/* get the method patch address */

	pa = md_jit_method_patch_address(sfi.pv, (void *) ra, mptr);

	/* patch the method entry point */

	p = (uintptr_t*) pa;

	*p = (uintptr_t) entrypoint;

	/* flush the instruction cache */

	md_icacheflush(pa, SIZEOF_VOID_P);

	return entrypoint;
}
}
#endif

/* jit_compile_handle **********************************************************

   This method is called from the appropriate signal handler which
   handles compiler-traps and does the following:

     - compile the method
     - patch the entrypoint of the method into the calculated address in
       the JIT code
     - flush the instruction cache

*******************************************************************************/

void *jit_compile_handle(methodinfo *m, void *pv, void *ra, void *mptr)
{
	void      *newpv;                               /* new compiled method PV */
	void      *pa;                                           /* patch address */
	uintptr_t *p;                                      /* convenience pointer */

	/* Compile the method. */

	newpv = jit_compile(m);

	/* There was a problem during compilation. */

	if (newpv == NULL)
		return NULL;

	/* Get the method patch address. */

	pa = md_jit_method_patch_address(pv, ra, mptr);

	/* Patch the method entry point. */

	p = (uintptr_t *) pa;

	*p = (uintptr_t) newpv;

	/* Flush both caches. */

	md_cacheflush(pa, SIZEOF_VOID_P);

	return newpv;
}
#endif /* defined(ENABLE_JIT) */


/* jit_complement_condition ****************************************************

   Returns the complement of the passed conditional instruction.

   We use the order of the different conditions, e.g.:

   ICMD_IFEQ         153
   ICMD_IFNE         154

   If the passed opcode is odd, we simply add 1 to get the complement.
   If the opcode is even, we subtract 1.

   Exception:

   ICMD_IFNULL       198
   ICMD_IFNONNULL    199

*******************************************************************************/

s4 jit_complement_condition(s4 opcode)
{
	switch (opcode) {
	case ICMD_IFNULL:
		return ICMD_IFNONNULL;

	case ICMD_IFNONNULL:
		return ICMD_IFNULL;

	default:
		/* check if opcode is odd */

		if (opcode & 0x1)
			return opcode + 1;
		else
			return opcode - 1;
	}
}


/* jit_renumber_basicblocks ****************************************************

   Set the ->nr of all blocks so it increases when traversing ->next.

   IN:
       jitdata..........the current jitdata

*******************************************************************************/

void jit_renumber_basicblocks(jitdata *jd)
{
	s4          nr;
	basicblock *bptr;

	nr = 0;
	for (bptr = jd->basicblocks; bptr != NULL; bptr = bptr->next) {
		bptr->nr = nr++;
	}

	/* we have one block more than jd->basicblockcount (the end marker) */

	assert(nr == jd->basicblockcount + 1);
}


/* jit_check_basicblock_numbers ************************************************

   Assert that the ->nr of the first block is zero and increases by 1 each
   time ->next is traversed.
   This function should be called before any analysis that relies on
   the basicblock numbers.

   IN:
       jitdata..........the current jitdata

   NOTE: Aborts with an assertion if the condition is not met!

*******************************************************************************/

#if !defined(NDEBUG)
void jit_check_basicblock_numbers(jitdata *jd)
{
	s4          nr;
	basicblock *bptr;

	nr = 0;
	for (bptr = jd->basicblocks; bptr != NULL; bptr = bptr->next) {
		assert(bptr->nr == nr);
		nr++;
	}

	/* we have one block more than jd->basicblockcount (the end marker) */

	assert(nr == jd->basicblockcount + 1);
}
#endif /* !defined(NDEBUG) */


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
