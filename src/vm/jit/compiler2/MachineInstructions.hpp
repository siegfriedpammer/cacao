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

// forward declaration
class MachineLabelStub;
class MachineJumpStub;

class MachineStubVisitor {
public:
	virtual void visit(MachineLabelStub *MS) {}
	virtual void visit(MachineJumpStub *MS) {}
};
/**
 * Stub. A stub is an intermediate machine instruction used during
 * the construction of a MachineInstructionSchedule. It will be replaced by
 * a concrete MachineInst.
 */
class MachineInstStub : public MachineInstruction {
public:
	MachineInstStub(const char * name, MachineOperand* result, unsigned num_operands, const char* comment = NULL)
		: MachineInstruction(name, result, num_operands, comment) {}
	virtual bool is_stub() const {
		return true;
	}
	virtual MachineInstStub* to_MachineInstStub() {
		return this;
	}
	virtual void accepts(MachineStubVisitor &visitor) = 0;
};

class MachineJumpStub : public MachineInstStub {
private:
	BeginInst *begin;
public:
	MachineJumpStub(const char * name, BeginInst* begin, MachineOperand* result, unsigned num_operands, const char* comment = NULL)
		: MachineInstStub(name, result, num_operands, comment), begin(begin) {}
	BeginInst* get_BeginInst() const {
		return begin;
	}
	virtual void accepts(MachineStubVisitor &visitor) {
		visitor.visit(this);
	}
	virtual MachineInstruction* transform(MachineBasicBlock *MBB) = 0;
};

class MachineLabelInst : public MachineInstruction {
private:
	MachineBasicBlock *MBB;
public:
	MachineLabelInst(MachineBasicBlock *MBB) : MachineInstruction("MLabel", &NoOperand, 0), MBB(MBB) {}
	virtual void emit(CodeMemory* CM) const;
	virtual bool is_label() const {
		return true;
	}
	MachineBasicBlock* get_MachineBasicBlock() const {
		return MBB;
	}
};

class MachineLabelStub : public MachineInstStub {
private:
	BeginInst *begin;
public:
	MachineLabelStub(BeginInst *begin) : MachineInstStub("MLabelStub", &NoOperand, 0), begin(begin) {}
	BeginInst* get_BeginInst() const {
		return begin;
	}
	virtual void accepts(MachineStubVisitor &visitor) {
		visitor.visit(this);
	}
	virtual MachineInstruction* transform(MachineBasicBlock *MBB) {
		return new MachineLabelInst(MBB);
	}
};

class MachinePhiInst : public MachineInstruction {
public:
	MachinePhiInst(unsigned num_operands, Type::TypeID type)
			: MachineInstruction("MPhi", new VirtualRegister(type),
			  num_operands) {
		for(unsigned i = 0; i < num_operands; ++i) {
			operands[i].op = new UnassignedReg(type);
		}
	}

	virtual bool is_phi() const {
		return true;
	}
	// phis are never emitted
	virtual void emit(CodeMemory* CM) const {};
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
