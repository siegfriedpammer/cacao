/* src/vm/jit/compiler2/aachr64/Aarch64Backend.cpp

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

#include "vm/jit/compiler2/aarch64/Aarch64.hpp"
#include "vm/jit/compiler2/aarch64/Aarch64Backend.hpp"
// #include "vm/jit/compiler2/x86_64/X86_64Instructions.hpp"
// #include "vm/jit/compiler2/x86_64/X86_64Register.hpp"
// #include "vm/jit/compiler2/x86_64/X86_64MachineMethodDescriptor.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MethodDescriptor.hpp"
#include "vm/jit/compiler2/CodeMemory.hpp"
#include "vm/jit/compiler2/StackSlotManager.hpp"
#include "vm/jit/compiler2/MatcherDefs.hpp"
#include "vm/jit/PatcherNew.hpp"
#include "vm/jit/jit.hpp"
#include "vm/jit/code.hpp"
#include "vm/class.hpp"
#include "vm/field.hpp"

#include "md-trap.hpp"

#include "toolbox/OStream.hpp"
#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/aarch64"

namespace cacao {
namespace jit {
namespace compiler2 {

// BackendBase must be specialized in namespace compiler2!
using namespace aarch64;

template<>
const char* BackendBase<Aarch64>::get_name() const {
	return "aarch64";
}

template<>
MachineInstruction* BackendBase<Aarch64>::create_Move(MachineOperand *src,
		MachineOperand* dst) const {
	Type::TypeID type = dst->get_type();
	ABORT_MSG("aarch64: Move not implemented",
		"Inst: " << src << " -> " << dst << " type: " << type);
	return NULL;
}

template<>
MachineInstruction* BackendBase<Aarch64>::create_Jump(MachineBasicBlock *target) const {
	ABORT_MSG("aarch64: Jump not implemented", "");
	return NULL;
}

namespace {

template <unsigned size, class T>
inline T align_to(T val) {
	T rem =(val % size);
	return val + ( rem == 0 ? 0 : size - rem);
}

} // end anonymous namespace

template<>
void BackendBase<Aarch64>::create_frame(CodeMemory* CM, StackSlotManager *SSM) const {
	ABORT_MSG("aarch64: Create frame not implemented", "");
}

void Aarch64LoweringVisitor::visit(LOADInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64 type not supported: ", I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(CMPInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported", 
			"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(IFInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
			"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(NEGInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
			"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(ADDInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
			"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(ANDInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
			"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(SUBInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
			"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(MULInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
			"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(DIVInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
			"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(REMInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
			"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(ALOADInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
			"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(ASTOREInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
			"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(ARRAYLENGTHInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
			"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(ARRAYBOUNDSCHECKInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
			"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(RETURNInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
		"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(CASTInst *I, bool copyOperands) {
	Type::TypeID from = I->get_operand(0)->to_Instruction()->get_type();
	Type::TypeID to = I->get_type();
	ABORT_MSG("aarch64: Cast not supported!", "From " << from << " to " << to );
}

void Aarch64LoweringVisitor::visit(INVOKESTATICInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
		"Inst: " << I << " type: " << type);
}


namespace {

inline bool is_floatingpoint(Type::TypeID type) {
	return type == Type::DoubleTypeID || type == Type::FloatTypeID;
}
template <class I,class Seg>
static void write_data(Seg seg, I data) {
	assert(seg.size() == sizeof(I));

	for (int i = 0, e = sizeof(I) ; i < e ; ++i) {
		seg[i] = (u1) 0xff & *(reinterpret_cast<u1*>(&data) + i);
	}

}
} // end anonymous namespace


void Aarch64LoweringVisitor::visit(GETSTATICInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
		"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(LOOKUPSWITCHInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
		"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(TABLESWITCHInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
		"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::lowerComplex(Instruction* I, int ruleId){
	ABORT_MSG("Rule not supported", "Rule " 
			<< ruleId << " is not supported by method lowerComplex!");
}

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
