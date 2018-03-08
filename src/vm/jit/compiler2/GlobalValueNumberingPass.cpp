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
#include "vm/jit/compiler2/MethodC2.hpp"
#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/SSAConstructionPass.hpp"
#include "vm/jit/compiler2/DeadCodeEliminationPass.hpp"
#include "vm/jit/compiler2/ScheduleEarlyPass.hpp"
#include "toolbox/logging.hpp"

// define name for debugging (see logging.hpp)
#define DEBUG_NAME "compiler2/GlobalValueNumberingPass"

STAT_DECLARE_GROUP(compiler2_stat)
STAT_REGISTER_SUBGROUP(compiler2_globalvaluenumberingpass_stat,
	"globalvaluenumberingpass","globalvaluenumberingpass",compiler2_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,num_total_const,0,"# CONSTInsts",
	"total number of CONSTInst nodes",compiler2_globalvaluenumberingpass_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,num_total_arith,0,"# arithmetical nodes",
	"total number of arithmetical nodes",compiler2_globalvaluenumberingpass_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,num_total_phi,0,"# PHIInsts",
	"total number of PHIInst nodes",compiler2_globalvaluenumberingpass_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,num_total_arraybc,0,"# ARRAYBOUNDSCHECKInsts",
	"total number of ARRAYBOUNDSCHECKInst nodes",compiler2_globalvaluenumberingpass_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,num_redundant_const,0,"# redundant CONSTInsts",
	"number of redundant CONSTInst nodes",compiler2_globalvaluenumberingpass_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,num_redundant_arith,0,"# redundant arithmetical nodes",
	"number of redundant arithmetical nodes",compiler2_globalvaluenumberingpass_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,num_redundant_phi,0,"# redundant PHIInsts",
	"number of redundant PHIInst nodes",compiler2_globalvaluenumberingpass_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,num_redundant_arraybc,0,"# redundant ARRAYBOUNDSCHECKInsts",
	"number of redundant ARRAYBOUNDSCHECKInst nodes",compiler2_globalvaluenumberingpass_stat)

#define STAT_NODE_COUNT_HELPER(INST, CNT, INFIX)								\
	if ((INST)->get_opcode() == Instruction::CONSTInstID) {						\
		STATISTICS(num_##INFIX##_const += (CNT));								\
	} else if ((INST)->is_arithmetic()) {										\
		STATISTICS(num_##INFIX##_arith += (CNT));								\
	} else if ((INST)->get_opcode() == Instruction::PHIInstID) {				\
		STATISTICS(num_##INFIX##_phi += (CNT));									\
	} else if ((INST)->get_opcode() == Instruction::ARRAYBOUNDSCHECKInstID) {	\
		STATISTICS(num_##INFIX##_arraybc += (CNT));								\
	}
#define STAT_TOTAL_NODES(INST) STAT_NODE_COUNT_HELPER(INST, 1, total)
#define STAT_REDUNDANT_NODES(INST, CNT) STAT_NODE_COUNT_HELPER(INST, CNT, redundant)

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

		STAT_TOTAL_NODES(I)

		if (I->has_side_effects()
				|| !I->is_floating()
				|| I->get_opcode() == Instruction::LOADInstID
				|| I->get_opcode() == Instruction::ALOADInstID
				|| I->get_opcode() == Instruction::SourceStateInstID
				|| I->get_opcode() == Instruction::DeoptimizeInstID
				|| I->get_opcode() == Instruction::AssumptionInstID
				|| I->get_opcode() == Instruction::CHECKNULLInstID) {
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
					block = create_block();
					break;
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
	std::size_t max = 0;
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

GlobalValueNumberingPass::InstructionListTy *
GlobalValueNumberingPass::get_users(Instruction *inst, int op_index) {
	OperandIndex2UsersTy *inverseVector = operandInverseMap[inst];
	if (!inverseVector) {
		inverseVector = new OperandIndex2UsersTy();
		operandInverseMap[inst] = inverseVector;
		for (int i = 0; i < max_arity; i++) {
			InstructionListTy *users = new InstructionListTy();
			inverseVector->push_back(users);
		}
	}
	return (*inverseVector)[op_index];
}

void GlobalValueNumberingPass::init_operand_inverse(Method::const_iterator begin, Method::const_iterator end) {
	for (Method::const_iterator i = begin, e = end; i != e; ++i) {
		Instruction *I = *i;
		int op_index = 0;
		for (Instruction::const_op_iterator oi = I->op_begin(),
			oe = I->op_end(); oi != oe; oi++) {
			Instruction *Op = (*oi)->to_Instruction();
			assert(Op);
			InstructionListTy *users = get_users(Op, op_index);
			users->push_back(I);
			op_index++;
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

void GlobalValueNumberingPass::eliminate_redundancies() {
	for (PartitionTy::const_iterator i = partition.begin(),
			e = partition.end(); i != e; i++) {
		BlockTy *block = *i;
		if (block->size() > 1) {
			eliminate_redundancies_in_block(block);
		}
	}
}

void GlobalValueNumberingPass::eliminate_redundancies_in_block(BlockTy *block) {
	Instruction *inst = *(block->begin());

	// A CONSTInst does not represent a computation, hence CONSTInsts that
	// represent the same constant value don't need to be consolidated.
	if (inst->to_CONSTInst()) {
		return;
	}

	// the first node in the block will be used to replace all the others in
	// the block, the rest of them (i.e. block->size() - 1) is redundant
	STAT_REDUNDANT_NODES(inst, block->size() - 1);

	for (auto i = std::next(block->begin()); i != block->end(); i++) {
		Instruction *replacable = *i;
		replacable->replace_value(inst);

		alloc::vector<Instruction*>::type rdeps(replacable->rdep_begin(), replacable->rdep_end());

		for (Instruction *dependent : rdeps) {
			dependent->append_dep(inst);
			dependent->remove_dep(replacable);
		}
	}
}

void GlobalValueNumberingPass::print_instructions(TouchedInstListTy *instructions) {
	#if ENABLE_LOGGING
	for (TouchedInstListTy::const_iterator i = instructions->begin(),
		e = instructions->end(); i != e; i++) {
		LOG(*i << nl);
	}
	#endif
}

void GlobalValueNumberingPass::print_block(BlockTy *block) {
	#if ENABLE_LOGGING
	for (BlockTy::const_iterator i = block->begin(),
			e = block->end(); i != e; i++) {
		Instruction *I = *i;
		LOG(I << nl);
	}
	#endif
}

void GlobalValueNumberingPass::print_blocks() {
	#if ENABLE_LOGGING
	for (PartitionTy::const_iterator i = partition.begin(),
			e = partition.end(); i != e; i++) {
		LOG("=============================================================" << nl);
		LOG("Block:" << nl);
		BlockTy *block = *i;
		print_block(block);
	}
	#endif
}

template <typename T>
void delete_all(T *lst) {
	while (!lst->empty()) {
		delete lst->back();
		lst->pop_back();
	}
}

template <typename T1, typename T2, typename Iterator>
void delete_in_range(std::unordered_map<T1,T2> *map, Iterator begin, Iterator end) {
	Iterator i = begin;
	while (i != end) {
		delete i->second;
		map->erase(i++);
	}
}

template <typename T1, typename T2>
void delete_all(std::unordered_map<T1,T2> *map) {
	delete_in_range(map, map->begin(), map->end());
}

void GlobalValueNumberingPass::clean_up_operand_inverse() {
	OperandInverseMapTy::const_iterator i = operandInverseMap.begin();
	OperandInverseMapTy::const_iterator e = operandInverseMap.end();
	while (i != e) {
		OperandIndex2UsersTy *opUsers = i->second;
		delete_all(opUsers);
		delete opUsers;
		operandInverseMap.erase(i++);
	}
}

bool GlobalValueNumberingPass::run(JITData &JD) {
	Method *M = JD.get_Method();
	
	max_arity = compute_max_arity(M->begin(), M->end());
	init_partition(M->begin(), M->end());
	init_worklist_and_touchedblocks();
	init_operand_inverse(M->begin(), M->end());

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
			InstructionListTy *usersForIndex = get_users(I, operandIndex);

			for (InstructionListTy::const_iterator ui = usersForIndex->begin(),
					ue = usersForIndex->end(); ui != ue; ui++) {
				Instruction *user = *ui;
				BlockTy *userBlock = get_block(user);

				// the user's block can be 0 if it is an end inst
				if (userBlock) {
					touched.push_back(userBlock);
					TouchedInstListTy *touchedInstructions =
						get_touched_instructions(userBlock);
					touchedInstructions->insert(user);
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
	eliminate_redundancies();

	delete_all(&partition);
	delete_in_range(&block2TouchedInstListMap, block2TouchedInstListMap.begin(),
		block2TouchedInstListMap.end());
	delete_in_range(&inWorkList, inWorkList.begin(), inWorkList.end());
	clean_up_operand_inverse();

	return true;
}

// pass usage
PassUsage& GlobalValueNumberingPass::get_PassUsage(PassUsage &PU) const {
	PU.requires<HIRInstructionsArtifact>();
	PU.before<DeadCodeEliminationPass>();
	PU.before<ScheduleEarlyPass>();
	return PU;
}

Option<bool> GlobalValueNumberingPass::enabled("GlobalValueNumberingPass","compiler2: enable global value numbering (default = true)",true,::cacao::option::xx_root());

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
