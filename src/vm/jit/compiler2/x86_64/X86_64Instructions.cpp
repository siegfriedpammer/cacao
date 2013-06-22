/* src/vm/jit/compiler2/X86_64Instructions.hpp - X86_64Instructions

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

#include "vm/jit/compiler2/x86_64/X86_64Instructions.hpp"
#include "vm/jit/compiler2/x86_64/X86_64EmitHelper.hpp"
#include "vm/jit/compiler2/MachineInstructions.hpp"
#include "vm/jit/compiler2/CodeMemory.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/x86_64"

namespace cacao {
namespace jit {
namespace compiler2 {

void X86_64CmpInst::emit(CodeMemory* CM) const {
	#if 0
	X86_64Register *src1_reg = operands[0].op->to_Register()->to_MachineRegister()->to_NaviveRegister();
	X86_64Register *src2_reg = operands[1].op->to_Register()->to_MachineRegister()->to_NaviveRegister();

	X86_64InstructionEncoding::reg2reg<u1>(CM, 0x39, src1_reg, src2_reg);
	#endif
	MachineOperand *src1 = operands[0].op;
	MachineOperand *src2 = operands[1].op;
	Register *src1_reg = src1->to_Register();
	Register *src2_reg = src2->to_Register();
	Immediate *imm = src2->to_Immediate();
	if (src1_reg) {
		X86_64Register *src1_nreg = src1_reg->to_MachineRegister()->to_NaviveRegister();
		assert(src1_nreg);
		if (src2_reg) {
			X86_64Register *src2_nreg = src1_reg->to_MachineRegister()->to_NaviveRegister();
			assert(src2_nreg);
			X86_64InstructionEncoding::reg2reg<u1>(CM, 0x39, src1_nreg, src2_nreg);
			return;
		}
		if (imm) {
			s4 value = imm->get_value();
			if (fits_into<s1>(value)) {
				X86_64InstructionEncoding::reg2imm_modrm<u1,s1>(CM,0x83,7,src1_nreg,(u1)value);
			} else {
				X86_64InstructionEncoding::reg2imm_modrm<u1,s4>(CM,0x81,7,src1_nreg,value);
			}
			return;
		}
	}
	ABORT_MSG(this << "Operands not supported","src1: " << src1 << " src2: " << src2);
}

void X86_64EnterInst::emit(CodeMemory* CM) const {
	CodeFragment code = CM->get_CodeFragment(4);
	code[0] = 0xc8;
	code[1] = (0xff & (framesize >> 0));
	code[2] = (0xff & (framesize >> 8));
	code[3] = 0x0;
}

void X86_64LeaveInst::emit(CodeMemory* CM) const {
	CodeFragment code = CM->get_CodeFragment(1);
	code[0] = 0xc9;
}

void X86_64RetInst::emit(CodeMemory* CM) const {
	CodeFragment code = CM->get_CodeFragment(1);
	code[0] = 0xc3;
}

namespace {
/**
 * stack from/to reg
 * @param stack2reg true if we move from stack to register
 */
inline void emit_stack_move(CodeMemory* CM, StackSlot *slot,
		X86_64Register *reg, bool stack2reg) {
	s4 index = slot->get_index() * 8;
	u1 opcode = stack2reg ? 0x8b : 0x89;
	if (fits_into<s1>(index)) {
		X86_64InstructionEncoding::reg2rbp_disp8<u1>(CM, opcode, reg,
			(s1)index);
	} else {
		X86_64InstructionEncoding::reg2rbp_disp32<u1>(CM, opcode, reg,
			(s4)index);
	}
}
/**
 * reg to reg
 */
inline void emit_reg2reg(CodeMemory* CM, X86_64Register *dst,
		X86_64Register *src) {
	X86_64InstructionEncoding::reg2reg<u1>(CM, 0x89, src, dst);
}

/**
 * imm to reg
 */
inline void emit_imm2reg_move(CodeMemory* CM, Immediate *imm,
		X86_64Register *dst) {

	u1 opcode = 0xb8 + dst->get_index();
	X86_64InstructionEncoding::reg2imm<u1>(CM, opcode, dst, imm->get_value());
}

} // end anonymous namespace

void X86_64MovInst::emit(CodeMemory* CM) const {
	MachineOperand *src = operands[0].op;
	MachineOperand *dst = result.op;
	if (src == dst) return;
	Register *src_reg = src->to_Register();
	Register *dst_reg = dst->to_Register();
	if( dst_reg) {
		MachineRegister *dst_mreg = dst_reg->to_MachineRegister();
		assert(dst_mreg);
		if (src_reg) {
			MachineRegister *src_mreg = src_reg->to_MachineRegister();
			assert(src_mreg);

			// reg to reg move
			emit_reg2reg(CM,dst_mreg->to_NaviveRegister(),
				src_mreg->to_NaviveRegister());
			return;
		}
		StackSlot *slot = src->to_StackSlot();
		if (slot) {
			// stack to reg move
			emit_stack_move(CM,slot,dst_mreg->to_NaviveRegister(),true);
			return;
		}
		Immediate *imm = src->to_Immediate();
		if (imm) {
			// im to reg move
			emit_imm2reg_move(CM,imm,dst_mreg->to_NaviveRegister());
			return;
		}
	} else {
		// dst is not a reg
		if (src_reg) {
			MachineRegister *src_mreg = src_reg->to_MachineRegister();
			assert(src_mreg);
			StackSlot *slot = dst->to_StackSlot();
			if (slot) {
				// reg to stack move
				emit_stack_move(CM,slot,src_mreg->to_NaviveRegister(),false);
				return;
			}
		}
	}
	ABORT_MSG("x86_64 TODO","non reg-to-reg moves not yet implemented (src:"
		<< src << " dst:" << dst << ")");
}

void X86_64CondJumpInst::emit(CodeMemory* CM) const {
	BeginInst *BI = get_BeginInst();
	s4 offset = CM->get_offset(BI);
	switch (offset) {
	case 0:
		ABORT_MSG("x86_64 ERROR","CondJump offset 0 oO!");
	case CodeMemory::INVALID_OFFSET:
		LOG2("X86_64CondJumpInst: target not yet known (" << this << " to "
		     << BI << ")"  << nl);
		// reserve memory and add to resolve later
		// worst case -> 32bit offset
		CodeFragment CF = CM->get_CodeFragment(5);
		// FIXME pass this the resolve me
		CM->resolve_later(BI,this,CF);
		return;
	#if 0
	default:
		// create jump
	#endif
	}
	LOG2("found offset of " << BI << ": " << offset << nl);

	// only 32bit offset for the time being
	X86_64InstructionEncoding::imm_op<u2>(CM, 0x0f80 + cond.code, offset);
}
void X86_64IMulInst::emit(CodeMemory* CM) const {
	X86_64Register *src_reg = operands[1].op->to_Register()->to_MachineRegister()->to_NaviveRegister();
	X86_64Register *dst_reg = result.op->to_Register()->to_MachineRegister()->to_NaviveRegister();

	X86_64InstructionEncoding::reg2reg<u2>(CM, 0x0faf, dst_reg, src_reg);
}

void X86_64AddInst::emit(CodeMemory* CM) const {
	X86_64Register *src_reg = operands[1].op->to_Register()->to_MachineRegister()->to_NaviveRegister();
	X86_64Register *dst_reg = result.op->to_Register()->to_MachineRegister()->to_NaviveRegister();

	X86_64InstructionEncoding::reg2reg<u1>(CM, 0x03, src_reg, dst_reg);
}
void X86_64SubInst::emit(CodeMemory* CM) const {
	MachineOperand *src = operands[1].op;
	MachineOperand *dst = result.op;
	Register *src_reg = src->to_Register();
	Register *dst_reg = dst->to_Register();
	Immediate *imm = src->to_Immediate();
	if (dst_reg) {
		X86_64Register *dst_nreg = dst_reg->to_MachineRegister()->to_NaviveRegister();
		assert(dst_nreg);
		if (src_reg) {
			X86_64Register *src_nreg = src_reg->to_MachineRegister()->to_NaviveRegister();
			assert(src_nreg);
			X86_64InstructionEncoding::reg2reg<u1>(CM, 0x29, src_nreg, dst_nreg);
			return;
		}
		if (imm) {
			s4 value = imm->get_value();
			X86_64InstructionEncoding::reg2imm_modrm<u1,s4>(CM,0x81,5,dst_nreg,value);
			return;
		}
	}
	ABORT_MSG(this << "Operands not supported","src: " << src << " dst: " << dst);
}

namespace {
void emit_jump(CodeFragment &code, s4 offset) {
	LOG2("emit_jump codefragment offset: " << hex << offset << nl);
	code[0] = 0xe9;
	code[1] = u1( 0xff & (offset >> (8 * 0)));
	code[2] = u1( 0xff & (offset >> (8 * 1)));
	code[3] = u1( 0xff & (offset >> (8 * 2)));
	code[4] = u1( 0xff & (offset >> (8 * 3)));
}

} // end anonymous namespace
void X86_64JumpInst::emit(CodeMemory* CM) const {
	BeginInst *BI = get_BeginInst();
	s4 offset = CM->get_offset(BI);
	switch (offset) {
	case 0:
		LOG2("emit_Jump: jump to the next instruction -> can be omitted ("
		     << this << " to " << BI << ")"  << nl);
		return;
	case CodeMemory::INVALID_OFFSET:
		LOG2("emit_Jump: target not yet known (" << this << " to "
		     << BI << ")"  << nl);
		// reserve memory and add to resolve later
		// worst case -> 32bit offset
		CodeFragment CF = CM->get_CodeFragment(5);
		// FIXME pass this the resolve me
		CM->resolve_later(BI,this,CF);
		return;
	}
	CodeFragment CF = CM->get_CodeFragment(5);
	emit_jump(CF,offset);
}

void X86_64JumpInst::emit(CodeFragment &CF) const {
	BeginInst *BI = get_BeginInst();
	s4 offset = CF.get_offset(BI);
	assert(offset != 0);
	assert(offset != CodeMemory::INVALID_OFFSET);

	emit_jump(CF,offset);
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
