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
	X86_64Register *src1_reg = operands[0].op->to_Register()->to_MachineRegister()->to_NaviveRegister();
	X86_64Register *src2_reg = operands[1].op->to_Register()->to_MachineRegister()->to_NaviveRegister();

	X86_64InstructionEncoding::reg2reg<u1>(CM, 0x39, src1_reg, src2_reg);
}
void X86_64RetInst::emit(CodeMemory* CM) const {
	CodeFragment code = CM->get_CodeFragment(1);
	code[0] = 0xc3;
}

void X86_64MovInst::emit(CodeMemory* CM) const {
	X86_64Register *src_reg = operands[0].op->to_Register()
		->to_MachineRegister()->to_NaviveRegister();
	X86_64Register *dst_reg = result.op->to_Register()
		->to_MachineRegister()->to_NaviveRegister();

	X86_64InstructionEncoding::reg2reg<u1>(CM, 0x89, src_reg, dst_reg);
}

namespace {
/**
 * @param stack2reg true if we move from stack to register
 */
inline void emit_stack_move(CodeMemory* CM, StackSlot *slot,
		X86_64Register *reg, bool stack2reg) {
	s4 index = slot->get_index();
	u1 opcode = stack2reg ? 0x8b : 0x89;
	if (fits_into<s1>(index)) {
		X86_64InstructionEncoding::reg2rbp_disp8<u1>(CM, opcode, reg,
			(s1)index);
	} else {
		X86_64InstructionEncoding::reg2rbp_disp32<u1>(CM, opcode, reg,
			(s4)index);
	}
}

} // end anonymous namespace

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
	X86_64InstructionEncoding::imm<u2>(CM, 0x0f80 + cond.code, offset);
}
void X86_64MovStack2RegInst::emit(CodeMemory* CM) const {
	StackSlot *slot = operands[0].op->to_StackSlot();
	X86_64Register *dst_reg = result.op->to_Register()
		->to_MachineRegister()->to_NaviveRegister();

	emit_stack_move(CM,slot,dst_reg,true);
}

void X86_64MovImm2RegInst::emit(CodeMemory* CM) const {
	Immediate *imm = operands[0].op->to_Immediate();
	X86_64Register *dst_reg = result.op->to_Register()
		->to_MachineRegister()->to_NaviveRegister();

	u1 opcode = 0xb8 + dst_reg->get_index();
	X86_64InstructionEncoding::reg2imm<u1>(CM, opcode, dst_reg, imm->get_value());
}

void X86_64MovReg2StackInst::emit(CodeMemory* CM) const {
	StackSlot *slot = result.op->to_StackSlot();
	X86_64Register *src_reg = operands[0].op->to_Register()
		->to_MachineRegister()->to_NaviveRegister();

	emit_stack_move(CM,slot,src_reg,false);
}

void X86_64IMulInst::emit(CodeMemory* CM) const {
	X86_64Register *src_reg = operands[1].op->to_Register()->to_MachineRegister()->to_NaviveRegister();
	X86_64Register *dst_reg = result.op->to_Register()->to_MachineRegister()->to_NaviveRegister();

	X86_64InstructionEncoding::reg2reg<u2>(CM, 0x0faf, src_reg, dst_reg);
}

void X86_64SubInst::emit(CodeMemory* CM) const {
	X86_64Register *src_reg = operands[1].op->to_Register()->to_MachineRegister()->to_NaviveRegister();
	X86_64Register *dst_reg = result.op->to_Register()->to_MachineRegister()->to_NaviveRegister();

	X86_64InstructionEncoding::reg2reg<u1>(CM, 0x29, src_reg, dst_reg);
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
