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

#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/Method.hpp"
#include "vm/jit/compiler2/Instruction.hpp"
#include "future/unordered_set.hpp"
#include "future/unordered_map.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

/**
 * GlobalValueNumberingPass
 *
 * TODO: comment
 */
class GlobalValueNumberingPass : public Pass {
private:
	typedef unordered_set<Instruction*> BlockTy;
	typedef std::vector<BlockTy*> PartitionTy;
	typedef std::pair<BlockTy*,int> WorkListPairTy;
	typedef std::list<WorkListPairTy*> WorkListTy;
	typedef unordered_map<BlockTy*,std::vector<bool>* > InWorkListTy;
	typedef unordered_map<Instruction*,BlockTy*> Inst2BlockMapTy;

	/// these types are needed for the creation of the inital blocks
	typedef unordered_map<Instruction::InstID,BlockTy*,hash<int> > OpcodeBlockMapTy;
	typedef unordered_map<int32_t,BlockTy*> IntBlockMapTy;
	typedef unordered_map<int64_t,BlockTy*> LongBlockMapTy;
	typedef unordered_map<float,BlockTy*> FloatBlockMapTy;
	typedef unordered_map<double,BlockTy*> DoubleBlockMapTy;
	typedef unordered_map<BeginInst*,BlockTy*> BBBlockMapTy;

	typedef unordered_set<Instruction*> TouchedInstListTy;
	typedef unordered_map<BlockTy*,TouchedInstListTy*> Block2TouchedInstListMapTy;

	int max_arity;
	PartitionTy partition;
	Inst2BlockMapTy inst2BlockMap;
	Block2TouchedInstListMapTy block2TouchedInstListMap;
	
	InWorkListTy inWorkList;
	WorkListTy workList;

	BlockTy *create_block();
	void init_partition(Method::const_iterator begin,
		Method::const_iterator end);
	void init_worklist_and_touchedblocks();
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

	void consolidate_blocks();
	void consolidate_block(BlockTy *block);
	
	template <typename T1, typename T2>
	BlockTy *get_or_create_block(T1 &map, T2 key) {
		BlockTy *block = map[key];
		if (!block) {
			block = create_block();
			map[key] = block;
		}
		return block;
	}
public:
	static char ID;
	GlobalValueNumberingPass() : Pass() {}
	virtual bool run(JITData &JD);
	virtual PassUsage& get_PassUsage(PassUsage &PU) const;

	// virtual void initialize();   (optional)
	// virtual void finalize();     (optional)
	// virtual bool verify() const; (optional)
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
