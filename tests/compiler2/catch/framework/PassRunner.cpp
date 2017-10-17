/* tests/compiler2/catch/framework/PassRunner.cpp - PassRunner

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

#include "PassRunner.hpp"

#include <stdexcept>
#include <string>

#include "toolbox/OStream.hpp"

#include "vm/class.hpp"
#include "vm/jit/jit.hpp"
#include "vm/loader.hpp"
#include "vm/method.hpp"
#include "vm/string.hpp"
#include "vm/utf8.hpp"

#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/PassManager.hpp"

#include "vm/jit/compiler2/BasicBlockSchedulingPass.hpp"
#include "vm/jit/compiler2/CFGConstructionPass.hpp"
#include "vm/jit/compiler2/CFGMetaPass.hpp"
#include "vm/jit/compiler2/CodeGenPass.hpp"
#include "vm/jit/compiler2/DominatorPass.hpp"
#include "vm/jit/compiler2/InstructionMetaPass.hpp"
#include "vm/jit/compiler2/ListSchedulingPass.hpp"
#include "vm/jit/compiler2/LoopPass.hpp"
#include "vm/jit/compiler2/MachineDominatorPass.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/MachineLoopPass.hpp"
#include "vm/jit/compiler2/ParserPass.hpp"
#include "vm/jit/compiler2/RegisterAllocatorPass.hpp"
#include "vm/jit/compiler2/SSAConstructionPass.hpp"
#include "vm/jit/compiler2/ScheduleClickPass.hpp"
#include "vm/jit/compiler2/ScheduleEarlyPass.hpp"
#include "vm/jit/compiler2/ScheduleLatePass.hpp"
#include "vm/jit/compiler2/SourceStateAttachmentPass.hpp"
#include "vm/jit/compiler2/StackAnalysisPass.hpp"
#include "vm/jit/compiler2/VerifierPass.hpp"
#include "vm/jit/compiler2/lsra/NewLivetimeAnalysisPass.hpp"
#include "vm/jit/compiler2/lsra/RegisterAssignmentPass.hpp"
#include "vm/jit/compiler2/lsra/SpillPass.hpp"

using cacao::bold;
using cacao::err;
using cacao::nl;
using cacao::Red;
using cacao::reset_color;
using cacao::jit::compiler2::Instruction;
using cacao::jit::compiler2::PassUsage;

TestPassSchedule TestPassSchedule::default_schedule;

TestPassSchedule& TestPassSchedule::get_default()
{
	if (default_schedule.schedule.size() > 0)
		return default_schedule;

	default_schedule.add<cacao::jit::compiler2::ParserPass>()
	    .add<cacao::jit::compiler2::StackAnalysisPass>()
	    .add<cacao::jit::compiler2::VerifierPass>()
	    .add<cacao::jit::compiler2::CFGConstructionPass>()
	    .add<cacao::jit::compiler2::SSAConstructionPass>()
	    .add<cacao::jit::compiler2::CFGMetaPass>()
	    .add<cacao::jit::compiler2::LoopPass>()
	    .add<cacao::jit::compiler2::InstructionMetaPass>()
	    .add<cacao::jit::compiler2::DominatorPass>()
	    .add<cacao::jit::compiler2::ScheduleEarlyPass>()
	    .add<cacao::jit::compiler2::ScheduleLatePass>()
	    .add<cacao::jit::compiler2::ScheduleClickPass>()
	    .add<cacao::jit::compiler2::ListSchedulingPass>()
	    .add<cacao::jit::compiler2::SourceStateAttachmentPass>()
	    .add<cacao::jit::compiler2::BasicBlockSchedulingPass>()
	    .add<cacao::jit::compiler2::MachineInstructionSchedulingPass>()
	    .add<cacao::jit::compiler2::MachineLoopPass>()
	    .add<cacao::jit::compiler2::NewLivetimeAnalysisPass>()
	    .add<cacao::jit::compiler2::SpillPass>()
	    .add<cacao::jit::compiler2::MachineDominatorPass>()
	    .add<cacao::jit::compiler2::RegisterAssignmentPass>()
	    .add<cacao::jit::compiler2::RegisterAllocatorPass>()
	    .add<cacao::jit::compiler2::CodeGenPass>();
	return default_schedule;
}

void CustomPassRunner::runPasses(JITData& JD)
{
	for (const auto& id : schedule) {
		result_ready[id] = false;
		auto&& P = get_Pass(id);
		P->initialize();
		if (!P->run(JD)) {
			err() << bold << Red << "error" << reset_color << " during pass " << id << nl;
			os::abort("compiler2: error");
		}

		// invalidating results
		PassUsage PU;
		PU = P->get_PassUsage(PU);
		for (PassUsage::const_iterator i = PU.destroys_begin(), e = PU.destroys_end(); i != e;
		     ++i) {
			result_ready[*i] = false;
		}
#ifndef NDEBUG
		if (!P->verify()) {
			err() << bold << Red << "verification error" << reset_color << " during pass " << id
			      << nl;
			os::abort("compiler2: error");
		}
#endif
		result_ready[id] = true;
		P->finalize();

		if (id == until)
			break;
	}
}

void TestPassRunner::run_until(const std::string& clazz,
                               const std::string& method,
                               const std::string& desc)
{
	methodinfo* m = load_method(clazz, method, desc);

	DumpMemoryArea dma;
	Instruction::reset();

	jitdata* jd = jit_jitdata_new(m);
	jit_jitdata_init_for_recompilation(jd);
	jd->flags |= JITDATA_FLAG_DEOPTIMIZE;
	JITData JD(jd);

	runner->runPasses(JD);
}

methodinfo* TestPassRunner::load_method(const std::string& clazz,
                                        const std::string& method,
                                        const std::string& desc) const
{
	auto clazz_string = Utf8String::from_utf8(clazz.c_str());
	auto method_string = Utf8String::from_utf8_dot_to_slash(method.c_str());
	auto desc_string = Utf8String::from_utf8_dot_to_slash(desc.c_str());

	classinfo* class_info = load_class_from_sysloader(clazz_string);

	// find the method of the class
	methodinfo* m = class_resolveclassmethod(class_info, method_string, desc_string, NULL, false);
	if (!m) {
		throw std::logic_error("Not able to resolve method!");
	}

	if (exceptions_get_exception()) {
		exceptions_print_stacktrace();
		throw std::logic_error("Not able to resolve method!");
	}

	return m;
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
