/* src/vm/jit/compiler2/aarch64/Aarch64Instructions.cpp

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

#include "vm/jit/compiler2/aarch64/Aarch64Instructions.hpp"
#include "vm/jit/compiler2/aarch64/Aarch64Emitter.hpp"
#include "vm/jit/compiler2/aarch64/Aarch64Register.hpp"
#include "vm/jit/compiler2/MachineInstructions.hpp"
#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/CodeMemory.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/aarch64"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace aarch64 {

template <class A,class B>
inline A* cast_to(B*);

template <class A,A>
inline A* cast_to(A* a) {
	assert(a);
	return a;
}

template <>
inline Aarch64Register* cast_to<Aarch64Register>(MachineOperand* op) {
	Register* reg = op->to_Register();
	assert(reg);
	MachineRegister *mreg = reg->to_MachineRegister();
	assert(mreg);
	NativeRegister *nreg = mreg->to_NativeRegister();
	assert(nreg);
	Aarch64Register *areg = nreg->get_Aarch64Register();
	assert(areg);
	return areg;
}

template <>
inline Immediate* cast_to<Immediate>(MachineOperand* op) {
	Immediate* imm = op->to_Immediate();
	assert(imm);
	return imm;
}

// TODO: find out how I can write a u4 to cm using existing methods
static void emitRaw(CodeMemory* cm, u4 inst) {
	CodeFragment cf = cm->get_CodeFragment(4);
	cf[0] = inst & 0xff;
	cf[1] = (inst >> 8) & 0xff;
	cf[2] = (inst >> 16) & 0xff;
	cf[3] = (inst >> 24) & 0xff;
}


void MovInst::emit(CodeMemory* cm) const {
	MachineOperand* src = operands[0].op;
	MachineOperand* dst = result.op;
	assert(dst->is_Register());
	Aarch64Register* reg_dst = cast_to<Aarch64Register>(dst);

	Emitter em;
	if (src->is_Register()) {
		Aarch64Register* reg_src = cast_to<Aarch64Register>(src);
		em.mov(X(reg_dst->index), X(reg_src->index));
	} else if (src->is_Immediate()) {
		Immediate* imm = cast_to<Immediate>(src);
		em.movz(X(reg_dst->index), (u2) imm->get_Long());
	}
	em.emit(cm);
}


void AddInst::emit(CodeMemory* cm) const {
	MachineOperand* src1 = operands[0].op;
	MachineOperand* src2 = operands[1].op;
	MachineOperand* dst = result.op;
	Aarch64Register* reg_dst = cast_to<Aarch64Register>(dst);

	Emitter em;
	if (src2->is_Register()) {
		Aarch64Register* reg_src1 = cast_to<Aarch64Register>(src1);
		Aarch64Register* reg_src2 = cast_to<Aarch64Register>(src2);
		em.add(X(reg_dst->index), X(reg_src1->index), X(reg_src2->index));
	} else {
		ABORT_MSG("aarch64: SubInst with immediate not implemented.", "");	
	}
	em.emit(cm);
}


void SubInst::emit(CodeMemory* cm) const {
	MachineOperand* src1 = operands[0].op;
	MachineOperand* src2 = operands[1].op;
	MachineOperand* dst = result.op;
	Aarch64Register* reg_dst = cast_to<Aarch64Register>(dst);

	Emitter em;
	if (src2->is_Register()) {
		Aarch64Register* reg_src1 = cast_to<Aarch64Register>(src1);
		Aarch64Register* reg_src2 = cast_to<Aarch64Register>(src2);
		em.sub(X(reg_dst->index), X(reg_src1->index), X(reg_src2->index));
	} else {
		ABORT_MSG("aarch64: SubInst with immediate not implemented.", "");	
	}
	em.emit(cm);
}


void MulInst::emit(CodeMemory* cm) const {
	MachineOperand* src1 = operands[0].op;
	MachineOperand* src2 = operands[1].op;
	MachineOperand* dst = result.op;
	Aarch64Register* reg_dst = cast_to<Aarch64Register>(dst);

	Emitter em;
	if (src2->is_Register()) {
		Aarch64Register* reg_src1 = cast_to<Aarch64Register>(src1);
		Aarch64Register* reg_src2 = cast_to<Aarch64Register>(src2);
		em.mul(X(reg_dst->index), X(reg_src1->index), X(reg_src2->index));
	} else {
		ABORT_MSG("aarch64: SubInst with immediate not implemented.", "");	
	}
	em.emit(cm);
}


void DivInst::emit(CodeMemory* cm) const {
	MachineOperand* src1 = operands[0].op;
	MachineOperand* src2 = operands[1].op;
	MachineOperand* dst = result.op;
	Aarch64Register* reg_dst = cast_to<Aarch64Register>(dst);

	Emitter em;
	if (src2->is_Register()) {
		Aarch64Register* reg_src1 = cast_to<Aarch64Register>(src1);
		Aarch64Register* reg_src2 = cast_to<Aarch64Register>(src2);
		em.sdiv(X(reg_dst->index), X(reg_src1->index), X(reg_src2->index));
	} else {
		ABORT_MSG("aarch64: SubInst with immediate not implemented.", "");	
	}
	em.emit(cm);
}


void CmpInst::emit(CodeMemory* cm) const {
	MachineOperand* src1 = operands[0].op;
	MachineOperand* src2 = operands[1].op;
	Aarch64Register* reg_src1 = cast_to<Aarch64Register>(src1);
	Aarch64Register* reg_src2 = cast_to<Aarch64Register>(src2);

	Emitter em;
	em.cmp(X(reg_src1->index), X(reg_src2->index));
	em.emit(cm);
}

void JumpInst::emit(CodeMemory* cm) const {
	MachineBasicBlock *MBB = successor_front();
	CodeSegment &CS = cm->get_CodeSegment();
	CodeSegment::IdxTy idx = CS.get_index(CSLabel(MBB));
	if (CodeSegment::is_invalid(idx)) {
		LOG2("JumpInst::emit: target not yet known (" << this << " to "
			<< *MBB << ")" << nl);
		
		// reserve memory and add to resolve later
		CodeFragment cf = cm->get_CodeFragment(4);
		cm->require_linking(this, cf);
		return;
	}
	s4 offset = cm->get_offset(idx);
	if (offset == 0) {
		LOG2("JumpInst::emit: jump to the next instruction -> can be omitted ("
			<< this << " to " << *MBB << ")" << nl);
		return;
	}
	CodeFragment cf = cm->get_CodeFragment(4);
	Emitter em;
	em.b(offset);
	em.emit(cf);
}

void JumpInst::link(CodeFragment& cf) const {
	MachineBasicBlock *mbb = successor_front();
	CodeSegment &cs = cf.get_Segment();
	CodeSegment::IdxTy idx = cs.get_index(CSLabel(mbb));
	LOG2("JumpInst::link BI: " << *mbb << " idx: " << idx.idx << " CF begin: "
		 << cf.get_begin().idx << " CF end: " << cf.get_end().idx << nl);
	s4 offset = cs.get_CodeMemory().get_offset(idx, cf);
	assert(offset != 0);

	Emitter em;
	em.b(offset);
	em.emit(cf);
}

void CondJumpInst::emit(CodeMemory* cm) const {
	// update jump target (might have changed)
	jump.set_target(successor_back());
	// emit else jump (if needed)
	jump.emit(cm);

	MachineBasicBlock *mbb = successor_front();
	CodeSegment &cs = cm->get_CodeSegment();
	CodeSegment::IdxTy idx = cs.get_index(CSLabel(mbb));
	if (CodeSegment::is_invalid(idx)) {
		LOG2("CondJumpInst::emit:  target not yet known (" << this << " to "
			 << *mbb << ")" << nl);
		// TODO: implement this
		return;
	}
	s4 offset = cm->get_offset(idx);
	if (offset == 0) {
		// XXX fix me! remove empty blocks?
		ABORT_MSG("Aarch64 ERROR","CondJump offset 0 oO!");
		//return;
	}
	LOG2("found offset of " << *mbb << ": " << offset << nl);

	Emitter em;
	em.bcond(cond.code, offset);
	em.emit(cm);
}

void CondJumpInst::link(CodeFragment& cf) const {
	assert(false);
	// TODO: implement me
}

void EnterInst::emit(CodeMemory* cm) const {
	// TODO: handle differently for leaf methods
	
	// stp x29, x30, [sp, -size]!
	u4 stp = 0xa9800000 | 0x1d | (0x1f << 5) | (0x1e << 10);
	s2 imm = - (framesize / 8);
	stp |= (imm & 0x7f) << 15;

	// mov x29, sp
	u4 mov = 0x91000000 | 0x1d | (0x1f << 5);

	emitRaw(cm, mov);
	emitRaw(cm, stp);
}

void LeaveInst::emit(CodeMemory* cm) const {
	// TODO: handle differently for leaf methods
	
	// ldp x29, x30, [sp] size
	u4 ldp = 0xa8c00000 | 0x1d | (0x1f << 5) | (0x1e << 10);
	s2 imm = (framesize / 8);
	ldp |= (imm & 0x7f) << 15;
	emitRaw(cm, ldp);
}

void RetInst::emit(CodeMemory* cm) const {
	u4 ret = 0xd65f0000 | (0x1e << 5);
	emitRaw(cm, ret);
}

} // end namespace aarch64
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
