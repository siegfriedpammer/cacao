/* src/vm/jit/compiler2/ExamplePass.hpp - ExamplePass

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

#ifndef _JIT_COMPILER2_EXAMPLEPASS
#define _JIT_COMPILER2_EXAMPLEPASS

#include <limits>

#include <boost/dynamic_bitset.hpp>

#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MachineLoopPass.hpp"
#include "vm/jit/compiler2/Pass.hpp"

MM_MAKE_NAME(NewLivetimeAnalysisPass)

namespace cacao {
namespace jit {
namespace compiler2 {

using LiveTy = boost::dynamic_bitset<>;
using LiveMapTy = alloc::map<MachineBasicBlock*, LiveTy>::type;

struct NextUse2 {
	MachineOperand* operand = nullptr;
	unsigned distance = 0;
};

constexpr unsigned kInfinity = std::numeric_limits<unsigned>::max();

using NextUseSet = std::vector<NextUse2>;
using NextUseSetUPtrTy = std::unique_ptr<NextUseSet>;

class NewLivetimeAnalysisPass;

class NextUseInformation {
public:
	NextUseInformation(NewLivetimeAnalysisPass& P) : LTA(P) {}

	void initialize_empty_sets_for(MachineBasicBlock* block);
	void add_operands(NextUseSet& set, const LiveTy& live, unsigned distance);
	void remove_operands(NextUseSet& set, const LiveTy& live);
	void copy_out_to_in(MachineBasicBlock* block);
	void transfer(NextUseSet&, MachineInstruction*, unsigned) const;

	NextUseSet& get_next_use_out(MachineBasicBlock* block);
	NextUseSet& get_next_use_in(MachineBasicBlock* block);

private:
	NewLivetimeAnalysisPass& LTA;

	using NextUseMap = std::map<MachineBasicBlock*, NextUseSetUPtrTy>;
	NextUseMap next_use_outs;
	NextUseMap next_use_ins;
};

class MachinePhiInst;
/*
class Occurrence {
public:
    explicit Occurrence(MachineOperandDesc* desc, const MIIterator& iter)
        : op(desc->op), desc(desc), mi_iter(&iter), phi_instr(nullptr)
    {
    }

    explicit Occurrence(MachineOperandDesc* desc, MachinePhiInst* phi)
        : op(desc->op), desc(desc), mi_iter(nullptr), phi_instr(phi)
    {
    }

    MachineOperand* operand() { return op; }
    const MachineOperand* operand() const { return op; }
    MachineOperandDesc* descriptor() { return desc; }
    const MachineOperandDesc* descriptor() const { return desc; }
    const MIIterator iter() const {
        assert(mi_iter && "Do not call iter on PHI occurrences!");
        return *mi_iter;
    }

    const MachineInstruction* instruction() const { return phi_instr ? phi_instr : **mi_iter; }

private:
    MachineOperand* op;
    MachineOperandDesc* desc;
    const MIIterator* mi_iter;

    MachinePhiInst* phi_instr;
};*/

struct Occurrence {
	explicit Occurrence(MachineOperandDesc* desc, const MIIterator& iter)
	    : operand(desc->op), descriptor(desc), instruction(iter)
	{
	}
	MachineOperand* operand;
	MachineOperandDesc* descriptor;
	const MIIterator instruction;
};

class DefUseChains {
public:
	using OccurrenceUPtrTy = std::unique_ptr<Occurrence>;
	struct DefUseChain {
		const MachineOperand* operand;
		OccurrenceUPtrTy definition; // using ptr cause we cant initialize it at creation time
		std::list<Occurrence> uses;
	};

	void add_definition(MachineOperandDesc* desc, const MIIterator& iter);
	void add_use(MachineOperandDesc* desc, const MIIterator& iter);
	// void add_use(MachineOperandDesc* desc, MachinePhiInst* phi);

	const Occurrence& get_definition(const MachineOperand*) const;
	const std::list<Occurrence>& get_uses(const MachineOperand*) const;

	template <typename UnaryFunction>
	UnaryFunction for_each_use(const MachineOperand* operand, UnaryFunction f);

private:
	using DefUseChainUPtrTy = std::unique_ptr<DefUseChain>;
	std::map<std::size_t, DefUseChainUPtrTy> chains;

	DefUseChain& get(const MachineOperand*);
	const DefUseChain& get(const MachineOperand*) const;
};

class NewLivetimeAnalysisPass : public Pass, public memory::ManagerMixin<NewLivetimeAnalysisPass> {
public:
	NewLivetimeAnalysisPass() : Pass(), next_use(*this) {}
	virtual bool run(JITData& JD);
	virtual PassUsage& get_PassUsage(PassUsage& PU) const;

	bool is_live_in(MachineBasicBlock* block, MachineOperand* operand)
	{
		return get_live_in(block)[operand->get_id()];
	}

	template <typename UnaryFunction>
	UnaryFunction for_each(const LiveTy& live, UnaryFunction f)
	{
		for (std::size_t i = 0, e = live.size(); i < e; ++i) {
			if (live[i]) {
				f(operands[i]);
			}
		}
		return f;
	}

	template <typename UnaryFunction>
	UnaryFunction for_each_live_in(MachineBasicBlock* block, UnaryFunction f)
	{
		auto& live_in = get_live_in(block);
		return for_each(live_in, f);
	}

	template <typename UnaryFunction>
	UnaryFunction for_each_live_out(MachineBasicBlock* block, UnaryFunction f)
	{
		auto& live_out = get_live_out(block);
		return for_each(live_out, f);
	}

	/**
	 * Updates a liveness set over a single step.
	 * @param live The live out set of instruction
	 * @return The live in set of instruction
	 */
	LiveTy
	liveness_transfer(const LiveTy& live, const MIIterator& mi_iter, bool record_defuse = false);

	LiveTy& get_live_in(MachineBasicBlock* block) { return get_livety(liveInMap, block); }
	LiveTy& get_live_out(MachineBasicBlock* block) { return get_livety(liveOutMap, block); }

	LiveTy phi_defs(MachineBasicBlock* block);

	std::size_t get_num_operands() const { return num_operands; }

	/// Returns the next_use_in set relative to the given instruction
	NextUseSetUPtrTy next_use_set_from(MachineInstruction* instruction);

#if !defined(NDEBUG)
	void log_livety(OStream& OS, const LiveTy& liveSet);
#endif

	DefUseChains& get_def_use_chains() { return def_use_chains; }

private:
	std::size_t num_operands;
	LiveMapTy liveInMap;
	LiveMapTy liveOutMap;

	NextUseInformation next_use;
	DefUseChains def_use_chains;

	std::map<MachineBasicBlock*, unsigned> max_pressures;

	void initialize_blocks() const;
	void initialize_instructions() const;

	/// Performs a post-order traversal while ignoring loop-back edges
	void dag_dfs(MachineBasicBlock* block);

	/// Phase 1 processing of a basic block
	void process_block(MachineBasicBlock* block);

	/// Calculates the live-out set of a basic block by using the live-in
	/// sets of its successors (ignoring loop-back edges) and special handling
	/// of PHI operands
	void calculate_liveout(MachineBasicBlock* block);

	/// Calculates the live-in set of a basic block by using the live-out set
	/// calculated previously and reverse iterating all instructions doing a
	/// liveness transfer along the way
	void calculate_livein(MachineBasicBlock* block);

	void looptree_dfs(MachineLoop* loop, const LiveTy& parent_loop_live);
	LiveTy phi_uses(MachineBasicBlock* block);

	LiveTy& get_livety(LiveMapTy& liveMap, MachineBasicBlock* block)
	{
		auto it = liveMap.find(block);
		if (it == liveMap.end()) {
			LiveTy live(num_operands, 0ul);
			it = liveMap.insert(std::make_pair(block, live)).first;
		}
		return it->second;
	}

	/// After liveness has been calculated, we use the result to propagate
	/// next-use information to loop blocks.
	void next_use_fixed_point();

	std::vector<MachineOperand*> operands;
};

NextUseSet::iterator find(NextUseSet& set, MachineOperand* operand);

inline bool reg_alloc_considers_operand(const MachineOperand& op)
{
	return op.needs_allocation();
}

inline bool reg_alloc_considers_instruction(MachineInstruction* instruction)
{
	// return instruction->to_MachineReplacementPointInst() == nullptr;
	return true;
}

inline OStream& operator<<(OStream& OS, NextUse2& next_use)
{
	OS << next_use.operand << "(";
	if (next_use.distance != kInfinity)
		OS << next_use.distance;
	else
		OS << "Inf";
	return OS << ")";
}

template <typename UnaryFunction>
UnaryFunction DefUseChains::for_each_use(const MachineOperand* operand, UnaryFunction f)
{
	auto& chain = get(operand);
	std::for_each(chain.uses.cbegin(), chain.uses.cend(), f);
}

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_EXAMPLEPASS */

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
