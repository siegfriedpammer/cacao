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

#include "toolbox/OStream.hpp"

#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/Compiler.hpp"
#include "vm/jit/compiler2/PassManager.hpp"

#include "vm/jit/compiler2/ParserPass.hpp"
#include "vm/jit/compiler2/StackAnalysisPass.hpp"
# if defined(ENABLE_VERIFIER)
#include "vm/jit/compiler2/VerifierPass.hpp"
#endif
#include "vm/jit/compiler2/CFGConstructionPass.hpp"
#include "vm/jit/compiler2/SSAConstructionPass.hpp"
#include "vm/jit/compiler2/DominatorPass.hpp"
#include "vm/jit/compiler2/ScheduleEarlyPass.hpp"
#include "vm/jit/compiler2/ScheduleLatePass.hpp"
#include "vm/jit/compiler2/ScheduleClickPass.hpp"
#include "vm/jit/compiler2/DomTreePrinterPass.hpp"
#include "vm/jit/compiler2/LoweringPass.hpp"
#include "vm/jit/compiler2/ListSchedulingPass.hpp"
#include "vm/jit/compiler2/BasicBlockSchedulingPass.hpp"
#include "vm/jit/compiler2/ResolveImmediatePass.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/LivetimeAnalysisPass.hpp"
#include "vm/jit/compiler2/LoopPass.hpp"
#include "vm/jit/compiler2/LoopSimplificationPass.hpp"
#include "vm/jit/compiler2/SSAPrinterPass.hpp"
#include "vm/jit/compiler2/MachineInstructionPrinterPass.hpp"
#include "vm/jit/compiler2/RegisterAllocatorPass.hpp"
#include "vm/jit/compiler2/CodeGenPass.hpp"

#include "vm/jit/compiler2/JITData.hpp"

#include "vm/options.hpp"

#if 0
RT_REGISTER_GROUP(compiler2_group,"compiler2","second-stage compiler")

RT_REGISTER_GROUP_TIMER(checks_timer,        "compiler2","checks at beginning",          compiler2_group)
RT_REGISTER_GROUP_TIMER(parse_timer,         "compiler2","parse",                        compiler2_group)
RT_REGISTER_GROUP_TIMER(stack_timer,         "compiler2","analyse_stack",                compiler2_group)
RT_REGISTER_GROUP_TIMER(typechecker_timer,   "compiler2","typecheck",                    compiler2_group)
RT_REGISTER_GROUP_TIMER(loop_timer,          "compiler2","loop",                         compiler2_group)
RT_REGISTER_GROUP_TIMER(ifconversion_timer,  "compiler2","if conversion",                compiler2_group)
RT_REGISTER_GROUP_TIMER(ra_timer,            "compiler2","register allocation",          compiler2_group)
RT_REGISTER_GROUP_TIMER(rp_timer,            "compiler2","replacement point generation", compiler2_group)
RT_REGISTER_GROUP_TIMER(codegen_timer,       "compiler2","codegen",                      compiler2_group)
#endif

STAT_REGISTER_GROUP(compiler2_stat,"compiler2","statistics for compiler2")

namespace {

/* dummy function, used when there is no JavaVM code available                */

u1 *do_nothing_function(void)
{
	return NULL;
}
} // end anonymous namespace

namespace cacao {
namespace jit {
namespace compiler2 {


#define DEBUG_NAME "compiler2"

MachineCode* compile(methodinfo* m)
{

	// reset instructions
	Instruction::reset();

	PassManager PM;

	LOG(bold << bold << "Compiler Start: " << reset_color << *m << nl);

	PM.add_Pass(&ParserPass::ID);
	PM.add_Pass(&StackAnalysisPass::ID);
#ifdef ENABLE_VERIFIER
	PM.add_Pass(&VerifierPass::ID);
#endif
	PM.add_Pass(&CFGConstructionPass::ID);
	PM.add_Pass(&SSAConstructionPass::ID);
	PM.add_Pass(&LoopPass::ID);
	PM.add_Pass(&DominatorPass::ID);
	PM.add_Pass(&SSAPrinterPass::ID);
	PM.add_Pass(&ScheduleEarlyPass::ID);
	PM.add_Pass(&ScheduleLatePass::ID);
	PM.add_Pass(&ScheduleClickPass::ID);
	PM.add_Pass(&InstructionLinkSchedulePrinterPass<ScheduleEarlyPass>::ID);
	PM.add_Pass(&InstructionLinkSchedulePrinterPass<ScheduleLatePass>::ID);
	PM.add_Pass(&InstructionLinkSchedulePrinterPass<ScheduleClickPass>::ID);
	//PM.add_Pass(&LoopSimplificationPass::ID);
	PM.add_Pass(&DomTreePrinterPass::ID);
	PM.add_Pass(&LoweringPass::ID);
	PM.add_Pass(&ListSchedulingPass::ID);
	PM.add_Pass(&BasicBlockSchedulingPass::ID);
	PM.add_Pass(&ResolveImmediatePass::ID);
	PM.add_Pass(&MachineInstructionSchedulingPass::ID);
	PM.add_Pass(&LivetimeAnalysisPass::ID);
	PM.add_Pass(&MachineInstructionPrinterPass::ID);
	PM.add_Pass(&RegisterAllocatorPass::ID);
	PM.add_Pass(&CodeGenPass::ID);

/*****************************************************************************/
/** prolog start jit_compile **/
/*****************************************************************************/
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

	//DEBUG_JIT_COMPILEVERBOSE("Recompiling start: ");

	//STATISTICS(count_jit_calls++);

#if 0 && defined(ENABLE_STATISTICS)
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
/*****************************************************************************/
/** prolog end jit_compile **/
/*****************************************************************************/
	JITData JD(jd);
/*****************************************************************************/
/** prolog start jit_compile_intern **/
/*****************************************************************************/

	codegendata *cd;
	codeinfo    *code;

	//RT_TIMER_START(checks_timer);

	/* get required compiler data */

#if defined(ENABLE_LSRA) || defined(ENABLE_SSA)
	JD.get_jitdata()->ls = NULL;
#endif
	code = JD.get_jitdata()->code;
	cd   = JD.get_jitdata()->cd;

#if defined(ENABLE_DEBUG_FILTER)
	show_filters_apply(JD.get_jitdata()->m);
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
		//DEBUG_JIT_COMPILEVERBOSE("No code given for: ");

		code->entrypoint = (u1 *) (ptrint) do_nothing_function;
		m->code = code;

		return code->entrypoint;        /* return empty method                */
	}

#if 0 && defined(ENABLE_STATISTICS)
	if (opt_stat) {
		count_javacodesize += m->jcodelength + 18;
		count_tryblocks    += JD.get_jitdata()->exceptiontablelength;
		count_javaexcsize  += JD.get_jitdata()->exceptiontablelength * SIZEOF_VOID_P;
	}
#endif

#if defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)
	/* Code for Sun's OpenJDK (see
	   hotspot/src/share/vm/classfile/verifier.cpp
	   (Verifier::is_eligible_for_verification)): Don't verify
	   dynamically-generated bytecodes. */

# if defined(ENABLE_VERIFIER)
	if (class_issubclass(m->clazz, class_sun_reflect_MagicAccessorImpl))
		JD.get_jitdata()->flags &= ~JITDATA_FLAG_VERIFY;
# endif
#endif

/*****************************************************************************/
/** prolog end jit_compile_intern **/
/*****************************************************************************/
	PM.runPasses(JD);
	assert(code);
	assert(code->entrypoint);


	code->prev = m->code;
	m->code = code;
	r = JD.get_jitdata()->code->entrypoint;
/*****************************************************************************/
/** epilog  start jit_compile **/
/*****************************************************************************/
	opt_RegallocSpillAll = bak;

	if (r == NULL) {
		/* We had an exception! Finish stuff here if necessary. */

		/* release codeinfo */

		code_codeinfo_free(JD.get_jitdata()->code);
	}

#if 0 && defined(ENABLE_STATISTICS)
	/* measure time */

	if (opt_getcompilingtime)
		compilingtime_stop();
#endif

	// Hook point just after code was generated.
	Hook::jit_generated(m, m->code);

	//DEBUG_JIT_COMPILEVERBOSE("Recompiling done: ");

	/* return pointer to the methods entry point */

/** epilog end jit_compile **/

	LOG(bold << bold << "Compiler End: " << reset_color << *m << nl);

	//return mc;
	return r;
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
