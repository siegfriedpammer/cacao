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
#include "vm/types.hpp"

#include <vector>
#include <cassert>

namespace cacao {

// forward declarations
class OStream;

namespace jit {
namespace compiler2 {

class StackSlotManager;

class VoidOperand;
class Register;
class StackSlot;
class ManagedStackSlot;
class Immediate;
class Address;

/**
 * Operands that can be directly used by the machine (register, memory, stackslot)
 */
class MachineOperand {
public:
	enum OperandID {
		MachineOperandID,
		RegisterID,
		StackSlotID,
		ManagedStackSlotID,
		ImmediateID,
		AddressID,
		VoidOperandID
	};
private:
	OperandID type;
public:

	explicit MachineOperand(OperandID type) : type(type) {}

	OperandID get_Type() const { return type; }
	virtual const char* get_name() const  = 0;

	virtual ~MachineOperand() {}
	virtual MachineOperand*   to_MachineOperand()   { return this; }
	virtual VoidOperand*      to_VoidOperand()      { return 0; }
	virtual Register*         to_Register()         { return 0; }
	virtual StackSlot*        to_StackSlot()        { return 0; }
	virtual ManagedStackSlot* to_ManagedStackSlot() { return 0; }
	virtual Immediate*        to_Immediate()        { return 0; }
	virtual Address*          to_Addresss()         { return 0; }

	bool is_MachineOperand()   const { return type == MachineOperandID; }
	bool is_VoidOperand()      const { return type == VoidOperandID; }
	bool is_Register()         const { return type == RegisterID; }
	bool is_StackSlot()        const { return type == StackSlotID; }
	bool is_ManagedStackSlot() const { return type == ManagedStackSlotID; }
	bool is_Immediate()        const { return type == ImmediateID; }
	bool is_Address()          const { return type == AddressID; }

	virtual OStream& print(OStream &OS) const {
		return OS << get_name();
	}
};

class VoidOperand : public MachineOperand {
public:
	VoidOperand() : MachineOperand(VoidOperandID) {}
	virtual const char* get_name() const {
		return "VoidOperand";
	}
	virtual VoidOperand* to_VoidOperand() { return this; }
};

class UnassignedReg;
class VirtualRegister;
class MachineRegister;

class Register : public MachineOperand {
public:
	Register() : MachineOperand(RegisterID) {}
	virtual const char* get_name() const {
		return "Register";
	}
	virtual Register* to_Register()               { return this; }
	virtual UnassignedReg* to_UnassignedReg()     { return 0; }
	virtual VirtualRegister* to_VirtualRegister() { return 0; }
	virtual MachineRegister* to_MachineRegister() { return 0; }
	virtual ~Register() {}
};

class UnassignedReg : public Register {
private:
	UnassignedReg() {}
public:
	static UnassignedReg* factory() {
		static UnassignedReg instance;
		return &instance;
	}
	virtual const char* get_name() const {
		return "UnassignedReg";
	}
	virtual UnassignedReg* to_UnassignedReg()     { return this; }
};

class VirtualRegister : public Register {
private:
	static unsigned vreg_counter;
	const unsigned vreg;
public:
	VirtualRegister() : vreg(vreg_counter++) {}

	virtual VirtualRegister* to_VirtualRegister() { return this; }
	virtual const char* get_name() const {
		return "vreg";
	}
	virtual OStream& print(OStream &OS) const {
		return OS << get_name() << get_id();
	}
	unsigned get_id() const { return vreg; }
};

class StackSlot : public MachineOperand {
private:
	int index; ///< index of the stackslot
public:
	/**
	 * @param index  index of the stackslot
	 */
	StackSlot(int index) : MachineOperand(StackSlotID), index(index) {}
	virtual StackSlot* to_StackSlot() { return this; }
	int get_index() const { return index; }
	virtual const char* get_name() const {
		return "StackSlot";
	}
	virtual OStream& print(OStream &OS) const {
		return OS << get_name() << get_index();
	}
};

class ManagedStackSlot : public MachineOperand {
private:
	StackSlotManager *parent;
	unsigned id;
	ManagedStackSlot(StackSlotManager *SSM,unsigned id)
		: MachineOperand(ManagedStackSlotID), parent(SSM), id(id) {}
public:
	/**
	 * FIXME this should be managed
	 */
	virtual StackSlot* to_StackSlot();
	virtual ManagedStackSlot* to_ManagedStackSlot() { return this; }
	virtual const char* get_name() const {
		return "ManagedStackSlot";
	}
	unsigned get_id() const { return id; }
	virtual OStream& print(OStream &OS) const {
		return OS << get_name() << get_id();
	}
	friend class StackSlotManager;
};

class Immediate : public MachineOperand {
private:
	s8 value;
public:
	Immediate(s8 value) : MachineOperand(ImmediateID), value(value) {}
	virtual Immediate* to_Immediate() { return this; }
	virtual const char* get_name() const {
		return "Immediate";
	}
	s8 get_value() const { return value; }
};

class Address : public MachineOperand {
public:
	Address() : MachineOperand(AddressID) {}
	virtual Address* to_Address() { return this; }
	virtual const char* get_name() const {
		return "Address";
	}
};

#if 0
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
#endif

extern VoidOperand NoOperand;
#if 0
OStream& operator<<(OStream &OS, const MachineOperandType &MO);
#endif
inline OStream& operator<<(OStream &OS, const MachineOperand &MO) {
	return MO.print(OS);
}
inline OStream& operator<<(OStream &OS, const MachineOperand *MO) {
	if (!MO) {
		return OS << "(MachineOperand) NULL";
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
