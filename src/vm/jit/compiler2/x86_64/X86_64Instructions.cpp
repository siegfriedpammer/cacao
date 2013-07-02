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
namespace x86_64 {

namespace {

template <class A,class B>
inline A* cast_to(B*);

template <class A,A>
inline A* cast_to(A* a) {
	assert(a);
	return a;
}

template <>
inline X86_64Register* cast_to<X86_64Register>(MachineOperand *op) {
	Register *reg = op->to_Register();
	assert(reg);
	MachineRegister *mreg = reg->to_MachineRegister();
	assert(mreg);
	NativeRegister *nreg = mreg->to_NativeRegister();
	assert(nreg);
	X86_64Register *xreg = nreg->get_X86_64Register();
	assert(xreg);
	return xreg;
}

template <>
inline X86_64Register* cast_to<X86_64Register>(X86_64Register *reg) {
	assert(reg);
	return reg;
}

template <>
inline StackSlot* cast_to<StackSlot>(MachineOperand *op) {
	switch (op->get_OperandID()) {
	case MachineOperand::ManagedStackSlotID:
		{
			ManagedStackSlot *mslot = op->to_ManagedStackSlot();
			assert(mslot);
			StackSlot *slot = mslot->to_StackSlot();
			assert(slot);
			return slot;
		}
	case MachineOperand::StackSlotID:
		{
			StackSlot *slot = op->to_StackSlot();
			assert(slot);
			return slot;
		}
	default: break;
	}
	assert(0 && "Not a stackslot");
	return NULL;
}
template <>
inline X86_64Register* cast_to<X86_64Register>(MachineRegister *mreg) {
	assert(mreg);
	NativeRegister *nreg = mreg->to_NativeRegister();
	assert(nreg);
	X86_64Register *xreg = nreg->get_X86_64Register();
	assert(xreg);
	return xreg;
}

template <>
inline Immediate* cast_to<Immediate>(MachineOperand *op) {
	Immediate* imm = op->to_Immediate();
	assert(imm);
	return imm;
}

} // end anonymous namespace

GPInstruction::OperandSize get_OperandSize_from_Type(const Type::TypeID type) {
	switch (type) {
	case Type::ByteTypeID:   return GPInstruction::OS_8;
	case Type::IntTypeID:    return GPInstruction::OS_32;
	case Type::LongTypeID:   return GPInstruction::OS_64;
	case Type::FloatTypeID:  return GPInstruction::OS_32;
	case Type::DoubleTypeID: return GPInstruction::OS_64;
	default: break;
	}
	ABORT_MSG("Type Not supported!","get_OperandSize_from_Type: " << type);
	return GPInstruction::NO_SIZE;
}

#if 0
void ALUInstruction::emit_impl_RI(CodeMemory* CM,
		X86_64Register* src1, Immediate* imm) const {
	if (fits_into<s1>(imm->get_value())) {
		// switch register width
		// 64
	}
	assert(0);
}
#endif

GPInstruction::OpEncoding get_OpEncoding(MachineOperand *src1,
		MachineOperand *src2, GPInstruction::OperandSize op_size) {

	switch (src1->get_OperandID()) {
	case MachineOperand::RegisterID:
	{
		//X86_64Register *reg1 = cast_to<X86_64Register>(src1);

		// switch second operand
		switch (src2->get_OperandID()) {
		case MachineOperand::RegisterID:
		{
			switch (op_size) {
			case GPInstruction::OS_8 : return GPInstruction::RegReg8;
			case GPInstruction::OS_16: return GPInstruction::RegReg16;
			case GPInstruction::OS_32: return GPInstruction::RegReg32;
			case GPInstruction::OS_64: return GPInstruction::RegReg64;
			default: break;
			}
		}
		case MachineOperand::ImmediateID:
		{
			Immediate *imm = cast_to<Immediate>(src2);
			if (fits_into<s1>(imm->get_value())) {
				// 8bit
				switch (op_size) {
				case GPInstruction::OS_8 : return GPInstruction::Reg8Imm8;
				case GPInstruction::OS_16: return GPInstruction::Reg16Imm8;
				case GPInstruction::OS_32: return GPInstruction::Reg32Imm8;
				case GPInstruction::OS_64: return GPInstruction::Reg64Imm8;
				default: break;
				}
			}
			switch (op_size) {
			case GPInstruction::OS_8 : // can this happen?
			case GPInstruction::OS_16: return GPInstruction::Reg16Imm16;
			case GPInstruction::OS_32: return GPInstruction::Reg32Imm32;
			case GPInstruction::OS_64: return GPInstruction::Reg64Imm64;
			default: break;
			}
		}
		case MachineOperand::StackSlotID:
		case MachineOperand::ManagedStackSlotID:
		{
			switch (op_size) {
			case GPInstruction::OS_8 : return GPInstruction::RegMem8;
			case GPInstruction::OS_16: return GPInstruction::RegMem16;
			case GPInstruction::OS_32: return GPInstruction::RegMem32;
			case GPInstruction::OS_64: return GPInstruction::RegMem64;
			default: break;
			}
		}
		default: break;
		}
	}
	case MachineOperand::ManagedStackSlotID:
	{
		// switch second operand
		switch (src2->get_OperandID()) {
		case MachineOperand::RegisterID:
		{
			switch (op_size) {
			case GPInstruction::OS_8 : return GPInstruction::MemReg8;
			case GPInstruction::OS_16: return GPInstruction::MemReg16;
			case GPInstruction::OS_32: return GPInstruction::MemReg32;
			case GPInstruction::OS_64: return GPInstruction::MemReg64;
			default: break;
			}
		}
		default: break;
		}
	}
	default: break;
	}
	return GPInstruction::NO_ENCODING;
}

void ALUInstruction::emit(CodeMemory* CM) const {
	MachineOperand *src1 = get(0).op;
	MachineOperand *src2 = get(1).op;

	switch (get_OpEncoding(src1,src2,get_op_size())) {
	case GPInstruction::RegReg64:
	{
		X86_64Register *reg1 = cast_to<X86_64Register>(src1);
		X86_64Register *reg2 = cast_to<X86_64Register>(src2);

		CodeFragment code = CM->get_CodeFragment(3);
		code[0] = get_rex(reg1,reg2);
		code[1] = (alu_id * 0x8) + 3;
		code[2] = get_modrm_reg2reg(reg1,reg2);
		return;
	}
	case GPInstruction::RegReg32:
	{
		X86_64Register *reg1 = cast_to<X86_64Register>(src1);
		X86_64Register *reg2 = cast_to<X86_64Register>(src2);

		CodeFragment code = CM->get_CodeFragment(2);
		code[0] = (alu_id * 0x8) + 3;
		code[1] = get_modrm_reg2reg(reg1,reg2);
		return;
	}
	case GPInstruction::Reg64Imm8:
	{
		X86_64Register *reg1 = cast_to<X86_64Register>(src1);
		Immediate *imm = cast_to<Immediate>(src2);
		CodeFragment code = CM->get_CodeFragment(4);
		code[0] = get_rex(reg1);
		code[1] = 0x83;
		code[2] = get_modrm_1reg(alu_id, reg1);
		code[3] = (s1)imm->get_value();
		return;
	}
	//case GPInstruction::Reg64Imm64: break;
	default: break;
	}

	ABORT_MSG(this << ": Operand(s) not supported",
		"src1: " << src1 << " src2: " << src2 << " op_size: "
		<< get_op_size() * 8 << "bit");
}

void EnterInst::emit(CodeMemory* CM) const {
	CodeFragment code = CM->get_CodeFragment(4);
	code[0] = 0xc8;
	code[1] = (0xff & (framesize >> 0));
	code[2] = (0xff & (framesize >> 8));
	code[3] = 0x0;
}

void LeaveInst::emit(CodeMemory* CM) const {
	CodeFragment code = CM->get_CodeFragment(1);
	code[0] = 0xc9;
}

void RetInst::emit(CodeMemory* CM) const {
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
		InstructionEncoding::reg2rbp_disp8<u1>(CM, opcode, reg,
			(s1)index);
	} else {
		InstructionEncoding::reg2rbp_disp32<u1>(CM, opcode, reg,
			(s4)index);
	}
}
/**
 * reg to reg
 */
inline void emit_reg2reg(CodeMemory* CM, X86_64Register *dst,
		X86_64Register *src) {
	InstructionEncoding::reg2reg<u1>(CM, 0x89, src, dst);
}

/**
 * imm to reg
 */
inline void emit_imm2reg_move(CodeMemory* CM, Immediate *imm,
		X86_64Register *dst) {

	u1 opcode = 0xb8 + dst->get_index();
	InstructionEncoding::reg2imm<u1>(CM, opcode, dst, imm->get_value());
}

} // end anonymous namespace

void MovInst::emit(CodeMemory* CM) const {
	MachineOperand *src = operands[0].op;
	MachineOperand *dst = result.op;

	switch (get_OpEncoding(dst,src,get_op_size())) {
	case GPInstruction::RegReg64:
	{
		X86_64Register *reg_dst = cast_to<X86_64Register>(dst);
		X86_64Register *reg_src = cast_to<X86_64Register>(src);
		if (reg_dst == reg_src) return;

		CodeFragment code = CM->get_CodeFragment(3);
		code[0] = get_rex(reg_dst,reg_src);
		code[1] = 0x8b;
		code[2] = get_modrm_reg2reg(reg_dst,reg_src);
		return;
	}
	case GPInstruction::RegReg32:
	{
		X86_64Register *reg_dst = cast_to<X86_64Register>(dst);
		X86_64Register *reg_src = cast_to<X86_64Register>(src);
		if (reg_dst == reg_src) return;

		CodeFragment code = CM->get_CodeFragment(2);
		code[0] = 0x8b;
		code[1] = get_modrm_reg2reg(reg_dst,reg_src);
		return;
	}
	case GPInstruction::RegMem64:
	{
		StackSlot *slot = cast_to<StackSlot>(src);
		X86_64Register *reg_dst = cast_to<X86_64Register>(dst);
		emit_stack_move(CM,slot,cast_to<X86_64Register>(reg_dst),true);
		return;
	}
	case GPInstruction::MemReg64:
	{
		X86_64Register *reg_src = cast_to<X86_64Register>(src);
		StackSlot *slot = cast_to<StackSlot>(dst);
		emit_stack_move(CM,slot,cast_to<X86_64Register>(reg_src),false);
		return;
	}
	case GPInstruction::Reg64Imm8:
	case GPInstruction::Reg64Imm64:
	{
		Immediate *imm = cast_to<Immediate>(src);
		X86_64Register *reg_dst = cast_to<X86_64Register>(dst);

		u1 opcode = 0xb8 + reg_dst->get_index();
		InstructionEncoding::reg2imm<u1,s8>(CM, opcode, reg_dst, imm->get_value());
		return;
	}
	case GPInstruction::Reg32Imm8:
	case GPInstruction::Reg32Imm32:
	{
		s4 imm = cast_to<Immediate>(src)->get_value();
		X86_64Register *reg_dst = cast_to<X86_64Register>(dst);

		CodeFragment code = CM->get_CodeFragment(5);
		code[0] = 0xb8 + reg_dst->get_index();
		code[1] = (u1) 0xff & (imm >> 0x00);
		code[2] = (u1) 0xff & (imm >> 0x08);
		code[3] = (u1) 0xff & (imm >> 0x10);
		code[4] = (u1) 0xff & (imm >> 0x18);
		return;
	}
	#if 0
	case GPInstruction::Reg64Imm8:
	{
		X86_64Register *reg_dst = cast_to<X86_64Register>(dst);
		Immediate *imm = cast_to<Immediate>(src);
		CodeFragment code = CM->get_CodeFragment(4);
		code[0] = get_rex(reg_dst);
		code[1] = 0x83;
		code[2] = get_modrm_1reg(alu_id, reg_dst);
		code[3] = (s1)imm->get_value();
		return;
	}
	#endif
	//case GPInstruction::Reg64Imm64: break;
	default: break;
	}

	ABORT_MSG(this << ": Operand(s) not supported",
		"dst: " << dst << " src: " << src << " op_size: "
		<< get_op_size() * 8 << "bit");






#if 0



	if( dst_reg) {
		MachineRegister *dst_mreg = dst_reg->to_MachineRegister();
		assert(dst_mreg);
		if (src_reg) {
			MachineRegister *src_mreg = src_reg->to_MachineRegister();
			assert(src_mreg);

			// reg to reg move
			emit_reg2reg(CM,cast_to<X86_64Register>(dst_mreg),
				cast_to<X86_64Register>(src_mreg));
			return;
		}
		StackSlot *slot = src->to_StackSlot();
		if (slot) {
			// stack to reg move
			emit_stack_move(CM,slot,cast_to<X86_64Register>(dst_mreg),true);
			return;
		}
		Immediate *imm = src->to_Immediate();
		if (imm) {
			// im to reg move
			emit_imm2reg_move(CM,imm,cast_to<X86_64Register>(dst_mreg));
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
				emit_stack_move(CM,slot,cast_to<X86_64Register>(src_mreg),false);
				return;
			}
		}
	}
	ABORT_MSG("x86_64 TODO","non reg-to-reg moves not yet implemented (src:"
		<< src << " dst:" << dst << ")");
#endif
}

void MovSXInst::emit(CodeMemory* CM) const {
	MachineOperand *src = operands[0].op;
	MachineOperand *dst = result.op;
	X86_64Register *src_reg = cast_to<X86_64Register>(src);
	X86_64Register *dst_reg = cast_to<X86_64Register>(dst);

	switch (from) {
	case GPInstruction::OS_8: break;
	case GPInstruction::OS_16: break;
	case GPInstruction::OS_32:
		switch (to) {
		case GPInstruction::OS_64:
		{
			// r32 -> r64
			CodeFragment code = CM->get_CodeFragment(3);
			code[0] = get_rex(dst_reg,src_reg);
			code[1] = 0x63;
			code[2] = get_modrm_reg2reg(dst_reg,src_reg);
			return;
		}
		default: break;
		}
		break;
	default: break;
	}

	ABORT_MSG("x86_64 sext cast not supported","from " << from << "bits to "
		<< to << "bits");
}
void CondJumpInst::emit(CodeMemory* CM) const {
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
	InstructionEncoding::imm_op<u2>(CM, 0x0f80 + cond.code, offset);
}
void IMulInst::emit(CodeMemory* CM) const {
	X86_64Register *src_reg = cast_to<X86_64Register>(operands[1].op);
	X86_64Register *dst_reg = cast_to<X86_64Register>(result.op);

	InstructionEncoding::reg2reg<u2>(CM, 0x0faf, dst_reg, src_reg);
}

#if 0
/**
 * @todo create custom wrapper classes for the opcodes
 */
void emit_arithmetic_with_imm(CodeMemory* CM, const MachineInstruction *MI,
		u1 opcode_reg_reg, 	u1 opcode_reg_imm8, u1 opcode_reg_imm64,
		u1 opcode_modrm) {
	MachineOperand *src1 = MI->get(0).op;
	MachineOperand *src2 = MI->get(1).op;
	Register *src1_reg = src1->to_Register();
	Register *src2_reg = src2->to_Register();
	Immediate *imm = src2->to_Immediate();
	if (src1_reg) {
		NativeRegister *src1_nreg = src1_reg->to_MachineRegister()
			->to_NativeRegister();
		assert(src1_nreg);
		if (src2_reg) {
			NativeRegister *src2_nreg = src2_reg->to_MachineRegister()
				->to_NativeRegister();
			assert(src2_nreg);
			InstructionEncoding::reg2reg<u1>(CM,
				opcode_reg_reg, src2_nreg, src1_nreg);
			return;
		}
		if (imm) {
			s4 value = imm->get_value();
			if (fits_into<s1>(value)) {
				InstructionEncoding::reg2imm_modrm<u1,s1>(CM,
					opcode_reg_imm8,opcode_modrm,src1_nreg,(u1)value);
			} else {
				InstructionEncoding::reg2imm_modrm<u1,s4>(CM,
					opcode_reg_imm64,opcode_modrm,src1_nreg,value);
			}
			return;
		}
	}
	ABORT_MSG(MI << "Operands not supported","src1: " << src1 << " src2: " << src2);
}
void CmpInst::emit(CodeMemory* CM) const {
	emit_arithmetic_with_imm(CM,this,0x39,0x83,0x81,7);
}

void AddInst::emit(CodeMemory* CM) const {
	emit_arithmetic_with_imm(CM,this,0x01,0x83,0x81,0);
}
void SubInst::emit(CodeMemory* CM) const {
	emit_arithmetic_with_imm(CM,this,0x29,0x83,0x81,5);
}
#endif

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
void JumpInst::emit(CodeMemory* CM) const {
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

void JumpInst::emit(CodeFragment &CF) const {
	BeginInst *BI = get_BeginInst();
	s4 offset = CF.get_offset(BI);
	assert(offset != 0);
	assert(offset != CodeMemory::INVALID_OFFSET);

	emit_jump(CF,offset);
}


void AddSDInst::emit(CodeMemory* CM) const {
	MachineOperand *src = operands[1].op;
	MachineOperand *dst = result.op;

	switch (get_OpEncoding(dst,src,get_op_size())) {
	case GPInstruction::RegReg64:
	{
		X86_64Register *src_reg = cast_to<X86_64Register>(src);
		X86_64Register *dst_reg = cast_to<X86_64Register>(dst);

		CodeFragment code = CM->get_CodeFragment(4);
		code[0] = 0xf2;
		code[1] = 0x0f;
		code[2] = 0x58;
		code[3] = get_modrm_reg2reg(dst_reg,src_reg);
		return;
	}
	default: break;
	}
	ABORT_MSG(this << ": Operand(s) not supported",
		"dst: " << dst << " src: " << src << " op_size: "
		<< get_op_size() * 8 << "bit");
}

void AddSSInst::emit(CodeMemory* CM) const {
	MachineOperand *src = operands[1].op;
	MachineOperand *dst = result.op;

	switch (get_OpEncoding(dst,src,get_op_size())) {
	case GPInstruction::RegReg64:
	{
		X86_64Register *src_reg = cast_to<X86_64Register>(src);
		X86_64Register *dst_reg = cast_to<X86_64Register>(dst);

		CodeFragment code = CM->get_CodeFragment(4);
		code[0] = 0xf3;
		code[1] = 0x0f;
		code[2] = 0x58;
		code[3] = get_modrm_reg2reg(dst_reg,src_reg);
		return;
	}
	default: break;
	}
	ABORT_MSG(this << ": Operand(s) not supported",
		"dst: " << dst << " src: " << src << " op_size: "
		<< get_op_size() * 8 << "bit");
}

void MovSDInst::emit(CodeMemory* CM) const {
	MachineOperand *src = operands[0].op;
	MachineOperand *dst = result.op;

	switch (get_OpEncoding(dst,src,get_op_size())) {
	case GPInstruction::RegReg64:
	{
		X86_64Register *src_reg = cast_to<X86_64Register>(src);
		X86_64Register *dst_reg = cast_to<X86_64Register>(dst);
		if (src_reg == dst_reg) return;

		CodeFragment code = CM->get_CodeFragment(4);
		code[0] = 0xf2;
		code[1] = 0x0f;
		code[2] = 0x10;
		code[3] = get_modrm_reg2reg(dst_reg,src_reg);
		return;
	}
	case GPInstruction::RegMem64:
	{
		StackSlot *src_slot = cast_to<StackSlot>(src);
		X86_64Register *dst_reg = cast_to<X86_64Register>(dst);

		s4 index = src_slot->get_index() * 8;

		if (fits_into<s1>(index)) {
			CodeFragment code = CM->get_CodeFragment(5);
			code[0] = 0xf2;
			code[1] = 0x0f;
			code[2] = 0x10;
			code[3] = get_modrm(0x1,dst_reg,&RBP);
			code[4] = u1( 0xff & (index >> (0 * 8)));
		} else {
			CodeFragment code = CM->get_CodeFragment(8);
			code[0] = 0xf2;
			code[1] = 0x0f;
			code[2] = 0x10;
			code[3] = get_modrm(0x2,dst_reg,&RBP);
			code[4] = u1( 0xff & (index >> (0 * 8)));
			code[5] = u1( 0xff & (index >> (1 * 8)));
			code[6] = u1( 0xff & (index >> (2 * 8)));
			code[7] = u1( 0xff & (index >> (3 * 8)));
		}
		return;
	}
	case GPInstruction::MemReg64:
	{
		StackSlot *dst_slot = cast_to<StackSlot>(dst);
		X86_64Register *src_reg = cast_to<X86_64Register>(src);

		s4 index = dst_slot->get_index() * 8;

		if (fits_into<s1>(index)) {
			CodeFragment code = CM->get_CodeFragment(5);
			code[0] = 0xf2;
			code[1] = 0x0f;
			code[2] = 0x11;
			code[3] = get_modrm(0x1,src_reg,&RBP);
			code[4] = u1( 0xff & (index >> (0 * 8)));
		} else {
			CodeFragment code = CM->get_CodeFragment(8);
			code[0] = 0xf2;
			code[1] = 0x0f;
			code[2] = 0x11;
			code[3] = get_modrm(0x2,src_reg,&RBP);
			code[4] = u1( 0xff & (index >> (0 * 8)));
			code[5] = u1( 0xff & (index >> (1 * 8)));
			code[6] = u1( 0xff & (index >> (2 * 8)));
			code[7] = u1( 0xff & (index >> (3 * 8)));
		}
		return;
	}
	default: break;
	}
	ABORT_MSG(this << ": Operand(s) not supported",
		"dst: " << dst << " src: " << src << " op_size: "
		<< get_op_size() * 8 << "bit");
}

void MovSSInst::emit(CodeMemory* CM) const {
	MachineOperand *src = operands[0].op;
	MachineOperand *dst = result.op;

	switch (get_OpEncoding(dst,src,get_op_size())) {
	case GPInstruction::RegReg64:
	{
		X86_64Register *src_reg = cast_to<X86_64Register>(src);
		X86_64Register *dst_reg = cast_to<X86_64Register>(dst);

		CodeFragment code = CM->get_CodeFragment(4);
		code[0] = 0xf3;
		code[1] = 0x0f;
		code[2] = 0x10;
		code[3] = get_modrm_reg2reg(dst_reg,src_reg);
		return;
	}
	default: break;
	}
	ABORT_MSG(this << ": Operand(s) not supported",
		"dst: " << dst << " src: " << src << " op_size: "
		<< get_op_size() * 8 << "bit");
}

} // end namespace x86_64
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
