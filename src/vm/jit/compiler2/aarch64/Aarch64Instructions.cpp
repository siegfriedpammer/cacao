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


// TODO: find out how I can write a u4 to cm using existing methods
static void emitRaw(CodeMemory* cm, u4 inst) {
	CodeFragment cf = cm->get_CodeFragment(4);
	cf[0] = inst & 0xff;
	cf[1] = (inst >> 8) & 0xff;
	cf[2] = (inst >> 16) & 0xff;
	cf[3] = (inst >> 24) & 0xff;
}

void LoadInst::emit(Emitter& em) const {
	s4 off = this->offset();
	Reg dst = this->reg_res();
	Reg base = this->reg_base();

	if (off < 0) {
		em.ldur(dst, base, off);
	} else {
		em.ldr(dst, base, off);
	}
}


void StoreInst::emit(Emitter& em) const {
	s4 off = offset();
	Reg src = reg_src();
	Reg base = reg_base();
	if (off < 0) {
		em.stur(src, base, off);
	} else {
		em.str(src, base, off);
	}
}


void MovInst::emit(Emitter& em) const {
	if (reg_res().r() == reg_op(0).r()) return;

	em.mov(reg_res(), reg_op(0));
}


void MovImmInst::emit(Emitter& em) const {
	if (resultT() == Type::IntTypeID) emitIConst(em);
	else emitLConst(em);
}

void MovImmInst::emitIConst(Emitter& em) const {
    Immediate* immediate = cast_to<Immediate>(this->get(0).op);
	s4 value = immediate->get_value<s4>();
	Reg reg = this->reg_res();

	// For small negative immediates, use MOVN
	if (value < 0 && -value-1 < 0xffff) {
		em.movn(reg, -value - 1);
		return;
	}

	em.movz(reg, value & 0xffff);
	
	u4 v = (u4) value;
	if (v > 0xffff) {
		u2 imm = (value >> 16) & 0xffff;
		em.movk(reg, imm, 1);
	}
}


void MovImmInst::emitLConst(Emitter& em) const {
    Immediate* immediate = cast_to<Immediate>(this->get(0).op);
	s8 value = immediate->get_value<s8>();
	Reg reg = this->reg_res();

	// For small negative immediates, use MOVN
	if (value < 0 && -value-1 < 0xffff) {
		em.movn(reg, -value - 1);
		return;
	}

	em.movz(reg, value & 0xffff);

	u8 v = (u8) value;
	if (v > 0xffff) {
		u2 imm = (value >> 16) & 0xffff;
		em.movk(reg, imm, 1);
	}

	if (v > 0xffffffff) {
		u2 imm = (value >> 32) & 0xffff;
		em.movk(reg, imm, 2);
	}

	if (v > 0xffffffffffff) {
		u2 imm = (value >> 48) & 0xffff;
		em.movk(reg, imm, 3);
	}
}

void CSelInst::emit(Emitter& em) const {
	em.csel(reg_res(), reg_op(0), reg_op(1), cond);
}

void AndInst::emit(Emitter& em) const {
	em.andd(reg_res(), reg_op(0), reg_op(1));
}

void CmpInst::emit(Emitter& em) const {
	Reg reg0 = this->reg_op(0);

	if (this->get(1).op->is_Register()) {
		em.cmp(reg0, this->reg_op(1));
	} else if (this->get(1).op->is_Immediate()) {
		Immediate *imm = cast_to<Immediate>(this->get(1).op);
		em.cmp(reg0, imm->get_Int());
	}
}

void MulInst::emit(Emitter& em) const {
	Reg res = reg_res();
	em.mul(res, reg_op(0), reg_op(1));

	if (resultT() == Type::IntTypeID) {
		em.ubfx(res, res);
	}
}

void AddInst::emit(Emitter& em) const {
	if (get(0).op->is_Register() && get(1).op->is_Register()) {
		em.add(reg_res(), reg_op(0), reg_op(1), shift, amount);
	} else {
		ABORT_MSG("AddInst<T>::emit", "Operand not in register.");
	}
}

void SubInst::emit(Emitter& em) const {
	em.sub(reg_res(), reg_op(0), reg_op(1));
}

void NegInst::emit(Emitter& em) const { em.neg(reg_res(), reg_op(0)); }

void DivInst::emit(Emitter& em) const {
	em.sdiv(reg_res(), reg_op(0), reg_op(1));
}

void MulSubInst::emit(Emitter& em) const {
	em.msub(reg_res(), reg_op(0), reg_op(1), reg_op(2));
}

void FAddInst::emit(Emitter& em) const { 
	em.fadd(reg_res(), reg_op(0), reg_op(1));
}

void FSubInst::emit(Emitter& em) const {
	em.fsub(reg_res(), reg_op(0), reg_op(1));
}

void FNegInst::emit(Emitter& em) const { em.fneg(reg_res(), reg_op(0)); }

void FMulInst::emit(Emitter& em) const {
	em.fmul(reg_res(), reg_op(0), reg_op(1));
}

void FDivInst::emit(Emitter& em) const { 
	em.fdiv(reg_res(), reg_op(0), reg_op(1));
}

void FCmpInst::emit(Emitter& em) const {
	em.fcmp(reg_op(0), reg_op(1));
}

void FMovInst::emit(Emitter& em) const {
	if (reg_res().r() == reg_op(0).r()) return;

	em.fmov(reg_res(), reg_op(0));
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
	Emitter em;

	// stp x29, x30, [sp, -16]!
	u4 stp = 0xa9800000 | 0x1d | (0x1f << 5) | (0x1e << 10);
	s2 imm = - 2;
	stp |= (imm & 0x7f) << 15;
	em.emitRaw(stp);

	// mov x29, sp
	u4 mov = 0x91000000 | 0x1d | (0x1f << 5);
	em.emitRaw(mov);

	// sub sp, sp, size
	if (framesize - 16 > 0)
		em.sub(Reg::XSP, Reg::XSP, framesize - 16);
	
	em.emit(cm);
}

void LeaveInst::emit(CodeMemory* cm) const {
	// TODO: handle differently for leaf methods
	Emitter em;

	// mov sp, x29
	em.add(Reg::XSP, Reg::XFP, 0);

	// ldp x29, x30, [sp] 16
	u4 ldp = 0xa8c00000 | 0x1d | (0x1f << 5) | (0x1e << 10);
	s2 imm = 2;
	ldp |= (imm & 0x7f) << 15;
	em.emitRaw(ldp);

	em.emit(cm);
}

void RetInst::emit(CodeMemory* cm) const {
	u4 ret = 0xd65f0000 | (0x1e << 5);
	emitRaw(cm, ret);
}

void FcvtInst::emit(Emitter& em) const {
	em.fcvt(reg_res(), reg_from());
}

void IntToFpInst::emit(Emitter& em) const {
	em.scvtf(reg_res(), reg_from());
}

void IntToLongInst::emit(Emitter& em) const {
	em.sxtw(reg_res(), reg_op(0));
}

void IntToCharInst::emit(Emitter& em) const {
	em.uxth(reg_res(), reg_op(0));
}

void IntegerToByteInst::emit(Emitter& em) const {
	em.sxtb(reg_res(), reg_op(0));
}

void IntegerToShortInst::emit(Emitter& em) const {
	em.sxth(reg_res(), reg_op(0));
}

void PatchInst::emit(CodeMemory* CM) const {
	UNIMPLEMENTED_MSG("aarch64: PatchInst::emit");
	#if 0
	CodeFragment code = CM->get_aligned_CodeFragment(2);
	code[0] = 0x0f;
	code[1] = 0x0b;
	emit_nop(code + 2, code.size() - 2);
	CM->require_linking(this,code);
	#endif
}

void PatchInst::link(CodeFragment &CF) const {
	UNIMPLEMENTED_MSG("aarch64: PatchInst::emit");
	#if 0
	CodeMemory &CM = CF.get_Segment().get_CodeMemory();
	patcher->reposition(CM.get_offset(CF.get_begin()));
	LOG2(this << " link: reposition: " << patcher->get_mpc() << nl);
	#endif
}

void DsegAddrInst::emit(CodeMemory* cm) const {
	// Load DSeg address
	CodeFragment code = cm->get_CodeFragment(4);
	cm->require_linking(this, code);
}

void DsegAddrInst::link(CodeFragment &cf) const {
	CodeMemory &cm = cf.get_Segment().get_CodeMemory();
	s4 offset = cm.get_offset(data_index, cf);
	#if 0
	LOG2(this << " offset: " << offset << " data index: " << data_index.idx
	     << " CF end index: " << cf.get_end().idx << nl);
	LOG2("dseg->size " << cm.get_DataSegment().size() << " cseg->size "
		 << cm.get_CodeSegment().size() << nl);
	#endif
	assert(offset != 0);

	Emitter em;
	em.ldr(reg_res(), offset);
	em.emit(cf);
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
