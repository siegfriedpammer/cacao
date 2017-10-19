/* src/vm/jit/compiler2/lsra/LoopPressurePass.cpp - LoopPressurePass

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

#include "vm/jit/compiler2/lsra/LoopPressurePass.hpp"

#include "vm/jit/compiler2/MachineLoopPass.hpp"
#include "vm/jit/compiler2/lsra/NewLivetimeAnalysisPass.hpp"

#define DEBUG_NAME "compiler2/LoopPressurePass"

namespace cacao {
namespace jit {
namespace compiler2 {

namespace {

/**
 * Merges two PressureTys by taking the maximum for each register class
 */
static PressureTy max(const RegisterInfo& RI, const PressureTy& lhs, const PressureTy& rhs)
{
	PressureTy result(lhs.size());
	for (unsigned i = 0; i < RI.class_count(); ++i) {
		result[i] = std::max(lhs[i], rhs[i]);
	}
	return result;
}

} // end anonymous namespace

const PressureTy& LoopPressurePass::get_block_pressure(MachineBasicBlock* block)
{
	auto iter = block_pressures.find(block);
	if (iter == block_pressures.end()) {
		compute_block_pressure(block);
	}
	return *block_pressures.at(block);
}

void LoopPressurePass::compute_block_pressure(MachineBasicBlock* block)
{
	auto LTA = get_Pass<NewLivetimeAnalysisPass>();
	auto pressures_ptr = std::make_unique<PressureTy>(RI->class_count());
	auto& pressures = *pressures_ptr;

	// Initial pressures are taken from live out
	auto live = LTA->get_live_out(block);
	for (unsigned i = 0, e = RI->class_count(); i < e; ++i) {
		const auto live_rc = live & class_operands[i];
		pressures[i] = live_rc.size();
	}

	// Now do the usual liveness transfer and compute max pressures at each
	// instruction for each register class
	for (auto i = block->rbegin(), e = block->rend(); i != e; ++i) {
		live = LTA->liveness_transfer(live, block->convert(i));

		for (unsigned j = 0, je = RI->class_count(); j < je; ++j) {
			const auto live_rc = live & class_operands[j];
			pressures[j] = std::max<unsigned>(pressures[j], live_rc.size());
		}
	}

	// Finally, also take PHI definitions into account
	live |= LTA->mbb_phi_defs(block);
	for (unsigned i = 0, e = RI->class_count(); i < e; ++i) {
		const auto live_rc = live & class_operands[i];
		pressures[i] = std::max<unsigned>(pressures[i], live_rc.size());
	}

	block_pressures[block] = std::move(pressures_ptr);
}

PressureTy LoopPressurePass::looptree_walk(MachineLoop* loop)
{
	LOG1(Yellow << "Processing loop: " << loop << reset_color << nl);

	PressureTy loop_pressure(RI->class_count());

	// First, get the max pressure of all basic blocks
	for (auto i = loop->child_begin(), e = loop->child_end(); i != e; ++i) {
		loop_pressure = max(*RI, loop_pressure, get_block_pressure(*i));
	}

	// Second, calculate it for all nested loop
	for (auto i = loop->loop_begin(), e = loop->loop_end(); i != e; ++i) {
		auto tmp_pressure = looptree_walk(*i);
		loop_pressure = max(*RI, loop_pressure, tmp_pressure);
	}

	loop->set_register_pressures(loop_pressure);

	return loop_pressure;
}

bool LoopPressurePass::run(JITData& JD)
{
	LOG(nl << BoldYellow << "Running LoopPressurePass" << reset_color << nl);

	RI = JD.get_Backend()->get_RegisterInfo();

	// Initialize the class operand sets, they are used to restrict
	// liveness transfer results to a certain register class
	auto MOF = JD.get_MachineOperandFactory();
	for (unsigned i = 0, e = RI->class_count(); i < e; ++i) {
		const auto& rc = RI->get_class(i);
		class_operands.push_back(MOF->OperandsInClass(rc));
	}

	auto MLP = get_Pass<MachineLoopPass>();
	for (auto i = MLP->loop_begin(), e = MLP->loop_end(); i != e; ++i) {
		looptree_walk(*i);
	}
	return true;
}

// pass usage
PassUsage& LoopPressurePass::get_PassUsage(PassUsage& PU) const
{
	PU.add_requires<MachineLoopPass>();
	PU.add_requires<NewLivetimeAnalysisPass>();
	return PU;
}

// register pass
static PassRegistry<LoopPressurePass> X("LoopPressurePass");

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

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
