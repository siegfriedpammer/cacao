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

#ifdef __X86_64__

#include "vm/jit/compiler2/x86_64/X86_64Backend.hpp"
cacao::jit::compiler2::Backend* cacao::jit::compiler2::Backend::factory() {
	static cacao::jit::compiler2::BackendBase<cacao::jit::compiler2::X86_64> BE;
	return &BE;
}

#else
#error Target not yet ported to the compiler2 backend!
#endif

namespace cacao {
namespace jit {
namespace compiler2 {

LoweredInstDAG* Backend::lower(Instruction *I) const {
	switch(I->get_opcode()) {
	case Instruction::BeginInstID:  return lowerBeginInst(I->to_BeginInst());
	case Instruction::LOADInstID:   return lowerLOADInst(I->to_LOADInst());
	case Instruction::GOTOInstID:   return lowerGOTOInst(I->to_GOTOInst());
	case Instruction::PHIInstID:    return lowerPHIInst(I->to_PHIInst());
	case Instruction::IFInstID:     return lowerIFInst(I->to_IFInst());
	case Instruction::CONSTInstID:  return lowerCONSTInst(I->to_CONSTInst());
	case Instruction::ADDInstID:    return lowerADDInst(I->to_ADDInst());
	case Instruction::SUBInstID:    return lowerSUBInst(I->to_SUBInst());
	case Instruction::RETURNInstID: return lowerRETURNInst(I->to_RETURNInst());
	case Instruction::MULInstID:    return lowerMULInst(I->to_MULInst());
	case Instruction::CASTInstID:   return lowerCASTInst(I->to_CASTInst());
	default: break;
	}
	err() << BoldRed << "error: " << reset_color
		  << " instruction " << BoldWhite
		  << I << reset_color << " not yet handled by the Backend" << nl;
	return NULL;
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
