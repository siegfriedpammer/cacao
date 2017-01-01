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
	virtual void emit(CodeMemory* CM) const;
	virtual bool is_label() const {
		return true;
	}
};

class MachinePhiInst : public MachineInstruction {
private:
	PHIInst *phi;
public:
	MachinePhiInst(unsigned num_operands, Type::TypeID type, PHIInst* phi)
			: MachineInstruction("MPhi", new VirtualRegister(type),
			  num_operands), phi(phi) {
		for(unsigned i = 0; i < num_operands; ++i) {
			operands[i].op = new UnassignedReg(type);
		}
	}

	virtual bool is_phi() const {
		return true;
	}
	virtual MachinePhiInst* to_MachinePhiInst() {
		return this;
	}
	// phis are never emitted
	virtual void emit(CodeMemory* CM) const {};

	PHIInst* get_PHIInst() const {
		return phi;
	}
};
#if 0
class MachineConstInst : public MachineInstruction {
public:
	/**
	 * TODO: get const parameter
	 */
	MachineConstInst(s8 value) : MachineInstruction("MConst",
			new Immediate(value), 0) {}
	// mconsts are never emitted
	virtual void emit(CodeMemory* CM) const {};
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
		operands[0].op = src;
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
		operands[0].op = src;
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
#endif
class MachineJumpInst : public MachineInstruction {
public:
	MachineJumpInst(const char *name)
		: MachineInstruction(name, &NoOperand, 0) {}
	virtual void emit(CodeMemory* CM) const = 0;
	virtual void link(CodeFragment &CF) const = 0;
};
#if 0
/**
 * Move operand to operand
 */
class MachineMoveInst : public MachineInstruction {
public:
	MachineMoveInst( const char* name,
			MachineOperand *src,
			MachineOperand *dst)
			: MachineInstruction(name, dst, 1) {
		operands[0].op = src;
	}
	virtual bool accepts_immediate(unsigned i, Immediate *imm) const {
		return true;
	}
	virtual MachineMoveInst* to_MachineMoveInst() {
		return this;
	}
	virtual void emit(CodeMemory* CM) const = 0;

};
#endif

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