/* src/vm/jit/compiler2/LoweredInstDAG.hpp - LoweredInstDAG

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

#ifndef _JIT_COMPILER2_LOWEREDINSTDAG
#define _JIT_COMPILER2_LOWEREDINSTDAG

#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/MachineInstruction.hpp"

#include <cassert>
#include <vector>
#include <algorithm>

namespace cacao {
namespace jit {
namespace compiler2 {

/**
 * DAG of machine instruction that replace one Instruction.
 *
 * Often a target independent instruction can not be replaced by one but by a
 * DAG of machine instructions. A LoweredInstDAG stores these machine
 * instructions and contains a mapping from the instruction operands to the
 * operands of the machine instructions.
 * 
 * Note all machine instructions must be linked i.e. all operands are either
 * connected to the operand map, another machine instruction or a immediate
 * value.
 */
class LoweredInstDAG {
private:
	Instruction * inst;
	std::vector<MachineInstruction*> minst;
	MachineInstruction *result;
public:
	LoweredInstDAG(Instruction* I) : inst(I), minst(I->op_size()),
			result(NULL) {}
	Instruction * get_Instruction() const {
		return inst;
	}

	void add(MachineInstruction *MI) {
		assert(std::find(minst.begin(),minst.end(),MI) == minst.end());
		assert(MI);
		minst.push_back(MI);
	}
	void set_result(MachineInstruction *MI) {
		assert(MI);
		result = MI;
	}

};


} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_LOWEREDINSTDAG */


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
