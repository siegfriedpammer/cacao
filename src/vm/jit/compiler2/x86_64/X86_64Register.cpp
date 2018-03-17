/* src/vm/jit/compiler2/X86_64Register.cpp - X86_64Register

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

#include "vm/jit/compiler2/x86_64/X86_64Register.hpp"
#include "toolbox/Option.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

namespace option {
	Option<int> available_registers("AvailableRegisters","compiler2: set the number of available integer registers for register allocation (min 2)",-1,::cacao::option::xx_root());
}

namespace x86_64 {

NativeOperandFactory NOF;

FPUStackRegister ST0(0);
FPUStackRegister ST1(1);
FPUStackRegister ST2(2);
FPUStackRegister ST3(3);
FPUStackRegister ST4(4);
FPUStackRegister ST5(5);
FPUStackRegister ST6(6);
FPUStackRegister ST7(7);

const uint8_t GPRegister::base = 0;
const uint8_t SSERegister::base = 0;

GPRegister& RAX = *NOF.CreateGPRegister("RAX", 0x0, false, 0x0 * 8, 8);
GPRegister& RCX = *NOF.CreateGPRegister("RCX", 0x1, false, 0x1 * 8, 8);
GPRegister& RDX = *NOF.CreateGPRegister("RDX", 0x2, false, 0x2 * 8, 8);
GPRegister& RBX = *NOF.CreateGPRegister("RBX", 0x3, false, 0x3 * 8, 8);
GPRegister& RSP = *NOF.CreateGPRegister("RSP", 0x4, false, 0x4 * 8, 8);
GPRegister& RBP = *NOF.CreateGPRegister("RBP", 0x5, false, 0x5 * 8, 8);
GPRegister& RSI = *NOF.CreateGPRegister("RSI", 0x6, false, 0x6 * 8, 8);
GPRegister& RDI = *NOF.CreateGPRegister("RDI", 0x7, false, 0x7 * 8, 8);
GPRegister& R8 = *NOF.CreateGPRegister("R8", 0x0, true, 0x8 * 8, 8);
GPRegister& R9 = *NOF.CreateGPRegister("R9", 0x1, true, 0x9 * 8, 8);
GPRegister& R10 = *NOF.CreateGPRegister("R10", 0x2, true, 0xa * 8, 8);
GPRegister& R11 = *NOF.CreateGPRegister("R11", 0x3, true, 0xb * 8, 8);
GPRegister& R12 = *NOF.CreateGPRegister("R12", 0x4, true, 0xc * 8, 8);
GPRegister& R13 = *NOF.CreateGPRegister("R13", 0x5, true, 0xd * 8, 8);
GPRegister& R14 = *NOF.CreateGPRegister("R14", 0x6, true, 0xe * 8, 8);
GPRegister& R15 = *NOF.CreateGPRegister("R15", 0x7, true, 0xf * 8, 8);

GPRegister* IntegerArgumentRegisters[] = {&RDI, &RSI, &RDX, &RCX, &R8, &R9};
GPRegister* IntegerCallerSavedRegisters[] = {&RAX, &R11, &R10, &R9, &R8, &RCX, &RDX, &RSI, &RDI};
GPRegister* IntegerCalleeSavedRegisters[] = {&R12, &R13, &R14, &R15};

SSERegister& XMM0 = *NOF.CreateSSERegister("XMM0", 0x0, false, 0x0 * 16, 16);
SSERegister& XMM1 = *NOF.CreateSSERegister("XMM1", 0x1, false, 0x1 * 16, 16);
SSERegister& XMM2 = *NOF.CreateSSERegister("XMM2", 0x2, false, 0x2 * 16, 16);
SSERegister& XMM3 = *NOF.CreateSSERegister("XMM3", 0x3, false, 0x3 * 16, 16);
SSERegister& XMM4 = *NOF.CreateSSERegister("XMM4", 0x4, false, 0x4 * 16, 16);
SSERegister& XMM5 = *NOF.CreateSSERegister("XMM5", 0x5, false, 0x5 * 16, 16);
SSERegister& XMM6 = *NOF.CreateSSERegister("XMM6", 0x6, false, 0x6 * 16, 16);
SSERegister& XMM7 = *NOF.CreateSSERegister("XMM7", 0x7, false, 0x7 * 16, 16);
SSERegister& XMM8 = *NOF.CreateSSERegister("XMM8", 0x0, true, 0x8 * 16, 16);
SSERegister& XMM9 = *NOF.CreateSSERegister("XMM9", 0x1, true, 0x9 * 16, 16);
SSERegister& XMM10 = *NOF.CreateSSERegister("XMM10", 0x2, true, 0xa * 16, 16);
SSERegister& XMM11 = *NOF.CreateSSERegister("XMM11", 0x3, true, 0xb * 16, 16);
SSERegister& XMM12 = *NOF.CreateSSERegister("XMM12", 0x4, true, 0xc * 16, 16);
SSERegister& XMM13 = *NOF.CreateSSERegister("XMM13", 0x5, true, 0xd * 16, 16);
SSERegister& XMM14 = *NOF.CreateSSERegister("XMM14", 0x6, true, 0xe * 16, 16);
SSERegister& XMM15 = *NOF.CreateSSERegister("XMM15", 0x7, true, 0xf * 16, 16);

SSERegister* FloatArgumentRegisters[] = {&XMM0, &XMM1, &XMM2, &XMM3, &XMM4, &XMM5, &XMM6, &XMM7};
SSERegister* FloatCallerSavedRegisters[] = {&XMM0,  &XMM1, &XMM2,  &XMM3,  &XMM4,  &XMM5,
                                            &XMM6,  &XMM7,  &XMM8,  &XMM9,  &XMM10,
                                            &XMM11, &XMM12, &XMM13, &XMM14, &XMM15};

GPRegister* NativeOperandFactory::CreateGPRegister(const char* name,
                                                   unsigned index,
                                                   bool extented_gpr,
                                                   MachineOperand::IdentifyOffsetTy offset,
                                                   MachineOperand::IdentifySizeTy size)
{
	auto operand = new GPRegister(name, index, extented_gpr, offset, size);
	return (GPRegister*)register_ownership(operand);
}

SSERegister* NativeOperandFactory::CreateSSERegister(const char* name,
                                                     unsigned index,
                                                     bool extented_gpr,
                                                     MachineOperand::IdentifyOffsetTy offset,
                                                     MachineOperand::IdentifySizeTy size)
{
	auto operand = new SSERegister(name, index, extented_gpr, offset, size);
	return (SSERegister*)register_ownership(operand);
}

template <>
X86_64RegisterClass<X86_64Class::GP>::X86_64RegisterClass()
    : all(NOF.EmptySet()), caller_saved(NOF.EmptySet()), callee_saved(NOF.EmptySet())
{
	auto available_registers = ::cacao::jit::compiler2::option::available_registers;
	if (available_registers == -1) {
		for (const auto reg : IntegerCallerSavedRegisters) {
			caller_saved.add(reg);
		}

		for (const auto reg : IntegerCalleeSavedRegisters) {
			// callee_saved.add(reg);
		}
	} else if (available_registers >= 2 && available_registers <= 7) {
		for (std::size_t i = 0, e = available_registers - 1; i < e; ++i) {
			caller_saved.add(IntegerArgumentRegisters[i]);
		}
		caller_saved.add(&RAX);
	} else {
		ABORT_MSG("Invalid available registers", "Only return + arguments are available when reducing register count!");
	}

	all |= caller_saved;
	all |= callee_saved;
}

template <>
bool X86_64RegisterClass<X86_64Class::GP>::handles_type(Type::TypeID type) const
{
	return (type != Type::FloatTypeID && type != Type::DoubleTypeID);
}

template <>
Type::TypeID X86_64RegisterClass<X86_64Class::GP>::default_type() const 
{
	return Type::LongTypeID;
}

template <>
X86_64RegisterClass<X86_64Class::FP>::X86_64RegisterClass()
    : all(NOF.EmptySet()), caller_saved(NOF.EmptySet()), callee_saved(NOF.EmptySet())
{
	for (const auto reg : FloatCallerSavedRegisters) {
		caller_saved.add(reg);
	}

	all |= caller_saved;
}

template <>
bool X86_64RegisterClass<X86_64Class::FP>::handles_type(Type::TypeID type) const
{
	return (type == Type::FloatTypeID || type == Type::DoubleTypeID);
}

template <>
Type::TypeID X86_64RegisterClass<X86_64Class::FP>::default_type() const 
{
	return Type::DoubleTypeID;
}

} // end namespace x86_64
using namespace x86_64;

template <>
MachineOperandFactory* BackendBase<X86_64>::get_NativeFactory() const
{
	return &NOF;
}

template <>
OperandFile& BackendBase<X86_64>::get_OperandFile(OperandFile& OF, MachineOperand* MO) const
{
	Type::TypeID type = MO->get_type();

	assert(false && "Use the new RegisterInfo stuff please!");

	switch (type) {
		case Type::CharTypeID:
		case Type::ByteTypeID:
		case Type::ShortTypeID:
		case Type::IntTypeID:
		case Type::LongTypeID:
		case Type::ReferenceTypeID:
#if 1
			for (unsigned i = 0; i < IntegerCallerSavedRegistersSize; ++i) {
				//OF.push_back(new x86_64::NativeRegister(type, IntegerCallerSavedRegisters[i]));
			}
			//assert(OF.size() == IntegerCallerSavedRegistersSize);
			//OF.push_back(new x86_64::NativeRegister(type, &R12));
#else
			OF.push_back(new x86_64::NativeRegister(type, &RDI));
			OF.push_back(new x86_64::NativeRegister(type, &RSI));
			OF.push_back(new x86_64::NativeRegister(type, &RDX));
#endif
			return OF;
		case Type::FloatTypeID:
		case Type::DoubleTypeID:
#if 1
			for (unsigned i = 0; i < FloatArgumentRegisterSize; ++i) {
				//OF.push_back(new x86_64::NativeRegister(type, FloatArgumentRegisters[i]));
			}
			//assert(OF.size() == FloatArgumentRegisterSize);
#else
			regs.push_back(&XMM0);
			regs.push_back(&XMM1);
			regs.push_back(&XMM2);
#endif
			return OF;
		default: break;
	}
	ABORT_MSG("X86_64 Register File Type Not supported!", "Type: " << type);
	return OF;
}

RegisterInfoBase<X86_64>::RegisterInfoBase(JITData* JD) : RegisterInfo(JD)
{
	this->classes.emplace_back(new X86_64RegisterClass<X86_64Class::GP>());
	this->classes.emplace_back(new X86_64RegisterClass<X86_64Class::FP>());
}

template <>
const char* NativeRegister::get_name() const { return reg->get_name(); }

template <>
MachineOperand::IdentifyTy NativeRegister::id_base() const { return reg->id_base(); }

template <>
MachineOperand::IdentifyOffsetTy NativeRegister::id_offset() const { return reg->id_offset(); }

template <>
MachineOperand::IdentifySizeTy NativeRegister::id_size() const { return reg->id_size(); }

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
