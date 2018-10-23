/* src/vm/jit/compiler2/PhiCoalescingPass.cpp - PhiCoalescingPass

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

#include "vm/jit/compiler2/PhiCoalescingPass.hpp"

#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/MachineDominatorPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/PhiLiftingPass.hpp"
#include "vm/jit/compiler2/ReversePostOrderPass.hpp"
#include "vm/jit/compiler2/treescan/LivetimeAnalysisPass.hpp"
#include "vm/jit/compiler2/treescan/LogHelper.hpp"
#include "vm/jit/compiler2/treescan/SpillPass.hpp"

#include <limits>
#include <stack>

#define DEBUG_NAME "compiler2/PhiCoalescingPass"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace {

class OperandUnionFind {
private:
	auto FindInternal(MachineOperand* op)
	{
		assert_msg(op, "FindInternal called with nullptr!");

		auto iter = std::find_if(sets.begin(), sets.end(),
		                         [&](const auto& p) { return p.second.contains(op); });

		if (iter == sets.end()) {
			sets.emplace_back(op, mof->EmptySet());
			iter = std::prev(sets.end());
			iter->second.add(op);
		}

		return iter;
	}

public:
	OperandUnionFind(MachineOperandFactory* mof) : mof(mof) {}

	MachineOperand* Find(MachineOperand* op) { return FindInternal(op)->first; }

	MachineOperand* Union(MachineOperand* op1, MachineOperand* op2)
	{
		assert_msg(op1 && op2, "Do not pass nullptrs!");

		// Because of iterator invalidation, we have to make sure the
		// actual iterators are valid
		FindInternal(op1);
		FindInternal(op2);
		auto iter1 = FindInternal(op1);
		auto iter2 = FindInternal(op2);

		// They are in the same set already
		if (iter1->first->get_id() == iter2->first->get_id())
			return op1;

		// Merge the smaller with the bigger one
		if (iter1->second.size() >= iter2->second.size()) {
			iter1->second |= iter2->second;
			sets.erase(iter2);
			return iter1->first;
		}
		else {
			iter2->second |= iter1->second;
			sets.erase(iter1);
			return iter2->first;
		}
	}

	void Print(OStream& OS) const
	{
		OS << "OperandUnionFind [\n";
		for (const auto& p : sets) {
			OS << "\t" << p.first << ": ";
			print_container(OS, p.second.begin(), p.second.end());
			OS << "\n";
		}
		OS << "]\n";
	}

private:
	MachineOperandFactory* mof;

	std::vector<std::pair<MachineOperand*, OperandSet>> sets;

	friend class ::cacao::jit::compiler2::PhiCoalescingPass;
};

OStream& operator<<(OStream& OS, const OperandUnionFind& uf)
{
	uf.Print(OS);
	return OS;
}

static OperandUnionFind
Build(ReversePostOrderPass* rpo, PhiLiftingPass* pl, MachineOperandFactory* mof)
{
	OperandUnionFind uf(mof);

	LOG2(Cyan << "Collecting PHI instructions from all basic blocks: " << reset_color << nl);

	for (auto& block : *rpo) {
		for (auto i = block->phi_begin(), e = block->phi_end(); i != e; ++i) {
			auto instr = *i;
			auto op = uf.Find(instr->get_result().op);

			for (auto& argument : *instr) {
				uf.Union(op, argument.op);
			}

			// Also union together all the arguments and results of the PHI-lifting we did earlier
			// even though half of these are already in the UnionFind, we simply do not care
			for (auto move : pl->inserted_moves[instr->get_id()]) {
				uf.Union(op, move->get(0).op);
				uf.Union(op, move->get_result().op);
			}
		}
	}

	LOG2("Result: " << uf << nl);

	return uf;
}

using DominanceForest = std::unordered_map<MachineOperand*, MachineOperand*>;

static std::vector<MachineOperand*> get_roots(DominanceForest& df)
{
	std::vector<MachineOperand*> roots;
	for (const auto& p : df) {
		if (!p.second) {
			roots.push_back(p.first);
		}
	}
	return roots;
}

static std::vector<MachineOperand*> get_children(DominanceForest& df, MachineOperand* node)
{
	std::vector<MachineOperand*> children;
	for (const auto& p : df) {
		if (p.second && p.second->get_id() == node->get_id()) {
			children.push_back(p.first);
		}
	}
	return children;
}

} // end anonymous namespace

void PhiCoalescingPass::handle_phi(MachinePhiInst* instruction)
{
	LOG2(Cyan << "Handling PHI instruction: " << *instruction << reset_color << nl);
}

void PhiCoalescingPass::dom_dfs(MachineBasicBlock* block, unsigned& nr)
{
	preorder_nrs[block->get_id()] = nr;

	auto DOM = get_Artifact<MachineDominatorPass>();
	for (auto& child : DOM->get_children(block)) {
		nr++;
		dom_dfs(child, nr);
	}

	max_preorder_nrs[block->get_id()] = nr;
}

unsigned PhiCoalescingPass::preorder(MachineOperand* op)
{
	auto LTA = get_Artifact<LivetimeAnalysisPass>();
	auto& def_use = LTA->get_def_use_chains();
	auto def_bb = def_use.get_definition(op).block();

	return preorder_nrs[def_bb->get_id()];
}

unsigned PhiCoalescingPass::max_preorder(MachineOperand* op)
{
	if (op == nullptr) {
		return std::numeric_limits<unsigned>::max();
	}

	auto LTA = get_Artifact<LivetimeAnalysisPass>();
	auto& def_use = LTA->get_def_use_chains();
	auto def_bb = def_use.get_definition(op).block();

	return max_preorder_nrs[def_bb->get_id()];
}

bool PhiCoalescingPass::dominates(MachineBasicBlock* b1, MachineBasicBlock* b2)
{
	unsigned nr1 = preorder_nrs[b1->get_id()];
	unsigned nr2 = preorder_nrs[b2->get_id()];
	unsigned max_nr1 = max_preorder_nrs[b1->get_id()];
	unsigned max_nr2 = max_preorder_nrs[b2->get_id()];

	return max_nr1 >= max_nr2 && nr1 <= nr2;
}

bool PhiCoalescingPass::run(JITData& JD)
{
	LOG1(BoldYellow << "\nRunning PhiCoalescingPass" << reset_color << nl);
	backend = JD.get_Backend();

	auto PL = get_Artifact<PhiLiftingPass>();
	auto RPO = get_Artifact<ReversePostOrderPass>();
	auto LTA = get_Artifact<LivetimeAnalysisPass>();
	auto MOF = JD.get_MachineOperandFactory();
	auto& def_use = LTA->get_def_use_chains();

	// 1. Step: Build the sets of variables that could potentially share a name
	auto uf = Build(RPO, PL, MOF);

	// 2. Step: Build the dominance forest

	// 2.1. Step: Number dominance tree once in DFS like manner
	unsigned counter = 0;
	dom_dfs(*RPO->begin(), counter);

	// 2.2 Step: Build a Dominance forest for all the Sets??
	for (auto& p : uf.sets) {
		auto operand_set = p.second.ToList();
		std::sort(operand_set->begin(), operand_set->end(),
		          [&](auto op1, auto op2) { return preorder(op1) < preorder(op2); });

		LOG3_NAMED_PTR_CONTAINER("Sorted by dominance: ", *operand_set);

		std::stack<MachineOperand*> stack;
		MachineOperand* current_parent = nullptr;
		stack.push(current_parent);

		std::unordered_map<MachineOperand*, MachineOperand*> edges;

		for (auto operand : *operand_set) {
			auto preorder_nr = preorder(operand);
			while (preorder_nr > max_preorder(current_parent)) {
				stack.pop();
				current_parent = stack.top();
			}

			edges[operand] = current_parent;
			stack.push(operand);
			current_parent = operand;
		}

		LOG3("Dominance Forest (non-existing operands do not have a parent): \n");
		for (const auto& p : edges) {
			LOG3("parent( " << p.first << " ) = " << p.second << nl);
		}

		// 3. Step Iterate over the dominance forest in dfs order and find interferences
		OperandSet final_set(p.second);

		std::vector<std::pair<MachineOperand*, MachineOperand*>> local_checks_same;
		std::vector<std::pair<MachineOperand*, MachineOperand*>> local_checks_live_in;

		std::stack<MachineOperand*> dfs_stack;
		auto roots = get_roots(edges);
		std::for_each(roots.begin(), roots.end(), [&](auto root) { dfs_stack.push(root); });

		while (!dfs_stack.empty()) {
			auto operand = dfs_stack.top();
			dfs_stack.pop();

			auto children = get_children(edges, operand);
			std::for_each(children.begin(), children.end(),
			              [&](auto child) { dfs_stack.push(child); });

			auto& def_operand = def_use.get_definition(operand);
			auto def_bb_operand = def_operand.block();
			for (auto child : children) {
				auto& def_child = def_use.get_definition(child);
				auto def_bb_child = def_child.block();
				if (LTA->get_live_out(def_bb_child).contains(operand)) {
					// Check whether operand can interfere with any other of its children
					bool interferes_with_other_child = false;
					for (auto c : children) {
						if (c->get_id() == child->get_id())
							continue;
						auto def_bb_c = def_use.get_definition(c).block();

						if (LTA->get_live_out(def_bb_c).contains(operand)) {
							interferes_with_other_child = true;
							break;
						}
						else if (LTA->get_live_in(def_bb_c).contains(operand) &&
						         !def_child.phi_instr) {
							interferes_with_other_child = true;
							break;
						}
						else if (def_bb_operand->get_id() == def_bb_c->get_id() &&
						         (!def_operand.phi_instr || !def_child.phi_instr)) {
							interferes_with_other_child = true;
							break;
						}
					}

					if (!interferes_with_other_child) {
						// Remove child from the equivalence class and add all the children of
						// 'child' to 'operand'
						LOG3(Red << "\tRemoving " << child
						         << " (child) from the set, it interferes with others!\n"
						         << reset_color);
						final_set.remove(child);
						auto child_children = get_children(edges, child);
						for (auto c : child_children) {
							edges[c] = operand;
							dfs_stack.push(c);
						}
					}
					else {
						// Remove operand from the equivalence class
						LOG3(Red << "\tRemoving " << operand
						         << " (parent) from the set, it interferes with others!\n"
						         << reset_color);
						final_set.remove(operand);
					}
				}
				else if (LTA->get_live_in(def_bb_child).contains(operand) && !def_child.phi_instr) {
					LOG3(Yellow << "\tVariable pair (" << operand << ", " << child
					            << ") needs local interference check (live in)!\n"
					            << reset_color);
					local_checks_live_in.emplace_back(operand, child);
				}
				else if (def_bb_operand->get_id() == def_bb_child->get_id() &&
				         (!def_operand.phi_instr || !def_child.phi_instr)) {
					LOG3(Yellow << "\tVariable pair (" << operand << ", " << child
					            << ") needs local interference check (same def block)!\n"
					            << reset_color);
					local_checks_same.emplace_back(operand, child);
				}
			}
		}

		// 3.1. Check for local interference, there are 2 cases for two variables:
		//      - Definition for both is in the same block
		//      - One is in the live-in set of the defining block of the other
		for (const auto& p : local_checks_same) {
			auto& def_parent = def_use.get_definition(p.first);
			auto& def_child = def_use.get_definition(p.second);

			if ((def_parent.phi_instr && !def_child.phi_instr) ||
			    def_parent.instruction >= def_child.instruction) {
				// Check whether the parent is live-out at the "child-defining" instruction, if so,
				// they interfere
				OperandSet live_out = LTA->get_live_out(def_child.block());
				MIIterator iter = def_child.block()->mi_last();
				while (iter != def_child.instruction) {
					live_out = LTA->liveness_transfer(live_out, iter);
					--iter;
				}

				if (live_out.contains(p.first)) {
					assert_msg(false, "Local interference detected between " << p.first << " and "
					                                                         << p.second);
				}
			}
			else if ((def_child.phi_instr && !def_parent.phi_instr) ||
			         def_parent.instruction < def_child.instruction) {
				// Check whether the child is live-out at the "parent-defining" instruction, if so,
				// they interfere
				OperandSet live_out = LTA->get_live_out(def_parent.block());
				MIIterator iter = def_parent.block()->mi_last();
				while (iter != def_parent.instruction) {
					live_out = LTA->liveness_transfer(live_out, iter);
					--iter;
				}

				if (live_out.contains(p.second)) {
					// TODO: When compiling java/util/concurrent/ConcurrentLinkedQueue.offer(Ljava/lang/Object;)Z
					//       this assertion fails. Investigate why.
					// assert_msg(false, "Local interference detected between " << p.first << " and "
					//                                                         << p.second);
					throw std::runtime_error("Local interference detected between two variables!");
				}
			}
		}

		for (const auto& p : local_checks_live_in) {
			auto& def_child = def_use.get_definition(p.second);

			OperandSet live_out = LTA->get_live_out(def_child.block());
			MIIterator iter = def_child.block()->mi_last();
			assert_msg(!def_child.phi_instr, "Child defined by a PHI instruction!");

			while (iter != def_child.instruction) {
				live_out = LTA->liveness_transfer(live_out, iter);
				--iter;
			}

			if (live_out.contains(p.first)) {
				LOG3(Red << "\tRemoving " << p.first
				         << " from the set because it interferes locally!\n"
				         << reset_color);
				final_set.remove(p.first);
			}
		}

		LOG(BoldBlue);
		LOG_NAMED_CONTAINER("Final equivalnce class operand set: ", final_set);
		LOG(reset_color);

		equivalence_classes.push_back(*final_set.ToList());
	}

	return true;
}

const std::vector<MachineOperand*>* PhiCoalescingPass::get_equivalence_class_for(MachineOperand* op) const
{
	for (const auto& set : equivalence_classes) {
		for (const auto operand : set) {
			if (operand->get_id() == op->get_id()) return &set;
		}
	}
	return nullptr;
}

// pass usage
PassUsage& PhiCoalescingPass::get_PassUsage(PassUsage& PU) const
{
	PU.provides<PhiCoalescingPass>();

	PU.requires<ReversePostOrderPass>();
	PU.requires<MachineDominatorPass>();
	PU.requires<LivetimeAnalysisPass>();
	PU.requires<PhiLiftingPass>();

	PU.before<SpillPass>();
	return PU;
}

Option<bool> PhiCoalescingPass::enabled("RACoalescePhis","compiler2: tries to assign the same register to PHI equivalence classes (default = true)",true,::cacao::option::xx_root());

// register pass
static PassRegistry<PhiCoalescingPass> X("PhiCoalescingPass");
static ArtifactRegistry<PhiCoalescingPass> Y("PhiCoalescingPass");

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
