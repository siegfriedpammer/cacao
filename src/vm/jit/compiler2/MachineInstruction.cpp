/* src/vm/jit/compiler2/MachineInstruction.cpp - MachineInstruction

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

#include "vm/jit/compiler2/MachineInstruction.hpp"
#include "vm/jit/compiler2/LoweredInstDAG.hpp"
#include "vm/jit/compiler2/Instructions.hpp"

#include "toolbox/OStream.hpp"

#define DEBUG_NAME "compiler2/MachineInstruction"

namespace cacao {
namespace jit {
namespace compiler2 {

unsigned MachineInstruction::id_counter = 0;

OStream& MachineInstruction::print(OStream &OS) const {
	OS << "[" << setw(4) << fillzero << get_id() << "] " << get_name();
	for (MachineInstruction::const_operand_iterator i = begin(),
			e = end(); i != e ; ++i) {
		OS << ' ' << (*i);
	}
	MachineOperand *result = get_result().op;
	if (!result->to_VoidOperand()) {
		OS << " -> " << result;
	}
	if (get_parent()) {
		assert(get_parent()->get_Instruction());
		EndInst* EI = get_parent()->get_Instruction()->to_EndInst();
		if (EI) {
			for (EndInst::SuccessorListTy::const_iterator i = EI->succ_begin(),
					e = EI->succ_end(); i != e; ++i) {
				OS << ' ' << *i;
			}
		}
	}
	return OS;
}

void MachineInstruction::add_before(MachineInstruction *MI) {
	assert(parent);
	LOG2("add before " << this << " : " << MI << nl);
	LOG2("minst.size: " << parent->mi_size() << nl);
	//FIXME
	parent->add_before(this,MI);
}

void MachineInstruction::emit(CodeMemory* CM) const {
	ABORT_MSG("emit not yet implemented", "emit for " << this << " is not yet implemented");
}

void MachineInstruction::emit(CodeFragment &CF) const {
	ABORT_MSG("emit CodeFragment not implemented", "emit for "
		<< this << " is not implemented");
}

OStream& operator<<(OStream &OS, const MachineInstruction *MI) {
	if (!MI) {
		return OS << "(MachineInstruction) NULL";
	}
	return OS << *MI;
}
OStream& operator<<(OStream &OS, const MachineInstruction &MI) {
	return MI.print(OS);
}

OStream& operator<<(OStream &OS, const MachineOperandDesc *MOD) {
	if (!MOD) {
		return OS << "(MachineOperandDesc) NULL";
	}
	return OS << *MOD;
}
OStream& operator<<(OStream &OS, const MachineOperandDesc &MOD) {
	return OS << MOD.op;
}

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao


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
