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
#include <list>
#include <vector>
#include <algorithm>

namespace cacao {
namespace jit {
namespace compiler2 {

class LoweringPass;
class LoweredInstDAG;

OStream& operator<<(OStream &OS, const LoweredInstDAG &lid);
inline OStream& operator<<(OStream &OS, const LoweredInstDAG *lid) {
	if (!lid) {
		return OS << "(LoweredInstDAG*) NULL";
	}
	return OS << *lid;
}

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
public:
	typedef std::list<MachineInstruction*> MachineInstListTy;
	typedef std::pair<MachineInstruction*,unsigned> InputParameterTy;
	typedef std::vector<InputParameterTy> InputMapTy;
	typedef MachineInstListTy::iterator mi_iterator;
	typedef MachineInstListTy::const_iterator const_mi_iterator;
	typedef InputMapTy::iterator input_iterator;
	typedef InputMapTy::const_iterator const_input_iterator;
private:
	Instruction * inst;
	MachineInstListTy minst;
	InputMapTy input_map;
	MachineInstruction *result;
public:
	LoweredInstDAG(Instruction* I) : inst(I), minst(), input_map(I->op_size()),
			result(NULL) {}
	Instruction * get_Instruction() const {
		return inst;
	}
	void add(MachineInstruction *MI) {
		assert(MI);
		assert(std::find(minst.begin(),minst.end(),MI) == minst.end());
		minst.push_back(MI);
	}
	void set_input(MachineInstruction *MI) {
		assert(MI);
		assert(std::find(minst.begin(),minst.end(),MI) != minst.end() );
		assert( input_map.size() == MI->size_op());
		for (unsigned i = 0, e = input_map.size(); i < e ; ++i) {
			input_map[i] = std::make_pair(MI, i);
		}
	}
	void set_input(unsigned dag_input_idx, MachineInstruction *MI,
			unsigned minst_input_idx) {
		assert(MI);
		assert(std::find(minst.begin(),minst.end(),MI) != minst.end() );
		assert(dag_input_idx < input_map.size());
		assert(minst_input_idx < MI->size_op());
		input_map[dag_input_idx] = std::make_pair(MI, minst_input_idx);
	}
	void set_result(MachineInstruction *MI) {
		assert(MI);
		assert(std::find(minst.begin(),minst.end(),MI) != minst.end() );
		result = MI;
	}
	InputParameterTy operator[](unsigned i) const {
		return get(i);
	}
	InputParameterTy get(unsigned i) const {
		assert(i < input_map.size());
		return input_map[i];
	}
	MachineInstruction* get_result() const {
		if(!result) {
			err() << BoldRed << "Error: " << reset_color
			      << "result == NULL " << *this << nl;
		}
		assert(result);
		return result;
	}
	MachineOperand* get_operand(unsigned i) const {
		assert(i < input_map.size());
		InputParameterTy param = get(i);
		return param.first->get(param.second).op;
	}

	mi_iterator mi_begin() { return minst.begin(); }
	mi_iterator mi_end() { return minst.end(); }
	const_mi_iterator mi_begin() const { return minst.begin(); }
	const_mi_iterator mi_end() const { return minst.end(); }

	mi_iterator mi_insert(mi_iterator position, MachineInstruction* val) {
		return minst.insert(position, val);
	}

	input_iterator input_begin() { return input_map.begin(); }
	input_iterator input_end() { return input_map.end(); }
	const_input_iterator input_begin() const { return input_map.begin(); }
	const_input_iterator input_end() const { return input_map.end(); }
	unsigned input_size() const { return input_map.size(); }

	friend class LoweringPass;

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
