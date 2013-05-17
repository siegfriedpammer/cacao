/* src/vm/jit/compiler2/Backend.cpp - Backend

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

#include "vm/jit/compiler2/Backend.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/LoweredInstDAG.hpp"
#include "vm/jit/compiler2/MachineInstructions.hpp"

#include "vm/jit/compiler2/x86_64/X86_64Backend.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {


Backend* Backend::factory() {
	static X86_64Backend BE;
	return &BE;
}

LoweredInstDAG* Backend::lower(Instruction *I) const {
	switch(I->get_opcode()) {
	case Instruction::BeginInstID: return lowerBeginInst(I->to_BeginInst());
	case Instruction::LOADInstID:  return lowerLOADInst(I->to_LOADInst());
	}
	err() << BoldRed << "error: " << reset_color
		  << " instruction " << BoldWhite
		  << I << reset_color << " not yet handled by the Backend" << nl;
	return NULL;
}

LoweredInstDAG* Backend::lowerBeginInst(BeginInst *I) const {
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	MachineLabelInst *label = new MachineLabelInst();
	dag->add(label);
	dag->set_result(label);
	return dag;
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
