/* src/vm/jit/compiler2/MachineRegister.hpp - MachineRegister

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

#ifndef _JIT_COMPILER2_MACHINEREGISTER
#define _JIT_COMPILER2_MACHINEREGISTER

#include <memory>

#include "Target.hpp"
#include "vm/jit/compiler2/MachineOperand.hpp"

#include "toolbox/logging.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

/**
 * A MachineRegister is either a concrete low-level machine register
 * that is subclassed by a specific backend, or its a TypedNativeRegister that
 * ties a concrete low-level machine register and a type together, so it can be
 * used for code emitting
 */
class MachineRegister : public Register {
protected:
	MachineRegister(Type::TypeID type) : Register(type) {}

public:
	virtual MachineRegister* to_MachineRegister() { return this; }
	virtual NativeRegister* to_NativeRegister() = 0;
	virtual ~MachineRegister() {}

	using MachineOperand::id_offset;
	using MachineOperand::id_size;
};

/**
 * This represents a machine register usage.
 *
 * It consists of a reference to the physical register and a type. This
 * abstraction is needed because registers can be used several times
 * with different types, e.g. DH vs. eDX vs. EDX vs. RDX.
 *
 * NativeRegister is just a typedef using the backend-specific register
 * type as the template parameter.
 */
template <class T>
class TypedNativeRegister : public MachineRegister {
private:
	T* reg;

	TypedNativeRegister(Type::TypeID type, MachineOperand* reg)
	    : MachineRegister(type), reg((T*)reg) {}

public:
	TypedNativeRegister<T>* to_NativeRegister() { return this; }
	T* get_PlatformRegister() const { return reg; }

	// The following methods are specialized by the specific backends, since
	// we do not know the concrete type of T, only a forward declaration in Target.hpp

	const char* get_name() const;
	IdentifyTy id_base() const;
	IdentifyOffsetTy id_offset() const;
	IdentifySizeTy id_size() const;

	friend class MachineOperandFactory;
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_MACHINEREGISTER */

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
