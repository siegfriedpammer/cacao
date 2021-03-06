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

#include "vm/jit/compiler2/memory/Manager.hpp"
#include "vm/jit/compiler2/alloc/vector.hpp"
#include "vm/jit/compiler2/alloc/unordered_map.hpp"

MM_MAKE_NAME(MachineInstruction)

namespace cacao {

// forward declaration
class OStream;

namespace jit {
namespace compiler2 {

// forward declaration
class MachineMoveInst;
class MachinePhiInst;
class CodeMemory;
class MachineInstruction;
class MachineReplacementPointInst;

/**
 * Descriptor of a MachineOperand
 *
 * Besides a pointer to the actual MachineOperand meta information like
 * operand index, etc. are stored.
 */
class MachineOperandDesc : public memory::ManagerMixin<MachineOperandDesc>  {
private:
	MachineInstruction *parent;
	std::size_t index;
	MachineRegisterRequirementUPtrTy requirement;
public:
	MachineOperand *op;
	explicit MachineOperandDesc(MachineInstruction* parent, std::size_t index)
		: parent(parent), index(index), op(NULL) {}
	explicit MachineOperandDesc(MachineInstruction* parent, std::size_t index,
		MachineOperand *op) : parent(parent), index(index), op(op) {}
	explicit MachineOperandDesc(MachineInstruction* parent, MachineOperand *op)
		: parent(parent), index(0), op(op) {}
	std::size_t get_index() const { return index; }
	MachineInstruction* get_MachineInstruction() const { return parent; }
	
	void set_MachineRegisterRequirement(MachineRegisterRequirementUPtrTy& new_requirement) {
		requirement = std::move(new_requirement);
	}
	MachineRegisterRequirement* get_MachineRegisterRequirement() const { return requirement.get(); }
};

class MachineRegisterTwoAddressRequirement : public MachineRegisterRequirement {
public:
	MachineRegisterTwoAddressRequirement(MachineOperandDesc* desc) : MachineRegisterRequirement(nullptr), descriptor(desc) {}

	virtual MachineOperand* get_required() {
		return descriptor->op;
	}

private:
	MachineOperandDesc* descriptor;
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
#if 0
class SuccessorProxy : public memory::ManagerMixin<SuccessorProxy>  {
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
#endif

/**
 * Superclass for all machine dependent instructions
 */
class MachineInstruction : public memory::ManagerMixin<MachineInstruction>  {
public:
	typedef alloc::vector<MachineOperandDesc>::type operand_list;
	typedef operand_list::iterator operand_iterator;
	typedef operand_list::const_iterator const_operand_iterator;
	typedef alloc::vector<MachineOperandDesc>::type dummy_operand_list;
	typedef dummy_operand_list::iterator dummy_operand_iterator;
	typedef dummy_operand_list::const_iterator const_dummy_operand_iterator;
	typedef alloc::unordered_map<EmbeddedMachineOperand *,int>::type RefMapTy;
	typedef alloc::list<MachineBasicBlock*>::type successor_list;
	typedef successor_list::iterator successor_iterator;
	typedef successor_list::const_iterator const_successor_iterator;
private:
	static std::size_t id_counter;
	void set_dummy_operand(EmbeddedMachineOperand *op) {
		std::size_t pos;
		pos = dummy_operands.size();
		dummy_operands.push_back(MachineOperandDesc(this,pos));	
		dummy_operands[pos].op = op->dummy;

		ref_map.insert(std::make_pair(op,pos));
	}
protected:
	std::size_t step = 0;		//< Schedule numbering inside a basic block (used by next use analysis)
	u2 line = 0;                //< Corresponding line in the Java code.
	const std::size_t id;
	operand_list operands;
	operand_list results;
	/**
  	 * dummy_operands is a list of operands embedded in the real operands 
	 * of this instruction that need register allocation 
	 * (for example indirect addressing on x86)
 	 */
	dummy_operand_list dummy_operands;
	RefMapTy ref_map;
	successor_list successors;
	const char *name;
	const char *comment;
	MachineBasicBlock *block;
public:
	#if 0
	MachineInstruction(const char * name, MachineOperand* result, std::size_t num_operands, MachineOperand* dflt)
		: id(id_counter++), operands(num_operands,dflt), result(result), name(name) {
	}
	#endif
	MachineInstruction(const char * name, MachineOperand* result, std::size_t num_operands, const char* comment = NULL)
		: line(0), id(id_counter++), operands(), dummy_operands(), ref_map(), name(name), comment(comment), block(NULL) {
		for (std::size_t i = 0; i < num_operands ; ++i) {
			operands.push_back(MachineOperandDesc(this,i));
		}
		if (result->has_embedded_operands()) {
			for (MachineOperand::operand_iterator it = result->begin(), e = result->end(); it != e ; ++it) {
				set_dummy_operand(&(*it));
			}
		}
		results.emplace_back(this, result);
	}

	void set_comment(const char* c) { comment = c; }
	const char* get_comment() const { return comment; }

	void set_operand(std::size_t i,MachineOperand* op) {
		assert(i < operands.size());
		operands[i].op = op;
		if (op->has_embedded_operands()) {
			for (MachineOperand::operand_iterator it = op->begin(), e = op->end(); it != e ; ++it) {
				set_dummy_operand(&(*it));
			}
		}
	}

	/// has to be called after all operands with embedded operands have been added
	void finalize_operands() {
		for (RefMapTy::iterator it = ref_map.begin(), e = ref_map.end(); it != e; ++it) {
			it->first->real = &dummy_operands[it->second];
		}
		ref_map.clear();
	}
	virtual void set_block(MachineBasicBlock* MBB) {
		block = MBB;
	}
	MachineBasicBlock* get_block() const {
		return block;
	}

	std::size_t op_size() const {
		return operands.size();
	}
	MachineOperandDesc& operator[](std::size_t i) {
		assert(i < operands.size());
		assert(operands[i].get_index() == i);
		return operands[i];
	}
	const MachineOperandDesc& get(std::size_t i) const {
		assert(i < operands.size());
		assert(operands[i].get_index() == i);
		return operands[i];
	}
	MachineOperandDesc& get(std::size_t i) {
		// TODO: This assert fails while compiling javax/crypto/CryptoPolicyParser#peek
		//       Investigate why, fix and re-enable assertion.
		// assert(i < operands.size());
		if (i >= operands.size()) {
			throw std::runtime_error("MachineInstruction#get out of bounds access");
		}
		assert(operands[i].get_index() == i);
		return operands[i];
	}
	operand_iterator begin() {
		return operands.begin();
	}
	operand_iterator end() {
		return operands.end();
	}
	operand_iterator results_begin() {
		return results.begin();
	}
	operand_iterator results_end() {
		return results.end();
	}
	operand_iterator find(MachineOperand *op) {
		for (operand_iterator i = begin(), e =end(); i != e; ++i) {
			if (op->aquivalent(*i->op))
				return i;
		}
		return end();
	}
	MachineOperandDesc& front() {
		return operands.front();
	}
	MachineOperandDesc& back() {
		return operands.back();
	}
	const_operand_iterator begin() const {
		return operands.begin();
	}
	const_operand_iterator end() const {
		return operands.end();
	}
	const_operand_iterator results_begin() const {
		return results.begin();
	}
	const_operand_iterator results_end() const {
		return results.end();
	}
	std::size_t dummy_op_size() const {
		return dummy_operands.size();
	}
	const MachineOperandDesc& get_dummy(std::size_t i) const {
		assert(i < dummy_operands.size());
		assert(dummy_operands[i].get_index() == i);
		return dummy_operands[i];
	}
	MachineOperandDesc& get_dummy(std::size_t i) {
		assert(i < dummy_operands.size());
		assert(dummy_operands[i].get_index() == i);
		return dummy_operands[i];
	}
	dummy_operand_iterator dummy_begin() {
		return dummy_operands.begin();
	}
	dummy_operand_iterator dummy_end() {
		return dummy_operands.end();
	}
	dummy_operand_iterator find_dummy(MachineOperand *op) {
		for (dummy_operand_iterator i = dummy_begin(), e =dummy_end(); i != e; ++i) {
			if (op->aquivalent(*i->op))
				return i;
		}
		return dummy_end();
	}
	MachineOperandDesc& dummy_front() {
		return dummy_operands.front();
	}
	MachineOperandDesc& dummy_back() {
		return dummy_operands.back();
	}
	const_dummy_operand_iterator dummy_begin() const {
		return dummy_operands.begin();
	}
	const_dummy_operand_iterator dummy_end() const {
		return dummy_operands.end();
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
	MachineBasicBlock* successor_front() const {
		return successors.front();
	}
	MachineBasicBlock* successor_back() const {
		return successors.back();
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
	std::size_t get_id() const {
		return id;
	}
	const char* get_name() const {
		return name;
	}
	const MachineOperandDesc& get_result() const {
		assert(results.size() > 0);
		return results.front();
	}
	MachineOperandDesc& get_result() {
		assert(results.size() > 0);
		return results.front();
	}
	void set_result(MachineOperand *MO) {
		assert(results.size() > 0);
		results[0] = MachineOperandDesc(0,MO);
	}
	void add_result(MachineOperand *MO) {
		assert(MO);
		results.emplace_back(this, MO);
	}
	const std::size_t get_step() const {
		return step;
	}
	void set_step(const std::size_t s) {
		step = s;
	}
	u2 get_line() const { return line; }
	void set_line(u2 l) {line = l; }
	virtual bool accepts_immediate(std::size_t i, Immediate *imm) const {
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
		return false;
	}
	virtual bool is_call() const {
		return false;
	}
	virtual bool is_trap() const {
		return false;
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
	virtual MachineReplacementPointInst* to_MachineReplacementPointInst() {
		return NULL;
	}
	/// print instruction
	OStream& print(OStream &OS) const;
	/// print successor label
	virtual OStream& print_successor_label(OStream &OS,std::size_t index) const;
	/// print operands
	virtual OStream& print_operands(OStream &OS) const;
	/// print result
	virtual OStream& print_result(OStream &OS) const;

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
