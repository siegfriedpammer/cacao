/* src/vm/jit/compiler2/ConstantPropagationPass.cpp - ConstantPropagationPass

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

#include "vm/jit/compiler2/ConstantPropagationPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/DeadcodeEliminationPass.hpp"
#include "vm/jit/compiler2/ScheduleClickPass.hpp"

#define DEBUG_NAME "compiler2/constantpropagationpass"

namespace cacao {
namespace jit {
namespace compiler2 {

/// Operation function object class
template <typename T, Instruction::InstID ID>
struct Operation : public std::binary_function<T,T,T> {
	T operator()(const T &lhs, const T &rhs) const ;
};

/// Template specialization for ADDInstID
template <typename T>
struct Operation<T, Instruction::ADDInstID> :
		public std::binary_function<T,T,T> {
	T operator()(const T &lhs, const T &rhs) const {
		return lhs + rhs;
	}
};

/// Template specialization for SUBInstID
template <typename T>
struct Operation<T, Instruction::SUBInstID> :
		public std::binary_function<T,T,T> {
	T operator()(const T &lhs, const T &rhs) const {
		return lhs - rhs;
	}
};

/// Template specialization for MULInstID
template <typename T>
struct Operation<T, Instruction::MULInstID> :
		public std::binary_function<T,T,T> {
	T operator()(const T &lhs, const T &rhs) const {
		return lhs * rhs;
	}
};

/// Template specialization for DIVInstID
template <typename T>
struct Operation<T, Instruction::DIVInstID> :
		public std::binary_function<T,T,T> {
	T operator()(const T &lhs, const T &rhs) const {
		return lhs / rhs;
	}
};

/// function wrapper for Operation
template <typename T>
inline T operate(Instruction::InstID ID, const T &lhs, const T &rhs) {
	switch (ID) {
		case Instruction::ADDInstID:
			return Operation<T,Instruction::ADDInstID>()(lhs, rhs);
		case Instruction::SUBInstID:
			return Operation<T,Instruction::SUBInstID>()(lhs, rhs);
		case Instruction::MULInstID:
			return Operation<T,Instruction::MULInstID>()(lhs, rhs);
		case Instruction::DIVInstID:
			return Operation<T,Instruction::DIVInstID>()(lhs, rhs);
		default:
			assert(0 && "not implemented");
			break;
	}
	// unreachable - dummy result
	return lhs;
}

CONSTInst *foldBinaryInst(BinaryInst *inst) {
	CONSTInst *op1 = inst->get_operand(0)->to_Instruction()->to_CONSTInst();
	CONSTInst *op2 = inst->get_operand(1)->to_Instruction()->to_CONSTInst();
	assert(op1 && op2);

	switch (inst->get_type()) {
		case Type::IntTypeID:
			return new CONSTInst(operate(inst->get_opcode(), op1->get_Int(),
				op2->get_Int()));
		case Type::LongTypeID:
			return new CONSTInst(operate(inst->get_opcode(), op1->get_Long(),
				op2->get_Long()));
		case Type::FloatTypeID:
			return new CONSTInst(operate(inst->get_opcode(), op1->get_Float(),
				op2->get_Float()));
		case Type::DoubleTypeID:
			return new CONSTInst(operate(inst->get_opcode(), op1->get_Double(),
				op2->get_Double()));
		default:
			assert(0);
			return 0;
	}
}

CONSTInst *foldInstruction(Instruction *inst) {
	// TODO: introduce is_arithmetic() in Instruction
	if (inst->to_BinaryInst()) {
		return foldBinaryInst(inst->to_BinaryInst());
	}
	return 0;
}

bool ConstantPropagationPass::run(JITData &JD) {
	Method *M = JD.get_Method();

	// this work list is used by the algorithm to store the instructions which
	// have to be reconsidered. at the beginning it therefore contains all
	// instructions.
	Method::InstructionListTy workList(M->begin(), M->end());

	// will be used to look up whether an instruction is currently contained in the
	// worklist to avoid inserting an instruction which is already in the list.
	InstBoolMapTy inWorkList;
	for (Method::const_iterator i = workList.begin(), e = workList.end();
			i != e; i++) {
		inWorkList[*i] = true;
	}
	
	// used to track for each instruction the number of its operands which are
	// already known to be constant
	InstIntMapTy constantOperands;

	while (!workList.empty()) {
		Instruction *I = workList.front();
		workList.pop_front();
		inWorkList[I] = false;

		if (constantOperands[I] == I->op_size()) {
			for (Value::UserListTy::const_iterator i = I->user_begin(),
					e = I->user_end(); i != e; i++) {
				Instruction *user = *i;
				constantOperands[user]++;
				if (!inWorkList[user]) {
					workList.push_back(user);
					inWorkList[user] = true;
				}
			}

			if (I->get_opcode() != Instruction::CONSTInstID) {
				CONSTInst *foldedInst = foldInstruction(I);
	
				if (foldedInst) {
					LOG("replace " << I
						<< " by " << foldedInst
						<< " (with value " << foldedInst->get_Int() << ")"
						<< nl);
					I->replace_value(foldedInst);
					M->add_Instruction(foldedInst);
				}
			}
		}
	}

	return true;
}

// pass usage
PassUsage& ConstantPropagationPass::get_PassUsage(PassUsage &PU) const {
	PU.add_schedule_before<DeadcodeEliminationPass>();
	return PU;
}

// the address of this variable is used to identify the pass
char ConstantPropagationPass::ID = 0;

// register pass
static PassRegistery<ConstantPropagationPass> X("ConstantPropagationPass");

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
