/* src/vm/jit/compiler2/MachineOperand.hpp - MachineOperand

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

#ifndef _JIT_COMPILER2_MACHINEOPERAND
#define _JIT_COMPILER2_MACHINEOPERAND

#include "toolbox/OStream.hpp"

#include <vector>
#include <cassert>

namespace cacao {

// forward declarations
class OStream;

namespace jit {
namespace compiler2 {

class VoidOperand;
class Register;
class StackSlot;
class Immediate;
class Address;

/**
 * Operands that can be directly used by the machine (register, memory, stackslot)
 */
class MachineOperand {
public:
	virtual const char* get_name() const  = 0;
	virtual ~MachineOperand() {}
	virtual MachineOperand* to_MachineOperand()     { return this; }
	virtual VoidOperand*    to_VoidOperand() { return 0; }
	virtual Register*       to_Register()    { return 0; }
	virtual StackSlot*      to_StackSlot()   { return 0; }
	virtual Immediate*      to_Immediate()   { return 0; }
	virtual Address*        to_Addresss()    { return 0; }
};

class VoidOperand : public MachineOperand {
public:
	virtual const char* get_name() const {
		return "VoidOperand";
	}
	virtual VoidOperand* to_VoidOperand() { return this; }
};

class UnassignedReg;
class VirtualRegister;
class MachineRegister;

class Register : public MachineOperand {
protected:
	const char *name;
public:
	Register() {}
	Register(const char *name) : name(name) {}
	virtual const char* get_name() const {
		return name;
	}
	virtual Register* to_Register()               { return this; }
	virtual UnassignedReg* to_UnassignedReg()     { return 0; }
	virtual VirtualRegister* to_VirtualRegister() { return 0; }
	virtual MachineRegister* to_MachineRegister() { return 0; }
	virtual ~Register() {}
};

class UnassignedReg : public Register {
private:
	UnassignedReg() : Register("UnassignedReg") {}
public:
	static UnassignedReg* factory() {
		static UnassignedReg instance;
		return &instance;
	}
	virtual UnassignedReg* to_UnassignedReg()     { return this; }
};

class VirtualRegister : public Register {
private:
	static unsigned vreg_counter;
	const unsigned vreg;
public:
	VirtualRegister();

	virtual VirtualRegister* to_VirtualRegister() { return this; }
	virtual ~VirtualRegister() {
		delete[] name;
	}
};

class StackSlot : public MachineOperand {
private:
	int index; ///< index of the stackslot
public:
	/**
	 * @param index  index of the stackslot
	 */
	StackSlot(int index) : index(index) {}
	virtual StackSlot* to_StackSlot() { return this; }
	virtual const char* get_name() const {
		return "StackSlot";
	}
};

class Immediate : public MachineOperand {
public:
	virtual Immediate* to_Immediate() { return this; }
	virtual const char* get_name() const {
		return "Immediate";
	}
};

class Address : public MachineOperand {
public:
	virtual Address* to_Address() { return this; }
	virtual const char* get_name() const {
		return "Address";
	}
};


class MachineOperandType {
public:
	enum TYPE {
		NONE            = 0,
		REGISTER_VALUE  = 1<<0, ///< register with a value
		REGISTER_MEM    = 1<<1, ///< register with a memory address
		IMMEDIATE       = 1<<2, ///< immediate value
		ABSOLUTE_ADDR   = 1<<3, ///< absolute code address
		PIC_ADDR        = 1<<4, ///< absolute code address (position independent code).
		                        ///< The absolute value might changes but offsets are still valid.
		PC_REL_ADDR     = 1<<5, ///< code address offset
		ALL = REGISTER_VALUE | REGISTER_MEM | IMMEDIATE | ABSOLUTE_ADDR | PIC_ADDR | PC_REL_ADDR
	};
private:
	unsigned type;
public:
	/// default constructor
	MachineOperandType() {
		type = NONE;
	}
	MachineOperandType(unsigned t) {
		set_type(t);
	}
	/// copy constructor
	MachineOperandType(const MachineOperandType &MO) {
		type = MO.type;
	}
	/// copy assignment operator
	MachineOperandType& operator=(const MachineOperandType &rhs) {
		type = rhs.type;
		return *this;
	}
	bool takes(const TYPE t) const {
		return (type & t);
	}
	unsigned get_type() const {
		return type;
	}
	void set_type(unsigned t) {
		assert( t <= ALL);
		type = t;
	}
};

extern VoidOperand NoOperand;

OStream& operator<<(OStream &OS, const MachineOperandType &MO);

inline OStream& operator<<(OStream &OS, const MachineOperand &MO) {
	return OS << "MachineOperand";
}
inline OStream& operator<<(OStream &OS, const MachineOperand *MO) {
	if (!MO) {
		return OS << "(MachineOperand) NULL";
	}
	return OS << *MO;
}

inline OStream& operator<<(OStream &OS, const Register &MO) {
	return OS << "Register";
}
inline OStream& operator<<(OStream &OS, const Register *MO) {
	if (!MO) {
		return OS << "(Register) NULL";
	}
	return OS << *MO;
}

inline OStream& operator<<(OStream &OS, const StackSlot &MO) {
	return OS << "StackSlot";
}
inline OStream& operator<<(OStream &OS, const StackSlot *MO) {
	if (!MO) {
		return OS << "(StackSlot) NULL";
	}
	return OS << *MO;
}

inline OStream& operator<<(OStream &OS, const Immediate &MO) {
	return OS << "Immediate";
}
inline OStream& operator<<(OStream &OS, const Immediate *MO) {
	if (!MO) {
		return OS << "(Immediate) NULL";
	}
	return OS << *MO;
}

inline OStream& operator<<(OStream &OS, const Address &MO) {
	return OS << "Address";
}
inline OStream& operator<<(OStream &OS, const Address *MO) {
	if (!MO) {
		return OS << "(Address) NULL";
	}
	return OS << *MO;
}

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_MACHINEOPERAND */


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
