/* src/vm/jit/compiler2/MachineInstruction.hpp - MachineInstruction

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

#ifndef _JIT_COMPILER2_MACHINEINSTRUCTION
#define _JIT_COMPILER2_MACHINEINSTRUCTION

#include "vm/jit/compiler2/MachineOperand.hpp"

#include <vector>

namespace cacao {

// forward declaration
class OStream;

namespace jit {
namespace compiler2 {

// forward declaration
class MachineMoveInst;
class LoweredInstDAG;

/**
 * Descriptor of a MachineOperand
 *
 * Besides a pointer to the actual MachineOperand meta information like
 * operand index, etc. are stored.
 */
class MachineOperandDesc {
private:
	unsigned index;
public:
	MachineOperand *op;
	explicit MachineOperandDesc(unsigned index) : index(index), op(NULL) {}
	explicit MachineOperandDesc(unsigned index, MachineOperand *op) : index(index), op(op) {}
	explicit MachineOperandDesc(MachineOperand *op) : index(0), op(op) {}
	unsigned get_index() const { return index; }
};

/**
 * Superclass for all machine dependent instructions
 */
class MachineInstruction {
public:
	typedef std::vector<MachineOperandDesc> operand_list;
	typedef operand_list::iterator operand_iterator;
	typedef operand_list::const_iterator const_operand_iterator;
private:
	static unsigned id_counter;
	LoweredInstDAG *parent;
protected:
	const unsigned id;
	operand_list operands;
	MachineOperandDesc result;
	const char *name;
public:
	#if 0
	MachineInstruction(const char * name, MachineOperand* result, unsigned num_operands, MachineOperand* dflt)
		: id(id_counter++), operands(num_operands,dflt), result(result), name(name) {
	}
	#endif
	MachineInstruction(const char * name, MachineOperand* result, unsigned num_operands)
		: parent(NULL), id(id_counter++), operands(), result(result), name(name) {
		for (unsigned i = 0; i < num_operands ; ++i) {
			//operands[i].index = i;
			operands.push_back(MachineOperandDesc(i));
		}
	}

	LoweredInstDAG* get_parent() const { return parent; }
	void set_parent(LoweredInstDAG* dag) { parent = dag; }

	void add_before(MachineInstruction *MI);

	unsigned size_op() const {
		return operands.size();
	}
	MachineOperandDesc& operator[](unsigned i) {
		assert(i < operands.size());
		assert(operands[i].get_index() == i);
		return operands[i];
	}
	const MachineOperandDesc& get(unsigned i) const {
		assert(i < operands.size());
		assert(operands[i].get_index() == i);
		return operands[i];
	}
	operand_iterator begin() {
		return operands.begin();
	}
	operand_iterator end() {
		return operands.end();
	}
	const_operand_iterator begin() const {
		return operands.begin();
	}
	const_operand_iterator end() const {
		return operands.end();
	}
	unsigned get_id() const {
		return id;
	}
	const char* get_name() const {
		return name;
	}
	const MachineOperandDesc& get_result() const {
		return result;
	}
	MachineOperandDesc& get_result() {
		return result;
	}
	void set_result(MachineOperand *MO) {
		result = MachineOperandDesc(0,MO);
	}
	virtual bool accepts_immediate(unsigned i) const {
		return false;
	}
	virtual bool is_phi() const {
		return false;
	}
	virtual MachineMoveInst* to_MachineMoveInst() {
		return NULL;
	}
	virtual OStream& print(OStream &OS) const;
};

OStream& operator<<(OStream &OS, const MachineInstruction *MI);
OStream& operator<<(OStream &OS, const MachineInstruction &MI);

OStream& operator<<(OStream &OS, const MachineOperandDesc *MOD);
OStream& operator<<(OStream &OS, const MachineOperandDesc &MOD);


} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_MACHINEINSTRUCTION */


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
