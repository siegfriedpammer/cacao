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
#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/CodeMemory.hpp"
#include "vm/jit/PatcherNew.hpp"

#include "vm/jit/compiler2/alloc/deque.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/x86_64"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace x86_64 {

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
#if 0
template <>
inline X86_64Register* cast_to<X86_64Register>(MachineRegister *mreg) {
	assert(mreg);
	NativeRegister *nreg = mreg->to_NativeRegister();
	assert(nreg);
	X86_64Register *xreg = nreg->get_X86_64Register();
	assert(xreg);
	return xreg;
}
#endif
template <>
inline Immediate* cast_to<Immediate>(MachineOperand *op) {
	Immediate* imm = op->to_Immediate();
	assert(imm);
	return imm;
}


GPInstruction::OperandSize get_OperandSize_from_Type(const Type::TypeID type) {
	switch (type) {
	case Type::ByteTypeID:
	case Type::CharTypeID:
		return GPInstruction::OS_8;
	case Type::ShortTypeID:	return GPInstruction::OS_16;
	case Type::IntTypeID:    return GPInstruction::OS_32;
	case Type::LongTypeID:   return GPInstruction::OS_64;
	case Type::ReferenceTypeID: return GPInstruction::OS_64;
	case Type::FloatTypeID:  return GPInstruction::OS_32;
	case Type::DoubleTypeID: return GPInstruction::OS_64;
	default: break;
	}
	ABORT_MSG("Type Not supported!","get_OperandSize_from_Type: " << type);
	return GPInstruction::NO_SIZE;
}


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
			if ( (imm->get_type() == Type::IntTypeID
					&& fits_into<s1>(imm->get_Int())) ||
					(imm->get_type() == Type::LongTypeID
					&& fits_into<s1>(imm->get_Long())) ) {
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
		break;
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
		break;
	}
	default: break;
	}
	return GPInstruction::NO_ENCODING;
}

GPInstruction::OperandSize get_operand_size_from_Type(Type::TypeID type) {
	switch (type) {
	case Type::ByteTypeID:
		return GPInstruction::OS_8;
	case Type::IntTypeID:
	case Type::FloatTypeID:
		return GPInstruction::OS_32;
	case Type::LongTypeID:
	case Type::DoubleTypeID:
	case Type::ReferenceTypeID:
		return GPInstruction::OS_64;
	default: break;
	}
	ABORT_MSG("x86_64: get operand size not support",
		"type: " << type);
	return GPInstruction::NO_SIZE;
}

class CodeSegmentBuilder {
private:
	typedef alloc::deque<u1>::type Container;
public:
	typedef Container::iterator iterator;
	typedef Container::const_iterator const_iterator;
	typedef Container::value_type value_type;

	void push_front(u1 d) { data.push_front(d); }
	void push_back(u1 d) { data.push_back(d); }
	CodeSegmentBuilder& operator+=(u1 d) {
		push_back(d);
		return *this;
	}
	u1& operator[](std::size_t i) {
		if (i >= size()) {
			data.resize(i+1,0);
		}
		return data[i];
	}
	u1 operator[](std::size_t i) const {
		assert(i < size());
		return data[i];
	}
	std::size_t size() const { return data.size(); }
	const_iterator begin() const { return data.begin(); }
	const_iterator end()   const { return data.end(); }
	iterator begin() { return data.begin(); }
	iterator end()   { return data.end(); }
private:
	Container data;
};

void add_CodeSegmentBuilder(CodeMemory *CM, const CodeSegmentBuilder &CSB) {
	CodeFragment CF = CM->get_CodeFragment(CSB.size());
	for (std::size_t i = 0, e = CSB.size(); i < e; ++i) {
		CF[i] = CSB[i];
	}
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

		u1 rex  = get_rex(reg1,reg2,false);
		if (rex == 0x40) { // no rex
			CodeFragment code = CM->get_CodeFragment(2);
			code[0] = (alu_id * 0x8) + 3;
			code[1] = get_modrm_reg2reg(reg1,reg2);
		}
		else {
			CodeFragment code = CM->get_CodeFragment(3);
			code[0] = rex;
			code[1] = (alu_id * 0x8) + 3;
			code[2] = get_modrm_reg2reg(reg1,reg2);
		}
		return;
	}
	case GPInstruction::Reg32Imm8:
	{
		X86_64Register *reg1 = cast_to<X86_64Register>(src1);
		Immediate *imm = cast_to<Immediate>(src2);
		CodeFragment code = CM->get_CodeFragment(4);
		code[0] = get_rex(NULL, reg1);
		code[1] = 0x83;
		code[2] = get_modrm_1reg(alu_id, reg1);
		code[3] = imm->get_value<s1>();
		return;
	}
	case GPInstruction::Reg32Imm32:
	{
		X86_64Register *reg1 = cast_to<X86_64Register>(src1);
		Immediate *imm = cast_to<Immediate>(src2);
		CodeFragment code = CM->get_CodeFragment(7);
		code[0] = get_rex(NULL, reg1);
		code[1] = 0x81;
		code[2] = get_modrm_1reg(alu_id, reg1);
		InstructionEncoding::imm<s4>(code+3, imm->get_value<s4>());
		return;
	}
	case GPInstruction::Reg64Imm8:
	{
		X86_64Register *reg1 = cast_to<X86_64Register>(src1);
		Immediate *imm = cast_to<Immediate>(src2);
		CodeFragment code = CM->get_CodeFragment(4);
		code[0] = get_rex(NULL, reg1);
		code[1] = 0x83;
		code[2] = get_modrm_1reg(alu_id, reg1);
		code[3] = imm->get_value<s1>();
		return;
	}
	//case GPInstruction::Reg64Imm64: break;
	default: break;
	}

	ABORT_MSG(this << ": Operand(s) not supported",
		"src1: " << src1 << " src2: " << src2 << " op_size: "
		<< get_op_size() * 8 << "bit");
}

void emit_nop(CodeFragment code, int length) {
    assert(length >= 0 && length <= 9);
	unsigned mcodeptr = 0;
    switch (length) {
	case 0:
		break;
    case 1:
        code[mcodeptr++] = 0x90;
        break;
    case 2:
        code[mcodeptr++] = 0x66;
        code[mcodeptr++] = 0x90;
        break;
    case 3:
        code[mcodeptr++] = 0x0f;
        code[mcodeptr++] = 0x1f;
        code[mcodeptr++] = 0x00;
        break;
    case 4:
        code[mcodeptr++] = 0x0f;
        code[mcodeptr++] = 0x1f;
        code[mcodeptr++] = 0x40;
        code[mcodeptr++] = 0x00;
        break;
    case 5:
        code[mcodeptr++] = 0x0f;
        code[mcodeptr++] = 0x1f;
        code[mcodeptr++] = 0x44;
        code[mcodeptr++] = 0x00;
        code[mcodeptr++] = 0x00;
        break;
    case 6:
        code[mcodeptr++] = 0x66;
        code[mcodeptr++] = 0x0f;
        code[mcodeptr++] = 0x1f;
        code[mcodeptr++] = 0x44;
        code[mcodeptr++] = 0x00;
        code[mcodeptr++] = 0x00;
        break;
    case 7:
        code[mcodeptr++] = 0x0f;
        code[mcodeptr++] = 0x1f;
        code[mcodeptr++] = 0x80;
        code[mcodeptr++] = 0x00;
        code[mcodeptr++] = 0x00;
        code[mcodeptr++] = 0x00;
        code[mcodeptr++] = 0x00;
        break;
    case 8:
        code[mcodeptr++] = 0x0f;
        code[mcodeptr++] = 0x1f;
        code[mcodeptr++] = 0x84;
        code[mcodeptr++] = 0x00;
        code[mcodeptr++] = 0x00;
        code[mcodeptr++] = 0x00;
        code[mcodeptr++] = 0x00;
        code[mcodeptr++] = 0x00;
        break;
    case 9:
        code[mcodeptr++] = 0x66;
        code[mcodeptr++] = 0x0f;
        code[mcodeptr++] = 0x1f;
        code[mcodeptr++] = 0x84;
        code[mcodeptr++] = 0x00;
        code[mcodeptr++] = 0x00;
        code[mcodeptr++] = 0x00;
        code[mcodeptr++] = 0x00;
        code[mcodeptr++] = 0x00;
        break;
    }
}
void PatchInst::emit(CodeMemory* CM) const {
	CodeFragment code = CM->get_aligned_CodeFragment(2);
	code[0] = 0x0f;
	code[1] = 0x0b;
	emit_nop(code + 2, code.size() - 2);
	CM->require_linking(this,code);
}
void PatchInst::link(CodeFragment &CF) const {
	CodeMemory &CM = CF.get_Segment().get_CodeMemory();
	patcher->reposition(CM.get_offset(CF.get_begin()));
	LOG2(this << " link: reposition: " << patcher->get_mpc() << nl);
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

void BreakInst::emit(CodeMemory* CM) const {
	CodeFragment code = CM->get_CodeFragment(1);
	// INT3 - drop to debugger
	code[0] = 0xcc;
}

void RetInst::emit(CodeMemory* CM) const {
	CodeFragment code = CM->get_CodeFragment(1);
	code[0] = 0xc3;
}

void NegInst::emit(CodeMemory* CM) const {
	X86_64Register *reg = cast_to<X86_64Register>(operands[0].op);
	CodeSegmentBuilder code;

	u1 rex = get_rex(reg,NULL,get_op_size() == OS_64);
	// rex
	if (rex != 0x40) {
		code += rex;
	}
	// opcode
	if (get_op_size() == OS_8)
		code += 0xf6;
	else
		code += 0xf7;
	code += get_modrm_u1(0x3,0x3,reg->get_index());
	add_CodeSegmentBuilder(CM,code);
}

void CallInst::emit(CodeMemory* CM) const {
	CodeFragment code = CM->get_CodeFragment(2);
	X86_64Register *reg_src = cast_to<X86_64Register>(operands[0].op);
	//code[0] = get_rex(reg_src);
	code[0] = 0xff;
	code[1] = get_modrm_1reg(2, reg_src);
}

void MovInst::emit(CodeMemory* CM) const {
	MachineOperand *src = operands[0].op;
	MachineOperand *dst = result.op;

	GPInstruction::OpEncoding openc = get_OpEncoding(dst,src,get_op_size());
	switch (openc) {
	case GPInstruction::RegReg64:
	{
		X86_64Register *reg_dst = cast_to<X86_64Register>(dst);
		X86_64Register *reg_src = cast_to<X86_64Register>(src);
		if (reg_dst == reg_src && !forceSameRegisters) return;

		CodeFragment code = CM->get_CodeFragment(3);
		code[0] = get_rex(reg_dst,reg_src);
		code[1] = 0x8b;
		code[2] = get_modrm_reg2reg(reg_dst,reg_src);
		return;
	}
	case GPInstruction::RegReg8:
	case GPInstruction::RegReg16:
	case GPInstruction::RegReg32:
	{
		X86_64Register *reg_dst = cast_to<X86_64Register>(dst);
		X86_64Register *reg_src = cast_to<X86_64Register>(src);
		if (reg_dst == reg_src && !forceSameRegisters) return;

		u1 rex  = get_rex(reg_dst,reg_src,false);
		if (rex == 0x40) { // no rex
			CodeFragment code = CM->get_CodeFragment(2);
			code[0] = 0x8b;
			code[1] = get_modrm_reg2reg(reg_dst,reg_src);
		}
		else {
			CodeFragment code = CM->get_CodeFragment(3);
			code[0] = rex;
			code[1] = 0x8b;
			code[2] = get_modrm_reg2reg(reg_dst,reg_src);
		}
		return;
	}
	case GPInstruction::MemReg32:
	case GPInstruction::MemReg64:
	case GPInstruction::RegMem32:
	case GPInstruction::RegMem64:
	{
		u1 opcode = 0;
		REX rex;
		StackSlot *slot = 0;
		X86_64Register *reg = 0;
		// set operands and opcode
		switch(openc) {
		case GPInstruction::MemReg32:
		case GPInstruction::MemReg64:
			opcode = 0x89;
			reg = cast_to<X86_64Register>(src);
			slot = cast_to<StackSlot>(dst);
			break;
		case GPInstruction::RegMem32:
		case GPInstruction::RegMem64:
			opcode = 0x8b;
			slot = cast_to<StackSlot>(src);
			reg = cast_to<X86_64Register>(dst);
			break;
		default: assert(0);
			break;
		}
		// set rex
		if ( openc ==  GPInstruction::MemReg64 || openc == GPInstruction::RegMem64) {
			rex + REX::W;
		}
		if (reg->extented) {
			rex + REX::R;
		}
		CodeSegmentBuilder code;
		// rex
		if (rex != 0x40) {
			code += rex;
		}
		// opcode
		code += opcode;
		// modmr + imm
		s4 index = slot->get_index() * 8;
		if (fits_into<s1>(index)) {
			code += get_modrm(0x1,reg,&RBP);
			code += (u1) 0xff & (index >> 0x00);
		} else {
			code += get_modrm(0x2,reg,&RBP);
			code += (u1) 0xff & (index >> 0x00);
			code += (u1) 0xff & (index >> 0x08);
			code += (u1) 0xff & (index >> 0x10);
			code += (u1) 0xff & (index >> 0x18);
		}
		add_CodeSegmentBuilder(CM,code);
		return;
	}
	case GPInstruction::Reg64Imm8:
	case GPInstruction::Reg64Imm64:
	{
		Immediate *imm = cast_to<Immediate>(src);
		X86_64Register *reg_dst = cast_to<X86_64Register>(dst);
		CodeFragment code = CM->get_CodeFragment(10);
		code[0] = REX() + REX::W + OpReg(reg_dst);
		code[1] = 0xb8 + reg_dst->get_index();
		InstructionEncoding::imm<s8>(code+2, imm->get_value<s8>());
		return;
	}
	case GPInstruction::Reg32Imm8:
	case GPInstruction::Reg32Imm32:
	{
		s4 imm = cast_to<Immediate>(src)->get_value<s4>();
		X86_64Register *reg_dst = cast_to<X86_64Register>(dst);

		CodeSegmentBuilder code;
		if (reg_dst->extented) {
			code += (REX() + REX::B);
		}
		code += 0xb8 + reg_dst->get_index();
		code += (u1) 0xff & (imm >> 0x00);
		code += (u1) 0xff & (imm >> 0x08);
		code += (u1) 0xff & (imm >> 0x10);
		code += (u1) 0xff & (imm >> 0x18);
		add_CodeSegmentBuilder(CM,code);
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


inline u1 get_rex(X86_64Register *reg, const ModRMOperandDesc &modrm, bool opsize64) {
	REX rex;
	if (opsize64)
		rex + REX::W;
	if (reg->extented)
		rex + REX::R;
	if (modrm.base.op != &NoOperand && cast_to<X86_64Register>(modrm.base.op)->extented)
		rex + REX::B;
	if (modrm.index.op != &NoOperand && cast_to<X86_64Register>(modrm.index.op)->extented)
		rex + REX::X;

	return rex;
}

void MovModRMInst::emit(CodeMemory* CM) const {
	X86_64Register *reg;
	u1 opcode;
	u1 floatopcode[3];
	floatopcode[0] = (get_op_size() == OS_64 ? 0xf2 : 0xf3);
	floatopcode[1] = 0x0f;
	// check operand encoding
	switch (enc) {
	case RM:
		{
			MachineOperand *op = get_result().op;
			reg = (op?cast_to<X86_64Register>(op) : 0);
			opcode = 0x8b;
			floatopcode[2] = 0x10;
			break;
		}
	case MR:
		{
			reg = cast_to<X86_64Register>(operands[Value].op);
			opcode = 0x89;
			floatopcode[2] = 0x11;
			break;
		}
	default:
	  ABORT_MSG("x86_64::MovModRMInst","not implemented");
	  reg = 0;
	}
	assert(reg);
	CodeSegmentBuilder code;
	if (!floatingpoint) {
		// set rex
		u1 rex = get_rex(reg,modrm,get_op_size() == GPInstruction::OS_64);
		if (rex != 0x40)
			code += rex;
		// set opcode
		code += opcode;
	} else {
		code += floatopcode[0];
		code += floatopcode[1];
		code += floatopcode[2];
	}
	// set modrm byte
	// mod
	u1 modrm_mod;
	if (modrm.disp == 0) {
		// no disp
		modrm_mod = 0;
	}
	else if (fits_into<s1>(modrm.disp)) {
		// disp8
		modrm_mod = 1;
	} else {
		// disp32
		modrm_mod = 2;
	}
	X86_64Register *index_reg = (modrm.index.op != &NoOperand ? cast_to<X86_64Register>(modrm.index.op) : 0);
	X86_64Register *base_reg  = (modrm.base.op  != &NoOperand ? cast_to<X86_64Register>(modrm.base.op ) : 0);
	// r/m
	u1 modrm_rm;
	bool need_sib;
	// XXX for the time being always use SIB byte
	if (!index_reg && base_reg && base_reg->get_index() != 0x4) { // 0b100
		modrm_rm = base_reg->get_index();
		need_sib = false;
	} else {
		modrm_rm = 4; // 0b100
		need_sib = true;
	}
	code += get_modrm_u1(modrm_mod,reg->get_index(),modrm_rm);

	if (need_sib) {
		// set sib
		u1 sib=0;
		sib |= modrm.scale << 6;
		if(index_reg) {
			sib |= (0x7 & index_reg->get_index()) << 3;
		}
		else {
			sib |= 0x4 << 3; // 0b100 << 3
		}
		assert(base_reg);
		sib |= (0x7 & base_reg->get_index()) << 0 ;
		code += sib;
	}
	if (modrm_mod == 1) {
		code += s1(modrm.disp);
	}
	if (modrm_mod == 2) {
		code += (0xff && (modrm.disp >>  0));
		code += (0xff && (modrm.disp >>  8));
		code += (0xff && (modrm.disp >> 16));
		code += (0xff && (modrm.disp >> 24));
	}
	add_CodeSegmentBuilder(CM,code);
}

OStream& operator<<(OStream &OS,const ModRMOperandDesc &modrm) {
	if (modrm.disp)
	  OS << modrm.disp;
	OS << '(';
	if (modrm.base.op != &NoOperand)
	  OS << modrm.base.op;
	OS << ',';
	if (modrm.index.op != &NoOperand)
	  OS << modrm.index.op;
	OS << ',';
	OS << 1 << modrm.scale;
	OS << ')';
	return OS;
}

OStream& MovModRMInst::print_operands(OStream &OS) const {
	switch (enc) {
	case RM:
		OS << modrm;
		break;
	case MR:
		OS << operands[Value].op;
		// no real operands - fake it!
		OS << " -> ";
		OS = print_result(OS);
		break;
	default:
	  ABORT_MSG("x86_64::MovModRMInst","not implemented");
	}
	return OS;
}
OStream& MovModRMInst::print_result(OStream &OS) const {
	switch (enc) {
	case RM:
		OS << get_result().op;
		break;
	case MR:
		OS << modrm;
		break;
	default:
	  ABORT_MSG("x86_64::MovModRMInst","not implemented");
	}
	return OS;
}

void MovSXInst::emit(CodeMemory* CM) const {
	MachineOperand *src = operands[0].op;
	MachineOperand *dst = result.op;
	X86_64Register *src_reg = cast_to<X86_64Register>(src);
	X86_64Register *dst_reg = cast_to<X86_64Register>(dst);

	switch (from) {
	case GPInstruction::OS_8:
		switch (to) {
		case GPInstruction::OS_32:
		{
			CodeFragment code = CM->get_CodeFragment(4);

			code[0] = get_rex(dst_reg, src_reg);
			code[1] = 0x0f;
			code[2] = 0xbe;
			code[3] = get_modrm_reg2reg(dst_reg, src_reg);
			return;
		}
		default: break;
		}
		break;
	case GPInstruction::OS_16:
		switch (to) {
		case GPInstruction::OS_32:
		{
			CodeFragment code = CM->get_CodeFragment(4);

			code[0] = get_rex(dst_reg, src_reg);
			code[1] = 0x0f;
			code[2] = 0xbf;
			code[3] = get_modrm_reg2reg(dst_reg, src_reg);
			return;
		}
		default: break;
		}
		break;
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

void MovDSEGInst::emit(CodeMemory* CM) const {
	X86_64Register *dst = cast_to<X86_64Register>(result.op);
	switch (get_op_size()) {
	case OS_8:
	case OS_16:
	case OS_32:
		break;
	case OS_64:
		{
			CodeFragment code = CM->get_CodeFragment(7);
			code[0] = get_rex(dst);
			code[1] = 0x8b;
			// RIP-Relateive addressing mod=00b, rm=101b, + disp32
			code[2] = get_modrm_u1(0x0,dst->get_index(),0x5);
			#if 1
			code[3] = 0xaa;
			code[4] = 0xaa;
			code[5] = 0xaa;
			code[6] = 0xaa;
			#endif
			CM->require_linking(this,code);
		}
		return;
	default: break;
	}
	ABORT_MSG(this << ": Operand(s) not supported",
		"op_size: " << get_op_size() * 8 << "bit");
}

void MovDSEGInst::link(CodeFragment &CF) const {
	CodeMemory &CM = CF.get_Segment().get_CodeMemory();
	s4 offset = CM.get_offset(data_index,CF);
	LOG2(this << " offset: " << offset << " data index: " << data_index.idx << " CF end index "
			<< CF.get_end().idx << nl);
	LOG2("dseg->size " << CM.get_DataSegment().size()
		<< " cseg->size " << CM.get_CodeSegment().size() << nl );
	assert(offset != 0);
	switch (get_op_size()) {
	case OS_8:
	case OS_16:
	case OS_32:
		break;
	case OS_64:
		{
			CF[3] = u1(0xff & (offset >>  0));
			CF[4] = u1(0xff & (offset >>  8));
			CF[5] = u1(0xff & (offset >> 16));
			CF[6] = u1(0xff & (offset >> 24));
		}
		return;
	default: break;
	}
	ABORT_MSG(this << ": Operand(s) not supported",
		"op_size: " << get_op_size() * 8 << "bit");
}

OStream& CondJumpInst::print_successor_label(OStream &OS,std::size_t index) const{
	switch(index){
	case 0: return OS << "then";
	case 1: return OS << "else";
	default: assert(0); break;
	}
	return OS << index;
}

void CondTrapInst::emit(CodeMemory* CM) const {
	CodeFragment code = CM->get_CodeFragment(8);
	// XXX make more readable
	X86_64Register *src_reg = cast_to<X86_64Register>(operands[0].op);
	// move
	code[0] = 0x48; // rex
	code[1] = 0x8b; // move
	code[2] = (0x7 & src_reg->get_index()) << 3 | 0x4;
	                // ModRM: mod=0x00, reg=src_reg, rm=0b100 (SIB)
	code[3] = 0x25; // SIB scale=0b00, index=0b100, base=0b101 (illegal -> trap)
	// trap
	code[4] = u1( 0xff & (trap >> (8 * 0)));
	code[5] = u1( 0xff & (trap >> (8 * 1)));
	code[6] = u1( 0xff & (trap >> (8 * 2)));
	code[7] = u1( 0xff & (trap >> (8 * 3)));
	// skip jump
	s4 offset = 8;
	InstructionEncoding::imm_op<u2>(CM, 0x0f80 + cond.code, offset);

}

void CMovInst::emit(CodeMemory* CM) const {
	X86_64Register *src = cast_to<X86_64Register>(operands[1].op);
	X86_64Register *dst = cast_to<X86_64Register>(get_result().op);

	CodeSegmentBuilder code;
	u1 rex = get_rex(dst,src,get_op_size() == GPInstruction::OS_64);
	if (rex != 0x40)
	  code += rex;
	code += 0x0f;
	code += 0x40 | cond.code;
	code += get_modrm_reg2reg(dst,src);
	add_CodeSegmentBuilder(CM,code);
}

void CondJumpInst::emit(CodeMemory* CM) const {
	// update jump target (might have changed)
	jump.set_target(successor_back());
	// emit else jump (if needed)
	jump.emit(CM);
	// emit then jump
	MachineBasicBlock *MBB = successor_front();
	CodeSegment &CS = CM->get_CodeSegment();
	CodeSegment::IdxTy idx = CS.get_index(CSLabel(MBB));
	if (CodeSegment::is_invalid(idx)) {
		LOG2("X86_64CondJumpInst: target not yet known (" << this << " to "
		     << *MBB << ")"  << nl);
		// reserve memory and add to resolve later
		// worst case -> 32bit offset
		CodeFragment CF = CM->get_CodeFragment(6);
		// FIXME pass this the resolve me
		CM->require_linking(this,CF);
		return;
	}
	s4 offset = CM->get_offset(idx);
	if (offset == 0) {
		// XXX fix me! remove empty blocks?
		ABORT_MSG("x86_64 ERROR","CondJump offset 0 oO!");
		//return;
	}
	LOG2("found offset of " << *MBB << ": " << offset << nl);

	// only 32bit offset for the time being
	InstructionEncoding::imm_op<u2>(CM, 0x0f80 + cond.code, offset);
}

void CondJumpInst::link(CodeFragment &CF) const {
	MachineBasicBlock *MBB = successor_front();
	CodeSegment &CS = CF.get_Segment();
	CodeSegment::IdxTy idx = CS.get_index(CSLabel(MBB));
	s4 offset = CS.get_CodeMemory().get_offset(idx,CF);
	assert(offset != 0);

	InstructionEncoding::imm_op<u2>(CF, 0x0f80 + cond.code, offset);
	jump.link(CF);
}

void IMulInst::emit(CodeMemory* CM) const {
	X86_64Register *src_reg = cast_to<X86_64Register>(operands[1].op);
	X86_64Register *dst_reg = cast_to<X86_64Register>(result.op);

	InstructionEncoding::reg2reg<u2>(CM, 0x0faf, dst_reg, src_reg);
}

void IDivInst::emit(CodeMemory *CM) const {
	CodeFragment code = CM->get_CodeFragment(2);
	X86_64Register *divisor = cast_to<X86_64Register>(operands[1].op);
	code[0] = 0xf7;
	code[1] = get_modrm_1reg(7, divisor);
}

void CDQInst::emit(CodeMemory *CM) const {
	CodeFragment code = CM->get_CodeFragment(1);
	code[0] = 0x99;
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
	MachineBasicBlock *MBB = successor_front();
	CodeSegment &CS = CM->get_CodeSegment();
	CodeSegment::IdxTy idx = CS.get_index(CSLabel(MBB));
	if (CodeSegment::is_invalid(idx)) {
		LOG2("emit_Jump: target not yet known (" << this << " to "
		     << *MBB << ")"  << nl);
		// reserve memory and add to resolve later
		// worst case -> 32bit offset
		CodeFragment CF = CM->get_CodeFragment(5);
		// FIXME pass this the resolve me
		CM->require_linking(this,CF);
		return;
	}
	s4 offset = CM->get_offset(idx);
	if (offset == 0) {
		LOG2("emit_Jump: jump to the next instruction -> can be omitted ("
		     << this << " to " << *MBB << ")"  << nl);
		return;
	}
	CodeFragment CF = CM->get_CodeFragment(5);
	emit_jump(CF,offset);
}

void JumpInst::link(CodeFragment &CF) const {
	MachineBasicBlock *MBB = successor_front();
	CodeSegment &CS = CF.get_Segment();
	CodeSegment::IdxTy idx = CS.get_index(CSLabel(MBB));
	LOG2("JumpInst:link BI: " << *MBB << " idx: " << idx.idx << " CF begin: "
		<< CF.get_begin().idx << " CF end: " << CF.get_end().idx << nl);
	s4 offset = CS.get_CodeMemory().get_offset(idx,CF);
	assert(offset != 0);

	emit_jump(CF,offset);
}

void IndirectJumpInst::emit(CodeMemory* CM) const {
	X86_64Register *src_reg = cast_to<X86_64Register>(operands[0].op);
	CodeFragment code = CM->get_CodeFragment(2);
	code[0] = 0xff;
	WARNING_MSG("Not yet implemented","No support for indirect jump yet.");
	code[1] = get_modrm_1reg(0,src_reg);
	return;
}

void UCOMISInst::emit(CodeMemory* CM) const {
	X86_64Register *src1 = cast_to<X86_64Register>(operands[0].op);
	X86_64Register *src2 = cast_to<X86_64Register>(operands[1].op);

	CodeSegmentBuilder code;
	if (get_op_size() == GPInstruction::OS_64) {
		code += 0x66;
	}
	code += 0x0f;
	code += 0x2e;
	code += get_modrm_reg2reg(src1,src2);
	add_CodeSegmentBuilder(CM,code);
}

void XORPInst::emit(CodeMemory* CM) const {
	X86_64Register *dstsrc1 = cast_to<X86_64Register>(operands[0].op);
	X86_64Register *src2 = cast_to<X86_64Register>(operands[1].op);

	CodeSegmentBuilder code;
	if (get_op_size() == GPInstruction::OS_64) {
		code += 0x66;
	}
	code += 0x0f;
	code += 0x57;
	code += get_modrm_reg2reg(dstsrc1,src2);
	add_CodeSegmentBuilder(CM,code);
}

void SSEAluInst::emit(CodeMemory* CM) const {
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
		code[2] = opcode;
		code[3] = get_modrm_reg2reg(dst_reg,src_reg);
		return;
	}
	case GPInstruction::RegReg32:
	{
		X86_64Register *src_reg = cast_to<X86_64Register>(src);
		X86_64Register *dst_reg = cast_to<X86_64Register>(dst);

		CodeFragment code = CM->get_CodeFragment(4);
		code[0] = 0xf3;
		code[1] = 0x0f;
		code[2] = opcode;
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

		#if 0
		if (fits_into<s1>(index)) {
			CodeFragment code = CM->get_CodeFragment(5);
			code[0] = 0xf2;
			code[1] = 0x0f;
			code[2] = 0x10;
			code[3] = get_modrm(0x1,dst_reg,&RBP);
			code[4] = u1( 0xff & (index >> (0 * 8)));
		} else
		#endif
		{
			CodeFragment code = CM->get_CodeFragment(8);
			code[0] = 0xf2;
			code[1] = 0x0f;
			code[2] = 0x10;
			code[3] = get_modrm(0x2,dst_reg,&RBP);
			InstructionEncoding::imm(code+4,index);
			#if 0
			code[4] = u1( 0xff & (index >> (0 * 8)));
			code[5] = u1( 0xff & (index >> (1 * 8)));
			code[6] = u1( 0xff & (index >> (2 * 8)));
			code[7] = u1( 0xff & (index >> (3 * 8)));
			#endif
		}
		return;
	}
	case GPInstruction::MemReg64:
	{
		StackSlot *dst_slot = cast_to<StackSlot>(dst);
		X86_64Register *src_reg = cast_to<X86_64Register>(src);

		s4 index = dst_slot->get_index() * 8;

		#if 0
		if (fits_into<s1>(index)) {
			CodeFragment code = CM->get_CodeFragment(5);
			code[0] = 0xf2;
			code[1] = 0x0f;
			code[2] = 0x11;
			code[3] = get_modrm(0x1,src_reg,&RBP);
			code[4] = u1( 0xff & (index >> (0 * 8)));
		} else
		#endif
		{
			CodeFragment code = CM->get_CodeFragment(8);
			code[0] = 0xf2;
			code[1] = 0x0f;
			code[2] = 0x11;
			code[3] = get_modrm(0x2,src_reg,&RBP);
			InstructionEncoding::imm(code+4,index);
			#if 0
			code[4] = u1( 0xff & (index >> (0 * 8)));
			code[5] = u1( 0xff & (index >> (1 * 8)));
			code[6] = u1( 0xff & (index >> (2 * 8)));
			code[7] = u1( 0xff & (index >> (3 * 8)));
			#endif
		}
		return;
	}
	default: break;
	}
	ABORT_MSG(this << ": Operand(s) not supported",
		"dst: " << dst << " src: " << src << " op_size: "
		<< get_op_size() * 8 << "bit");
}

// TODO: refactor
void MovSSInst::emit(CodeMemory* CM) const {
	MachineOperand *src = operands[0].op;
	MachineOperand *dst = result.op;
	if (src->aquivalent(*dst)) return;

	switch (get_OpEncoding(dst,src,get_op_size())) {
	case GPInstruction::RegReg32:
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
	case GPInstruction::RegMem32:
	{
		StackSlot *src_slot = cast_to<StackSlot>(src);
		X86_64Register *dst_reg = cast_to<X86_64Register>(dst);

		s4 index = src_slot->get_index() * 8;

		CodeFragment code = CM->get_CodeFragment(8);
		code[0] = 0xf3;
		code[1] = 0x0f;
		code[2] = 0x10;
		code[3] = get_modrm(0x2, dst_reg, &RBP);
		InstructionEncoding::imm(code + 4, index);
		return;
	}
	case GPInstruction::MemReg32:
	{
		StackSlot *dst_slot = cast_to<StackSlot>(dst);
		X86_64Register *src_reg = cast_to<X86_64Register>(src);

		s4 index = dst_slot->get_index() * 8;

		CodeFragment code = CM->get_CodeFragment(8);
		code[0] = 0xf3;
		code[1] = 0x0f;
		code[2] = 0x11;
		code[3] = get_modrm(0x2, src_reg, &RBP);
		InstructionEncoding::imm(code + 4, index);
		return;
	}
	default: break;
	}
	ABORT_MSG(this << ": Operand(s) not supported",
		"dst: " << dst << " src: " << src << " op_size: "
		<< get_op_size() * 8 << "bit");

}

void FPRemInst::emit(CodeMemory* CM) const {

	CodeFragment code = CM->get_CodeFragment(2);
	code[0] = 0xd9;
	code[1] = 0xf8;
}

void MovImmSInst::emit(CodeMemory* CM) const {
	Immediate *imm = cast_to<Immediate>(operands[0].op);
	X86_64Register *dst = cast_to<X86_64Register>(result.op);

	CodeFragment code = CM->get_CodeFragment(dst->extented ? 9 : 8);
	if (dst->extented) {
		code[0] = get_rex(dst,NULL,get_op_size() == OS_64);
		code[1] = (get_op_size() == OS_64 ? 0xf2 : 0xf3);
		code[2] = 0x0f;
		code[3] = 0x10;
		// RIP-Relateive addressing mod=00b, rm=101b, + disp32
		code[4] = get_modrm_u1(0x0,dst->get_index(),0x5);
		#if 1
		code[5] = 0xaa;
		code[6] = 0xaa;
		code[7] = 0xaa;
		code[8] = 0xaa;
		#endif
	} else {
		code[0] = (get_op_size() == OS_64 ? 0xf2 : 0xf3);
		code[1] = 0x0f;
		code[2] = 0x10;
		// RIP-Relateive addressing mod=00b, rm=101b, + disp32
		code[3] = get_modrm_u1(0x0,dst->get_index(),0x5);
		#if 1
		code[4] = 0xaa;
		code[5] = 0xaa;
		code[6] = 0xaa;
		code[7] = 0xaa;
		#endif
	}
	switch (get_op_size()) {
	case OS_64:
		switch (imm->get_type()) {
		case Type::DoubleTypeID:
			{
				DataFragment DF = CM->get_DataSegment().get_Ref(sizeof(double));
				InstructionEncoding::imm<double>(DF,imm->get_Double());
				data_index = CM->get_DataSegment().insert_tag(DSDouble(imm->get_Double()),DF);
				break;
			}
		case Type::LongTypeID:
			{
				DataFragment DF = CM->get_DataSegment().get_Ref(sizeof(int64_t));
				InstructionEncoding::imm<int64_t>(DF,imm->get_Long());
				data_index = CM->get_DataSegment().insert_tag(DSLong(imm->get_Long()),DF);
				break;
			}
		default: assert(0);
			break;
		}
		break;
	case OS_32:
		switch (imm->get_type()) {
		case Type::FloatTypeID:
			{
				DataFragment DF = CM->get_DataSegment().get_Ref(sizeof(float));
				InstructionEncoding::imm<float>(DF,imm->get_Float());
				data_index = CM->get_DataSegment().insert_tag(DSFloat(imm->get_Float()),DF);
				break;
			}
		case Type::IntTypeID:
			{
				DataFragment DF = CM->get_DataSegment().get_Ref(sizeof(int32_t));
				InstructionEncoding::imm<int32_t>(DF,imm->get_Int());
				data_index = CM->get_DataSegment().insert_tag(DSInt(imm->get_Int()),DF);
				break;
			}
		default: assert(0);
			break;
		}
		break;
	default: assert(0);
		break;
	}
	CM->require_linking(this,code);
}

void MovImmSInst::link(CodeFragment &CF) const {
	CodeMemory &CM = CF.get_Segment().get_CodeMemory();
	s4 offset = CM.get_offset(data_index,CF);
	LOG2(this << " offset: " << offset << " data index: " << data_index.idx << " CF end index "
			<< CF.get_end().idx << nl);
	LOG2("dseg->size " << CM.get_DataSegment().size()
		<< " cseg->size " << CM.get_CodeSegment().size() << nl );

	assert(offset != 0);
	int i = 4;
	if (cast_to<X86_64Register>(result.op)->extented)
		i = 5;
	CF[i++] = u1(0xff & (offset >>  0));
	CF[i++] = u1(0xff & (offset >>  8));
	CF[i++] = u1(0xff & (offset >> 16));
	CF[i  ] = u1(0xff & (offset >> 24));
}




// TODO: refactor
void CVTSI2SDInst::emit(CodeMemory* CM) const {
	MachineOperand *src = operands[0].op;
	MachineOperand *dst = result.op;

	X86_64Register *src_reg = cast_to<X86_64Register>(src);
	X86_64Register *dst_reg = cast_to<X86_64Register>(dst);

	switch (from) {
	case GPInstruction::OS_32:
	{
		CodeFragment code = CM->get_CodeFragment(4);
		code[0] = 0xf2;
		code[1] = 0x0f;
		code[2] = 0x2a;
		code[3] = get_modrm_reg2reg(dst_reg,src_reg);
		return;
	}
	case GPInstruction::OS_64:
	{
		CodeFragment code = CM->get_CodeFragment(5);
		code[0] = 0xf2;
		code[1] = get_rex(dst_reg,src_reg);
		code[2] = 0x0f;
		code[3] = 0x2a;
		code[4] = get_modrm_reg2reg(dst_reg,src_reg);
		return;
	}
	default: break;
	}
	ABORT_MSG(this << ": Operand(s) not supported",
		"dst: " << dst << " src: " << src << " op_size: "
		<< get_op_size() * 8 << "bit");
}

// TODO: refactor
void CVTSI2SSInst::emit(CodeMemory* CM) const {
	MachineOperand *src = operands[0].op;
	MachineOperand *dst = result.op;

	X86_64Register *src_reg = cast_to<X86_64Register>(src);
	X86_64Register *dst_reg = cast_to<X86_64Register>(dst);

	switch (from) {
	case GPInstruction::OS_32:
	{
		CodeFragment code = CM->get_CodeFragment(4);
		code[0] = 0xf3;
		code[1] = 0x0f;
		code[2] = 0x2a;
		code[3] = get_modrm_reg2reg(dst_reg,src_reg);
		return;
	}
	case GPInstruction::OS_64:
	{
		CodeFragment code = CM->get_CodeFragment(5);
		code[0] = 0xf3;
		code[1] = get_rex(dst_reg,src_reg);
		code[2] = 0x0f;
		code[3] = 0x2a;
		code[4] = get_modrm_reg2reg(dst_reg,src_reg);
		return;
	}
	default: break;
	}
	ABORT_MSG(this << ": Operand(s) not supported",
		"dst: " << dst << " src: " << src << " op_size: "
		<< get_op_size() * 8 << "bit");
}


void CVTTSS2SIInst::emit(CodeMemory* CM) const {
	MachineOperand *src = operands[0].op;
	MachineOperand *dst = result.op;

	X86_64Register *src_reg = cast_to<X86_64Register>(src);
	X86_64Register *dst_reg = cast_to<X86_64Register>(dst);

	switch (to) {
	case GPInstruction::OS_32:
	{
		CodeFragment code = CM->get_CodeFragment(4);
		code[0] = 0xf3;
		code[1] = 0x0f;
		code[2] = 0x2c;
		code[3] = get_modrm_reg2reg(dst_reg,src_reg);
		return;
	}
	case GPInstruction::OS_64:
	{
		CodeFragment code = CM->get_CodeFragment(5);
		code[0] = 0xf3;
		code[1] = get_rex(dst_reg,src_reg);
		code[2] = 0x0f;
		code[3] = 0x2c;
		code[4] = get_modrm_reg2reg(dst_reg,src_reg);
		return;
	}
	default: break;
	}
	ABORT_MSG(this << ": Operand(s) not supported",
		"dst: " << dst << " src: " << src << " op_size: "
		<< get_op_size() * 8 << "bit");

}

void CVTTSD2SIInst::emit(CodeMemory* CM) const {
	MachineOperand *src = operands[0].op;
	MachineOperand *dst = result.op;

	X86_64Register *src_reg = cast_to<X86_64Register>(src);
	X86_64Register *dst_reg = cast_to<X86_64Register>(dst);

	switch (to) {
	case GPInstruction::OS_32:
	{
		CodeFragment code = CM->get_CodeFragment(4);
		code[0] = 0xf2;
		code[1] = 0x0f;
		code[2] = 0x2c;
		code[3] = get_modrm_reg2reg(dst_reg,src_reg);
		return;
	}
	case GPInstruction::OS_64:
	{
		CodeFragment code = CM->get_CodeFragment(5);
		code[0] = 0xf2;
		code[1] = get_rex(dst_reg,src_reg);
		code[2] = 0x0f;
		code[3] = 0x2c;
		code[4] = get_modrm_reg2reg(dst_reg,src_reg);
		return;
	}
	default: break;
	}
	ABORT_MSG(this << ": Operand(s) not supported",
		"dst: " << dst << " src: " << src << " op_size: "
		<< get_op_size() * 8 << "bit");

}

void CVTSS2SDInst::emit(CodeMemory* CM) const {
	MachineOperand *src = operands[0].op;
	MachineOperand *dst = result.op;

	X86_64Register *src_reg = cast_to<X86_64Register>(src);
	X86_64Register *dst_reg = cast_to<X86_64Register>(dst);

	CodeFragment code = CM->get_CodeFragment(4);
	code[0] = 0xf3;
	code[1] = 0x0f;
	code[2] = 0x5a;
	code[3] = get_modrm_reg2reg(dst_reg,src_reg);

}

void CVTSD2SSInst::emit(CodeMemory* CM) const {
	MachineOperand *src = operands[0].op;
	MachineOperand *dst = result.op;

	X86_64Register *src_reg = cast_to<X86_64Register>(src);
	X86_64Register *dst_reg = cast_to<X86_64Register>(dst);

	CodeFragment code = CM->get_CodeFragment(4);
	code[0] = 0xf2;
	code[1] = 0x0f;
	code[2] = 0x5a;
	code[3] = get_modrm_reg2reg(dst_reg,src_reg);

}

// TODO: refactor
void FLDInst::emit(CodeMemory *CM) const {
	StackSlot *slot;

	slot = cast_to<StackSlot>(result.op);
	CodeSegmentBuilder code;
	REX rex;
	OperandSize op_size = get_op_size();

	switch (op_size) {

	case GPInstruction::OS_32:
	{
		code += 0xd9;
		break;
	}
	case GPInstruction::OS_64:
	{
		rex + REX::W;
		code += rex;
		code += 0xdd;
		break;
	}
	default:
	{
		ABORT_MSG(this << ": Operand size for FLD not supported",
			"size: " << get_op_size() * 8 << "bit");
	}
	}


	// modmr + imm
	s4 index = slot->get_index() * 8;
	if (fits_into<s1>(index)) {
		code += get_modrm_u1(0x1,0,RBP.get_index());
		code += (u1) 0xff & (index >> 0x00);
	} else {
		code += get_modrm_u1(0x2,0,RBP.get_index());
		code += (u1) 0xff & (index >> 0x00);
		code += (u1) 0xff & (index >> 0x08);
		code += (u1) 0xff & (index >> 0x10);
		code += (u1) 0xff & (index >> 0x18);
	}
	add_CodeSegmentBuilder(CM,code);

}

// TODO: refactor
void FSTPInst::emit(CodeMemory *CM) const {
	StackSlot *slot;

	slot = cast_to<StackSlot>(result.op);
	CodeSegmentBuilder code;
	REX rex;
	OperandSize op_size = get_op_size();

	switch (op_size) {

	case GPInstruction::OS_32:
	{
		code += 0xd9;
		break;
	}
	case GPInstruction::OS_64:
	{
		rex + REX::W;
		code += rex;
		code += 0xdd;
		break;
	}
	default:
	{
		ABORT_MSG(this << ": Operand size for FSTP not supported",
			"size: " << get_op_size() * 8 << "bit");
	}
	}


	// modmr + imm
	s4 index = slot->get_index() * 8;
	if (fits_into<s1>(index)) {
		code += get_modrm_u1(0x1,3,RBP.get_index());
		code += (u1) 0xff & (index >> 0x00);
	} else {
		code += get_modrm_u1(0x2,3,RBP.get_index());
		code += (u1) 0xff & (index >> 0x00);
		code += (u1) 0xff & (index >> 0x08);
		code += (u1) 0xff & (index >> 0x10);
		code += (u1) 0xff & (index >> 0x18);
	}
	add_CodeSegmentBuilder(CM,code);


}

void FFREEInst::emit(CodeMemory* CM) const {

	CodeFragment code = CM->get_CodeFragment(2);
	code[0] = 0xdd;
	code[1] = 0xc0 + fpureg->index;
}

void FINCSTPInst::emit(CodeMemory* CM) const {

	CodeFragment code = CM->get_CodeFragment(2);
	code[0] = 0xd9;
	code[1] = 0xf7;
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
