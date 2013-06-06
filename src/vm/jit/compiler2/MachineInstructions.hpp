/* src/vm/jit/compiler2/MachineInstructions.hpp - Machine Instruction Types

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

#ifndef _JIT_COMPILER2_MACHINEINSTRUCTIONS
#define _JIT_COMPILER2_MACHINEINSTRUCTIONS

#include "vm/jit/compiler2/MachineInstruction.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

class MachineLabelInst : public MachineInstruction {
public:
	MachineLabelInst() : MachineInstruction("MLabel", &NoOperand, 0) {}

};

class MachineJumpInst : public MachineInstruction {
public:
	MachineJumpInst() : MachineInstruction("MJump", &NoOperand, 0) {}

};

class MachinePhiInst : public MachineInstruction {
public:
	MachinePhiInst(unsigned num_operands)
			: MachineInstruction("MPhi", new VirtualRegister(),
			  num_operands) {
		for(int i = 0; i < num_operands; ++i) {
			operands[i] = UnassignedReg::factory();
		}
	}

	virtual bool is_phi() const {
		return true;
	}
};

class MachineConstInst : public MachineInstruction {
public:
	/**
	 * TODO: get const parameter
	 */
	MachineConstInst() : MachineInstruction("MConst", new Immediate(), 0) {}

};

/**
 * Load from memory to register
 */
class MachineLoadInst : public MachineInstruction {
public:
	MachineLoadInst(
			MachineOperand *dst,
			MachineOperand *src)
			: MachineInstruction("MLoad", dst, 1) {
		operands[0] = src;
	}

};

/**
 * Store register to memory
 */
class MachineStoreInst : public MachineInstruction {
public:
	MachineStoreInst(
			MachineOperand *dst,
			MachineOperand *src)
			: MachineInstruction("MStore", dst, 1) {
		operands[0] = src;
	}

};

/**
 * Move register to register
 */
class MachineMoveInst : public MachineInstruction {
public:
	MachineMoveInst(
			MachineOperand *dst,
			MachineOperand *src)
			: MachineInstruction("MMove", dst, 1) {
		operands[0] = src;
	}
	virtual bool accepts_immediate(unsigned i) const {
		return true;
	}

};
class MachineOperandInst : public MachineInstruction {
private:
	MachineOperand *MO;
public:
	MachineOperandInst(MachineOperand *MO)
			: MachineInstruction("MachineOperandInst", MO, 0), MO(MO) {
	}

};


} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_MACHINEINSTRUCTIONS */


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
