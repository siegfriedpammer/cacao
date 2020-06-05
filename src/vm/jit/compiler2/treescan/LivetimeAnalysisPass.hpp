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
#include "vm/jit/compiler2/MachineOperandSet.hpp"
#include "vm/jit/compiler2/Pass.hpp"

MM_MAKE_NAME(LivetimeAnalysisPass)

namespace cacao {
namespace jit {
namespace compiler2 {

constexpr unsigned kInfinity = std::numeric_limits<unsigned>::max();

using NextUseSet = ExtendedOperandSet<unsigned>;
using NextUseSetUPtrTy = std::unique_ptr<NextUseSet>;

class LivetimeAnalysisPass;

class NextUseInformation {
public:
	NextUseInformation(LivetimeAnalysisPass& LTA) : LTA(LTA) {}

	void initialize_empty_sets_for(MachineBasicBlock* block);
	void add_operands(NextUseSet& set, OperandSet& live, unsigned distance);
	void copy_out_to_in(MachineBasicBlock* block);
	void transfer(NextUseSet&, MachineInstruction*, unsigned) const;

	NextUseSet& get_next_use_out(MachineBasicBlock* block);
	NextUseSet& get_next_use_in(MachineBasicBlock* block);

private:
	LivetimeAnalysisPass& LTA;

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
	    : operand(desc->op), descriptor(desc), instruction(iter), phi_instr(nullptr)
	{
	}

	explicit Occurrence(MachineOperandDesc* desc, MachinePhiInst* phi_instr)
	    : operand(desc->op), descriptor(desc), instruction(phi_instr->get_block()->self_iterator()),
	      phi_instr(phi_instr)
	{
	}
	MachineOperand* operand;
	MachineOperandDesc* descriptor;
	const MIIterator instruction;

	MachinePhiInst* phi_instr;

	MachineBasicBlock* block() const {
		return phi_instr ? phi_instr->get_block() : (*instruction)->get_block();
	}
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
	
	void add_phi_definition(MachineOperandDesc* desc, MachinePhiInst* phi);
	void add_phi_use(MachineOperandDesc* desc, MachinePhiInst* phi);

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

class LivetimeAnalysisPass : public Pass, public Artifact, public memory::ManagerMixin<LivetimeAnalysisPass> {
public:
	LivetimeAnalysisPass() : Pass(), next_use(*this) {}
	bool run(JITData& JD) override;
	PassUsage& get_PassUsage(PassUsage& PU) const override;

	LivetimeAnalysisPass* provide_Artifact(ArtifactInfo::IDTy) override {
		return this;
	}

	/**
	 * Updates a liveness set over a single step.
	 * @param live The live out set of instruction
	 * @return The live in set of instruction
	 */
	OperandSet liveness_transfer(const OperandSet& live,
	                             const MIIterator& mi_iter,
	                             bool record_defuse = false);

	OperandSet& get_live_in(MachineBasicBlock* block) { return get_liveset(live_in_map, block); }
	OperandSet& get_live_out(MachineBasicBlock* block) { return get_liveset(live_out_map, block); }

	/// Returns all operands defined by PHI functions in the given MBB
	/// @todo Maybe move this out of this class
	OperandSet mbb_phi_defs(MachineBasicBlock* block, bool record_defuse = false);

	/// Returns the next_use_in set relative to the given instruction
	NextUseSetUPtrTy next_use_set_from(MachineInstruction* instruction);

	DefUseChains& get_def_use_chains() { return def_use_chains; }

private:
	MachineOperandFactory* MOF;

	using LiveMapTy = std::map<MachineBasicBlock*, OperandSet>;
	LiveMapTy live_in_map;
	LiveMapTy live_out_map;

	NextUseInformation next_use;
	DefUseChains def_use_chains;

	void initialize_blocks();
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

	void looptree_dfs(MachineLoop* loop, const OperandSet& parent_loop_live);

	/// Returns all operands used by the PHI functions of the given MBB
	/// @todo Maybe move this out of this class
	OperandSet mbb_phi_uses(MachineBasicBlock* block, bool record_defuse = false);

	/// Get or create the operand set associated with the provided MBB
	OperandSet& get_liveset(LiveMapTy&, MachineBasicBlock*);

	/// After liveness has been calculated, we use the result to propagate
	/// next-use information to loop blocks.
	void next_use_fixed_point();

	friend class NextUseInformation;
};

inline bool reg_alloc_considers_operand(const MachineOperand& op)
{
	return op.needs_allocation();
}

inline bool reg_alloc_considers_instruction(MachineInstruction* instruction)
{
	return instruction->to_MachineReplacementPointInst() == nullptr;
}

template <typename UnaryFunction>
UnaryFunction DefUseChains::for_each_use(const MachineOperand* operand, UnaryFunction f)
{
	auto& chain = get(operand);
	return std::for_each(chain.uses.cbegin(), chain.uses.cend(), f);
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
