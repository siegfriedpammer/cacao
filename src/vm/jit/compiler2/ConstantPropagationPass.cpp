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
#include "vm/jit/compiler2/GlobalValueNumberingPass.hpp"
#include "vm/jit/compiler2/ScheduleClickPass.hpp"
#include "vm/jit/compiler2/InstructionMetaPass.hpp"
#include "vm/jit/compiler2/SSAPrinterPass.hpp"

#define DEBUG_NAME "compiler2/constantpropagationpass"

namespace cacao {
namespace jit {
namespace compiler2 {

/// BinaryOperation function object class
template <typename T, Instruction::InstID ID>
struct BinaryOperation : public std::binary_function<T,T,T> {
	T operator()(const T &lhs, const T &rhs) const ;
};

/// Template specialization for ADDInstID
template <typename T>
struct BinaryOperation<T, Instruction::ADDInstID> :
		public std::binary_function<T,T,T> {
	T operator()(const T &lhs, const T &rhs) const {
		return lhs + rhs;
	}
};

/// Template specialization for SUBInstID
template <typename T>
struct BinaryOperation<T, Instruction::SUBInstID> :
		public std::binary_function<T,T,T> {
	T operator()(const T &lhs, const T &rhs) const {
		return lhs - rhs;
	}
};

/// Template specialization for MULInstID
template <typename T>
struct BinaryOperation<T, Instruction::MULInstID> :
		public std::binary_function<T,T,T> {
	T operator()(const T &lhs, const T &rhs) const {
		return lhs * rhs;
	}
};

/// Template specialization for DIVInstID
template <typename T>
struct BinaryOperation<T, Instruction::DIVInstID> :
		public std::binary_function<T,T,T> {
	T operator()(const T &lhs, const T &rhs) const {
		return lhs / rhs;
	}
};

/// Template specialization for ANDInstID
template <typename T>
struct BinaryOperation<T, Instruction::ANDInstID> :
		public std::binary_function<T,T,T> {
	T operator()(const T &lhs, const T &rhs) const {
		return lhs & rhs;
	}
};

/// function wrapper for BinaryOperation
template <typename T>
inline T binaryOperate(Instruction::InstID ID, const T &lhs, const T &rhs) {
	switch (ID) {
		case Instruction::ADDInstID:
			return BinaryOperation<T,Instruction::ADDInstID>()(lhs, rhs);
		case Instruction::SUBInstID:
			return BinaryOperation<T,Instruction::SUBInstID>()(lhs, rhs);
		case Instruction::MULInstID:
			return BinaryOperation<T,Instruction::MULInstID>()(lhs, rhs);
		case Instruction::DIVInstID:
			return BinaryOperation<T,Instruction::DIVInstID>()(lhs, rhs);
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
			return new CONSTInst(binaryOperate(inst->get_opcode(),
				op1->get_Int(), op2->get_Int()), Type::IntType());
		case Type::LongTypeID:
			return new CONSTInst(binaryOperate(inst->get_opcode(),
				op1->get_Long(), op2->get_Long()), Type::LongType());
		case Type::FloatTypeID:
			return new CONSTInst(binaryOperate(inst->get_opcode(),
				op1->get_Float(), op2->get_Float()), Type::FloatType());
		case Type::DoubleTypeID:
			return new CONSTInst(binaryOperate(inst->get_opcode(),
				op1->get_Double(), op2->get_Double()), Type::DoubleType());
		default:
			assert(0);
			return 0;
	}
}

/// UnaryOperation function object class
template <typename T, Instruction::InstID ID>
struct UnaryOperation :public std::unary_function<T,T> {
	T operator()(const T &op) const;
};

/// Template specialization for NEGInstID
template <typename T>
struct UnaryOperation<T, Instruction::NEGInstID> :
		public std::unary_function<T,T> {
	T operator()(const T &op) const {
		return -op;
	}
};

/// function wrapper for UnaryOperation
template <typename T>
inline T unaryOperate(Instruction::InstID ID, const T &op) {
	switch (ID) {
		case Instruction::NEGInstID:
			return UnaryOperation<T,Instruction::NEGInstID>()(op);
		default:
			assert(0 && "not implemented");
			break;
	}
	// unreachable - dummy result
	return op;
}


CONSTInst *foldUnaryInst(UnaryInst *inst) {
	CONSTInst *op = inst->get_operand(0)->to_Instruction()->to_CONSTInst();
	assert(op);
	assert(inst->get_type() == op->get_type());
	
	switch (inst->get_type()) {
		case Type::IntTypeID:
			return new CONSTInst(unaryOperate(inst->get_opcode(),
				op->get_Int()), Type::IntType());
		case Type::LongTypeID:
			return new CONSTInst(unaryOperate(inst->get_opcode(),
				op->get_Long()), Type::LongType());
		case Type::FloatTypeID:
			return new CONSTInst(unaryOperate(inst->get_opcode(),
				op->get_Float()), Type::FloatType());
		case Type::DoubleTypeID:
			return new CONSTInst(unaryOperate(inst->get_opcode(),
				op->get_Double()), Type::DoubleType());
		default:
			assert(0);
			return 0;
	}
}

template <typename T>
CONSTInst *castOperate(CASTInst *inst, const T &op) {
	switch (inst->get_type()) {
		case Type::IntTypeID:
			return new CONSTInst((int32_t) op, Type::IntType());
		case Type::LongTypeID:
			return new CONSTInst((int64_t) op, Type::LongType());
		case Type::FloatTypeID:
			return new CONSTInst((float) op, Type::FloatType());
		case Type::DoubleTypeID:
			return new CONSTInst((double) op, Type::DoubleType());
		default:
			assert(0);
			return 0;
	}
}

CONSTInst *foldCASTInst(CASTInst *inst) {
	CONSTInst *op = inst->get_operand(0)->to_Instruction()->to_CONSTInst();
	assert(op);
	
	switch (op->get_type()) {
		case Type::IntTypeID:
			return castOperate(inst, op->get_Int());
		case Type::LongTypeID:
			return castOperate(inst, op->get_Long());
		case Type::FloatTypeID:
			return castOperate(inst, op->get_Float());
		case Type::DoubleTypeID:
			return castOperate(inst, op->get_Double());
		default:
			assert(0);
			return 0;
	}
}

CONSTInst *foldInstruction(Instruction *inst) {
	// TODO: distinguish arithmetical and bitwise operations
	if (inst->get_opcode() == Instruction::CASTInstID) {
		return foldCASTInst(inst->to_CASTInst());
	} else if (inst->to_UnaryInst()) {
		return foldUnaryInst(inst->to_UnaryInst());
	} else if (inst->to_BinaryInst()) {
		return foldBinaryInst(inst->to_BinaryInst());
	}

	return 0;
}

void ConstantPropagationPass::propagate(Instruction *inst) {
	for (Value::UserListTy::const_iterator i = inst->user_begin(),
			e = inst->user_end(); i != e; i++) {
		Instruction *user = *i;
		constantOperands[user]++;
		if (!inWorkList[user]) {
			workList.push_back(user);
			inWorkList[user] = true;
		}
	}
}

void ConstantPropagationPass::replace_by_constant(Instruction *inst,
		CONSTInst *c, Method *M) {
	assert(inst);
	assert(c);

	LOG("replace " << inst << " by " << c << nl);
	inst->replace_value(c);
	propagate(c);
}

bool has_only_constant_operands(Instruction *inst) {
	assert(inst->op_size() > 0);
	CONSTInst *firstOp = inst->get_operand(0)->to_Instruction()->to_CONSTInst();
	for (Instruction::const_op_iterator i = inst->op_begin(),
			e = inst->op_end(); i != e; i++) {
		CONSTInst *op = (*i)->to_Instruction()->to_CONSTInst();
		bool equal;

		switch (inst->get_type()) {
			case Type::IntTypeID:
				equal = firstOp->get_Int() == op->get_Int();
				break;
			case Type::LongTypeID:
				equal = firstOp->get_Long() == op->get_Long();
				break;
			case Type::FloatTypeID:
				equal = firstOp->get_Float() == op->get_Float();
				break;
			case Type::DoubleTypeID:
				equal = firstOp->get_Double() == op->get_Double();
				break;
			default:
				assert(0);
				return false;
		}

		if (!equal)
			return false;
	}
	return true;
}

bool can_be_statically_evaluated(Instruction *inst) {
	return inst->is_arithmetic()
			|| inst->get_opcode() == Instruction::CASTInstID;
}

bool ConstantPropagationPass::run(JITData &JD) {
	Method *M = JD.get_Method();

	workList.clear();
	workList.insert(workList.begin(), M->begin(), M->end());	

	inWorkList.clear();
	for (Method::const_iterator i = workList.begin(), e = workList.end();
			i != e; i++) {
		inWorkList[*i] = true;
	}

	constantOperands.clear();

	while (!workList.empty()) {
		Instruction *I = workList.front();
		workList.pop_front();
		inWorkList[I] = false;

		if (constantOperands[I] == I->op_size()) {
			if (I->get_opcode() == Instruction::CONSTInstID) {
				propagate(I);
			} else if (can_be_statically_evaluated(I)) {
				CONSTInst *foldedInst = foldInstruction(I);
				if (foldedInst) {
					M->add_Instruction(foldedInst);
					replace_by_constant(I, foldedInst, M);
				}
			} else if (I->get_opcode() == Instruction::PHIInstID
						&& has_only_constant_operands(I)) {
				assert(I->op_size() > 0);
				CONSTInst *firstOp = I->get_operand(0)->to_Instruction()
					->to_CONSTInst();
				replace_by_constant(I, firstOp, M);
			}
		}
	}

	return true;
}

// pass usage
PassUsage& ConstantPropagationPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires<InstructionMetaPass>();
	PU.add_schedule_before<DeadcodeEliminationPass>();
	PU.add_schedule_before<GlobalValueNumberingPass>();
	return PU;
}

// the address of this variable is used to identify the pass
char ConstantPropagationPass::ID = 0;

// register pass
static PassRegistry<ConstantPropagationPass> X("ConstantPropagationPass");

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
