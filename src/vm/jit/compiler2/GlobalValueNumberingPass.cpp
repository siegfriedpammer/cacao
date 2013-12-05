/* src/vm/jit/compiler2/GlobalValueNumberingPass.cpp - GlobalValueNumberingPass

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

#include "vm/jit/compiler2/GlobalValueNumberingPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/Method.hpp"
#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/InstructionMetaPass.hpp"
#include "vm/jit/compiler2/DeadcodeEliminationPass.hpp"
#include "toolbox/logging.hpp"

// define name for debugging (see logging.hpp)
#define DEBUG_NAME "compiler2/GlobalValueNumberingPass"

namespace cacao {
namespace jit {
namespace compiler2 {

void GlobalValueNumberingPass::init_partitions(Method::const_iterator begin, Method::const_iterator end) {

	// for each opcode which should be used for value numbering we will
	// create an initial partition, holding all the nodes belonging to
	// one specific opcode
	OpcodePartitionMapTy opcodeToPartition;
	LongPartitionMapTy longToPartition;

	for (Method::const_iterator i = begin, e = end; i != e; ++i) {
		Instruction *I = *i;
		PartitionTy *partition = (PartitionTy*) 0;
		if (I->get_opcode() == Instruction::CONSTInstID) {
			CONSTInst *constInst = I->to_CONSTInst();
			switch (constInst->get_type()) {
				case Type::IntTypeID:
					// TODO
					break;
				case Type::LongTypeID:
					// TODO: refactor (duplicate code)
					partition = longToPartition[constInst->get_Long()];
					if (!partition) {
						partition = new PartitionTy();
						longToPartition[constInst->get_Long()] = partition;
					}
					break;
				case Type::FloatTypeID:
					// TODO
					break;
				case Type::DoubleTypeID:
					// TODO
					break;
				default:
					assert(0 && "illegal type");
			}
		} else if (I->get_opcode() == Instruction::GETSTATICInstID) {
			partition = new PartitionTy();
			partitions.push_back(partition);
		} else if (I->get_opcode() != Instruction::BeginInstID
				&& !I->to_EndInst()) {

			partition = opcodeToPartition[I->get_opcode()];
			if (!partition) {
				partition = new PartitionTy();
				opcodeToPartition[I->get_opcode()] = partition;
			}
		}

		if (partition) {
			partition->insert(I);
		}
	}

	add_partitions_from_map(partitions, opcodeToPartition.begin(), opcodeToPartition.end());
	add_partitions_from_map(partitions, longToPartition.begin(), longToPartition.end());
}

int GlobalValueNumberingPass::arity(PartitionTy *partition) {
	PartitionTy::const_iterator i = partition->begin();
	Instruction *I = *i;
	return I->op_size();
}

int GlobalValueNumberingPass::compute_max_arity(Method::const_iterator begin,
		Method::const_iterator end) {
	int max = 0;
	for (Method::const_iterator i = begin, e = end; i != e; i++) {
		Instruction *I = *i;
		if (I->op_size() > max) {
			max = I->op_size();
		}
	}
	return max;
}

void GlobalValueNumberingPass::init_worklist_and_touchedpartitions() {	
	for (PartitionVectorTy::const_iterator i = partitions.begin(),
			e = partitions.end(); i != e; i++) {
		PartitionTy *partition = *i;

		for (int operandIndex = 0; operandIndex < max_arity; operandIndex++) {
			// create new work list pair
			WorkListPairTy *pair = new WorkListPairTy();
			pair->first = partition;
			pair->second = operandIndex;
			workList.push_back(pair);

			// mark work list pair
			set_in_worklist(partition, operandIndex, true);
		}
	}
}

std::vector<bool> *GlobalValueNumberingPass::get_worklist_flags(PartitionTy *partition) {
	std::vector<bool> *flags = inWorkList[partition];
	if (!flags) {
		flags = new std::vector<bool>(max_arity);
		inWorkList[partition] = flags;
	}
	return flags;
}

void GlobalValueNumberingPass::set_in_worklist(PartitionTy *partition, int index, bool flag) {
	std::vector<bool> *flags = get_worklist_flags(partition);
	(*flags)[index] = flag;
}

bool GlobalValueNumberingPass::is_in_worklist(PartitionTy *partition, int index) {
	std::vector<bool> *flags = get_worklist_flags(partition);
	return (*flags)[index];
}

GlobalValueNumberingPass::WorkListPairTy *GlobalValueNumberingPass::selectAndDeleteFromWorkList() {
	WorkListPairTy *pair = workList.front();
	workList.pop_front();
	set_in_worklist(pair->first, pair->second, false);
	return pair;
}

bool GlobalValueNumberingPass::run(JITData &JD) {
	Method *M = JD.get_Method();

	max_arity = compute_max_arity(M->begin(), M->end());
	init_partitions(M->begin(), M->end());
	init_worklist_and_touchedpartitions();

	while (!workList.empty()) {
		WorkListPairTy *workListPair = selectAndDeleteFromWorkList();
		PartitionTy *partition = workListPair->first;
		int operatorIndex = workListPair->second;
		delete workListPair;

//		// get the partition for the given partition id
//		PartitionTy *partition = partitions[partitionID];
//
//		std::list<PartitionIDTy> touched;
//		for (PartitionTy::const_iterator i = partition.begin(),
//				e = partition.end(); i != e; i++) {
//			Instruction *I = *i;
//			// TODO: insepct users of I
//		}
//
//		for (std::list<PartitionIDTy>::const_iterator i = touched.begin(),
//				e = touched.end(); i != e; i++) {
//			// TODO: split partitions
//		}

		// just some log output
		LOG("partition for operator " << operatorIndex << nl);
		for (PartitionTy::const_iterator i = partition->begin(),
				e = partition->end(); i != e; i++) {
			Instruction *I = *i;
			LOG(I << nl);
		}
	}

	return true;
}

// pass usage
PassUsage& GlobalValueNumberingPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires<InstructionMetaPass>();
	PU.add_run_before<DeadcodeEliminationPass>();
	return PU;
}

// the address of this variable is used to identify the pass
char GlobalValueNumberingPass::ID = 0;

// register pass
static PassRegistery<GlobalValueNumberingPass> X("GlobalValueNumberingPass");

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
