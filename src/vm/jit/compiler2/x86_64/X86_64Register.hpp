/* src/vm/jit/compiler2/X86_64Register.hpp - X86_64Register

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

#ifndef _JIT_COMPILER2_X86_64REGISTER
#define _JIT_COMPILER2_X86_64REGISTER

#include "vm/jit/compiler2/x86_64/X86_64.hpp"
#include "vm/jit/compiler2/MachineRegister.hpp"

#include "toolbox/logging.hpp"

#include "vm/jit/compiler2/alloc/vector.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace x86_64 {

/**
 * x86_64 Register
 */

class X86_64Register {
public:
	const unsigned index;
	const bool extented;
	const char* name;
	const MachineOperand::IdentifyOffsetTy offset;
	const MachineOperand::IdentifySizeTy size;

	X86_64Register(const char* name,unsigned index,bool extented,
		MachineOperand::IdentifyOffsetTy offset,
		MachineOperand::IdentifySizeTy size)
			: index(index), extented(extented), name(name), offset(offset),
			size(size) {}
	unsigned get_index() const {
		return index;
	}
	virtual MachineOperand::IdentifyTy id_base()         const = 0;
	virtual MachineOperand::IdentifyOffsetTy id_offset() const { return offset; }
	virtual MachineOperand::IdentifySizeTy id_size()     const { return size; }
};

class FPUStackRegister {
public:
	const unsigned index;

	FPUStackRegister(unsigned index) : index(index) {}
};

/**
 * This represents a machine register usage.
 *
 * It consists of a reference to the physical register and a type. This
 * abstraction is needed because registers can be used several times
 * with different types, e.g. DH vs. eDX vs. EDX vs. RDX.
 */
class NativeRegister : public MachineRegister {
private:
	X86_64Register *reg;
	
public:
	NativeRegister(Type::TypeID type, X86_64Register* reg);
	virtual NativeRegister* to_NativeRegister() {
		return this;
	}
	X86_64Register* get_X86_64Register() const {
		return reg;
	}
	virtual IdentifyTy id_base()         const { return reg->id_base(); }
	virtual IdentifyOffsetTy id_offset() const { return reg->id_offset(); }
	virtual IdentifySizeTy id_size()     const { return reg->id_size(); }
};

inline OStream& operator<<(OStream &OS, const X86_64Register& reg) {
	return OS << reg.name;
}

class GPRegister : public X86_64Register {
private:
	static const uint8_t base;
public:
	GPRegister(const char* name,unsigned index,bool extented_gpr,
		MachineOperand::IdentifyOffsetTy offset,
		MachineOperand::IdentifySizeTy size)
		: X86_64Register(name,index,extented_gpr, offset, size) {}
	virtual MachineOperand::IdentifyTy id_base()         const { return static_cast<const void*>(&base); }
};

class SSERegister : public X86_64Register {
private:
	static const uint8_t base;
public:
	SSERegister(const char* name,unsigned index,bool extented_gpr,
		MachineOperand::IdentifyOffsetTy offset,
		MachineOperand::IdentifySizeTy size)
		: X86_64Register(name,index,extented_gpr, offset, size) {}
	virtual MachineOperand::IdentifyTy id_base()         const { return static_cast<const void*>(&base); }
};

template <>
inline Register* cast_to<Register>(MachineOperand *op) {
	Register *reg = op->to_Register();
	assert(reg);
	return reg;
}

template <>
inline X86_64Register* cast_to<X86_64Register>(Register *reg) {
	MachineRegister *mreg = reg->to_MachineRegister();
	assert(mreg);
	NativeRegister *nreg = mreg->to_NativeRegister();
	assert(nreg);
	X86_64Register *xreg = nreg->get_X86_64Register();
	assert(xreg);
	return xreg;
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

extern GPRegister RAX;
extern GPRegister RCX;
extern GPRegister RDX;
extern GPRegister RBX;
extern GPRegister RSP;
extern GPRegister RBP;
extern GPRegister RSI;
extern GPRegister RDI;
extern GPRegister R8;
extern GPRegister R9;
extern GPRegister R10;
extern GPRegister R11;
extern GPRegister R12;
extern GPRegister R13;
extern GPRegister R14;
extern GPRegister R15;

const unsigned IntegerArgumentRegisterSize = 6;
extern GPRegister* IntegerArgumentRegisters[];
extern GPRegister* IntegerCallerSavedRegisters[];
const unsigned IntegerCallerSavedRegistersSize = 9;

extern SSERegister XMM0;
extern SSERegister XMM1;
extern SSERegister XMM2;
extern SSERegister XMM3;
extern SSERegister XMM4;
extern SSERegister XMM5;
extern SSERegister XMM6;
extern SSERegister XMM7;
extern SSERegister XMM8;
extern SSERegister XMM9;
extern SSERegister XMM10;
extern SSERegister XMM11;
extern SSERegister XMM12;
extern SSERegister XMM13;
extern SSERegister XMM14;
extern SSERegister XMM15;


extern FPUStackRegister ST0;
extern FPUStackRegister ST1;
extern FPUStackRegister ST2;
extern FPUStackRegister ST3;
extern FPUStackRegister ST4;
extern FPUStackRegister ST5;
extern FPUStackRegister ST6;
extern FPUStackRegister ST7;


const unsigned FloatArgumentRegisterSize = 8;
extern SSERegister* FloatArgumentRegisters[];

enum class X86_64Class {
	GP, FP
};

template<X86_64Class type>
class X86_64RegisterClass : public RegisterClass {
public:
	explicit X86_64RegisterClass();

	bool handles_type(Type::TypeID) const;

	unsigned count() const override { return all.size(); }
	const MRegisterSet& get_All() const override { return all; }

	unsigned caller_saved_count() const override { return caller_saved.size(); }
	const MRegisterSet& get_CallerSaved() const override { return caller_saved; }

	unsigned callee_saved_count() const override { return callee_saved.size(); }
	const MRegisterSet& get_CalleeSaved() const override { return callee_saved; }

private:
	MRegisterSet all;
	MRegisterSet caller_saved;
	MRegisterSet callee_saved;
};

} // end namespace x86_64
} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_X86_64REGISTER */


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
