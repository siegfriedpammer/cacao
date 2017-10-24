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

class LTNode {
public:
	LTNode(MachineOperand* operand) : op(operand) {}
	MachineOperand* operand() { return op; }

	void add_child(LTNode* node) { children.push_back(node); }

	void set_parent(LTNode* node) { parent = node; }
	LTNode* get_parent() { return parent; }

	void mark_visited() { visited = true; }
	bool is_visited() const { return visited; }

	auto begin() { return children.begin(); }
	auto end() { return children.end(); }

private:
	MachineOperand* op;
	LTNode* parent = nullptr;
	std::vector<LTNode*> children;
	bool visited = false;
};

class LocationTransferGraph {
public:
	void add_edge(MachineOperand* from, MachineOperand* to)
	{
		auto node_from = node_for(from);
		auto node_to = node_for(to);
		node_from->add_child(node_to);
		node_to->set_parent(node_from);
	}

	std::vector<LocationTransferGraph>& get_components()
	{
		components.clear();
		for (auto& pair : nodes) {
			auto node = pair.second.get();
			if (!node->is_visited()) {
				auto& component = *components.emplace(components.end());
				dfs(component, node);
			}
		}
		return components;
	}

	/**
	 * Only useful for components. Since they got created
	 * from a "spartan" location transfer graph a component is
	 * either a path or a cycle.
	 * @return true, if component is a cycle, false if it is a path
	 */
	bool is_cycle() const
	{
		LTNode* start = nullptr;
		for (auto& pair : nodes) {
			auto node = pair.second.get();
			if (node->get_parent() == nullptr) {
				start = node;
				break;
			}
		}
		return start == nullptr;
	}

private:
	std::map<MachineOperand*, std::unique_ptr<LTNode>> nodes;
	std::vector<LocationTransferGraph> components;

	LTNode* node_for(MachineOperand* operand)
	{
		auto iter = nodes.find(operand);
		if (iter == nodes.end()) {
			auto pair = nodes.emplace(std::make_pair(operand, std::make_unique<LTNode>(operand)));
			iter = pair.first;
		}
		return iter->second.get();
	}

	void dfs(LocationTransferGraph& component, LTNode* node)
	{
		node->mark_visited();
		for (auto child : *node) {
			component.add_edge(node->operand(), child->operand());
			if (!child->is_visited()) {
				dfs(component, child);
			}
		}
	}
};

void SSADeconstructionPass::insert_transfer_at_end(MachineBasicBlock* block,
                                                   MachineBasicBlock* predecessor,
                                                   unsigned phi_column)
{
	LOG3("Inserting transfers from [" << *predecessor << "] to [" << *block << "]\n");

	std::vector<MachineOperand*> src;
	std::vector<MachineOperand*> dst;

	for (auto i = block->phi_begin(), e = block->phi_end(); i != e; ++i) {
		auto phi = *i;

		auto dst_op = phi->get_result().op;
		auto src_op = phi->get(phi_column).op;

		if (dst_op->aquivalent(*src_op))
			continue;

		dst.push_back(dst_op);
		src.push_back(src_op);
	}

	LOG2("Inserting parallel copy: \n");
	LOG2_NAMED_PTR_CONTAINER("Source operands: ", src);
	LOG2_NAMED_PTR_CONTAINER("Dest operands:   ", dst);

	std::vector<unsigned> diff;
	find_safe_operations(src, dst, diff);
	implement_safe_operations(src, dst, diff, predecessor);

	implement_swaps(src, dst, predecessor);

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

void SSADeconstructionPass::find_safe_operations(const std::vector<MachineOperand*>& src,
                                                 const std::vector<MachineOperand*>& dst,
                                                 std::vector<unsigned>& diff)
{
	for (unsigned i = 0; i < dst.size(); ++i) {
		if (dst[i] == nullptr)
			continue;

		auto iter = std::find_if(src.begin(), src.end(), [&](const auto operand) {
			return dst[i]->aquivalent(*operand);
		});

		if (iter == src.end()) {
			diff.push_back(i);
		}
	}
}

void SSADeconstructionPass::implement_safe_operations(std::vector<MachineOperand*>& src,
                                                      std::vector<MachineOperand*>& dst,
                                                      std::vector<unsigned>& diff,
                                                      MachineBasicBlock* block)
{
	while (diff.size() > 0) {
		unsigned index = diff.back();
		diff.pop_back();

		auto src_op = src[index];
		auto dst_op = dst[index];
		assert(src_op->is_Register() && dst_op->is_Register() &&
		       "Load/stores not implementedf yet!");

		auto move = JD->get_Backend()->create_Move(src_op, dst_op);
		insert_before(block->mi_last(), move);

		LOG1("Inserting move at end of (" << *block << "): " << *move << nl);

		src[index] = nullptr;
		dst[index] = nullptr;
		find_safe_operations(src, dst, diff);
	}
}

void SSADeconstructionPass::implement_swaps(std::vector<MachineOperand*>& src,
                                            std::vector<MachineOperand*>& dst,
                                            MachineBasicBlock* block)
{
	bool has_changed = false;

	do {
		has_changed = swap_registers(src, dst, block);

		std::vector<unsigned> diff;
		find_safe_operations(src, dst, diff);
		if (!diff.empty()) {
			has_changed = true;
			implement_safe_operations(src, dst, diff, block);
		}
	} while (has_changed);
}

bool SSADeconstructionPass::swap_registers(std::vector<MachineOperand*>& src,
                                           std::vector<MachineOperand*>& dst,
                                           MachineBasicBlock* block)
{
	bool has_changed = false;

	for (unsigned i = 0; i < dst.size(); ++i) {
		auto src_op = src[i];
		auto dst_op = dst[i];

		if (!src_op)
			continue;

		if (!src_op->aquivalent(*dst_op)) {
			auto swap = JD->get_Backend()->create_Swap(src_op, dst_op);
			insert_before(block->mi_last(), swap);

			LOG1("Inserting swap at end of (" << *block << "): " << *swap << nl);

			auto find_lambda = [&](const auto operand) {
				if (!operand)
					return false;
				return dst_op->aquivalent(*operand);
			};

			// Replace all dst_ops in src with src_op
			for (auto iter = std::find_if(src.begin(), src.end(), find_lambda), e = src.end(); iter != e;
				iter = std::find_if(std::next(iter), src.end(), find_lambda)) {
				LOG1("Replacing occurrence of " << dst_op << " with " << src_op << nl);
				*iter = src_op;
			}

			has_changed = true;
		} else {
			auto move = JD->get_Backend()->create_Move(src_op, dst_op);
			insert_before(block->mi_last(), move);

			LOG1("Inserting move at end of (" << *block << "): " << *move << nl);
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
	/*
	for (auto& block : *MIS) {
	    if (block->phi_size() == 0)
	        continue;

	    std::vector<LocationTransferGraph> graphs;

	    LOG1(Cyan << *block << " has PHIs. Building graphs." << reset_color << nl);
	    for (unsigned i = 0, e = block->pred_size(); i < e; ++i) {

	        LOG2("\tBuilding location transfer graph for predecessor " << *block->get_predecessor(i)
	                                                                   << nl);
	        auto& graph = *graphs.emplace(graphs.end());
	        for (auto k = block->phi_begin(), ke = block->phi_end(); k != ke; ++k) {
	            auto phi_instruction = *k;
	            auto operand_to = phi_instruction->get_result().op;
	            auto operand_from = phi_instruction->get(i).op;
	            if (operand_to->aquivalent(*operand_from))
	                continue;

	            graph.add_edge(operand_from, operand_to);
	            LOG2("\t\tAdding edge " << *operand_from << " -> " << *operand_to << nl);
	        }

	        auto& components = graph.get_components();
	        LOG2("\tLocation transfer graph has " << components.size() << " components.\n");
	        for (const auto& component : components) {
	            auto is_cycle = component.is_cycle();
	            LOG2("\t\tComponent is a " << (is_cycle ? "cycle" : "path") << nl);
	        }
	    }
	}
	*/
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
