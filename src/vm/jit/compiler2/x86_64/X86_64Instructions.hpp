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

#ifndef _JIT_COMPILER2_X86_64INSTRUCTIONS
#define _JIT_COMPILER2_X86_64INSTRUCTIONS

#include "vm/jit/compiler2/x86_64/X86_64.hpp"
#include "vm/jit/compiler2/x86_64/X86_64Cond.hpp"
#include "vm/jit/compiler2/x86_64/X86_64Register.hpp"
#include "vm/jit/compiler2/x86_64/X86_64ModRMOperand.hpp"
#include "vm/jit/compiler2/x86_64/X86_64MachineMethodDescriptor.hpp"
#include "vm/jit/compiler2/MachineInstruction.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/DataSegment.hpp"
#include "vm/types.hpp"

namespace cacao {
// forward declaration
class Patcher;

namespace jit {
namespace compiler2 {

namespace x86_64 {

/**
 * @addtogroup x86_64_operand x86_64 Operand Helper
 *
 * These wrapper classes are used to enforce the correct
 * ordering of the parameters.
 *
 * See Item 18 in "Efficient C++" @cite Meyers2005.
 *
 * @{
 */

/**
 * Simple wrapper for the operand of an
 * single operand x86_64 instruction.
 */
struct SrcOp {
	MachineOperand *op;
	explicit SrcOp(MachineOperand *op) : op(op) {}
};

struct SrcMemOp {
	MachineOperand *op;
	explicit SrcMemOp(StackSlot *op) : op(op) {

	}
	explicit SrcMemOp(ManagedStackSlot *op) : op(op) {

	}
};


struct DstMemOp {
	MachineOperand *op;
	explicit DstMemOp(StackSlot *op) : op(op) {

	}
	explicit DstMemOp(ManagedStackSlot *op) : op(op) {

	}
};


/**
 * Simple wrapper for first operand of an
 * x86_64 instruction.
 */
struct Src1Op {
	MachineOperand *op;
	explicit Src1Op(MachineOperand *op) : op(op) {}
};
/**
 * Simple wrapper for second operand of an
 * x86_64 instruction.
 */
struct Src2Op {
	MachineOperand *op;
	explicit Src2Op(MachineOperand *op) : op(op) {}
};
/**
 * Simple wrapper for first operand of an
 * x86_64 instruction which is also used for the result.
 */
struct DstSrc1Op {
	MachineOperand *op;
	explicit DstSrc1Op(MachineOperand *op) : op(op) {}
};

struct DstSrc2Op {
	MachineOperand *op;
	explicit DstSrc2Op(MachineOperand *op) :
			op(op) {
	}
};

/**
 * Simple wrapper for first operand of an
 * x86_64 instruction which is also used for the result.
 */
struct DstSrcOp {
	MachineOperand *op;
	explicit DstSrcOp(MachineOperand *op) : op(op) {}
};

/**
 * Simple wrapper for destination of an
 * x86_64 instruction.
 */
struct DstOp {
	MachineOperand *op;
	explicit DstOp(MachineOperand *op) : op(op) {}
};

class ModRMOperand {
public:
	enum ScaleFactor {
	  Scale1 = 0,
	  Scale2 = 1,
	  Scale4 = 2,
	  Scale8 = 3
	};
	ScaleFactor scale;
	MachineOperand *index;
	MachineOperand *base;
	int32_t disp;
	/// constructor. base and disp only
	explicit ModRMOperand(const BaseOp& base, int32_t disp=0)
			: scale(Scale1), index(&NoOperand), base(base.op), disp(disp) {}
	/// constructor. Full
	ModRMOperand(ScaleFactor scale, const IndexOp &index, const BaseOp& base, int32_t disp=0)
			: scale(scale), index(index.op), base(base.op), disp(disp) {}
	/// constructor. Full (with Type::TypeID for scale)
	ModRMOperand(Type::TypeID type, const IndexOp &index, const BaseOp& base, int32_t disp=0)
			: scale(get_scale(type)), index(index.op), base(base.op), disp(disp) {}

	/// covert type to scale
	static ScaleFactor get_scale(Type::TypeID type) {
		switch (type) {
		case Type::ByteTypeID:
			return Scale1;
		case Type::ShortTypeID:
			return Scale2;
		case Type::IntTypeID:
		case Type::FloatTypeID:
			return Scale4;
		case Type::LongTypeID:
		case Type::DoubleTypeID:
		case Type::ReferenceTypeID:
			return Scale8;
		default:
			break;
		}
		ABORT_MSG("type not supported", "x86_64 ModRMOperand::get_scale() type: " << type);
		return Scale1;
	}

	/// covert type to scale
	static ScaleFactor get_scale(int32_t value) {
		switch (value) {
		case 1:
			return Scale1;
		case 2:
			return Scale2;
		case 4:
			return Scale4;
		case 8:
			return Scale8;
		default:
			break;
		}
		ABORT_MSG("constant value not supported", "x86_64 ModRMOperand::get_scale() value: " << value);
		return Scale1;
	}
};
class ModRMOperandDesc {
public:
	ModRMOperand::ScaleFactor scale;
	MachineOperandDesc &index;
	MachineOperandDesc &base;
	int32_t disp;
	/// constructor
	ModRMOperandDesc(ModRMOperand::ScaleFactor scale, MachineOperandDesc &index, MachineOperandDesc &base, int32_t disp=0)
			: scale(scale), index(index), base(base), disp(disp) {}
};

OStream& operator<<(OStream &OS,const ModRMOperandDesc &modrm);


/// Simple wrapper for a ModRM destination operand
struct DstModRM {
	ModRMOperand &op;
	explicit DstModRM(ModRMOperand &op) : op(op) {}
};
/// Simple wrapper for a ModRM source operand
struct SrcModRM {
	ModRMOperand &op;
	explicit SrcModRM(ModRMOperand &op) : op(op) {}
};
/**
 * @}
 */

/**
 * Superclass for general purpose register instruction
 */
class X86_64Instruction : public MachineInstruction {
public:
	X86_64Instruction(const char *name, MachineOperand* result,
		std::size_t num_operands) :
			MachineInstruction(name, result, num_operands) {}
};
/**
 * Superclass for general purpose register instruction
 */
class GPInstruction : public X86_64Instruction {
public:
	enum OperandSize {
		OS_8  = 1,
		OS_16 = 2,
		OS_32 = 4,
		OS_64 = 8,
		NO_SIZE = 0
	};
	enum OperandType {
		// Reg
		Reg8,
		Reg16,
		Reg32,
		Reg64,
		// Memory
		Mem8,
		Mem16,
		Mem32,
		Mem64,
		//
		NO_RESULT
	};
	enum OpEncoding {
		// Immediate to Register
		Reg8Imm8,
		Reg16Imm16,
		Reg32Imm32,
		Reg64Imm64,
		// Immediate to Memory
		MemImm8,
		MemImm16,
		MemImm32,
		MemImm64,
		// Immediate 8 to Register
		Reg16Imm8,
		Reg32Imm8,
		Reg64Imm8,
		// Register to Register
		RegReg8,
		RegReg16,
		RegReg32,
		RegReg64,
		// Memory to Register
		RegMem8,
		RegMem16,
		RegMem32,
		RegMem64,
		// Register to Memory
		MemReg8,
		MemReg16,
		MemReg32,
		MemReg64,
		//
		NO_ENCODING
	};
private:
	OperandSize op_size;

public:
	GPInstruction(const char * name, MachineOperand* result,
		OperandSize op_size, std::size_t num_operands, bool is_two_address_mode = false) :
			X86_64Instruction(name, result, num_operands),
			op_size(op_size) {
		if (is_two_address_mode) {
			assert(num_operands >= 1 && "Two-Address mode for no operands??");

			MachineRegisterRequirementUPtrTy requirement = std::make_unique<MachineRegisterTwoAddressRequirement>(&operands[0]);
			get_result().set_MachineRegisterRequirement(requirement);
		}
	}

	OperandSize get_op_size() const { return op_size; }
};

GPInstruction::OperandSize get_operand_size_from_Type(Type::TypeID type);
void emit_nop(CodeFragment code, int length);

/**
 * Move super instruction
 */
class MoveInst : public GPInstruction {
public:
	MoveInst( const char* name,
			MachineOperand *src,
			MachineOperand *dst,
			OperandSize op_size)
			: GPInstruction(name, dst, op_size, 1) {
		set_operand(0,src);
		finalize_operands();
	}
	virtual bool is_move() const { return true; }
};

GPInstruction::OperandSize get_OperandSize_from_Type(const Type::TypeID type);

// Just a "meta" instruction that produces a result for each register parameter
// that is bound to the correct machine register
class ParamInst : public GPInstruction {
public:
	ParamInst(MachineOperand* dst, MachineOperand* required_operand)
		: GPInstruction("X86_64Parameter", dst,
		  get_operand_size_from_Type(dst->get_type()), 0) {

		if (required_operand->is_Register()) {
			auto machine_reg = cast_to<X86_64Register>(required_operand);
			auto requirement = std::make_unique<MachineRegisterRequirement>(machine_reg);
			get_result().set_MachineRegisterRequirement(requirement);
		}		
	}

	void add_result(MachineOperand* dst, MachineOperand* required_operand) {
		MachineInstruction::add_result(dst);

		if (required_operand->is_Register()) {
			auto machine_reg = cast_to<X86_64Register>(required_operand);
			auto requirement = std::make_unique<MachineRegisterRequirement>(machine_reg);
			results.back().set_MachineRegisterRequirement(requirement);
		}
	}
	
	virtual void emit(CodeMemory* CM) const {}
};


/**
 * This abstract class represents a x86_64 ALU instruction.
 *
 *
 */
class ALUInstruction : public GPInstruction {
private:
	u1 alu_id;

#if 0
void emit_impl_I(CodeMemory* CM, Immediate* imm) const;
void emit_impl_RI(CodeMemory* CM, NativeRegister* src1,
	Immediate* imm) const;
void emit_impl_MR(CodeMemory* CM, NativeRegister* src1,
	NativeRegister* src2) const;
void emit_impl_RR(CodeMemory* CM, NativeRegister* src1,
	NativeRegister* src2) const;
#endif

public:
	/**
	 * @param alu_id this parameter is used to identify the
	 * instruction. The mapping is as follows:
	 * - 0 ADD
	 * - 1 OR
	 * - 2 ADC
	 * - 3 SBB
	 * - 4 AND
	 * - 5 SUB
	 * - 6 XOR
	 * - 7 CMP
	 */
	ALUInstruction(const char * name, const DstSrc1Op &dstsrc1,
			const Src2Op &src2, OperandSize op_size, u1 alu_id)
			: GPInstruction(name, dstsrc1.op, op_size, 2, true) , alu_id(alu_id) {
		assert(alu_id < 8);
		set_operand(0,dstsrc1.op);
		set_operand(1,src2.op);
		finalize_operands();
	}
	ALUInstruction(const char * name, const Src1Op &src1,
			const Src2Op &src2, OperandSize op_size, u1 alu_id)
			: GPInstruction(name, &NoOperand, op_size, 2, true) , alu_id(alu_id) {
		assert(alu_id < 8);
		set_operand(0,src1.op);
		set_operand(1,src2.op);
		finalize_operands();
	}
	ALUInstruction(const char * name, const DstOp& dst, const Src1Op& src1,
			const Src2Op& src2, OperandSize op_size, u1 alu_id)
			: GPInstruction(name, dst.op, op_size, 2, true), alu_id(alu_id) {
		assert(alu_id < 8);
		set_operand(0, src1.op);
		set_operand(1, src2.op);
		finalize_operands();
	}

	/**
	 * This must be implemented by subclasses.
	 */
	virtual void emit(CodeMemory* CM) const;

	virtual bool accepts_immediate(std::size_t i, Immediate *imm) const {
		if (i != 1) return false;
		switch (imm->get_type()) {
		case Type::IntTypeID: return true;
		case Type::LongTypeID: return fits_into<s4>(imm->get_Long());
		default: break;
		}
		return false;
	}
};




// ALU instructions
class AddInst : public ALUInstruction {
public:
	AddInst(const Src2Op &src2, const DstSrc1Op &dstsrc1,
		OperandSize op_size)
			: ALUInstruction("X86_64AddInst", dstsrc1, src2, op_size, 0) {
	}
	AddInst(const Src1Op &src1, const Src2Op &src2, const DstOp &dst,
		OperandSize op_size)
			: ALUInstruction("X86_64AddInst", dst, src1, src2, op_size, 0) {
	}
};
class OrInst : public ALUInstruction {
public:
	OrInst(const Src2Op &src2, const DstSrc1Op &dstsrc1,
		OperandSize op_size)
			: ALUInstruction("X86_64OrInst", dstsrc1, src2, op_size, 1) {
	}
};
class AdcInst : public ALUInstruction {
public:
	AdcInst(const Src2Op &src2, const DstSrc1Op &dstsrc1,
		OperandSize op_size)
			: ALUInstruction("X86_64AdcInst", dstsrc1, src2, op_size, 2) {
	}
};
class SbbInst : public ALUInstruction {
public:
	SbbInst(const Src2Op &src2, const DstSrc1Op &dstsrc1,
		OperandSize op_size)
			: ALUInstruction("X86_64SbbInst", dstsrc1, src2, op_size, 3) {
	}
};
class AndInst : public ALUInstruction {
public:
	AndInst(const Src1Op &src1, const Src2Op &src2, const DstOp &dst,
		OperandSize op_size)
			: ALUInstruction("X86_64AndInst", dst, src1, src2, op_size, 4) {
	}
};
class SubInst : public ALUInstruction {
public:
	SubInst(const Src2Op &src2, const DstSrc1Op &dstsrc1,
		OperandSize op_size)
			: ALUInstruction("X86_64SubInst", dstsrc1, src2, op_size, 5) {
	}
	SubInst(const DstOp& dst, const Src1Op& src1, const Src2Op& src2, 
		OperandSize op_size)
			:ALUInstruction("X86_64SubInst", dst, src1, src2, op_size, 5) {}
};
class XorInst : public ALUInstruction {
public:
	XorInst(const Src2Op &src2, const DstSrc1Op &dstsrc1,
		OperandSize op_size)
			: ALUInstruction("X86_64XorInst", dstsrc1, src2, op_size, 6) {
	}
};
class CmpInst : public ALUInstruction {
public:
	/**
	 * TODO: return type actually is status-flags
	 */
	CmpInst(const Src2Op &src2, const Src1Op &src1,
		OperandSize op_size)
			: ALUInstruction("X86_64CmpInst", src1, src2, op_size, 7) {
	}
};


class PatchInst : public X86_64Instruction {
private:
	Patcher *patcher;
public:
	PatchInst(Patcher *patcher)
			: X86_64Instruction("X86_64PatchInst", &NoOperand, 0),
			patcher(patcher) {}
	virtual void emit(CodeMemory* CM) const;
	virtual void link(CodeFragment &CF) const;
};


class EnterInst : public X86_64Instruction {
private:
	u2 framesize;
public:
	EnterInst(u2 framesize)
			: X86_64Instruction("X86_64EnterInst", &NoOperand, 0),
			framesize(framesize) {}
	virtual void emit(CodeMemory* CM) const;
};

class LeaveInst : public X86_64Instruction {
public:
	LeaveInst()
			: X86_64Instruction("X86_64LeaveInst", &NoOperand, 0) {}
	virtual void emit(CodeMemory* CM) const;
};

class BreakInst : public X86_64Instruction {
public:
	BreakInst()
			: X86_64Instruction("X86_64BreakInst", &NoOperand, 0) {}
	virtual void emit(CodeMemory* CM) const;
};

class IMulInst : public GPInstruction {
public:
	IMulInst(const Src2Op &src2, const DstSrc1Op &dstsrc1,
		OperandSize op_size)
			: GPInstruction("X86_64IMulInst", dstsrc1.op, op_size, 2, true) {
		set_operand(0,dstsrc1.op);
		set_operand(1,src2.op);
		finalize_operands();
	}
	IMulInst(const DstOp& dst, const Src1Op& src1, const Src2Op& src2,
		OperandSize op_size)
			: GPInstruction("X86_64IMulInst", dst.op, op_size, 2, true) {
		set_operand(0, src1.op);
		set_operand(1, src2.op);
		finalize_operands();
	}

	virtual void emit(CodeMemory* CM) const;
};

class IMulImmInst : public GPInstruction {
public:
	IMulImmInst(const Src1Op &src1, const Src2Op &src2, const DstOp &dst,
		OperandSize op_size)
			: GPInstruction("X86_64IMulImmInst", dst.op, op_size, 3) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
		operands[2].op = dst.op;
	}
	virtual void emit(CodeMemory* CM) const;
};

class IDivInst: public GPInstruction {
public:
	/**
	 * The destination operands are only used to guide
	 * the register allocator. The user must ensure that the values
	 * are moved to the desired destination registers afterwards (e.g. by inserting a move)
	 */
	IDivInst(const Src1Op &src1, const Src2Op &src2, const Src2Op &src3, const DstOp &dst1, const DstOp &dst2, OperandSize op_size) :
			GPInstruction("X86_64IDivInst", dst1.op, op_size, 3) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
		operands[2].op = src3.op;

		add_result(dst2.op);

		auto op1_requirement = std::make_unique<MachineRegisterRequirement>(&RDX);
		operands[1].set_MachineRegisterRequirement(op1_requirement);

		auto op2_requirement = std::make_unique<MachineRegisterRequirement>(&RAX);
		operands[2].set_MachineRegisterRequirement(op2_requirement);

		auto res0_requirement = std::make_unique<MachineRegisterRequirement>(&RAX);
		results[0].set_MachineRegisterRequirement(res0_requirement);

		auto res1_requirement = std::make_unique<MachineRegisterRequirement>(&RDX);
		results[1].set_MachineRegisterRequirement(res1_requirement);

	}
	virtual void emit(CodeMemory* CM) const;
};

class CDQInst : public GPInstruction {
public:
	CDQInst(const SrcOp &src, const DstOp &dst1, const DstOp &dst2, OperandSize op_size) : GPInstruction("X86_64CDQInst", dst1.op, NO_SIZE, 1) {
		operands[0].op = src.op;

		add_result(dst2.op);

		auto op1_requirement = std::make_unique<MachineRegisterRequirement>(&RAX);
		operands[0].set_MachineRegisterRequirement(op1_requirement);

		auto res1_requirement = std::make_unique<MachineRegisterRequirement>(&RDX);
		results[0].set_MachineRegisterRequirement(res1_requirement);

		auto res2_requirement = std::make_unique<MachineRegisterRequirement>(&RAX);
		results[1].set_MachineRegisterRequirement(res2_requirement);		
	}

	virtual void emit(CodeMemory* CM) const;
};

class RetInst : public GPInstruction {
private:
	bool emit_leave = false;
public:
	/// void return
	RetInst() : GPInstruction("X86_64RetInst", &NoOperand, NO_SIZE, 0) {}
	//RetInst(OperandSize op_size)
	/**
	 * Non-void return. The source operand is only used to guide
	 * the register allocator. The user must ensure that the value
	 * really is in the correct register (e.g. by inserting a move)
	 */
	RetInst(OperandSize op_size, const SrcOp &src)
			: GPInstruction("X86_64RetInst", &NoOperand, op_size, 1) {
		operands[0].op = src.op;
		
		X86_64Register* result_reg = nullptr;
		if (src.op->get_type() == Type::FloatTypeID || src.op->get_type() == Type::DoubleTypeID)
			result_reg = &XMM0;
		else
			result_reg = &RAX;

		auto requirement = std::make_unique<MachineRegisterRequirement>(result_reg);
		operands[0].set_MachineRegisterRequirement(requirement);
	}
	void set_emit_leave(bool emit) { emit_leave = emit; }
	virtual bool is_end() const { return true; }
	virtual void emit(CodeMemory* CM) const;
};


class NegInst : public GPInstruction {
public:
	NegInst(const SrcOp &src, const DstOp &dst, OperandSize op_size)
			: GPInstruction("X86_64NegInst", dst.op, op_size, 1, true) {
		operands[0].op = src.op;
	}
	virtual void emit(CodeMemory* CM) const;
};

class CallInst : public GPInstruction {
public:
	CallInst(const SrcOp &src, const DstOp &dst, std::size_t argc, const MachineMethodDescriptor& MMD, Backend* backend)
			: GPInstruction("X86_64CallInst", dst.op, OS_64, 1 + argc) {
		auto MOF = backend->get_JITData()->get_MachineOperandFactory();
		operands[0].op = src.op;
		
		// For void type, use a temporary new vreg, that can be discarded
		if (dst.op->get_type() == Type::VoidTypeID) {
			get_result().op = MOF->CreateVirtualRegister(Type::LongTypeID);
		}

		X86_64Register* result_reg = nullptr;
		X86_64Register* aux_reg = nullptr;

		if (dst.op->get_type() != Type::FloatTypeID && dst.op->get_type() != Type::DoubleTypeID) {
			result_reg = &RAX;
			aux_reg = &XMM0;
		} else {
			result_reg = &XMM0;
			aux_reg = &RAX;
		}

		// Result must be in result_reg, but aux_reg should also be caller saved
		auto result_requirement = std::make_unique<MachineRegisterRequirement>(result_reg);
		get_result().set_MachineRegisterRequirement(result_requirement);

		add_result(MOF->CreateVirtualRegister(Type::DoubleTypeID));
		auto result2_requirement = std::make_unique<MachineRegisterRequirement>(aux_reg);
		results[1].set_MachineRegisterRequirement(result2_requirement);

		// Set argument requirements
		for (unsigned i = 1; i <= argc; ++i) {
			if (MMD[i-1]->is_StackSlot())
				continue;
			auto op_requirement = std::make_unique<MachineRegisterRequirement>(cast_to<X86_64Register>(MMD[i-1]));
			operands[i].set_MachineRegisterRequirement(op_requirement);
		}

		// TODO: Adding virtual registers as results and binding them to
		//       their respective caller saved machine registers is the same for
		//       ALL call instructions. We can probably do better here

		// Add all caller saved registers as results of the call
		// Start at 1 since RAX is the first register and it is already bound
		for (unsigned i = 1; i < IntegerCallerSavedRegistersSize ; ++i) {
			add_result(MOF->CreateVirtualRegister(Type::LongTypeID));
			
			auto native_reg = IntegerCallerSavedRegisters[i];
			auto requirement = std::make_unique<MachineRegisterRequirement>(native_reg);
			results[i + 1].set_MachineRegisterRequirement(requirement);
		}

		auto base = IntegerCallerSavedRegistersSize;
		for (unsigned i = 1; i < FloatCallerSavedRegistersSize; ++i) {
			add_result(MOF->CreateVirtualRegister(Type::DoubleTypeID));

			auto native_reg = FloatCallerSavedRegisters[i];
			auto requirement = std::make_unique<MachineRegisterRequirement>(native_reg);
			results[base + i].set_MachineRegisterRequirement(requirement);
		}
	}
	virtual void emit(CodeMemory* CM) const;
};

class MovInst : public MoveInst {
private:
	bool forceSameRegisters;
public:
	MovInst(const SrcOp &src, const DstOp &dst,
		GPInstruction::OperandSize op_size, bool forceSameRegisters = false)
			: MoveInst("X86_64MovInst", src.op, dst.op, op_size), forceSameRegisters(forceSameRegisters)  {
	}

	virtual bool accepts_immediate(std::size_t i, Immediate *imm) const {
		return true;
	}
	virtual void emit(CodeMemory* CM) const;
};

/// Swaps 2 registers, or one memory location with a register using 'xchg'
class SwapInst : public GPInstruction {
public:
	SwapInst(const Src1Op& src1, const Src2Op& src2, GPInstruction::OperandSize op_size)
		: GPInstruction("X86_64SwapInst", &NoOperand, op_size, 2) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}

	virtual void emit(CodeMemory* CM) const;
};

/**
 * Load from pointer
 *
 * @todo merge with MovInst
 */
class MovModRMInst : public GPInstruction {
public:
	/// constructor. full arguments
	MovModRMInst(const DstOp &dst, GPInstruction::OperandSize op_size, const SrcModRM &src, bool floatingpoint)
			: GPInstruction("X86_64MovModRMInst", dst.op, op_size, 2),
				modrm(ModRMOperandDesc(src.op.scale,operands[Index],operands[Base],src.op.disp)),
				enc(RM), floatingpoint(floatingpoint) {
		operands[Base].op = src.op.base;
		operands[Index].op = src.op.index;
	}
	MovModRMInst(const SrcOp &src, GPInstruction::OperandSize op_size, const DstModRM &dst, bool floatingpoint)
			: GPInstruction("X86_64MovModRMInst", &NoOperand, op_size, 3),
				modrm(ModRMOperandDesc(dst.op.scale,operands[Index],operands[Base],dst.op.disp)),
				enc(MR), floatingpoint(floatingpoint) {
		operands[Base].op = dst.op.base;
		operands[Index].op = dst.op.index;
		operands[Value].op = src.op;
	}
	//virtual bool is_move() const { return true; }
	virtual void emit(CodeMemory* CM) const;
	virtual OStream& print_operands(OStream &OS) const;
	virtual OStream& print_result(OStream &OS) const;
private:
	enum OpIndex {
		Base = 0,
		Index = 1,
		Value = 2
	};
	enum OpEnc {
	  MR,
	  RM
	};
	ModRMOperandDesc modrm;
	OpEnc enc;
	bool floatingpoint;
};



class LEAInst : public GPInstruction {
private:
public:
	LEAInst( const DstOp &dst, OperandSize op_size, const SrcOp src )
			: GPInstruction("X86_64LEAInst", dst.op, op_size, 1) {
		set_operand(0,src.op);
		finalize_operands();

	}
	virtual void emit(CodeMemory* CM) const;
};

/**
 * Move with Sign-Extension
 */
class MovSXInst : public MoveInst {
GPInstruction::OperandSize from;
GPInstruction::OperandSize to;
public:
	MovSXInst(const SrcOp &src, const DstOp &dst,
			GPInstruction::OperandSize from, GPInstruction::OperandSize to)
				: MoveInst("X86_64MovSXInst", src.op, dst.op, to),
				from(from), to(to) {}
	virtual void emit(CodeMemory* CM) const;
};

/**
 * Move data seg to register
 */
class MovDSEGInst : public GPInstruction {
private:
	// mutable because changed in emit (which is const)
	DataSegment::IdxTy data_index;
public:
	MovDSEGInst(const DstOp &dst, DataSegment::IdxTy &data_index)
			: GPInstruction("X86_64MovDSEGInst", dst.op, get_operand_size_from_Type(dst.op->get_type()), 0),
			data_index(data_index) {}
	virtual void emit(CodeMemory* CM) const;
	virtual void link(CodeFragment &CF) const;
};

class JumpInst : public MachineJumpInst {
public:
	JumpInst(MachineBasicBlock *target) : MachineJumpInst("X86_64JumpInst") {
		successors.push_back(target);
	}
	virtual bool is_jump() const {
		return true;
	}
	void set_target(MachineBasicBlock *target) {
		successors.front() = target;
	}
	virtual void emit(CodeMemory* CM) const;
	virtual void link(CodeFragment &CF) const;
};

class CondJumpInst : public X86_64Instruction {
private:
	Cond::COND cond;
	/// jump to the else target
	/// @todo change this!
	mutable JumpInst jump;
public:
	CondJumpInst(Cond::COND cond, MachineBasicBlock *then_target, MachineBasicBlock *else_target)
			: X86_64Instruction("X86_64CondJumpInst", &NoOperand, 0),
			  cond(cond), jump(else_target) {
		successors.push_back(then_target);
		successors.push_back(else_target);
	}
	virtual bool is_jump() const {
		return true;
	}
	virtual void emit(CodeMemory* CM) const;
	virtual void link(CodeFragment &CF) const;
	virtual OStream& print_successor_label(OStream &OS,std::size_t index) const;
	MachineBasicBlock* get_then() const {
		return successor_front();
	}
	MachineBasicBlock* get_else() const {
		return successor_back();
	}
};

/// A TrapInst represents a hardware trap.
class TrapInst : public X86_64Instruction {
private:

	/// A trap number as defined in x86_64/md-trap.hpp
	s4 trap;

public:

	/// Construct a TrapInst.
	///
	/// @param trap  A trap number as defined in x86_64/md-trap.hpp.
	/// @param SrcOp TODO
	TrapInst(s4 trap, const SrcOp &index)
			: X86_64Instruction("X86_64TrapInst", &NoOperand, 1), trap(trap) {
		operands[0].op = index.op;
	}

	virtual bool is_trap() const { return true; }
	virtual void emit(CodeMemory* CM) const;
};

class CondTrapInst : public X86_64Instruction {
private:
	Cond::COND cond;
	s4 trap;
public:
	CondTrapInst(Cond::COND cond, s4 trap, const SrcOp &index)
			: X86_64Instruction("X86_64CondTrapInst", &NoOperand, 1), cond(cond), trap(trap) {
		operands[0].op = index.op;
	}
	virtual bool is_trap() const { return true; }
	virtual void emit(CodeMemory* CM) const;
};

class CMovInst : public GPInstruction {
private:
	Cond::COND cond;
public:
	CMovInst(Cond::COND cond, const Src1Op &src1, const Src2Op &src2, const DstOp &dst, GPInstruction::OperandSize op_size)
			: GPInstruction("X86_64CMovInst", dst.op, op_size, 2, true), cond(cond) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}
	virtual void emit(CodeMemory* CM) const;
};


class IndirectJumpInst : public X86_64Instruction {
public:
	IndirectJumpInst(const SrcOp &src)
			: X86_64Instruction("X86_64IndirectJumpInst", &NoOperand, 1) {
		operands[0].op = src.op;
	}
	virtual bool is_jump() const {
		return true;
	}
	virtual void emit(CodeMemory* CM) const;
	void add_target(MachineBasicBlock *MBB) {
		successors.push_back(MBB);
	}
};
// Double & Float operations
/**
 * Convert Dword Integer to Scalar Double-Precision FP Value
 */
class CVTSI2SDInst : public MoveInst {
	GPInstruction::OperandSize from;
public:
	CVTSI2SDInst(const SrcOp &src, const DstOp &dst,
			GPInstruction::OperandSize from, GPInstruction::OperandSize to)
				: MoveInst("X86_64CVTSI2SDInst", src.op, dst.op, to),
				from(from) {}
	virtual void emit(CodeMemory* CM) const;
};

/**
 * Convert Dword Integer to Scalar Single-Precision FP Value
 */
class CVTSI2SSInst: public MoveInst {
	GPInstruction::OperandSize from;
public:
	CVTSI2SSInst(const SrcOp &src, const DstOp &dst,
			GPInstruction::OperandSize from, GPInstruction::OperandSize to) :
			MoveInst("X86_64CVTSI2SSInst", src.op, dst.op, to), from(from) {
	}
	virtual void emit(CodeMemory* CM) const;
};

/**
 * Convert with truncation Scalar Single-FP Value to DW Integer
 */
class CVTTSS2SIInst: public MoveInst {
	GPInstruction::OperandSize to;

public:
	CVTTSS2SIInst(const SrcOp &src, const DstOp &dst,
			GPInstruction::OperandSize from, GPInstruction::OperandSize to) :
			MoveInst("X86_64CVTTSS2SIInst", src.op, dst.op, to), to(to) {
	}
	virtual void emit(CodeMemory* CM) const;
};

class CVTTSD2SIInst: public MoveInst {
	GPInstruction::OperandSize to;

public:
	CVTTSD2SIInst(const SrcOp &src, const DstOp &dst,
			GPInstruction::OperandSize from, GPInstruction::OperandSize to) :
			MoveInst("X86_64CVTTSD2SIInst", src.op, dst.op, to), to(to) {
	}
	virtual void emit(CodeMemory* CM) const;
};


class CVTSS2SDInst: public MoveInst {

public:
	CVTSS2SDInst(const SrcOp &src, const DstOp &dst,
			GPInstruction::OperandSize to) :
			MoveInst("X86_64CVTSS2SDInst", src.op, dst.op, to){
	}
	virtual void emit(CodeMemory* CM) const;
};

class CVTSD2SSInst: public MoveInst {

public:
	CVTSD2SSInst(const SrcOp &src, const DstOp &dst,
			GPInstruction::OperandSize from, GPInstruction::OperandSize to) :
			MoveInst("X86_64CVTSD2SSInst", src.op, dst.op, to){
	}
	virtual void emit(CodeMemory* CM) const;
};

class FLDInst: public GPInstruction {

public:
	FLDInst(const SrcMemOp &src, GPInstruction::OperandSize op_size) :
			GPInstruction("X86_64FLDInst", src.op, op_size, 1) {
		operands[0].op = src.op;

	}
	virtual void emit(CodeMemory* CM) const;
};

class FSTPInst: public GPInstruction {

public:
	FSTPInst(const DstMemOp &dst, GPInstruction::OperandSize op_size) :
			GPInstruction("X86_64FSTPInst", dst.op, op_size, 1) {
		operands[0].op = dst.op;
	}
	virtual void emit(CodeMemory* CM) const;
};

class FFREEInst: public X86_64Instruction {
private:
	FPUStackRegister *fpureg;
public:
	FFREEInst(FPUStackRegister *fpureg) :
		X86_64Instruction("X86_64FFREEInst", &NoOperand, 0), fpureg(fpureg) {}
	virtual void emit(CodeMemory* CM) const;
};

class FINCSTPInst: public X86_64Instruction {
public:
	FINCSTPInst() :
		X86_64Instruction("X86_64FINCSTPInst", &NoOperand, 0) {}
	virtual void emit(CodeMemory* CM) const;
};

// compare
class UCOMISInst : public GPInstruction {
protected:
	UCOMISInst(const char* name, const Src2Op &src2, const Src1Op &src1,
			GPInstruction::OperandSize op_size)
			: GPInstruction(name, &NoOperand, op_size, 2) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}
public:
	virtual void emit(CodeMemory* CM) const;
};

class UCOMISSInst : public UCOMISInst{
public:
	UCOMISSInst(const Src2Op &src2, const Src1Op &src1)
		: UCOMISInst("X86_64UCOMISSInst",src2,src1,GPInstruction::OS_32) {}
};
class UCOMISDInst : public UCOMISInst{
public:
	UCOMISDInst(const Src2Op &src2, const Src1Op &src1)
		: UCOMISInst("X86_64UCOMISDInst",src2,src1,GPInstruction::OS_64) {}
};

// XORP
class XORPInst : public GPInstruction {
protected:
	XORPInst(const char* name, const Src1Op &src1, const Src2Op &src2, const DstOp &dst,
			GPInstruction::OperandSize op_size)
			: GPInstruction(name, dst.op, op_size, 2, true)
	{
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}
public:
	virtual void emit(CodeMemory* CM) const;
};

class XORPSInst : public XORPInst{
public:
	XORPSInst(const Src1Op &src1, const Src2Op &src2, const DstOp &dst)
		: XORPInst("X86_64XORPSInst",src1,src2,dst,GPInstruction::OS_32) {}
};
class XORPDInst : public XORPInst{
public:
	XORPDInst(const Src1Op &src1, const Src2Op &src2, const DstOp &dst)
		: XORPInst("X86_64XORPDInst",src1,src2,dst,GPInstruction::OS_64) {}
};


/// SSE Alu Instruction
class SSEAluInst : public GPInstruction {
private:
	u1 opcode;
protected:
	SSEAluInst(const char* name, const Src1Op &src1, const Src2Op &src2, const DstOp &dst,
			u1 opcode, GPInstruction::OperandSize op_size)
			: GPInstruction(name, dst.op, op_size, 2, true), opcode(opcode) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}
public:
	virtual void emit(CodeMemory* CM) const;
};
/// SSE Alu Instruction (Double-Precision)
class SSEAluSDInst : public SSEAluInst {
protected:	
	SSEAluSDInst(const char* name, const Src1Op &src1, const Src2Op &src2, 
		const DstOp &dst, u1 opcode)
			: SSEAluInst(name, src1, src2, dst, opcode, GPInstruction::OS_64) {}
};
/// SSE Alu Instruction (Single-Precision)
class SSEAluSSInst : public SSEAluInst {
protected:	
	SSEAluSSInst(const char* name, const Src1Op &src1, const Src2Op &src2, 
		const DstOp &dst, u1 opcode)
			: SSEAluInst(name, src1, src2, dst, opcode, GPInstruction::OS_32) {}
};

// Double-Precision

/// Add Scalar Double-Precision Floating-Point Values
class AddSDInst : public SSEAluSDInst {
public:
	AddSDInst(const Src1Op &src1, const Src2Op &src2, const DstOp &dst)
		: SSEAluSDInst("X86_64AddSDInst", src1, src2, dst, 0x58) {}
};
/// Multiply Scalar Double-Precision Floating-Point Values
class MulSDInst : public SSEAluSDInst {
public:
	MulSDInst(const Src1Op &src1, const Src2Op &src2, const DstOp &dst)
		: SSEAluSDInst("X86_64MulSDInst", src1, src2, dst, 0x59) {}
};
/// Subtract Scalar Double-Precision Floating-Point Values
class SubSDInst : public SSEAluSDInst {
public:
	SubSDInst(const Src1Op &src1, const Src2Op &src2, const DstOp &dst)
		: SSEAluSDInst("X86_64SubSDInst", src1, src2, dst, 0x5c) {}
};
/// Divide Scalar Double-Precision Floating-Point Values
class DivSDInst : public SSEAluSDInst {
public:
	DivSDInst(const Src1Op &src1, const Src2Op &src2, const DstOp &dst)
		: SSEAluSDInst("X86_64DivSDInst", src1, src2, dst, 0x5e) {}
};

class FPRemInst: public GPInstruction {
private:
public:

	FPRemInst(OperandSize op_size) :
			GPInstruction("X86_64FPRemInst", &NoOperand, op_size, 0) {
	}

	virtual void emit(CodeMemory* CM) const;
};

// SQRTSD Compute Square Root of Scalar Double-Precision Floating-Point Value 0x51
// MINSD Return Minimum Scalar Double-Precision Floating-Point Value 0x5d
// MAXSD Return Maximum Scalar Double-Precision Floating-Point Value 0x5f


// Single-Precision

/// Add Scalar Single-Precision Floating-Point Values
class AddSSInst : public SSEAluSSInst {
public:
	AddSSInst(const Src1Op &src1, const Src2Op &src2, const DstOp &dst)
		: SSEAluSSInst("X86_64AddSSInst", src1, src2, dst, 0x58) {}
};
/// Multiply Scalar Single-Precision Floating-Point Values
class MulSSInst : public SSEAluSSInst {
public:
	MulSSInst(const Src1Op &src1, const Src2Op &src2, const DstOp &dst)
		: SSEAluSSInst("X86_64MulSSInst", src1, src2, dst, 0x59) {}
};
/// Subtract Scalar Single-Precision Floating-Point Values
class SubSSInst : public SSEAluSSInst {
public:
	SubSSInst(const Src1Op &src1, const Src2Op &src2, const DstOp &dst)
		: SSEAluSSInst("X86_64SubSSInst", src1, src2, dst, 0x5c) {}
};
/// Divide Scalar Single-Precision Floating-Point Values
class DivSSInst : public SSEAluSSInst {
public:
	DivSSInst(const Src1Op &src1, const Src2Op &src2, const DstOp &dst)
		: SSEAluSSInst("X86_64DivSSInst", src1, src2, dst, 0x5e) {}
};
// SQRTSS Compute Square Root of Scalar Single-Precision Floating-Point Value 0x51
// MINSS Return Minimum Scalar Single-Precision Floating-Point Value 0x5d
// MAXSS Return Maximum Scalar Single-Precision Floating-Point Value 0x5f


class MovSDInst : public MoveInst {
public:
	MovSDInst(const SrcOp &src, const DstOp &dst)
			: MoveInst("X86_64MovSDInst", src.op, dst.op, OS_64) {}
	virtual void emit(CodeMemory* CM) const;
};


class MovImmSInst : public MoveInst {
private:
	// mutable because changed in emit (which is const)
	mutable DataSegment::IdxTy data_index;
protected:
	MovImmSInst(const char *name, const SrcOp &src, const DstOp &dst, GPInstruction::OperandSize op_size)
			: MoveInst(name, src.op, dst.op, op_size),
				data_index(0) {}
public:
	virtual void emit(CodeMemory* CM) const;
	virtual void link(CodeFragment &CF) const;
};

class MovImmSDInst : public MovImmSInst {
public:
	MovImmSDInst(const SrcOp &src, const DstOp &dst)
			: MovImmSInst("X86_64MovImmSDInst", src, dst, OS_64) {}
};
class MovImmSSInst : public MovImmSInst {
public:
	MovImmSSInst(const SrcOp &src, const DstOp &dst)
			: MovImmSInst("X86_64MovImmSSInst", src, dst, OS_32) {}
};

class MovSSInst : public MoveInst {
public:
	MovSSInst(const SrcOp &src, const DstOp &dst)
			: MoveInst("X86_64MovSSInst", src.op, dst.op, OS_32) {}
	virtual void emit(CodeMemory* CM) const;
};

class TestInst : public GPInstruction {
public:
	TestInst(const SrcOp &src1, SrcOp &src2, OperandSize op_size)
			: GPInstruction("X86_64TestInst", &NoOperand, op_size, 2) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}
	virtual void emit(CodeMemory* CM) const;
};

} // end namespace x86_64
} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_X86_64INSTRUCTIONS */


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
