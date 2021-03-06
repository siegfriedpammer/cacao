/* src/vm/jit/compiler2/GlobalValueNumberingPass.hpp - GlobalValueNumberingPass

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

#ifndef _JIT_COMPILER2_GLOBALVALUENUMBERINGPASS
#define _JIT_COMPILER2_GLOBALVALUENUMBERINGPASS

#include <unordered_map>

#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/MethodC2.hpp"
#include "vm/jit/compiler2/Instruction.hpp"
#include "toolbox/Option.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

/**
 * GlobalValueNumberingPass
 *
 * This pass finds and removes redundant computations based on the high-level
 * intermediate representation of the compiler2, i.e., it removes redundant
 * nodes. It therefore uses the global value numbering algorithm in
 * @cite ReisingerBScThesis.
 */
class GlobalValueNumberingPass : public Pass {
private:
	typedef alloc::unordered_set<Instruction*>::type BlockTy;
	typedef std::list<BlockTy*> PartitionTy;
	typedef std::pair<BlockTy*,int> WorkListPairTy;
	typedef std::list<WorkListPairTy*> WorkListTy;
	typedef std::unordered_map<BlockTy*,std::vector<bool>* > InWorkListTy;
	typedef std::unordered_map<Instruction*,BlockTy*> Inst2BlockMapTy;
	typedef std::list<Instruction*> InstructionListTy;
	typedef std::vector<InstructionListTy*> OperandIndex2UsersTy;
	typedef std::unordered_map<Instruction*,OperandIndex2UsersTy*> OperandInverseMapTy;

	/// these types are needed for the creation of the inital blocks
	typedef std::unordered_map<Instruction::InstID,BlockTy*,std::hash<int> > OpcodeBlockMapTy;
	typedef std::unordered_map<int32_t,BlockTy*> IntBlockMapTy;
	typedef std::unordered_map<int64_t,BlockTy*> LongBlockMapTy;
	typedef std::unordered_map<float,BlockTy*> FloatBlockMapTy;
	typedef std::unordered_map<double,BlockTy*> DoubleBlockMapTy;
	typedef std::unordered_map<BeginInst*,BlockTy*> BBBlockMapTy;

	typedef alloc::unordered_set<Instruction*>::type TouchedInstListTy;
	typedef std::unordered_map<BlockTy*,TouchedInstListTy*> Block2TouchedInstListMapTy;

	int max_arity;
	PartitionTy partition;
	Inst2BlockMapTy inst2BlockMap;
	Block2TouchedInstListMapTy block2TouchedInstListMap;
	InWorkListTy inWorkList;
	WorkListTy workList;
	OperandInverseMapTy operandInverseMap;

	BlockTy *create_block();
	void init_partition(Method::const_iterator begin,
		Method::const_iterator end);
	void init_worklist_and_touchedblocks();
	void init_operand_inverse(Method::const_iterator begin,
		Method::const_iterator end);

	InstructionListTy *get_users(Instruction *inst, int op_index);
	void add_to_block(BlockTy *block, Instruction *inst);

	void add_to_worklist(BlockTy *block, int operandIndex);
	std::vector<bool> *get_worklist_flags(BlockTy *block);
	void set_in_worklist(BlockTy *block, int index, bool flag);
	bool is_in_worklist(BlockTy *block, int index);
	WorkListPairTy *selectAndDeleteFromWorkList();

	TouchedInstListTy *get_touched_instructions(BlockTy *block);
	BlockTy *get_block(Instruction *inst);

	void split(BlockTy *block, TouchedInstListTy *instructions);

	void print_block(BlockTy *block);
	void print_blocks();
	void print_instructions(TouchedInstListTy *instructions);
	
	static int arity(BlockTy *block);
	static int compute_max_arity(Method::const_iterator begin,
								Method::const_iterator end);

	void eliminate_redundancies();
	void eliminate_redundancies_in_block(BlockTy *block);
	
	template <typename T1, typename T2>
	BlockTy *get_or_create_block(T1 &map, T2 key) {
		BlockTy *block = map[key];
		if (!block) {
			block = create_block();
			map[key] = block;
		}
		return block;
	}

	void clean_up_operand_inverse();
public:
	static Option<bool> enabled;
	GlobalValueNumberingPass() : Pass() {}
	bool run(JITData &JD) override;
	PassUsage& get_PassUsage(PassUsage &PU) const override;

	bool is_enabled() const override {
		return GlobalValueNumberingPass::enabled;
	}
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_GLOBALVALUENUMBERINGPASS */


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
