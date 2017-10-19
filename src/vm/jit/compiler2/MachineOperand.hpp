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

#include "vm/jit/compiler2/memory/Manager.hpp"
#include "vm/jit/compiler2/alloc/list.hpp"
#include "vm/jit/compiler2/alloc/vector.hpp"
#include <cassert>

MM_MAKE_NAME(MachineOperand)

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

class MachineOperandDesc;
class MachineOperand;
class MachineInstruction;

class EmbeddedMachineOperand : public memory::ManagerMixin<EmbeddedMachineOperand> {
public:
	MachineOperand *dummy;
	MachineOperandDesc *real;

	explicit EmbeddedMachineOperand (MachineOperand *op) : dummy(op), real(NULL) {}
};

/**
 * Operands that can be directly used by the machine (register, memory, stackslot)
 */
class MachineOperand : public memory::ManagerMixin<MachineOperand>  {
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
	typedef const void* IdentifyTy;
	typedef std::size_t IdentifyOffsetTy;
	typedef std::size_t IdentifySizeTy;
	typedef alloc::vector<EmbeddedMachineOperand>::type embedded_operand_list;
	typedef embedded_operand_list::iterator operand_iterator;
	typedef embedded_operand_list::const_iterator const_operand_iterator;

	friend class MachineOperandFactory;

private:
	static std::size_t id_counter;
	std::size_t id;
	OperandID op_id;
	Type::TypeID type;
	MachineInstruction* defining_instruction; ///< Only used by MachinePhiInsts (needed in SpillPhase)
	void set_id(std::size_t new_id) { id = new_id; } ///< Used by the factory to assign IDs
protected:
	/**
	 * TODO describe
	 */
	embedded_operand_list embedded_operands;
	virtual IdentifyTy id_base()         const { return static_cast<IdentifyTy>(this); }
	virtual IdentifyOffsetTy id_offset() const { return 0; }
	virtual IdentifySizeTy id_size()     const { return 1; }

	explicit MachineOperand(OperandID op_id, Type::TypeID type)
		: id(id_counter++), op_id(op_id), type(type), embedded_operands() {}

public:
	static std::size_t get_id_counter() { return id_counter; }
	std::size_t get_id() const { return id; }

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
	virtual Address*          to_Address()          { return 0; }

	bool is_MachineOperand()   const { return op_id == MachineOperandID; }
	bool is_VoidOperand()      const { return op_id == VoidOperandID; }
	bool is_Register()         const { return op_id == RegisterID; }
	bool is_StackSlot()        const { return op_id == StackSlotID; }
	bool is_ManagedStackSlot() const { return op_id == ManagedStackSlotID; }
	bool is_Immediate()        const { return op_id == ImmediateID; }
	bool is_Address()          const { return op_id == AddressID; }

	bool is_stackslot() const { return is_StackSlot() || is_ManagedStackSlot(); }

	/**
	 */
	bool aquivalence_less(const MachineOperand& MO) const {
		if (id_base() != MO.id_base()) {
			return id_base() < MO.id_base();
		}
		return id_offset()+id_size() <= MO.id_offset();
	}
	bool aquivalent(const MachineOperand& MO) const {
		return !(aquivalence_less(MO) || MO.aquivalence_less(*this));
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
	bool has_embedded_operands()  { return op_size() != 0; }
	std::size_t op_size() const {
		return embedded_operands.size();
	}
	EmbeddedMachineOperand &operator[](std::size_t i) {
		assert(i < embedded_operands.size());
		return embedded_operands[i];
	}
	const EmbeddedMachineOperand &get(std::size_t i) const {
		assert(i < embedded_operands.size());
		return embedded_operands[i];
	}
	EmbeddedMachineOperand &get(std::size_t i) {
		assert(i < embedded_operands.size());
		return embedded_operands[i];
	}
	operand_iterator begin() {
		return embedded_operands.begin();
	}
	operand_iterator end() {
		return embedded_operands.end();
	}
	operand_iterator find(MachineOperand *op) {
		for (operand_iterator i = begin(), e =end(); i != e; ++i) {
			if (op->aquivalent(*i->dummy))
				return i;
		}
		return end();
	}
	EmbeddedMachineOperand &front() {
		return embedded_operands.front();
	}
	EmbeddedMachineOperand &back() {
		return embedded_operands.back();
	}
	const_operand_iterator begin() const {
		return embedded_operands.begin();
	}
	const_operand_iterator end() const {
		return embedded_operands.end();
	}

	virtual OStream& print(OStream &OS) const {
		return OS << get_name() /* << " (" << get_type() << ")" */;
	}

	MachineInstruction* get_defining_instruction() {
		return defining_instruction;
	}
	void set_defining_instruction(MachineInstruction* instruction) {
		defining_instruction = instruction;
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
class MachineAddress;

class Register : public MachineOperand {
protected:
	Register(Type::TypeID type) : MachineOperand(RegisterID, type) {}

public:
	virtual const char* get_name() const {
		return "Register";
	}
	virtual bool needs_allocation() const { return true; }
	virtual Register* to_Register()               { return this; }
	virtual UnassignedReg* to_UnassignedReg()     { return 0; }
	virtual VirtualRegister* to_VirtualRegister() { return 0; }
	virtual MachineRegister* to_MachineRegister() { return 0; }
	virtual ~Register() {}
};


class UnassignedReg : public Register {
private:
	UnassignedReg(Type::TypeID type) : Register(type) {}

public:
	virtual const char* get_name() const {
		return "UnassignedReg";
	}
	virtual UnassignedReg* to_UnassignedReg()     { return this; }

	friend class MachineOperandFactory;
};

class VirtualRegister : public Register {
private:
	static unsigned vreg_counter;
	const unsigned vreg;

	VirtualRegister(Type::TypeID type) : Register(type),
		vreg(vreg_counter++) {}
public:
	virtual VirtualRegister* to_VirtualRegister() { return this; }
	virtual const char* get_name() const {
		return "vreg";
	}
	virtual OStream& print(OStream &OS) const {
		return MachineOperand::print(OS) << vreg;
	}
	virtual bool is_virtual() const { return true; }
	// unsigned get_id() const { return vreg; }
	friend class MachineOperandFactory;
};

class StackSlot : public MachineOperand {
private:
	int index; ///< index of the stackslot
	/**
	 * @param index  index of the stackslot
	 */
	StackSlot(int index, Type::TypeID type)
	: MachineOperand(StackSlotID, type), index(index) {}
public:
	virtual StackSlot* to_StackSlot() { return this; }
	int get_index() const { return index; }
	virtual const char* get_name() const {
		return "StackSlot";
	}
	virtual OStream& print(OStream &OS) const {
		return MachineOperand::print(OS) << get_index();
	}

	friend class MachineOperandFactory;
};

/**
 * A "virtual" slot that will eventually be mapped to a machine-level slot.
 *
 * A ManagedStackSlot may be used to store spill-registers (as needed during
 * register allocation) or to store arguments of method invocations.
 * ManagedStackSlots may only be constructed via a corresponding StackSlotManager
 * and will eventually be mapped to actual machine-level slots during code
 * generation.
 *
 * Note that there is no one-to-one mapping between actual machine-level slots
 * and ManagedStackSlots. Multiple ManagedStackSlots might map to the same
 * machine-level slot which is the case for those slots that are used for
 * storing arguments for method invocations.
 */
class ManagedStackSlot : public MachineOperand {
private:

	/**
	 * The StackSlotManager that created (and owns) this slot.
	 */
	StackSlotManager *parent;

	/**
	 * Represents the slot's position in the virtual stack frame.
	 *
	 * Note that multiple ManagedStackSlots might use the same index, since they
	 * may be mapped to the same position in the stack frame (which is the case
	 * for slots that are used for storing arguments for method invocations).
	 */
	u4 index;

	/**
	 * Construct a ManagedStackSlot.
	 *
	 * @param SSM  The StackSlotManager that created (and owns) this slot.
	 * @param type The type of the slot.
	 */
	ManagedStackSlot(StackSlotManager *SSM, Type::TypeID type)
		: MachineOperand(ManagedStackSlotID,type), parent(SSM) {}

public:

	/**
	 * Conversion method.
	 */
	virtual ManagedStackSlot* to_ManagedStackSlot() { return this; }

	/**
	 * @return A human-readable name of this slot.
	 */
	virtual const char* get_name() const {
		return "ManagedStackSlot";
	}

	/**
	 * @return The index representing this slot's position in the virtual
	 *         stack frame.
	 */
	u4 get_index() const { return index; }

	/**
	 * Set the index of the slot.
	 *
	 * @param index The corresponding index.
	 */
	void set_index(unsigned index) { this->index = index; }

	/**
	 * @return The StackSlotManager that created (and owns) this slot.
	 */
	StackSlotManager *get_parent() const { return parent; }

	/**
	 * Print a human-readable representation of this slot.
	 *
	 * @param OS The stream to print to.
	 *
	 * @return The corresponding stream.
	 */
	virtual OStream& print(OStream &OS) const {
		return MachineOperand::print(OS) << get_id();
	}

	friend class MachineOperandFactory;
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

	/// TODO How to handle addresses as immediates?
	Immediate(s8 val, Type::ReferenceType type)
			: MachineOperand(ImmediateID, type) {
		value.l = val;
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

	/// Construct an Address.
	///
	/// @param type The type of the data referenced by this Address.
	Address(Type::TypeID type) : MachineOperand(AddressID, type) {}

	virtual Address* to_Address() { return this; }
	virtual MachineAddress* to_MachineAddress() = 0;
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

struct MachineOperandComp : public std::binary_function<MachineOperand*,MachineOperand*,bool> {
	bool operator()(MachineOperand* lhs, MachineOperand *rhs) const {
		return lhs->aquivalence_less(*rhs);
	}
};

typedef alloc::list<MachineOperand*>::type OperandFile;

class MachineRegisterRequirement {
public:
	MachineRegisterRequirement(MachineOperand* required_reg) : required(required_reg) {}

	virtual MachineOperand* get_required() { return required; }

private:
	MachineOperand* required;
};
using MachineRegisterRequirementUPtrTy = std::unique_ptr<MachineRegisterRequirement>;

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

namespace std {

template<>
struct hash<cacao::jit::compiler2::MachineOperand*> {
	std::size_t operator()(cacao::jit::compiler2::MachineOperand *v) const {
		return v->get_id();
	}
};

template<>
struct less<cacao::jit::compiler2::MachineOperand*> {
	bool operator()(cacao::jit::compiler2::MachineOperand *lhs,
			cacao::jit::compiler2::MachineOperand *rhs) const {
		return lhs->get_id() < rhs->get_id();
	}
};

#if 0
template<>
struct equal_to<cacao::jit::compiler2::MachineOperand*> {
	bool operator()(cacao::jit::compiler2::MachineOperand *lhs,
			cacao::jit::compiler2::MachineOperand *rhs) const {
		return lhs->aquivalent(*rhs);
	}
};
#endif

} // end namespace standard

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
