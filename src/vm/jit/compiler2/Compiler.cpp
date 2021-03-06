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
#include "vm/jit/code.hpp"
#include "vm/jit/ir/instruction.hpp"
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

#include "vm/jit/compiler2/ICMDPrinterPass.hpp"
#include "vm/jit/compiler2/ParserPass.hpp"
#include "vm/jit/compiler2/StackAnalysisPass.hpp"
# if defined(ENABLE_VERIFIER)
#include "vm/jit/compiler2/VerifierPass.hpp"
#endif
#include "vm/jit/compiler2/CFGConstructionPass.hpp"
#include "vm/jit/compiler2/SSAConstructionPass.hpp"
#include "vm/jit/compiler2/ExamplePass.hpp"
#include "vm/jit/compiler2/DominatorPass.hpp"
#include "vm/jit/compiler2/ScheduleEarlyPass.hpp"
#include "vm/jit/compiler2/ScheduleLatePass.hpp"
#include "vm/jit/compiler2/ScheduleClickPass.hpp"
#include "vm/jit/compiler2/DomTreePrinterPass.hpp"
#include "vm/jit/compiler2/BasicBlockSchedulingPass.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/ListSchedulingPass.hpp"
#include "vm/jit/compiler2/LoopPass.hpp"
#include "vm/jit/compiler2/LoopTreePrinterPass.hpp"
#include "vm/jit/compiler2/LoopSimplificationPass.hpp"
#include "vm/jit/compiler2/SSAPrinterPass.hpp"
#include "vm/jit/compiler2/MachineInstructionPrinterPass.hpp"
#include "vm/jit/compiler2/MachineLoopPass.hpp"
#include "vm/jit/compiler2/CodeGenPass.hpp"
#include "vm/jit/compiler2/DisassemblerPass.hpp"
#include "vm/jit/compiler2/ObjectFileWriterPass.hpp"
#include "vm/jit/compiler2/DeadCodeEliminationPass.hpp"
#include "vm/jit/compiler2/ConstantPropagationPass.hpp"
#include "vm/jit/compiler2/GlobalValueNumberingPass.hpp"
#include "vm/jit/compiler2/InliningPass.hpp"

#include "vm/jit/compiler2/treescan/LivetimeAnalysisPass.hpp"

#include "vm/jit/compiler2/JITData.hpp"

#include "vm/options.hpp"

#include "mm/dumpmemory.hpp"

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

#define DEBUG_NAME "compiler2/Compiler"

Option<bool> enabled("DebugCompiler2","compiler with compiler2",false,option::xx_root());

MachineCode* compile(methodinfo* m)
{

	// reset instructions
	Instruction::reset();

	// Currently we do not handle synchronized methods, since the monitors are not
	// emitted
	if (m->flags & ACC_SYNCHRONIZED) {
		throw std::runtime_error("Compiler.cpp: Method is flaged as synchronized, monitor enter/exit not implemented in c2!");
	}

	LOG(bold << bold << "Compiler Start: " << reset_color << *m << nl);

	// Create new dump memory area.
	DumpMemoryArea dma;

	/* create jitdata structure */

	jitdata *jd = jit_jitdata_new(m);
	jit_jitdata_init_for_recompilation(jd);
	jd->flags |= JITDATA_FLAG_DEOPTIMIZE;
	JITData JD(jd);

	/* set the current optimization level to the previous one plus 1 */

	u1 optlevel = (jd->m->code) ? jd->m->code->optlevel : 0;
	jd->code->optlevel = optlevel + 2;

	/* now call internal compile function */

	bool bak = opt_RegallocSpillAll;
	opt_RegallocSpillAll = true;

/*****************************************************************************/
/** prolog start jit_compile_intern **/
/*****************************************************************************/

	//codegendata *cd;
	codeinfo    *code;

	//RT_TIMER_START(checks_timer);

	/* get required compiler data */

#if defined(ENABLE_LSRA) || defined(ENABLE_SSA)
	JD.get_jitdata()->ls = NULL;
#endif
	code = JD.get_jitdata()->code;
	//cd   = JD.get_jitdata()->cd;

#if defined(ENABLE_DEBUG_FILTER)
	show_filters_apply(JD.get_jitdata()->m);
#endif

	// Handle native methods and create a native stub.
	if (m->flags & ACC_NATIVE) {
		NativeMethods& nm = VM::get_current()->get_nativemethods();
		void* f = nm.resolve_method(m);

		if (f == NULL)
			return NULL;

		code = NativeStub::generate(m, *reinterpret_cast<functionptr*>(&f));

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

	/* run the compiler2 passes */

	PassRunner runner;
	runner.runPasses(JD);
	
	assert(code);
	assert(code->entrypoint);

	u1 *entrypoint = JD.get_jitdata()->code->entrypoint;
	
/*****************************************************************************/
/** epilog  start jit_compile **/
/*****************************************************************************/
	opt_RegallocSpillAll = bak;

	if (entrypoint == NULL) {
		/* We had an exception! Finish stuff here if necessary. */

		/* release codeinfo */

		code_codeinfo_free(JD.get_jitdata()->code);
	}

#if 0 && defined(ENABLE_STATISTICS)
	/* measure time */

	if (opt_getcompilingtime)
		compilingtime_stop();
#endif

	code->prev = m->code;
	m->code = code;

	// Hook point just after code was generated.
	Hook::jit_generated(m, m->code);

	

	LOG(bold << bold << "Compiler End: " << reset_color << *m << nl);

	/* return pointer to the methods entry point */
	return entrypoint;
}

bool can_possibly_compile(void* jd_ptr)
{
	jitdata *jd = (jitdata*) jd_ptr;
	methodinfo *m = jd->m;

	// Currently we do not handle synchronized methods, since the monitors are
	// not emitted.
	if (m->flags & ACC_SYNCHRONIZED) return false;

	// Native methods are handled correctly anyway.
	if (m->flags & ACC_NATIVE) return true;

	// The method sun/nio/cs/StandardCharsets$Aliases.init
	// is only called once and takes too long to compile with the second stage.
	// So we skip it here :-)
	if (m->clazz && m->clazz->name.equals("sun/nio/cs/StandardCharsets$Aliases")
	    	&& m->name.equals("init"))
		return false;

	// This method has a bug during compilation, skip it.
	if (m->clazz && m->clazz->name.equals("com/sun/tools/javac/util/Messages"))
		return false;

	basicblock *bb;
	FOR_EACH_BASICBLOCK(jd,bb) {
	
	instruction *iptr;
	FOR_EACH_INSTRUCTION(bb,iptr) {
		switch (iptr->opc) {
			case ICMD_CHECKNULL:
			case ICMD_ISHL:
			case ICMD_LSHL:
			case ICMD_ISHR:
			case ICMD_LSHR:
			case ICMD_IUSHR:
			case ICMD_LUSHR:
			case ICMD_IMULPOW2:
			case ICMD_IDIVPOW2:
			case ICMD_IREMPOW2:
			case ICMD_IANDCONST:
			case ICMD_IORCONST:
			case ICMD_IXORCONST:
			case ICMD_ISHLCONST:
			case ICMD_ISHRCONST:
			case ICMD_IUSHRCONST:
			case ICMD_LSHLCONST:
			case ICMD_LSHRCONST:
			case ICMD_LUSHRCONST:
			case ICMD_RET:
			case ICMD_CHECKCAST:
			case ICMD_INSTANCEOF:
			case ICMD_JSR:
			case ICMD_ATHROW:
			case ICMD_GETEXCEPTION:
			case ICMD_LMULPOW2:
			case ICMD_LDIVPOW2:
			case ICMD_LREMPOW2:
			case ICMD_LORCONST:
			case ICMD_LXORCONST:
				return false;

			// Array loads/stores cause more bugs at the moment,
			// so we disable them for now.
			case ICMD_IALOAD:
			case ICMD_SALOAD:
			case ICMD_BALOAD:
			case ICMD_CALOAD:
			case ICMD_LALOAD:
			case ICMD_DALOAD:
			case ICMD_FALOAD:
			case ICMD_AALOAD:
			case ICMD_IASTORE:
			case ICMD_SASTORE:
			case ICMD_BASTORE:
			case ICMD_CASTORE:
			case ICMD_LASTORE:
			case ICMD_DASTORE:
			case ICMD_FASTORE:
			case ICMD_AASTORE:
			case ICMD_BASTORECONST:
			case ICMD_CASTORECONST:
			case ICMD_SASTORECONST:
			case ICMD_AASTORECONST:
			case ICMD_IASTORECONST:
			case ICMD_LASTORECONST:
				return false;

			case ICMD_INVOKESPECIAL:
			case ICMD_INVOKESTATIC:
				{
					// Only check if the instruction is resolved yet, otherwise
					// we might still be able to compile.
					if (INSTRUCTION_IS_RESOLVED(iptr)) {
						constant_FMIref *fmiref = nullptr;
						INSTRUCTION_GET_METHODREF(iptr, fmiref);

						// Compiler2 can't invoke native methods.
						if (fmiref && fmiref->p.method->flags & ACC_NATIVE) return false;
					}
				}
			default: 
				// Do nothing, we can handle this.
				break;
		}
	}

	}

	return true;
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
