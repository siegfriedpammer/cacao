/* src/vm/jit/compiler2/lsra/NextUsePass.cpp - NextUsePass

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

#include "vm/jit/compiler2/lsra/NextUsePass.hpp"

#include <limits>

#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MachineLoopPass.hpp"
#include "vm/jit/compiler2/ReversePostOrderPass.hpp"
// #include "vm/jit/compiler2/lsra/NewLivetimeAnalysisPass.hpp"

#define DEBUG_NAME "compiler2/NextUsePass"

namespace cacao {
namespace jit {
namespace compiler2 {

namespace {

MachineInstruction::successor_iterator get_successor_begin(MachineBasicBlock* MBB)
{
	return MBB->back()->successor_begin();
}

MachineInstruction::successor_iterator get_successor_end(MachineBasicBlock* MBB)
{
	return MBB->back()->successor_end();
}

} // end anonymous namespace

void NextUsePass::initialize_blocks() const
{
	LOG1("Marking loop exits with higher distance\n");

	auto RPO = get_Pass<ReversePostOrderPass>();
	auto MLP = get_Pass<MachineLoopPass>();
	for (const auto block : *RPO) {
		auto loop = MLP->get_Loop(block);
		if (!loop)
			continue;
		auto loopdepth = MLP->loop_nest(loop) + 1;

		for (auto i = get_successor_begin(block), e = get_successor_end(block); i != e; ++i) {
			auto successor = *i;
			auto successor_loop = MLP->get_Loop(successor);
			auto successor_loopdepth = MLP->loop_nest(successor_loop) + 1;

			if (successor_loopdepth < loopdepth) {
				LOG1(*successor << " is a loop exit block\n");
				successor->mark_loop_exit();
			}
		}
	}
}

void NextUsePass::initialize_instructions() const
{
	LOG1("Setting 'step' for each instruction in a basic block\n");
	auto RPO = get_Pass<ReversePostOrderPass>();

	for (const auto block : *RPO) {
		unsigned step = 0;
		for (auto instruction : *block) {
			instruction->set_step(step++);
		}
	}
}

const unsigned kInfinity = std::numeric_limits<unsigned>::max();

static void clone(const NextUseSet& src, NextUseSet& dst)
{
	dst.clear();
	std::copy(src.begin(), src.end(), std::back_inserter(dst));
}

static NextUse* get(NextUseSet& set, MachineOperand* operand)
{
	auto iter = std::find_if(set.begin(), set.end(), [operand](const auto& next_use) {
		return operand->aquivalent(*next_use.operand);
	});

	return iter == set.end() ? nullptr : &(*iter);
}

static NextUse&
get_or_create(NextUseSet& set, MachineOperand* operand, unsigned default_distance = 0)
{
	auto iter = std::find_if(set.begin(), set.end(), [operand](const auto& next_use) {
		return operand->aquivalent(*next_use.operand);
	});

	if (iter == set.end()) {
		iter = set.insert(set.end(), {operand, default_distance});
	}
	return *iter;
}

static void set(NextUseSet& set, MachineOperand* operand, unsigned distance)
{
	auto& next_use = get_or_create(set, operand);
	next_use.distance = distance;
}

static bool contains(const std::vector<MachineOperand*>& list, MachineOperand* operand)
{
	auto iter = std::find_if(list.begin(), list.end(), [operand](const auto& element) {
		return operand->aquivalent(*element);
	});
	return iter != list.end();
}

void NextUsePass::transfer(NextUseSet& next_use_set,
                           MachineInstruction* instruction,
                           unsigned block_distance) const
{
	std::vector<MachineOperand*> updated_operands;

	auto defs_lambda = [&](const auto& descriptor) {
		if (!descriptor.op->is_virtual())
			return;

		set(next_use_set, descriptor.op, kInfinity);
		updated_operands.push_back(descriptor.op);
	};
	std::for_each(instruction->results_begin(), instruction->results_end(), defs_lambda);

	auto uses_lambda = [&](const auto& descriptor) {
		if (!descriptor.op->is_virtual())
			return;

		set(next_use_set, descriptor.op, block_distance);
		updated_operands.push_back(descriptor.op);
	};
	std::for_each(instruction->begin(), instruction->end(), uses_lambda);
	std::for_each(instruction->dummy_begin(), instruction->dummy_end(), uses_lambda);

	for (auto& next_use : next_use_set) {
		if (!contains(updated_operands, next_use.operand)) {
			next_use.distance++;
		}
	}
}

unsigned NextUsePass::find_min_distance_successors(MachineBasicBlock* block,
                                                   MachineOperand* operand)
{
	unsigned distance = kInfinity;
	for (auto i = get_successor_begin(block), e = get_successor_end(block); i != e; ++i) {
		auto successor = *i;
		auto& succ_next_use_in = next_use_ins.at(successor);
		auto next_use = get(*succ_next_use_in, operand);
		if (next_use)
			distance = std::min(distance, next_use->distance);
	}
	return distance == kInfinity ? 0 : distance;
}

OStream& operator<<(OStream& OS, NextUse& next_use)
{
	OS << next_use.operand << "(";
	if (next_use.distance != kInfinity)
		OS << next_use.distance;
	else
		OS << "Inf";
	return OS << ")";
}

bool NextUsePass::run(JITData& JD)
{
	LOG1(nl << BoldYellow << "Running NextUsePass" << reset_color << nl);
	initialize_blocks();
	initialize_instructions();

	//auto LTA = get_Pass<NewLivetimeAnalysisPass>();
	auto RPO = get_Pass<ReversePostOrderPass>();
	for (auto i = RPO->rbegin(), e = RPO->rend(); i != e; ++i) {
		/*auto block = *i;
		next_use_ins.emplace(block, std::make_unique<NextUseSet>());
		auto& next_use_in = next_use_ins.at(block);

		auto& live_out = LTA->get_live_out(block);
		auto& live_in = LTA->get_live_in(block);

		// Operands that are both live in and live out, get as distance
		// the whole block length
		auto live_in_out = live_in & live_out;

		auto distance = block->size() + block->get_distance();
		LTA->for_each(live_in_out, [&](auto operand) {
			auto& next_use = get_or_create(*next_use_in, operand);
			next_use.distance += distance;
		});

		// Operands that are only live_in get their distance set to the first
		// instruction in the block that uses that operand
		auto only_live_in = live_in - live_out;

		for (auto j = block->rbegin(), je = block->rend(); j != je; ++j) {
			auto instruction = *j;
			auto distance = block->get_distance() + instruction->get_step();

			auto uses_lambda = [&](const auto& descriptor) {
				if (!descriptor.op->is_virtual() || !only_live_in[descriptor.op->get_id()])
					return;

				auto& next_use = get_or_create(*next_use_in, descriptor.op);
				next_use.distance = distance;
			};

			std::for_each(instruction->begin(), instruction->end(), uses_lambda);
			std::for_each(instruction->dummy_begin(), instruction->dummy_end(), uses_lambda);
		}*/
	}
/*
	for (auto i = RPO->rbegin(), e = RPO->rend(); i != e; ++i) {
		auto block = *i;
		auto& next_use_in = next_use_ins.at(block);

		auto& live_out = LTA->get_live_out(block);
		auto& live_in = LTA->get_live_in(block);

		// Operands that are both live in and live out, get as distance
		// the whole block length
		auto live_in_out = live_in & live_out;

		LTA->for_each(live_in_out, [&](auto operand) {
			auto next_use = get(*next_use_in, operand);
			//auto distance = this->find_min_distance_successors(block, operand);
			//next_use->distance += distance;
			LOG1("[" << *block << "]: " << operand << " is both live_in and live_out\n");
		});
	}

	// For next_use_out take the minimum distance from all successors
	// For PHI operands, this will result in a distance of 0 since phi
	// operands are not live_in in the block where the PHI instruction is
	for (const auto block : *RPO) {
		next_use_outs.emplace(block, std::make_unique<NextUseSet>());
		auto& next_use_out = next_use_outs.at(block);
		LTA->for_each_live_out(block, [&](const auto operand) {
			unsigned distance = this->find_min_distance_successors(block, operand);
			set(*next_use_out, operand, distance);
		});
	}

	if (DEBUG_COND_N(1)) {
		for (const auto block : *RPO) {
			auto& next_use_out = next_use_outs.at(block);
			auto& next_use_in = next_use_ins.at(block);

			LOG1(nl << Yellow << *block << reset_color << nl);
			LOG1("Next use in:  ");
			DEBUG1(print_container(dbg(), next_use_in->begin(), next_use_in->end()));
			LOG1(nl);
			LOG1("Next use out: ");
			DEBUG1(print_container(dbg(), next_use_out->begin(), next_use_out->end()));
			LOG1(nl);
		}
	}
*/
	return true;
}

// pass usage
PassUsage& NextUsePass::get_PassUsage(PassUsage& PU) const
{
	PU.add_requires<MachineLoopPass>();
	//PU.add_requires<NewLivetimeAnalysisPass>();
	PU.add_requires<ReversePostOrderPass>();
	return PU;
}

// register pass
static PassRegistry<NextUsePass> X("NextUsePass");

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
