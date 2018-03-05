/* src/vm/jit/compiler2/SSADeconstructionPass.cpp - SSADeconstructionPass

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

#include "vm/jit/compiler2/SSADeconstructionPass.hpp"

#include <set>

#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/MachineLoopPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/RegisterAllocatorPass.hpp"
#include "vm/jit/compiler2/ReversePostOrderPass.hpp"
#include "vm/jit/compiler2/lsra/LogHelper.hpp"

#define DEBUG_NAME "compiler2/SSADeconstructionPass"

namespace cacao {
namespace jit {
namespace compiler2 {

void ParallelCopyImpl::calculate()
{
	LOG2("Inserting parallel copy: \n");
	LOG2_NAMED_PTR_CONTAINER("Source operands: ", src);
	LOG2_NAMED_PTR_CONTAINER("Dest operands:   ", dst);

	std::set<unsigned> diff;
	find_safe_operations(diff);
	implement_safe_operations(diff);

	implement_swaps();

	bool empty = true;
	for (unsigned i = 0; i < dst.size(); ++i) {
		auto src_op = src[i];
		auto dst_op = dst[i];
		if (src_op || dst_op) {
			empty = false;
			break;
		}
	}

	assert(empty && "There are copies yet to be implemented!");
}

void SSADeconstructionPass::insert_transfer_at_end(MachineBasicBlock* block,
                                                   MachineBasicBlock* predecessor,
                                                   unsigned phi_column)
{
	LOG3("Inserting transfers from [" << *predecessor << "] to [" << *block << "]\n");

	ParallelCopyImpl parallel_copy(*JD);

	for (auto i = block->phi_begin(), e = block->phi_end(); i != e; ++i) {
		auto phi = *i;

		auto dst_op = phi->get_result().op;
		auto src_op = phi->get(phi_column).op;

		if (dst_op->aquivalent(*src_op))
			continue;

		parallel_copy.add(src_op, dst_op);
	}

	parallel_copy.calculate();
	auto& operations = parallel_copy.get_operations();
	predecessor->insert_before(--predecessor->end(), operations.begin(), operations.end());

	if (DEBUG_COND_N(1)) {
		for (auto operation : operations) {
			LOG1("Inserting operation at end of (" << *predecessor << "): " << *operation << nl);
		}
	}
}

void ParallelCopyImpl::find_safe_operations(std::set<unsigned>& diff)
{
	for (unsigned i = 0; i < dst.size(); ++i) {
		if (dst[i] == nullptr)
			continue;

		auto iter = std::find_if(src.begin(), src.end(), [&](const auto operand) {
			return operand == nullptr ? false : dst[i]->aquivalent(*operand);
		});

		if (iter == src.end()) {
			LOG1("Found safe operation: " << src[i] << " -> " << dst[i] << nl);
			diff.insert(i);
		}
	}
}

void ParallelCopyImpl::implement_safe_operations(std::set<unsigned>& diff)
{
	while (diff.size() > 0) {
		unsigned index = *diff.begin();
		diff.erase(index);

		auto src_op = src[index];
		auto dst_op = dst[index];	
		assert(src_op->is_Register() && dst_op->is_Register() &&
		       "Load/stores not implementedf yet!");

		LOG1("Implement safe operation: " << src_op << " -> " << dst_op << nl);

		auto move = JD.get_Backend()->create_Move(src_op, dst_op);
		operations.push_back(move);

		src[index] = nullptr;
		dst[index] = nullptr;
		find_safe_operations(diff);
	}
}

void ParallelCopyImpl::implement_swaps()
{
	bool has_changed = false;

	do {
		has_changed = swap_registers();

		std::set<unsigned> diff;
		find_safe_operations(diff);
		if (!diff.empty()) {
			has_changed = true;
			implement_safe_operations(diff);
		}
	} while (has_changed);
}

bool ParallelCopyImpl::swap_registers()
{
	bool has_changed = false;

	for (unsigned i = 0; i < dst.size(); ++i) {
		auto src_op = src[i];
		auto dst_op = dst[i];

		if (!src_op)
			continue;

		if (!src_op->aquivalent(*dst_op)) {
			auto swap = JD.get_Backend()->create_Swap(src_op, dst_op);
			operations.push_back(swap);

			auto find_lambda = [&](const auto operand) {
				if (!operand)
					return false;
				return dst_op->aquivalent(*operand);
			};

			// Replace all dst_ops in src with src_op
			for (auto iter = std::find_if(src.begin(), src.end(), find_lambda), e = src.end();
			     iter != e; iter = std::find_if(std::next(iter), src.end(), find_lambda)) {
				LOG1("Replacing occurrence of " << dst_op << " with " << src_op << nl);
				*iter = src_op;
			}

			has_changed = true;
		}
		else {
			// auto move = JD.get_Backend()->create_Move(src_op, dst_op);
			// operations.push_back(move);
		}

		src[i] = nullptr;
		dst[i] = nullptr;
	}

	return has_changed;
}

void SSADeconstructionPass::process_block(MachineBasicBlock* block)
{
	if (block->pred_size() >= 2 && block->phi_size() > 0) {
		LOG3("Block [" << *block << "] has 2 or more predecessors (and phi instructions).\n");
		for (unsigned i = 0, e = block->pred_size(); i < e; ++i) {
			insert_transfer_at_end(block, block->get_predecessor(i), i);
		}
	}
}

bool SSADeconstructionPass::run(JITData& JD)
{
	LOG1(BoldYellow << "\nRunning SSADeconstructionPass" << reset_color << nl);
	this->JD = &JD;

	auto RPO = get_Pass<ReversePostOrderPass>();
	for (auto& block : *RPO) {
		process_block(block);
	}

	return true;
}

// pass usage
PassUsage& SSADeconstructionPass::get_PassUsage(PassUsage& PU) const
{
	PU.add_requires<MachineInstructionSchedulingPass>();
	PU.add_requires<RegisterAllocatorPass>();
	PU.add_requires<ReversePostOrderPass>();
	PU.add_requires<MachineLoopPass>();
	return PU;
}

// register pass
static PassRegistry<SSADeconstructionPass> X("SSADeconstructionPass");

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
