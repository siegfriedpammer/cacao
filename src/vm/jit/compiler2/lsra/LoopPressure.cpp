/* src/vm/jit/compiler2/lsra/LoopPressure.cpp - LoopPressure

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

#include "vm/jit/compiler2/lsra/LoopPressure.hpp"

#include "vm/jit/compiler2/MachineLoopPass.hpp"
#include "vm/jit/compiler2/lsra/NewLivetimeAnalysisPass.hpp"
#include "vm/jit/compiler2/lsra/SpillPass.hpp"

#define DEBUG_NAME "compiler2/SpillPass/LoopPressure"

namespace cacao {
namespace jit {
namespace compiler2 {

void LoopPressureAnalysis::initialize(SpillPass* sp)
{
	SP = sp;
	compute_loop_pressures();
}

void LoopPressureAnalysis::compute_loop_pressures()
{
	auto MLP = SP->get_Pass<MachineLoopPass>();
	for (auto i = MLP->loop_begin(), e = MLP->loop_end(); i != e; ++i) {
		auto loop = *i;
		unsigned pressure = compute_loop_pressure(loop);
		loop->set_register_pressure(pressure);
	}
}

static void log_loop_entry_exit(MachineLoop* loop)
{
	LOG1("\nComputing loop pressure (loop head = " << *loop->get_header()
	                                               << ", exit = " << *loop->get_exit());
	if (loop->get_parent()) {
		LOG1(", parent head = " << *loop->get_parent()->get_header());
	}
	LOG1(")\n");
}

/**
 * Computes the highest register pressure in a block
 */
unsigned LoopPressureAnalysis::compute_block_pressure(MachineBasicBlock* block)
{
	LOG2("\tProcessing block " << *block << nl);

	auto LTA = SP->get_Pass<NewLivetimeAnalysisPass>();
	LiveTy live_set(LTA->get_live_out(block));
	unsigned max_live = live_set.count();

	LOG2("\t\tInitial max_live: " << max_live << nl);

	// Starting from LiveOut we calculate the live set at each instruction
	for (auto i = block->rbegin(), e = block->rend(); i != e; ++i) {
		auto instruction = *i;

		if (!reg_alloc_considers_instruction(instruction))
			continue;

		// Remove defined value
		auto result_op = instruction->get_result().op;
		if (reg_alloc_considers_operand(*result_op)) {
			live_set[result_op->get_id()] = false;
		}

		// Add used values
		for (const auto& operand : *instruction) {
			if (reg_alloc_considers_operand(*operand.op)) {
				live_set[operand.op->get_id()] = true;
			}
		}

		unsigned count = live_set.count();
		max_live = std::max(max_live, count);
	}

	LOG1("\tFinished Basic Block " << *block << " (max register pressure = " << max_live << ")\n");

	return max_live;
}

/**
 * Compute the highest register pressure in a loop and its sub-loops
 */
unsigned LoopPressureAnalysis::compute_loop_pressure(MachineLoop* loop)
{
	DEBUG1(log_loop_entry_exit(loop));

	unsigned pressure = 0;
	for (auto i = loop->child_begin(), e = loop->child_end(); i != e; ++i) {
		auto child = *i;
		unsigned child_pressure = compute_block_pressure(child);
		pressure = std::max(pressure, child_pressure);
	}

	for (auto i = loop->loop_begin(), e = loop->loop_end(); i != e; ++i) {
		auto inner_loop = *i;
		LOG1("\tHas inner loop with loop head = " << *inner_loop->get_header() << nl);
		unsigned child_pressure = compute_loop_pressure(inner_loop);
		pressure = std::max(pressure, child_pressure);
	}

	LOG1("Done with loop, pressure = " << pressure << nl);

	return pressure;
}

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
