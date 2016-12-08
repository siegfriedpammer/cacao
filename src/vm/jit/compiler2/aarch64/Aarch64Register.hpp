/* src/vm/jit/compiler2/aarch64/Aarch64Register.hpp

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

#ifndef _JIT_COMPILER2_AARCH64_REGISTER
#define _JIT_COMPILER2_AARCH64_REGISTER

#include "vm/jit/compiler2/LivetimeInterval.hpp"
#include "vm/jit/compiler2/MachineRegister.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace aarch64 {


class Aarch64Register {
public:
	const unsigned index;
	const char* name;
	const MachineOperand::IdentifyOffsetTy offset;
	const MachineOperand::IdentifySizeTy size;

	Aarch64Register(const char* name, unsigned index,
			MachineOperand::IdentifyOffsetTy offset,
			MachineOperand::IdentifySizeTy size)
		: index(index), name(name), offset(offset), size(size) {}

	unsigned get_index() const {
		return index;
	}

	virtual MachineOperand::IdentifyTy id_base()         const = 0;
	virtual MachineOperand::IdentifyOffsetTy id_offset() const { return offset; }
	virtual MachineOperand::IdentifySizeTy id_size()     const { return size; }
};


/**
 * This represents a machine register usage.
 *
 * It consists of a reference to the physical register and a type. This
 * abstraction is needed because registers can be used several times
 * with different types, e.g. x0 and w0
 */
class NativeRegister : public MachineRegister {
private:
	Aarch64Register *reg;
	LivetimeInterval fixedInterval;

public:
	NativeRegister(Type::TypeID type, Aarch64Register* reg);
	virtual NativeRegister* to_NativeRegister() {
		return this;
	}
	Aarch64Register* get_Aarch64Register() const {
		return reg;
	}
	virtual IdentifyTy id_base()            const { return reg->id_base(); }
	virtual IdentifyOffsetTy id_offset()    const { return reg->id_offset(); }
	virtual IdentifySizeTy id_size()        const { return reg->id_size(); }
};


class GPRegister : public Aarch64Register {
private:
	static const uint8_t base;

public:
	GPRegister(const char* name, unsigned index,
			MachineOperand::IdentifyOffsetTy offset,
			MachineOperand::IdentifySizeTy size)
		: Aarch64Register(name, index, offset, size) {}

	virtual MachineOperand::IdentifyTy id_base() const {
		return static_cast<const void*>(&base);
	}
};


class FPRegister : public Aarch64Register {
private:
	static const uint8_t base;

public:
	FPRegister(const char* name, unsigned index,
			MachineOperand::IdentifyOffsetTy offset,
			MachineOperand::IdentifySizeTy size)
		: Aarch64Register(name, index, offset, size) {}

	virtual MachineOperand::IdentifyTy id_base() const {
		return static_cast<const void*>(&base);
	}
};


extern GPRegister R0;
extern GPRegister R1;
extern GPRegister R2;
extern GPRegister R3;
extern GPRegister R4;
extern GPRegister R5;
extern GPRegister R6;
extern GPRegister R7;
extern GPRegister R8;
extern GPRegister R9;
extern GPRegister R10;
extern GPRegister R11;
extern GPRegister R12;
extern GPRegister R13;
extern GPRegister R14;
extern GPRegister R15;
extern GPRegister R16;
extern GPRegister R17;
extern GPRegister R18;
extern GPRegister R19;
extern GPRegister R20;
extern GPRegister R21;
extern GPRegister R22;
extern GPRegister R23;
extern GPRegister R24;
extern GPRegister R25;
extern GPRegister R26;
extern GPRegister R27;
extern GPRegister R28;
extern GPRegister R29;
extern GPRegister R30;
extern GPRegister SP;

extern FPRegister V0;
extern FPRegister V1;
extern FPRegister V2;
extern FPRegister V3;
extern FPRegister V4;
extern FPRegister V5;
extern FPRegister V6;
extern FPRegister V7;
extern FPRegister V8;
extern FPRegister V9;
extern FPRegister V10;
extern FPRegister V11;
extern FPRegister V12;
extern FPRegister V13;
extern FPRegister V14;
extern FPRegister V15;
extern FPRegister V16;
extern FPRegister V17;
extern FPRegister V18;
extern FPRegister V19;
extern FPRegister V20;
extern FPRegister V21;
extern FPRegister V22;
extern FPRegister V23;
extern FPRegister V24;
extern FPRegister V25;
extern FPRegister V26;
extern FPRegister V27;
extern FPRegister V28;
extern FPRegister V29;
extern FPRegister V30;
extern FPRegister V31;

const std::size_t IntegerArgumentRegisterSize = 8;
extern GPRegister* IntegerArgumentRegisters[];

const std::size_t FloatArgumentRegisterSize = 8;
extern FPRegister* FloatArgumentRegisters[];

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

} // end namespace aarch64
} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif // _JIT_COMPILER2_AARCH64_REGISTER

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
