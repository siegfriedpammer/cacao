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

// forward declarations
class OStream;

namespace jit {
namespace compiler2 {


/**
 * Superclass for all machine dependent instructions
 */
class MachineInstruction {
public:
	typedef std::vector<MachineOperand> operand_list;
	typedef std::vector<MachineOperand>::iterator operand_iterator;
	typedef std::vector<MachineOperand>::const_iterator const_operand_iterator;
protected:
	operand_list operands;
	MachineOperand result;
	const char *name;
public:
	MachineInstruction(const char * name, MachineOperand result, unsigned num_operands, MachineOperand dflt_type)
		: operands(num_operands,dflt_type), result(result), name(name) {
	}
	MachineInstruction(const char * name, MachineOperand result, unsigned num_operands)
		: operands(num_operands), result(result), name(name) {
	}
	unsigned size_op() const {
		return operands.size();
	}
	MachineOperand& operator[](unsigned i) {
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
	const char* get_name() const {
		return name;
	}
};

OStream& operator<<(OStream &OS, const MachineInstruction &MI);
OStream& operator<<(OStream &OS, const MachineInstruction *MI);

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
