/* src/vm/jit/compiler2/lsra/NextUse.cpp - NextUse

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

#include "vm/jit/compiler2/lsra/NextUse.hpp"

#include <limits>

#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MachineInstructions.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/lsra/SpillPass.hpp"

#define DEBUG_NAME "compiler2/SpillPass/NextUse"

namespace cacao {
namespace jit {
namespace compiler2 {

void NextUse::initialize(SpillPass* sp)
{
	auto MIS = sp->get_Pass<MachineInstructionSchedulingPass>();
	for (const auto& BB : *MIS) {
		std::size_t step = 0;
		for (auto& instruction : *BB) {
			instruction->set_step(step);
		}

		for (auto i = BB->phi_begin(), e = BB->phi_end(); i != e; ++i) {
			auto phi_instruction = *i;
			phi_instruction->set_step(0);
		}
	}
}

/**
 * Check if a value of the given definition is used in the given block
 * as a phi argument.
 */
static bool is_phi_argument(MachineBasicBlock* block, MachineOperand* operand) {
	for (const auto& phi_instruction : *block) {
		for (const auto& phi_op : *phi_instruction) {
			if (phi_op.op->aquivalent(*operand)) {
				return true;
			}
		}
	}
	return false;
}

NextUseTime NextUse::get_next_use(MachineBasicBlock* from, MachineInstruction* def)
{
	MachineInstruction* next_use_instr = nullptr;
	auto next_use_step = std::numeric_limits<std::size_t>::max();
	auto timestep = from->front()->get_step();

	auto defined_value = def->get_result().op;
	for (const auto& instruction : *from) {
		for (const auto& op : *instruction) {
			if (defined_value->aquivalent(*op.op)) {
				next_use_instr = instruction;
				next_use_step = instruction->get_step();
				break;
			}
		}

		if (next_use_instr) break;
	}

	if (next_use_instr) {
		NextUseTime result;
		result.time = next_use_step - timestep;
		result.before = next_use_instr;
		return result;
	}

	auto last_instruction = from->back();
	unsigned step = last_instruction->get_step() + 1 + timestep;

	if (is_phi_argument(from, defined_value)) {
		NextUseTime result;
		result.time = step;
		result.before = from->front();
		return result;
	}
	assert(0 && "get_next_use not implemented further!");
}

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao
