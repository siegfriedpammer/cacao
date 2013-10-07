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
#include "vm/jit/compiler2/CodeSegment.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedule.hpp"

#include <vector>

namespace cacao {

// forward declaration
class OStream;

namespace jit {
namespace compiler2 {

// forward declaration
class MachineMoveInst;
class MachinePhiInst;
class MachineLabelInst;
class CodeMemory;
class MachineInstruction;

/**
 * Descriptor of a MachineOperand
 *
 * Besides a pointer to the actual MachineOperand meta information like
 * operand index, etc. are stored.
 */
class MachineOperandDesc {
private:
	MachineInstruction *parent;
	unsigned index;
public:
	MachineOperand *op;
	explicit MachineOperandDesc(MachineInstruction* parent, unsigned index)
		: parent(parent), index(index), op(NULL) {}
	explicit MachineOperandDesc(MachineInstruction* parent, unsigned index,
		MachineOperand *op) : parent(parent), index(index), op(op) {}
	explicit MachineOperandDesc(MachineInstruction* parent, MachineOperand *op)
		: parent(parent), index(0), op(op) {}
	unsigned get_index() const { return index; }
	MachineInstruction* get_MachineInstruction() const { return parent; }
};

/**
 * Proxy to encode explicit and implicit successors.
 *
 * The target of an unconditional jump is an explicit successor (i.e. the
 * target is specified explicitly). The else branch of a conditional jump
 * (i.e. the following block) is an implicit successor.
 *
 * For implicit successors the source block is stored. Whenever the real
 * successor is requested the block following the source block is returned.
 * Therefore implicit successors might change if a block is inserted after the
 * source block.
 */
class SuccessorProxy {
public:
	struct Explicit {};
	struct Implicit {};
private:
	enum Type { ExplTy, ImplTy };
	MachineBasicBlock *entry;
	Type type;
public:
	/// Constructor. Construct explicit successor.
	explicit SuccessorProxy(MachineBasicBlock* MBB, Explicit)
		: entry(MBB), type(ExplTy) {}
	/// Constructor. Construct implicit successor.
	explicit SuccessorProxy(MachineBasicBlock* MBB, Implicit)
		: entry(MBB), type(ImplTy) {}
	/// convert to MachineBasicBlock
	operator MachineBasicBlock*() const;
};

/**
 * Superclass for all machine dependent instructions
 */
class MachineInstruction {
public:
	typedef std::vector<MachineOperandDesc> operand_list;
	typedef operand_list::iterator operand_iterator;
	typedef operand_list::const_iterator const_operand_iterator;
	typedef std::list<SuccessorProxy> successor_list;
	typedef successor_list::iterator successor_iterator;
	typedef successor_list::const_iterator const_successor_iterator;
private:
	static unsigned id_counter;
protected:
	const unsigned id;
	operand_list operands;
	successor_list successors;
	MachineOperandDesc result;
	const char *name;
	const char *comment;
	MachineBasicBlock *block;
public:
	#if 0
	MachineInstruction(const char * name, MachineOperand* result, unsigned num_operands, MachineOperand* dflt)
		: id(id_counter++), operands(num_operands,dflt), result(result), name(name) {
	}
	#endif
	MachineInstruction(const char * name, MachineOperand* result, unsigned num_operands, const char* comment = NULL)
		: id(id_counter++), operands(), result(this, result), name(name), comment(comment), block(NULL) {
		for (unsigned i = 0; i < num_operands ; ++i) {
			//operands[i].index = i;
			operands.push_back(MachineOperandDesc(this,i));
		}
	}

	void set_comment(const char* c) { comment = c; }
	const char* get_comment() const { return comment; }

	void set_operand(unsigned i,MachineOperand* op) {
		assert(i < operands.size());
		operands[i].op = op;
	}
	virtual void set_block(MachineBasicBlock* MBB) {
		block = MBB;
	}
	MachineBasicBlock* get_block() const {
		return block;
	}

	unsigned op_size() const {
		return operands.size();
	}
	MachineOperandDesc& operator[](unsigned i) {
		assert(i < operands.size());
		assert(operands[i].get_index() == i);
		return operands[i];
	}
	const MachineOperandDesc& get(unsigned i) const {
		assert(i < operands.size());
		assert(operands[i].get_index() == i);
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
	successor_iterator successor_begin() {
		return successors.begin();
	}
	successor_iterator successor_end() {
		return successors.end();
	}
	const_successor_iterator successor_begin() const {
		return successors.begin();
	}
	const_successor_iterator successor_end() const {
		return successors.end();
	}
	std::size_t successor_size() const {
		return successors.size();
	}
	bool successor_empty() const {
		return successors.empty();
	}
	unsigned get_id() const {
		return id;
	}
	const char* get_name() const {
		return name;
	}
	const MachineOperandDesc& get_result() const {
		return result;
	}
	MachineOperandDesc& get_result() {
		return result;
	}
	void set_result(MachineOperand *MO) {
		result = MachineOperandDesc(0,MO);
	}
	virtual bool accepts_immediate(unsigned i, Immediate *imm) const {
		return false;
	}
	virtual bool is_label() const {
		return false;
	}
	virtual bool is_phi() const {
		return false;
	}
	virtual bool is_move() const {
		return false;
	}
	virtual bool is_jump() const {
		return false;
	}
	virtual bool is_end() const {
		return is_jump();
	}
	virtual MachineMoveInst* to_MachineMoveInst() {
		return NULL;
	}
	#if 0
	virtual MachineLabelInst* to_MachineLabelInst() {
		return NULL;
	}
	#endif
	virtual MachinePhiInst* to_MachinePhiInst() {
		return NULL;
	}
	/// print instruction
	OStream& print(OStream &OS) const;
	/// print successor label
	virtual OStream& print_successor_label(OStream &OS,std::size_t index) const;

	/// emit machine code
	virtual void emit(CodeMemory* CM) const;
	/// link machine code
	virtual void link(CodeFragment &CF) const;

	/// destructor
	virtual ~MachineInstruction() {}
};

OStream& operator<<(OStream &OS, const MachineInstruction *MI);
OStream& operator<<(OStream &OS, const MachineInstruction &MI);

OStream& operator<<(OStream &OS, const MachineOperandDesc *MOD);
OStream& operator<<(OStream &OS, const MachineOperandDesc &MOD);


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
