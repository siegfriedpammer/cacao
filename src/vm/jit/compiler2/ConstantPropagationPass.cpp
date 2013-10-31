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

#define DEBUG_NAME "compiler2/constantpropagationpass"

namespace cacao {
namespace jit {
namespace compiler2 {

CONSTInst *foldBinaryInst(BinaryInst *inst) {
	CONSTInst *op1 = inst->get_operand(0)->to_Instruction()->to_CONSTInst();
	CONSTInst *op2 = inst->get_operand(1)->to_Instruction()->to_CONSTInst();
	assert(op1 && op2);

	switch (inst->get_opcode()) {
		case Instruction::ADDInstID:
			switch (inst->get_type()) {
				case Type::IntTypeID:
					return new CONSTInst(op1->get_Int() + op2->get_Int());
				case Type::LongTypeID:
					return new CONSTInst(op1->get_Long() + op2->get_Long());
				case Type::FloatTypeID:
					return new CONSTInst(op1->get_Float() + op2->get_Float());
				case Type::DoubleTypeID:
					return new CONSTInst(op1->get_Double() + op2->get_Double());
				default:
					assert(0);
					return 0;
			}
			break;
		case Instruction::SUBInstID:
			switch (inst->get_type()) {
				case Type::IntTypeID:
					return new CONSTInst(op1->get_Int() - op2->get_Int());
				case Type::LongTypeID:
					return new CONSTInst(op1->get_Long() - op2->get_Long());
				case Type::FloatTypeID:
					return new CONSTInst(op1->get_Float() - op2->get_Float());
				case Type::DoubleTypeID:
					return new CONSTInst(op1->get_Double() - op2->get_Double());
				default:
					assert(0);
					return 0;
			}
			break;
		case Instruction::MULInstID:
			switch (inst->get_type()) {
				case Type::IntTypeID:
					return new CONSTInst(op1->get_Int() * op2->get_Int());
				case Type::LongTypeID:
					return new CONSTInst(op1->get_Long() * op2->get_Long());
				case Type::FloatTypeID:
					return new CONSTInst(op1->get_Float() * op2->get_Float());
				case Type::DoubleTypeID:
					return new CONSTInst(op1->get_Double() * op2->get_Double());
				default:
					assert(0);
					return 0;
			}
			break;
		case Instruction::DIVInstID:
			switch (inst->get_type()) {
				case Type::IntTypeID:
					return new CONSTInst(op1->get_Int() / op2->get_Int());
				case Type::LongTypeID:
					return new CONSTInst(op1->get_Long() / op2->get_Long());
				case Type::FloatTypeID:
					return new CONSTInst(op1->get_Float() / op2->get_Float());
				case Type::DoubleTypeID:
					return new CONSTInst(op1->get_Double() / op2->get_Double());
				default:
					assert(0);
					return 0;
			}
			break;
		default:
			return 0;
	}
}

CONSTInst *foldInstruction(Instruction *inst) {
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
