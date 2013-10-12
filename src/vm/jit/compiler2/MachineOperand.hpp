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

#include "vm/jit/compiler2/Type.hpp"
#include "toolbox/OStream.hpp"
#include "vm/types.hpp"

#include <list>
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
class CONSTInst;

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
	OperandID op_id;
	Type::TypeID type;
protected:
	/**
	 * TODO describe
	 */
	typedef const void* IdentifyTy;
	typedef std::size_t IdentifyOffsetTy;
	typedef std::size_t IdentifySizeTy;
	virtual IdentifyTy id_base()         const { return static_cast<const void*>(this); }
	virtual IdentifyOffsetTy id_offset() const { return 0; }
	virtual IdentifySizeTy id_size()     const { return 1; }
public:

	explicit MachineOperand(OperandID op_id, Type::TypeID type)
		: op_id(op_id), type(type) {}

	OperandID get_OperandID() const { return op_id; }
	Type::TypeID get_type() const { return type; }

	virtual const char* get_name() const  = 0;

	virtual ~MachineOperand() {}
	virtual MachineOperand*   to_MachineOperand()   { return this; }
	virtual VoidOperand*      to_VoidOperand()      { return 0; }
	virtual Register*         to_Register()         { return 0; }
	virtual StackSlot*        to_StackSlot()        { return 0; }
	virtual ManagedStackSlot* to_ManagedStackSlot() { return 0; }
	virtual Immediate*        to_Immediate()        { return 0; }
	virtual Address*          to_Addresss()         { return 0; }

	bool is_MachineOperand()   const { return op_id == MachineOperandID; }
	bool is_VoidOperand()      const { return op_id == VoidOperandID; }
	bool is_Register()         const { return op_id == RegisterID; }
	bool is_StackSlot()        const { return op_id == StackSlotID; }
	bool is_ManagedStackSlot() const { return op_id == ManagedStackSlotID; }
	bool is_Immediate()        const { return op_id == ImmediateID; }
	bool is_Address()          const { return op_id == AddressID; }

	/**
	 */
	bool aquivalence_less(const MachineOperand& MO) const {
		if (id_base() != MO.id_base()) {
			return id_base() < MO.id_base();
		}
		return id_offset()+id_size() <= MO.id_offset();
	}

	/**
	 * True if operand is virtual and must be assigned
	 * during register allocation
	 */
	virtual bool is_virtual() const { return false; }
	/**
	 * Return true if operand is processed during register allocation.
	 * This implies is_virtual().
	 *
	 * @see is_virtual()
	 */
	virtual bool needs_allocation() const { return is_virtual(); }

	virtual OStream& print(OStream &OS) const {
		return OS << get_name() /* << " (" << get_type() << ")" */;
	}
};

class VoidOperand : public MachineOperand {
public:
	VoidOperand() : MachineOperand(VoidOperandID, Type::VoidTypeID) {}
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
	Register(Type::TypeID type) : MachineOperand(RegisterID, type) {}
	virtual const char* get_name() const {
		return "Register";
	}
	virtual bool needs_allocation() const { return true; }
	virtual Register* to_Register()               { return this; }
	virtual UnassignedReg* to_UnassignedReg()     { return 0; }
	virtual VirtualRegister* to_VirtualRegister() { return 0; }
	virtual MachineRegister* to_MachineRegister() { return 0; }
	virtual ~Register() {}
	virtual bool operator==(Register *other) const = 0;
};


class UnassignedReg : public Register {
public:
	UnassignedReg(Type::TypeID type) : Register(type) {}
	virtual const char* get_name() const {
		return "UnassignedReg";
	}
	virtual UnassignedReg* to_UnassignedReg()     { return this; }
	virtual bool operator==(Register *other) const {
		return false;
	}
};

class VirtualRegister : public Register {
private:
	static unsigned vreg_counter;
	const unsigned vreg;
public:
	VirtualRegister(Type::TypeID type) : Register(type),
		vreg(vreg_counter++) {}

	virtual VirtualRegister* to_VirtualRegister() { return this; }
	virtual const char* get_name() const {
		return "vreg";
	}
	virtual OStream& print(OStream &OS) const {
		return MachineOperand::print(OS) << get_id();
	}
	virtual bool is_virtual() const { return true; }
	unsigned get_id() const { return vreg; }
	virtual bool operator==(Register *other) const {
		VirtualRegister *vreg = other->to_VirtualRegister();
		if (!vreg) {
			return false;
		}
		return get_id() == vreg->get_id();
	}
};

class StackSlot : public MachineOperand {
private:
	int index; ///< index of the stackslot
public:
	/**
	 * @param index  index of the stackslot
	 */
	StackSlot(int index, Type::TypeID type)
		: MachineOperand(StackSlotID, type), index(index) {}
	virtual StackSlot* to_StackSlot() { return this; }
	int get_index() const { return index; }
	virtual const char* get_name() const {
		return "StackSlot";
	}
	virtual OStream& print(OStream &OS) const {
		return MachineOperand::print(OS) << get_index();
	}
};

class ManagedStackSlot : public MachineOperand {
private:
	StackSlotManager *parent;
	unsigned id;
	ManagedStackSlot(StackSlotManager *SSM,unsigned id, Type::TypeID type)
		: MachineOperand(ManagedStackSlotID,type), parent(SSM), id(id) {}
public:
	/**
	 * FIXME this should be managed
	 */
	virtual StackSlot* to_StackSlot();
	virtual ManagedStackSlot* to_ManagedStackSlot() { return this; }
	virtual const char* get_name() const {
		return "ManagedStackSlot";
	}
	virtual bool is_virtual() const { return true; }
	unsigned get_id() const { return id; }
	virtual OStream& print(OStream &OS) const {
		return MachineOperand::print(OS) << get_id();
	}
	friend class StackSlotManager;
};

class Immediate : public MachineOperand {
private:
	typedef union {
		int32_t                   i;
		int64_t                   l;
		float                     f;
		double                    d;
		void                     *anyptr;
		java_handle_t            *stringconst;       /* for ACONST with string    */
		classref_or_classinfo     c;                 /* for ACONST with class     */
	} val_operand_t;
	val_operand_t value;
public:
	Immediate(CONSTInst *I);
	Immediate(s4 val, Type::IntType type)
			: MachineOperand(ImmediateID, type) {
		value.i = val;
	}
	Immediate(s8 val, Type::LongType type)
			: MachineOperand(ImmediateID, type) {
		value.l = val;
	}
	Immediate(float val, Type::FloatType type)
			: MachineOperand(ImmediateID, type) {
		value.f = val;
	}
	Immediate(double val, Type::DoubleType type)
			: MachineOperand(ImmediateID, type) {
		value.d = val;
	}
	virtual Immediate* to_Immediate() { return this; }
	virtual const char* get_name() const {
		return "Immediate";
	}
	template<typename T>
	T get_value() const;

	double get_Int() const {
		assert(get_type() == Type::IntTypeID);
		return value.i;
	}
	double get_Long() const {
		assert(get_type() == Type::LongTypeID);
		return value.l;
	}
	double get_Float() const {
		assert(get_type() == Type::FloatTypeID);
		return value.f;
	}
	double get_Double() const {
		assert(get_type() == Type::DoubleTypeID);
		return value.d;
	}
};

class Address : public MachineOperand {
public:
	/**
	 * @todo ReturnAddressTypeID is not correct
	 */
	Address() : MachineOperand(AddressID, Type::ReturnAddressTypeID) {}
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

typedef std::list<MachineOperand*> OperandFile;


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
