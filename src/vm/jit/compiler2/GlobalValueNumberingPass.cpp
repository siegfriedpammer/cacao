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

void GlobalValueNumberingPass::init_partition(Method::const_iterator begin, Method::const_iterator end) {

	OpcodeBlockMapTy opcodeToBlock;
	LongBlockMapTy longToBlock;
	IntBlockMapTy intToBlock;
	FloatBlockMapTy floatToBlock;
	DoubleBlockMapTy doubleToBlock;
	BBBlockMapTy bbToBlock;

	for (Method::const_iterator i = begin, e = end; i != e; ++i) {
		Instruction *I = *i;
		BlockTy *block = (BlockTy*) 0;

		if (I->has_side_effects()
				|| I->get_opcode() == Instruction::LOADInstID
				|| I->get_opcode() == Instruction::ALOADInstID) {
			// instructions which change the global state or depend on it
			// will be in a separate block each, because they are congruent
			// only with themselves

			block = create_block();
		} else if (I->get_opcode() == Instruction::CONSTInstID) {
			CONSTInst *constInst = I->to_CONSTInst();
			// for each constant an initial block will be created,
			// holding all the nodes of the same constant type that have
			// equal values.

			switch (constInst->get_type()) {
				case Type::IntTypeID:
					block = get_or_create_block(intToBlock,
						constInst->get_Int());
					break;
				case Type::LongTypeID:
					block = get_or_create_block(longToBlock,
						constInst->get_Long());
					break;
				case Type::FloatTypeID:
					block = get_or_create_block(floatToBlock,
						constInst->get_Float());
					break;
				case Type::DoubleTypeID:
					block = get_or_create_block(doubleToBlock,
						constInst->get_Double());

					break;
				default:
					assert(0 && "illegal type");
			}
		} else if (I->get_opcode() == Instruction::PHIInstID) {
			// all PHIInsts which belong to the same basic block
			// will be pooled into a common initial block
			// TODO: generally this should hold for other instructions
			// that are floating

			PHIInst *phiInst = I->to_PHIInst();
			assert(phiInst);
			BeginInst *bb = phiInst->get_BeginInst();
			block = get_or_create_block(bbToBlock, bb);
		} else if (I->get_opcode() != Instruction::BeginInstID
				&& !I->to_EndInst()) {
			// all other instructions which are no control flow
			// instructions will be pooled into a common block
			// per opcode

			block = get_or_create_block(opcodeToBlock,
				I->get_opcode());
		}

		if (block) {
			add_to_block(block, I);
		}
	}
}

/**
 * creates and returns a new block and does some setup work needed
 * for the further execution of the partitioning algorithm
 */
GlobalValueNumberingPass::BlockTy *
GlobalValueNumberingPass::create_block() {
	BlockTy *block = new BlockTy();
	partition.push_back(block);
	block2TouchedInstListMap[block] = new TouchedInstListTy();
	return block;
}

void GlobalValueNumberingPass::add_to_block(BlockTy *block, Instruction *inst) {
	block->insert(inst);
	inst2BlockMap[inst] = block;
}

int GlobalValueNumberingPass::arity(BlockTy *block) {
	BlockTy::const_iterator i = block->begin();
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

void GlobalValueNumberingPass::init_worklist_and_touchedblocks() {
	// TODO: clear all the containers
	for (PartitionTy::const_iterator i = partition.begin(),
			e = partition.end(); i != e; i++) {
		BlockTy *block = *i;

		for (int operandIndex = 0; operandIndex < max_arity; operandIndex++) {
			// create new work list pair
			WorkListPairTy *pair = new WorkListPairTy();
			pair->first = block;
			pair->second = operandIndex;
			workList.push_back(pair);

			// mark work list pair
			set_in_worklist(block, operandIndex, true);
		}
	}
}

std::vector<bool> *GlobalValueNumberingPass::get_worklist_flags(BlockTy *block) {
	std::vector<bool> *flags = inWorkList[block];
	if (!flags) {
		flags = new std::vector<bool>(max_arity);
		inWorkList[block] = flags;
	}
	return flags;
}

void GlobalValueNumberingPass::set_in_worklist(BlockTy *block, int index, bool flag) {
	std::vector<bool> *flags = get_worklist_flags(block);
	(*flags)[index] = flag;
}

bool GlobalValueNumberingPass::is_in_worklist(BlockTy *block, int index) {
	std::vector<bool> *flags = get_worklist_flags(block);
	return (*flags)[index];
}

void GlobalValueNumberingPass::add_to_worklist(BlockTy *block, int index) {
	// create new work list pair
	WorkListPairTy *pair = new WorkListPairTy();
	pair->first = block;
	pair->second = index;
	workList.push_back(pair);

	// mark work list pair
	set_in_worklist(block, index, true);
}

GlobalValueNumberingPass::WorkListPairTy *GlobalValueNumberingPass::selectAndDeleteFromWorkList() {
	WorkListPairTy *pair = workList.front();
	workList.pop_front();
	set_in_worklist(pair->first, pair->second, false);
	return pair;
}

GlobalValueNumberingPass::TouchedInstListTy *
GlobalValueNumberingPass::get_touched_instructions(BlockTy *block) {
	return block2TouchedInstListMap[block];
}

GlobalValueNumberingPass::BlockTy *
GlobalValueNumberingPass::get_block(Instruction *inst) {
	return inst2BlockMap[inst];
}

void GlobalValueNumberingPass::split(BlockTy *block, TouchedInstListTy *instructions) {
	BlockTy *new_block = create_block();

	assert(block->size() > instructions->size());

	// remove the given instructions from the given block and move them 
	// to the new one
	for (TouchedInstListTy::const_iterator i = instructions->begin(),
			e = instructions->end(); i != e; i++) {
		Instruction *I = *i;
		block->erase(I);
		add_to_block(new_block, I);
	}

	for (int operandIndex = 0; operandIndex < max_arity; operandIndex++) {
		if (!is_in_worklist(block, operandIndex)
				&& block->size() <= new_block->size()) {
			add_to_worklist(block, operandIndex);
		} else {
			add_to_worklist(new_block, operandIndex);
		}
	}
}

bool GlobalValueNumberingPass::run(JITData &JD) {
	Method *M = JD.get_Method();
	
	max_arity = compute_max_arity(M->begin(), M->end());
	init_partition(M->begin(), M->end());
	init_worklist_and_touchedblocks();

	LOG("BEFORE PARTITIONING" << nl);
	print_blocks();

	while (!workList.empty()) {
		WorkListPairTy *workListPair = selectAndDeleteFromWorkList();
		BlockTy *block = workListPair->first;
		int operandIndex = workListPair->second;
		delete workListPair;

		std::list<BlockTy*> touched;

		for (BlockTy::const_iterator i = block->begin(),
				e = block->end(); i != e; i++) {
			Instruction *I = *i;

			for (Value::UserListTy::const_iterator ui = I->user_begin(),
					ue = I->user_end(); ui != ue; ui++) {
				Instruction *user = *ui;
				if (user->op_size() > operandIndex
						&& user->get_operand(operandIndex) == I) {
					BlockTy *userBlock = get_block(user);

					// the user's block can be 0 if it is and
					// end inst
					if (userBlock) {
						touched.push_back(userBlock);
						TouchedInstListTy *touchedInstructions = get_touched_instructions(userBlock);
						touchedInstructions->insert(user); // TODO: check possible duplicates
					}
				}
			}
		}

		// we have to look at every block that contains instructions
		// whose operand at index operandIndex is in the current block
		for (std::list<BlockTy*>::const_iterator i = touched.begin(),
				e = touched.end(); i != e; i++) {
			BlockTy *touchedBlock = *i;
			TouchedInstListTy *touchedInstructions = get_touched_instructions(touchedBlock);

			if (touchedInstructions->size() > 0
					&& touchedBlock->size() != touchedInstructions->size()) {
				LOG("split block (" << touchedBlock->size() << ", " << touchedInstructions->size() << ")" << nl);
				print_block(touchedBlock);
				LOG("remove" << nl);
				print_instructions(touchedInstructions);
				split(touchedBlock, touchedInstructions);
			}
			touchedInstructions->clear();
		}
	}

	LOG(nl << "AFTER PARTITIONING" << nl);
	print_blocks();
	consolidate_blocks();

	return true;
}

void GlobalValueNumberingPass::consolidate_blocks() {
	for (PartitionTy::const_iterator i = partition.begin(),
			e = partition.end(); i != e; i++) {
		BlockTy *block = *i;
		if (block->size() > 1) {
			consolidate_block(block);
		}
	}
}

void GlobalValueNumberingPass::consolidate_block(BlockTy *block) {
	BlockTy::iterator i = block->begin();
	Instruction *inst = *i;
	i++;

	for (BlockTy::iterator e = block->end(); i != e; i++) {
		Instruction *replacable = *i;
		replacable->replace_value(inst);

		if (inst->get_opcode() == Instruction::ARRAYBOUNDSCHECKInstID) {
			// TODO: consider scheduling dependencies of other instruction types
			// as well (but at the time of implementation the only instruction
			// type with scheduling dependencies are array bounds check nodes)

			Instruction::const_dep_iterator i;
			Instruction::const_dep_iterator e = replacable->rdep_end();
			while ((i = replacable->rdep_begin()) != e) {
				Instruction *dependent = *i;
				dependent->append_dep(inst);
				dependent->remove_dep(replacable);
			}
		}
	}
}

void GlobalValueNumberingPass::print_instructions(TouchedInstListTy *instructions) {
	for (TouchedInstListTy::const_iterator i = instructions->begin(),
		e = instructions->end(); i != e; i++) {
		LOG(*i << nl);
	}
}

void GlobalValueNumberingPass::print_block(BlockTy *block) {
	for (BlockTy::const_iterator i = block->begin(),
			e = block->end(); i != e; i++) {
		Instruction *I = *i;
		LOG(I << nl);
	}
}

void GlobalValueNumberingPass::print_blocks() {
	for (PartitionTy::const_iterator i = partition.begin(),
			e = partition.end(); i != e; i++) {
		LOG("=============================================================" << nl);
		LOG("Block:" << nl);
		BlockTy *block = *i;
		print_block(block);
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
