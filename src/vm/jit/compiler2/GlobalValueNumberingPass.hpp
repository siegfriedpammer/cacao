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
	typedef unordered_map<Instruction*,PartitionTy> Inst2PartitionMapTy;

	typedef unordered_map<Instruction::InstID,PartitionTy*,hash<int> > OpcodePartitionMapTy;
	typedef unordered_map<int,PartitionTy*> IntPartitionMapTy;
	typedef unordered_map<long,PartitionTy*> LongPartitionMapTy;

	int max_arity;
	PartitionVectorTy partitions;
	Inst2PartitionMapTy inst2PartitionMap;
	
	InWorkListTy inWorkList;
	WorkListTy workList;
//	PartitionBoolMapTy isTouched;

	void init_partitions(Method::const_iterator begin,
		Method::const_iterator end);
	void init_worklist_and_touchedpartitions();

	std::vector<bool> *get_worklist_flags(PartitionTy *partition);
	void set_in_worklist(PartitionTy *partition, int index, bool flag);
	bool is_in_worklist(PartitionTy *partition, int index);
	WorkListPairTy *selectAndDeleteFromWorkList();
	
	template<typename InputIterator>
	static void add_partitions_from_map(PartitionVectorTy &partitions,
			InputIterator begin, InputIterator end) {
		for (InputIterator i = begin,
				e = end; i != e; i++) {
			PartitionTy *partition = (*i).second;
			partitions.push_back(partition);
		}
	}

	static int arity(PartitionTy *partition);
	static int compute_max_arity(Method::const_iterator begin,
								Method::const_iterator end);
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
