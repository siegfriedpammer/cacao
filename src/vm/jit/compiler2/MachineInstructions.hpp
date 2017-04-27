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
#include "vm/jit/compiler2/DataSegment.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

class MachineReplacementEntryInst;
class MachineReplacementPointCallSiteInst;
class MachineReplacementPointStaticSpecialInst;
class MachineDeoptInst;

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

/**
 * Represents a point in the program, where it is possible to recover the source
 * state to perform on-stack replacement.
 */
class MachineReplacementPointInst : public MachineInstruction {
private:
	typedef alloc::vector<s4>::type JavalocalIndexMapTy;

	s4 source_id;
	JavalocalIndexMapTy javalocal_indices;

public:
	/**
	 * @param source_id the id of the baseline IR instruction this replacement
	 * point represents in the source state
	 * @param num_operands the number of operands, i.e., the number of values
	 * that are live at this OSR point
	 */
	MachineReplacementPointInst(const char *name, s4 source_id, std::size_t num_operands)
		: MachineInstruction(name, &NoOperand, num_operands),
		source_id(source_id), javalocal_indices(num_operands) {}
	virtual void emit(CodeMemory* CM) const {};
	virtual void link(CodeFragment &CF) const {};

	virtual MachineReplacementPointInst* to_MachineReplacementPointInst() {
		return this;
	}

	virtual MachineReplacementEntryInst* to_MachineReplacementEntryInst() {
		return NULL;
	}

	virtual MachineReplacementPointCallSiteInst* to_MachineReplacementPointCallSiteInst() {
		return NULL;
	}

	virtual MachineReplacementPointStaticSpecialInst* to_MachineReplacementPointStaticSpecialInst() {
		return NULL;
	}

	virtual MachineDeoptInst* to_MachineDeoptInst() {
		return NULL;
	}

	s4 get_source_id() const { return source_id; }

	/**
	 * Set the javalocal index of the i-th operand of this instruction. Pass a
	 * value >= 0 to indicate a valid javalocal index. Pass RPLALLOC_STACK to
	 * indicate that the i-th operand is a stack variable. Pass RPLALLOC_PARAM
	 * to indicate that it is a parameter at a call site.
	 */
	void set_javalocal_index(std::size_t i, s4 javalocal_index) {
		assert(i < op_size());
		javalocal_indices[i] = javalocal_index;
	}

	/**
	 * @return the javalocal index of the i-th operand of this instruction. For
	 * possible return values refer to set_javalocal_index().
	 */
	s4 get_javalocal_index(std::size_t i) {
		assert(i < op_size());
		return javalocal_indices[i];
	}

	virtual bool is_trappable() const = 0;
};

class MachineReplacementEntryInst : public MachineReplacementPointInst {
public:
	MachineReplacementEntryInst(s4 source_id, std::size_t num_operands)
		: MachineReplacementPointInst("MReplacementEntry", source_id, num_operands) {}
	virtual void emit(CodeMemory* CM) const {};
	virtual void link(CodeFragment &CF) const {};

	virtual MachineReplacementEntryInst* to_MachineReplacementEntryInst() {
		return this;
	}

	virtual bool is_trappable() const {
		return true;
	}
};

/**
 * Represents a replacement point at a call site (INVOKE* ICMDs)
 * The reference to the corresponding call MachineInstruction is needed since
 * the replacement point needs to know the size of the callsite (in bytes)
 */
class MachineReplacementPointCallSiteInst : public MachineReplacementPointInst {
private:
	MachineInstruction *call_inst;

public:
	MachineReplacementPointCallSiteInst(MachineInstruction* call_inst, s4 source_id, std::size_t num_operands)
		: MachineReplacementPointInst("MReplacementPointCallSite", source_id, num_operands), call_inst(call_inst) {}
	virtual void emit(CodeMemory* CM) const {};
	virtual void link(CodeFragment &CF) const {};

	virtual MachineReplacementPointCallSiteInst* to_MachineReplacementPointCallSiteInst() {
		return this;
	}

	virtual bool is_trappable() const {
		return false;
	}

	MachineInstruction* get_call_inst() const {
		return call_inst;
	}
};

/**
 * Specialication for INVOKESpecial and INVOKEStatic.
 * Since the target address is known during compilation, this class holds a reference
 * to the DataSegment index where the address is stored so it can be patched
 * during OSR
 */
class MachineReplacementPointStaticSpecialInst : public MachineReplacementPointCallSiteInst {
private:
	/**
	 * DataSegment index that contains the address of the call target
	 */
	DataSegment::IdxTy idx;

public:
	MachineReplacementPointStaticSpecialInst(MachineInstruction* call_inst, s4 source_id, std::size_t num_operands, DataSegment::IdxTy idx)
		: MachineReplacementPointCallSiteInst(call_inst, source_id, num_operands), idx(idx) {}
	
	virtual MachineReplacementPointStaticSpecialInst* to_MachineReplacementPointStaticSpecialInst() {
		return this;
	}

	DataSegment::IdxTy get_idx() const {
		return idx;
	}
};

class MachineDeoptInst : public MachineReplacementPointInst {
public:
	MachineDeoptInst(s4 source_id, std::size_t num_operands)
		: MachineReplacementPointInst("MDeopt", source_id, num_operands) {}
	virtual void emit(CodeMemory* CM) const {};
	virtual void link(CodeFragment &CF) const {};

	virtual MachineDeoptInst* to_MachineDeoptInst() {
		return this;
	}

	virtual bool is_trappable() const {
		return false;
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
