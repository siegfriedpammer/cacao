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
	typedef unordered_set<Instruction*> PartitionTy;
	typedef std::vector<PartitionTy*> PartitionVectorTy;
	typedef std::pair<PartitionTy*,int> WorkListPairTy;
	typedef std::list<WorkListPairTy*> WorkListTy;
	typedef unordered_map<PartitionTy*,std::vector<bool>* > InWorkListTy;
	typedef unordered_map<Instruction*,PartitionTy*> Inst2PartitionMapTy;

	/// these types are needed for the creation of the inital partitions
	typedef unordered_map<Instruction::InstID,PartitionTy*,hash<int> > OpcodePartitionMapTy;
	typedef unordered_map<int32_t,PartitionTy*> IntPartitionMapTy;
	typedef unordered_map<int64_t,PartitionTy*> LongPartitionMapTy;
	typedef unordered_map<float,PartitionTy*> FloatPartitionMapTy;
	typedef unordered_map<double,PartitionTy*> DoublePartitionMapTy;
	typedef unordered_map<BeginInst*,PartitionTy*> BBPartitionMapTy;

	typedef unordered_set<Instruction*> TouchedInstListTy;
	typedef unordered_map<PartitionTy*,TouchedInstListTy*> Partition2TouchedInstListMapTy;

	int max_arity;
	PartitionVectorTy partitions;
	Inst2PartitionMapTy inst2PartitionMap;
	Partition2TouchedInstListMapTy partition2TouchedInstListMap;
	
	InWorkListTy inWorkList;
	WorkListTy workList;

	PartitionTy *create_partition();
	void init_partitions(Method::const_iterator begin,
		Method::const_iterator end);
	void init_worklist_and_touchedpartitions();
	void add_to_partition(PartitionTy *partition, Instruction *inst);

	void add_to_worklist(PartitionTy *partition, int operandIndex);
	std::vector<bool> *get_worklist_flags(PartitionTy *partition);
	void set_in_worklist(PartitionTy *partition, int index, bool flag);
	bool is_in_worklist(PartitionTy *partition, int index);
	WorkListPairTy *selectAndDeleteFromWorkList();

	TouchedInstListTy *get_touched_instructions(PartitionTy *partition);
	PartitionTy *get_partition(Instruction *inst);

	void split(PartitionTy *partition, TouchedInstListTy *instructions);

	void print_partition(PartitionTy *partition);
	void print_partitions();
	void print_instructions(TouchedInstListTy *instructions);
	
	static int arity(PartitionTy *partition);
	static int compute_max_arity(Method::const_iterator begin,
								Method::const_iterator end);

	void consolidate_partitions();
	void consolidate_partition(PartitionTy *partition);
	
	template <typename T1, typename T2>
	PartitionTy *get_or_create_partition(T1 &map, T2 key) {
		PartitionTy *partition = map[key];
		if (!partition) {
			partition = create_partition();
			map[key] = partition;
		}
		return partition;
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
