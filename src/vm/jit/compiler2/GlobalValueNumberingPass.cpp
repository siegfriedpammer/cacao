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

	// for each opcode a separate initial partition will be created,
	// holding all the nodes belonging to one specific opcode
	OpcodePartitionMapTy opcodeToPartition;

	// for each constant a initial partition will be created,
	// holding all the nodes of the same constant type and with
	// equal values.
	LongPartitionMapTy longToPartition;
	IntPartitionMapTy intToPartition;
	FloatPartitionMapTy floatToPartition;
	DoublePartitionMapTy doubleToPartition;

	for (Method::const_iterator i = begin, e = end; i != e; ++i) {
		Instruction *I = *i;
		PartitionTy *partition = (PartitionTy*) 0;
		if (I->get_opcode() == Instruction::CONSTInstID) {
			CONSTInst *constInst = I->to_CONSTInst();
			switch (constInst->get_type()) {
				case Type::IntTypeID:
					partition = get_or_create_partition(intToPartition,
						constInst->get_Int());
					break;
				case Type::LongTypeID:
					partition = get_or_create_partition(longToPartition,
						constInst->get_Long());
					break;
				case Type::FloatTypeID:
					partition = get_or_create_partition(floatToPartition,
						constInst->get_Float());
					break;
				case Type::DoubleTypeID:
					partition = get_or_create_partition(doubleToPartition,
						constInst->get_Double());

					break;
				default:
					assert(0 && "illegal type");
			}
		} else if (I->get_opcode() == Instruction::GETSTATICInstID
					|| I->get_opcode() == Instruction::PHIInstID) {
			partition = create_partition();
			partitions.push_back(partition);
		} else if (I->get_opcode() != Instruction::BeginInstID
				&& !I->to_EndInst()) {

			partition = get_or_create_partition(opcodeToPartition,
				I->get_opcode());
		}

		if (partition) {
			add_to_partition(partition, I);
		}
	}
}

/**
 * creates and returns a new partition and does some setup work needed
 * for the further execution of the partitioning algorithm
 */
GlobalValueNumberingPass::PartitionTy *
GlobalValueNumberingPass::create_partition() {
	PartitionTy *partition = new PartitionTy();
	partitions.push_back(partition);
	partition2TouchedInstListMap[partition] = new TouchedInstListTy();
	return partition;
}

void GlobalValueNumberingPass::add_to_partition(PartitionTy *partition, Instruction *inst) {
	partition->insert(inst);
	inst2PartitionMap[inst] = partition;
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
	// TODO: clear all the containers
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

void GlobalValueNumberingPass::add_to_worklist(PartitionTy *partition, int index) {
	// create new work list pair
	WorkListPairTy *pair = new WorkListPairTy();
	pair->first = partition;
	pair->second = index;
	workList.push_back(pair);

	// mark work list pair
	set_in_worklist(partition, index, true);
}

GlobalValueNumberingPass::WorkListPairTy *GlobalValueNumberingPass::selectAndDeleteFromWorkList() {
	WorkListPairTy *pair = workList.front();
	workList.pop_front();
	set_in_worklist(pair->first, pair->second, false);
	return pair;
}

GlobalValueNumberingPass::TouchedInstListTy *
GlobalValueNumberingPass::get_touched_instructions(PartitionTy *partition) {
	return partition2TouchedInstListMap[partition];
}

GlobalValueNumberingPass::PartitionTy *
GlobalValueNumberingPass::get_partition(Instruction *inst) {
	return inst2PartitionMap[inst];
}

void GlobalValueNumberingPass::split(PartitionTy *partition, TouchedInstListTy *instructions) {
	PartitionTy *new_partition = create_partition();

	assert(partition->size() > instructions->size());

	// remove the given instructions from the given partition and move them 
	// to the new one
	for (TouchedInstListTy::const_iterator i = instructions->begin(),
			e = instructions->end(); i != e; i++) {
		Instruction *I = *i;
		partition->erase(I);
		add_to_partition(new_partition, I);
	}

	for (int operandIndex = 0; operandIndex < max_arity; operandIndex++) {
		if (!is_in_worklist(partition, operandIndex)
				&& partition->size() <= new_partition->size()) {
			add_to_worklist(partition, operandIndex);
		} else {
			add_to_worklist(new_partition, operandIndex);
		}
	}
}

bool GlobalValueNumberingPass::run(JITData &JD) {
	Method *M = JD.get_Method();
	
	max_arity = compute_max_arity(M->begin(), M->end());
	init_partitions(M->begin(), M->end());
	init_worklist_and_touchedpartitions();

	LOG("BEFORE PARTITIONING" << nl);
	print_partitions();

	while (!workList.empty()) {
		WorkListPairTy *workListPair = selectAndDeleteFromWorkList();
		PartitionTy *partition = workListPair->first;
		int operandIndex = workListPair->second;
		delete workListPair;

//	  	LOG("=============================================================" << nl);
//		LOG("pulled partition from worklist for index " << operandIndex << ":" << nl);
//		print_partition(partition);

		std::list<PartitionTy*> touched;

		for (PartitionTy::const_iterator i = partition->begin(),
				e = partition->end(); i != e; i++) {
			Instruction *I = *i;

			for (Value::UserListTy::const_iterator ui = I->user_begin(),
					ue = I->user_end(); ui != ue; ui++) {
				Instruction *user = *ui;
				if (user->op_size() > operandIndex
						&& user->get_operand(operandIndex) == I) {
					PartitionTy *userPartition = get_partition(user);

					// the user's partition can be 0 if it is and
					// end inst
					if (userPartition) {
						touched.push_back(userPartition);
						TouchedInstListTy *touchedInstructions = get_touched_instructions(userPartition);
						touchedInstructions->insert(user); // TODO: check possible duplicates
					}
				}
			}
		}

		// we have to look at every partition that contains instructions
		// whose operand at index operandIndex is in the current partition
		for (std::list<PartitionTy*>::const_iterator i = touched.begin(),
				e = touched.end(); i != e; i++) {
			PartitionTy *touchedPartition = *i;
			TouchedInstListTy *touchedInstructions = get_touched_instructions(touchedPartition);

			if (touchedInstructions->size() > 0
					&& touchedPartition->size() != touchedInstructions->size()) {
				LOG("split partition (" << touchedPartition->size() << ", " << touchedInstructions->size() << ")" << nl);
				print_partition(touchedPartition);
				LOG("remove" << nl);
				print_instructions(touchedInstructions);
				split(touchedPartition, touchedInstructions);
			}
			touchedInstructions->clear();
		}
	}

	LOG("AFTER PARTITIONING" << nl);
	print_partitions();
	consolidate_partitions();

	return true;
}

void GlobalValueNumberingPass::consolidate_partitions() {
	for (PartitionVectorTy::const_iterator i = partitions.begin(),
			e = partitions.end(); i != e; i++) {
		PartitionTy *partition = *i;
		if (partition->size() > 1) {
			consolidate_partition(partition);
		}
	}
}

void GlobalValueNumberingPass::consolidate_partition(PartitionTy *partition) {
	PartitionTy::iterator i = partition->begin();
	Instruction *instruction = *i;
	i++;

	for (PartitionTy::iterator e = partition->end(); i != e; i++) {
		Instruction *replacable = *i;
		replacable->replace_value(instruction);
	}
}

void GlobalValueNumberingPass::print_instructions(TouchedInstListTy *instructions) {
	for (TouchedInstListTy::const_iterator i = instructions->begin(),
		e = instructions->end(); i != e; i++) {
		LOG(*i << nl);
	}
}

void GlobalValueNumberingPass::print_partition(PartitionTy *partition) {
	for (PartitionTy::const_iterator i = partition->begin(),
			e = partition->end(); i != e; i++) {
		Instruction *I = *i;
		LOG(I << nl);
	}
}

void GlobalValueNumberingPass::print_partitions() {
	for (PartitionVectorTy::const_iterator i = partitions.begin(),
			e = partitions.end(); i != e; i++) {
		LOG("=============================================================" << nl);
		LOG("PARTITION" << nl);
		PartitionTy *partition = *i;
		print_partition(partition);
	}
}

// pass usage
PassUsage& GlobalValueNumberingPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires<InstructionMetaPass>();
	PU.add_schedule_before<DeadcodeEliminationPass>();
	return PU;
}

// the address of this variable is used to identify the pass
char GlobalValueNumberingPass::ID = 0;

// register pass
static PassRegistry<GlobalValueNumberingPass> X("GlobalValueNumberingPass");

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
