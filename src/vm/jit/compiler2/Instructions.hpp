/* src/vm/jit/compiler2/Instructions.hpp - Instructions

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

/**
 * This file contains the Instruction class.
 * The Instruction class is the base class for all other instruction types.
 * They are defined in Instructions.hpp.
 */

#ifndef _JIT_COMPILER2_INSTRUCTIONS
#define _JIT_COMPILER2_INSTRUCTIONS

#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/Conditional.hpp"
#include "vm/jit/compiler2/MethodDescriptor.hpp"
#include "vm/jit/compiler2/InstructionVisitor.hpp"
#include "vm/types.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {


// Instruction groups
class LoadInst : public Instruction {
public:
	explicit LoadInst(InstID id, Type::TypeID type, BeginInst *begin) : Instruction(id, type, begin) {}
	virtual bool is_floating() const { return false; }
};

class UnaryInst : public Instruction {
public:
	explicit UnaryInst(InstID id, Type::TypeID type, Value* S1) : Instruction(id, type) {
		append_op(S1);
	}
	virtual UnaryInst* to_UnaryInst() { return this; }
};

/**
 * Binary Instruction. Not a real instruction. For convenience only.
 *
 * @note Idea: create a ArithmeticInst superclass which features a method
 * e.g. simplify() which returns the result of the operands if they are
 * constants.
 */
class BinaryInst : public Instruction {
public:
	explicit BinaryInst(InstID id, Type::TypeID type, Value* S1, Value* S2) : Instruction(id, type) {
		append_op(S1);
		append_op(S2);
	}
	virtual BinaryInst* to_BinaryInst() { return this; }
};

class MultiOpInst : public Instruction {
public:
	explicit MultiOpInst(InstID id, Type::TypeID type) : Instruction(id, type) {}

	// exporting to the public
	using Instruction::append_op;
	using Instruction::replace_op;
};

/**
 * TODO not a real Instruction... hmm
 */
class CondInst {
protected:
	Conditional::CondID cond;
public:
	explicit CondInst(Conditional::CondID cond) : cond(cond){}
	Conditional::CondID get_condition() const { return cond; }
};

/// An Instruction that is aware of the source state it corresponds to.
class SourceStateAwareInst {
private:

	/// The source state that corresponds to this Instruction.
	SourceStateInst *source_state;

	/// Set the source state that corresonds to this instruction.
	///
	/// This will be done automatically by the SourceStateAttachmentPass.
	///
	/// @param s The source state.
	void set_source_state(SourceStateInst *s) {
		assert(s != NULL);
		assert(source_state == NULL);
		source_state = s;
	}

public:

	SourceStateAwareInst() : source_state(NULL) {}

	/// Get the source state that corresponds to this Instruction.
	///
	/// @return The source state if available, NULL otherwise.
	SourceStateInst *get_source_state() const { return source_state; }

	virtual ~SourceStateAwareInst() {};

	friend class SourceStateAttachmentPass;
};

/**
 * This Instruction mark the start of a basic block
 */
class BeginInst : public Instruction {
public:
	typedef alloc::vector<BeginInst*>::type PredecessorListTy;
	typedef PredecessorListTy::const_iterator const_pred_iterator;
private:
	EndInst *end;
	PredecessorListTy pred_list;

	void set_predecessor(int index, BeginInst *BI) {
		pred_list[index] = BI;
	}
	void set_successor(int index, BeginInst *BI);

public:
	explicit BeginInst() : Instruction(BeginInstID, Type::VoidTypeID) {
		end = NULL;
	}
	explicit BeginInst(EndInst *end) : Instruction(BeginInstID, Type::VoidTypeID), end(end) {}
	virtual BeginInst* to_BeginInst() { return this; }
	virtual BeginInst *get_BeginInst() const { return const_cast<BeginInst*>(this); }
	int get_predecessor_index(const BeginInst* BI) const {
		if (BI) {
			int index = 0;
			for (PredecessorListTy::const_iterator i = pred_list.begin(),
					e = pred_list.end(); i != e; ++i) {
				if (BI == (*i)) {
					return index;
				}
				index++;
			}
		}
		return -1;
	}
	BeginInst* get_predecessor(int index) const {
		if (index < 0 || (unsigned)index > pred_size()) {
			return NULL;
		}
		PredecessorListTy::const_iterator i = pred_list.begin();
		while(index--) {
			++i;
		}
		assert(i != pred_list.end());
		return *i;
	}
	int get_successor_index(const BeginInst* BI) const;

	EndInst *get_EndInst() const { return end; }
	void set_EndInst(EndInst* e) { end = e; }
	virtual bool is_floating() const { return false; }

	const_pred_iterator pred_begin() const { return pred_list.begin(); }
	const_pred_iterator pred_end()   const { return pred_list.end(); }
	std::size_t pred_size() const { return pred_list.size(); }

	friend class EndInst;
	friend class Method;
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

/**
 * this stores a reference to a begin instruction
 */
class BeginInstRef {
private:
	BeginInst* begin;
public:
	// constructor
	explicit BeginInstRef(BeginInst* BI) : begin(BI) {}
	/// copy constructor
	BeginInstRef(const BeginInstRef &other) : begin(other.begin) {}
	/// copy assignment operator
	BeginInstRef& operator=(const BeginInstRef &other) {
		begin = other.begin;
		return (*this);
	}
	// assign BeginInst
	BeginInstRef& operator=(BeginInst *BI) {
		begin = BI;
		return (*this);
	}
	// conversion
	operator BeginInst*() {
		return begin;
	}
	// getter
	BeginInst* get() const {
		return begin;
	}
};

inline OStream& operator<<(OStream &OS, const BeginInstRef &BIR) {
	return OS << BIR.get();
}

/**
 * This Instruction mark the end of a basic block
 */
class EndInst : public Instruction {
public:
	typedef alloc::vector<BeginInstRef>::type SuccessorListTy;
	typedef SuccessorListTy::const_iterator succ_const_iterator;
	typedef SuccessorListTy::const_reverse_iterator succ_const_reverse_iterator;
private:
	SuccessorListTy succ_list;
	void set_successor(int index, BeginInst *BI) {
		succ_list[index] = BI;
	}
public:
	explicit EndInst(BeginInst* begin) : Instruction(EndInstID, Type::VoidTypeID, begin) {
		assert(begin);
		begin->set_EndInst(this);
	}
	explicit EndInst(InstID id, BeginInst* begin) : Instruction(id, Type::VoidTypeID, begin) {
		assert(begin);
		begin->set_EndInst(this);
	}
	virtual EndInst* to_EndInst() { return this; }
	virtual bool is_floating() const { return false; }

	void append_succ(BeginInst* bi) {
		assert(bi);
		succ_list.push_back(BeginInstRef(bi));
		assert(begin);
		bi->pred_list.push_back(begin);
	}
	#if 0
	void replace_succ(BeginInst* s_old, BeginInst* s_new) {
		assert(s_new);
		// replace the successor of this EndInst with the new block s_new
		std::replace(succ_list.begin(), succ_list.end(), s_old, s_new);
		// update pred link of the new BeginInst
		assert(begin);
		s_new->pred_list.push_back(begin);

		// remove the predecessor of s_old
		s_old->pred_list.remove(begin);
	}
	#endif
	int get_successor_index(const BeginInst* BI) const {
		if (BI) {
			int index = 0;
			for (SuccessorListTy::const_iterator i = succ_list.begin(),
					e = succ_list.end(); i != e; ++i) {
				if (BI == i->get()) {
					return index;
				}
				index++;
			}
		}
		return -1;
	}
	SuccessorListTy::const_iterator succ_begin() const { return succ_list.begin(); }
	SuccessorListTy::const_iterator succ_end()   const { return succ_list.end(); }
	SuccessorListTy::const_reverse_iterator succ_rbegin() const { return succ_list.rbegin(); }
	SuccessorListTy::const_reverse_iterator succ_rend()   const { return succ_list.rend(); }
	BeginInstRef &succ_front() { return succ_list.front(); }
	BeginInstRef &succ_back() { return succ_list.back(); }
	size_t succ_size() const { return succ_list.size(); }

	BeginInstRef& get_successor(size_t i) {
		assert(i < succ_size());
		return succ_list[i];
	}

	friend class BeginInst;
	friend class Method;
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

inline int BeginInst::get_successor_index(const BeginInst* BI) const {
	return get_EndInst()->get_successor_index(BI);
}
inline void BeginInst::set_successor(int index, BeginInst *BI) {
	get_EndInst()->set_successor(index,BI);
}

// Instructions

class NOPInst : public Instruction {
public:
	explicit NOPInst() : Instruction(NOPInstID, Type::VoidTypeID) {}
	virtual NOPInst* to_NOPInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class POPInst : public Instruction {
public:
	explicit POPInst(Type::TypeID type) : Instruction(POPInstID, type) {}
	virtual POPInst* to_POPInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class CHECKNULLInst : public UnaryInst, public SourceStateAwareInst {
public:
	explicit CHECKNULLInst(Value *S1) : UnaryInst(CHECKNULLInstID, Type::VoidTypeID, S1) {}
	virtual CHECKNULLInst* to_CHECKNULLInst() { return this; }
	virtual bool is_homogeneous() const { return false; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
	virtual ~CHECKNULLInst() {};
};

class ARRAYLENGTHInst : public UnaryInst {
public:
	explicit ARRAYLENGTHInst(Value *S1) : UnaryInst(ARRAYLENGTHInstID, Type::IntTypeID, S1) {}
	virtual ARRAYLENGTHInst* to_ARRAYLENGTHInst() { return this; }
	virtual bool is_homogeneous() const { return false; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class NEGInst : public UnaryInst {
public:
	explicit NEGInst(Type::TypeID type,Value *S1) : UnaryInst(NEGInstID, type, S1) {}
	virtual NEGInst* to_NEGInst() { return this; }
	virtual bool is_arithmetic() const { return true; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class CASTInst : public UnaryInst {
public:
	explicit CASTInst(Type::TypeID type, Value *s1)
		: UnaryInst(CASTInstID, type, s1) {
	}
	virtual CASTInst* to_CASTInst() { return this; }
	virtual bool is_homogeneous() const { return false; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class ADDInst : public BinaryInst {
public:
	explicit ADDInst(Type::TypeID type, Value* S1, Value* S2) : BinaryInst(ADDInstID, type, S1, S2) {}
	virtual ADDInst* to_ADDInst() { return this; }
	virtual bool is_arithmetic() const { return true; }
	virtual bool is_commutable() const { return true; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class SUBInst : public BinaryInst{
public:
	explicit SUBInst(Type::TypeID type, Value* S1, Value* S2) : BinaryInst(SUBInstID, type, S1, S2) {}
	virtual SUBInst* to_SUBInst() { return this; }
	virtual bool is_arithmetic() const { return true; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class MULInst : public BinaryInst {
public:
	explicit MULInst(Type::TypeID type, Value* S1, Value* S2) : BinaryInst(MULInstID, type, S1, S2) {}
	virtual MULInst* to_MULInst() { return this; }
	virtual bool is_arithmetic() const { return true; }
	virtual bool is_commutable() const { return true; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class DIVInst : public BinaryInst {
public:
	explicit DIVInst(Type::TypeID type, Value* S1, Value* S2) : BinaryInst(DIVInstID, type, S1, S2) {}
	virtual DIVInst* to_DIVInst() { return this; }
	virtual bool is_arithmetic() const { return true; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class REMInst : public BinaryInst {
public:
	explicit REMInst(Type::TypeID type, Value* S1, Value* S2) : BinaryInst(REMInstID, type, S1, S2) {}
	virtual REMInst* to_REMInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class SHLInst : public Instruction {
public:
	explicit SHLInst(Type::TypeID type) : Instruction(SHLInstID, type) {}
	virtual SHLInst* to_SHLInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class USHRInst : public Instruction {
public:
	explicit USHRInst(Type::TypeID type) : Instruction(USHRInstID, type) {}
	virtual USHRInst* to_USHRInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class ANDInst : public BinaryInst {
public:
	explicit ANDInst(Type::TypeID type, Value* S1, Value* S2) : BinaryInst(ANDInstID, type, S1, S2) {}
	virtual ANDInst* to_ANDInst() { return this; }
	virtual bool is_commutable() const { return true; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class ORInst : public BinaryInst {
public:
	explicit ORInst(Type::TypeID type, Value* S1, Value* S2) : BinaryInst(ORInstID, type, S1, S2) {}
	virtual ORInst* to_ORInst() { return this; }
	virtual bool is_commutable() const { return true; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class XORInst : public BinaryInst {
public:
	explicit XORInst(Type::TypeID type, Value* S1, Value* S2) : BinaryInst(XORInstID, type, S1, S2) {}
	virtual XORInst* to_XORInst() { return this; }
	virtual bool is_commutable() const { return true; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class CMPInst : public BinaryInst {
public:
	enum FloatHandling {
		G,
		L,
		DontCare
	};
	explicit CMPInst(Value* S1, Value* S2, FloatHandling f) : BinaryInst(CMPInstID, Type::IntTypeID, S1, S2), f(f) {}
	virtual CMPInst* to_CMPInst() { return this; }
	virtual bool is_homogeneous() const { return false; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
	FloatHandling get_FloatHandling() const { return f; }
private:
	FloatHandling f;
};

class CONSTInst : public Instruction {
private:
	typedef union {
		int32_t                   i;
		int64_t                   l;
		float                     f;
		double                    d;
		void                     *anyptr;
		java_handle_t            *stringconst;       /* for ACONST with string    */
		classref_or_classinfo     c;                 /* for ACONST with class     */
	} val_operand_t;
	val_operand_t value;
public:
	explicit CONSTInst(int32_t i,Type::IntType) : Instruction(CONSTInstID, Type::IntTypeID) {
		value.i = i;
	}
	explicit CONSTInst(int64_t l,Type::LongType) : Instruction(CONSTInstID, Type::LongTypeID) {
		value.l = l;
	}
	explicit CONSTInst(float f,Type::FloatType) : Instruction(CONSTInstID, Type::FloatTypeID) {
		value.f = f;
	}
	explicit CONSTInst(double d,Type::DoubleType) : Instruction(CONSTInstID, Type::DoubleTypeID) {
		value.d = d;
	}
	explicit CONSTInst(void *anyptr,Type::ReferenceType) : Instruction(CONSTInstID, Type::ReferenceTypeID) {
		value.anyptr = anyptr;
	}
	virtual CONSTInst* to_CONSTInst() { return this; }

	/**
	 * @bug this does not what one might expect! Do not use!
	 */
	s8 get_value() const {
		switch(get_type()) {
		case Type::LongTypeID:   return (s8)value.l;
		case Type::IntTypeID:    return (s8)value.i;
		case Type::FloatTypeID:  return (s8)value.f;
		case Type::DoubleTypeID: return (s8)value.d;
		default: assert(0); return 0;
		}
		return 0;
	}
	int32_t get_Int() const {
		assert(get_type() == Type::IntTypeID);
		return value.i;
	}
	int64_t get_Long() const {
		assert(get_type() == Type::LongTypeID);
		return value.l;
	}
	float get_Float() const {
		assert(get_type() == Type::FloatTypeID);
		return value.f;
	}
	double get_Double() const {
		assert(get_type() == Type::DoubleTypeID);
		return value.d;
	}
	void *get_Reference() const {
		assert(get_type() == Type::ReferenceTypeID);
		return value.anyptr;
	}
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
	virtual OStream& print(OStream& OS) const {
		Instruction::print(OS);
		switch(get_type()){
		case Type::IntTypeID:    return OS << " = " << get_Int();
		case Type::LongTypeID:   return OS << " = " << get_Long();
		case Type::FloatTypeID:  return OS << " = " << get_Float();
		case Type::DoubleTypeID: return OS << " = " << get_Double();
		default: break;
		}
		return OS;
	}
};

class FieldAccessInst : public Instruction {
private:
	fieldinfo *field;

public:
	explicit FieldAccessInst(InstID id, Type::TypeID type, fieldinfo *field)
		: Instruction(id, type), field(field) {
		assert(field);
	}
	
	fieldinfo *get_field() const { return field; }
	
	virtual FieldAccessInst* to_FieldAccessInst() { return this; }
};

class GETFIELDInst : public FieldAccessInst {
public:
	explicit GETFIELDInst(Type::TypeID type, Value *objectref,
			fieldinfo *field, BeginInst *begin, Instruction *state_change)
		: FieldAccessInst(GETFIELDInstID, type, field) {
		assert(objectref);
		assert(begin);
		assert(state_change);

		append_op(objectref);
		append_dep(begin);
		append_dep(state_change);
	}

	virtual BeginInst* get_BeginInst() const {
		BeginInst *begin = (*dep_begin())->to_BeginInst();
		assert(begin);
		return begin;
	}

	virtual bool verify() const { return true; }
	virtual bool has_side_effects() const { return true; }
	virtual bool is_floating() const { return false; }
	virtual GETFIELDInst* to_GETFIELDInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class PUTFIELDInst : public FieldAccessInst {
public:
	explicit PUTFIELDInst(Value *objectref, Value *value, fieldinfo *field,
			BeginInst* begin, Instruction *state_change)
			: FieldAccessInst(PUTFIELDInstID, value->get_type(), field) {
		assert(objectref);
		assert(value);
		assert(begin);
		assert(state_change);

		append_op(objectref);
		append_op(value);
		append_dep(begin);
		append_dep(state_change);
	}

	virtual BeginInst* get_BeginInst() const {
		BeginInst *begin = (*dep_begin())->to_BeginInst();
		assert(begin);
		return begin;
	}

	virtual bool verify() const { return true; }
	virtual bool has_side_effects() const { return true; }
	virtual bool is_floating() const { return false; }
	virtual PUTFIELDInst* to_PUTFIELDInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class GETSTATICInst : public FieldAccessInst {
public:
	explicit GETSTATICInst(Type::TypeID type, fieldinfo *field,
			BeginInst *begin, Instruction *state_change)
			: FieldAccessInst(GETSTATICInstID, type, field) {
		assert(begin);
		assert(state_change);

		append_dep(begin);
		append_dep(state_change);
	}

	virtual BeginInst* get_BeginInst() const {
		BeginInst *begin = dep_front()->to_BeginInst();
		assert(begin);
		return begin;
	}

	virtual bool has_side_effects() const { return true; }
	virtual bool is_floating() const { return false; }
	virtual GETSTATICInst* to_GETSTATICInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class PUTSTATICInst : public FieldAccessInst {
public:
	explicit PUTSTATICInst(Value *value, fieldinfo *field,
			BeginInst* begin, Instruction *state_change)
			: FieldAccessInst(PUTSTATICInstID, value->get_type(), field) {
		assert(value);
		assert(begin);
		assert(state_change);

		append_op(value);
		append_dep(begin);
		append_dep(state_change);
	}

	virtual BeginInst* get_BeginInst() const {
		BeginInst *begin = (*dep_begin())->to_BeginInst();
		assert(begin);
		return begin;
	}

	virtual bool has_side_effects() const { return true; }
	virtual bool is_floating() const { return false; }
	virtual PUTSTATICInst* to_PUTSTATICInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class INCInst : public Instruction {
public:
	explicit INCInst(Type::TypeID type) : Instruction(INCInstID, type) {}
	virtual INCInst* to_INCInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class AREFInst : public Instruction {
public:
	explicit AREFInst(Type::TypeID type, Value* ref, Value* index)
			: Instruction(AREFInstID, type) {
		assert(ref->get_type() == Type::ReferenceTypeID);
		assert(index->get_type() == Type::IntTypeID);
		append_op(ref);
		append_op(index);
	}
	virtual AREFInst* to_AREFInst() { return this; }
	virtual bool is_homogeneous() const { return false; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class ASTOREInst : public Instruction {
public:
	explicit ASTOREInst(Type::TypeID type, Value* ref, Value* value,
			BeginInst *begin, Instruction *state_change)
			: Instruction(ASTOREInstID, Type::VoidTypeID) {
		append_op(ref);
		append_op(value);
		append_dep(begin);
		append_dep(state_change);
	}
	virtual ASTOREInst* to_ASTOREInst() { return this; }
	virtual bool is_homogeneous() const { return false; }
	virtual bool has_side_effects() const { return true; }
	virtual BeginInst* get_BeginInst() const {
		BeginInst *begin = dep_front()->to_BeginInst();
		assert(begin);
		return begin;
	}
	virtual bool is_floating() const { return false; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
	Type::TypeID get_array_type() const {
		return this->op_back()->get_type();
	}
};

class ALOADInst : public Instruction {
public:
	explicit ALOADInst(Type::TypeID type, Value* ref, 
			BeginInst *begin, Instruction *state_change)
			: Instruction(ALOADInstID, type) {
		append_op(ref);
		append_dep(begin);
		append_dep(state_change);
	}
	virtual ALOADInst* to_ALOADInst() { return this; }
	virtual bool is_homogeneous() const { return false; }
	virtual BeginInst* get_BeginInst() const {
		assert(dep_size() > 0);
		return dep_front()->to_BeginInst();
	}
	virtual bool is_floating() const { return false; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class ARRAYBOUNDSCHECKInst : public BinaryInst, public SourceStateAwareInst {
public:
	explicit ARRAYBOUNDSCHECKInst(Type::TypeID type, Value* ref, Value* index)
			: BinaryInst(ARRAYBOUNDSCHECKInstID, Type::VoidTypeID, ref, index) {
		assert(ref->get_type() == Type::ReferenceTypeID);
		assert(index->get_type() == Type::IntTypeID);
	}
	virtual ARRAYBOUNDSCHECKInst* to_ARRAYBOUNDSCHECKInst() { return this; }
	virtual bool is_homogeneous() const { return false; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
	virtual ~ARRAYBOUNDSCHECKInst() {}
};

class RETInst : public Instruction {
public:
	explicit RETInst(Type::TypeID type) : Instruction(RETInstID, type) {}
	virtual RETInst* to_RETInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class LOADInst : public LoadInst {
private:
	unsigned index;
public:
	explicit LOADInst(Type::TypeID type, unsigned index, BeginInst* begin ) : LoadInst(LOADInstID, type, begin), index(index) {}
	virtual LOADInst* to_LOADInst() { return this; }
	unsigned get_index() const { return index; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class STOREInst : public Instruction {
public:
	explicit STOREInst(Type::TypeID type) : Instruction(STOREInstID, type) {}
	virtual STOREInst* to_STOREInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class NEWInst : public Instruction {
public:
	explicit NEWInst(Type::TypeID type) : Instruction(NEWInstID, type) {}
	virtual NEWInst* to_NEWInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class NEWARRAYInst : public Instruction {
public:
	explicit NEWARRAYInst(Type::TypeID type) : Instruction(NEWARRAYInstID, type) {}
	virtual NEWARRAYInst* to_NEWARRAYInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class ANEWARRAYInst : public Instruction {
public:
	explicit ANEWARRAYInst(Type::TypeID type) : Instruction(ANEWARRAYInstID, type) {}
	virtual ANEWARRAYInst* to_ANEWARRAYInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class MULTIANEWARRAYInst : public Instruction {
public:
	explicit MULTIANEWARRAYInst(Type::TypeID type) : Instruction(MULTIANEWARRAYInstID, type) {}
	virtual MULTIANEWARRAYInst* to_MULTIANEWARRAYInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class CHECKCASTInst : public Instruction {
public:
	explicit CHECKCASTInst(Type::TypeID type) : Instruction(CHECKCASTInstID, type) {}
	virtual CHECKCASTInst* to_CHECKCASTInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class INSTANCEOFInst : public Instruction {
public:
	explicit INSTANCEOFInst(Type::TypeID type) : Instruction(INSTANCEOFInstID, type) {}
	virtual INSTANCEOFInst* to_INSTANCEOFInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class GOTOInst : public EndInst {
private:
	/**
	 * @note this is for expert use only.
	 * You will have to adjust the successor (and the backlink) manually.
	 */
	explicit GOTOInst(BeginInst *begin)
			: EndInst(GOTOInstID, begin) {}
public:
	explicit GOTOInst(BeginInst *begin, BeginInst* targetBlock)
			: EndInst(GOTOInstID, begin) {
		append_succ(targetBlock);
	}
	virtual GOTOInst* to_GOTOInst() { return this; }
	BeginInstRef& get_target() {
		return succ_front();
	}
	friend class Method;
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class JSRInst : public Instruction {
public:
	explicit JSRInst(Type::TypeID type) : Instruction(JSRInstID, type) {}
	virtual JSRInst* to_JSRInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class INVOKEInst : public MultiOpInst {
private:
	MethodDescriptor MD;
	constant_FMIref *fmiref;

public:
	explicit INVOKEInst(InstID ID, Type::TypeID type, unsigned size,
			constant_FMIref *fmiref, BeginInst *begin, Instruction *state_change)
			: MultiOpInst(ID, type), MD(size),
			fmiref(fmiref) {
		assert(state_change && state_change->get_name());
		append_dep(begin);
		append_dep(state_change);
	}
	virtual BeginInst* get_BeginInst() const {
		BeginInst *begin = (*dep_begin())->to_BeginInst();
		assert(begin);
		return begin;
	}
	virtual bool has_side_effects() const { return true; }
	virtual bool is_floating() const { return false; }
	virtual bool is_homogeneous() const { return false; }
	void append_parameter(Value *V) {
		std::size_t i = op_size();
		assert(i < MD.size());
		MD[i] = V->get_type();
		append_op(V);
	}
	MethodDescriptor& get_MethodDescriptor() {
		assert(MD.size() == op_size());
		return MD;
	}
	constant_FMIref* get_fmiref() const { return fmiref; }
	virtual INVOKEInst* to_INVOKEInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class INVOKESTATICInst : public INVOKEInst {
public:
	explicit INVOKESTATICInst(Type::TypeID type, unsigned size,
			constant_FMIref *fmiref, BeginInst *begin, Instruction *state_change)
		: INVOKEInst(INVOKESTATICInstID, type, size, fmiref, begin, state_change) {}
	virtual INVOKESTATICInst* to_INVOKESTATICInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class INVOKEVIRTUALInst : public INVOKEInst {
public:
	explicit INVOKEVIRTUALInst(Type::TypeID type, unsigned size,
			constant_FMIref *fmiref, BeginInst *begin, Instruction *state_change)
		: INVOKEInst(INVOKEVIRTUALInstID, type, size, fmiref, begin, state_change) {}
	virtual INVOKEVIRTUALInst* to_INVOKEVIRTUALInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class INVOKESPECIALInst : public INVOKEInst {
public:
	explicit INVOKESPECIALInst(Type::TypeID type, unsigned size,
			constant_FMIref *fmiref, BeginInst *begin, Instruction *state_change)
		: INVOKEInst(INVOKESPECIALInstID, type, size, fmiref, begin, state_change) {}
	virtual INVOKESPECIALInst* to_INVOKESPECIALInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class INVOKEINTERFACEInst : public INVOKEInst {
public:
	explicit INVOKEINTERFACEInst(Type::TypeID type, unsigned size,
			constant_FMIref *fmiref, BeginInst *begin, Instruction *state_change)
		: INVOKEInst(INVOKEINTERFACEInstID, type, size, fmiref, begin, state_change) {}
	virtual INVOKEINTERFACEInst* to_INVOKEINTERFACEInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class BUILTINInst : public INVOKEInst {
private:

	/// A pointer to the function that implements the builtin functionality.
	u1 *address;

public:
	explicit BUILTINInst(Type::TypeID type, u1 *address, unsigned size,
		BeginInst *begin, Instruction *state_change)
		: INVOKEInst(BUILTINInstID, type, size, NULL, begin, state_change), address(address) {}

	u1 *get_address() const {
		return address;
	}

	virtual BUILTINInst* to_BUILTINInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class IFInst : public EndInst, public CondInst {
public:
	explicit IFInst(BeginInst *begin, Value* S1, Value* S2, Conditional::CondID cond,
			BeginInst* trueBlock, BeginInst* falseBlock)
			: EndInst(IFInstID, begin), CondInst(cond) {
		append_op(S1);
		append_op(S2);
		append_succ(trueBlock);
		append_succ(falseBlock);
		assert(S1->get_type() == S2->get_type());
		set_type(S1->get_type());
	}
	virtual IFInst* to_IFInst() { return this; }
	BeginInstRef &get_then_target() { return succ_front(); }
	BeginInstRef &get_else_target() { return succ_back(); }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class IF_CMPInst : public Instruction {
public:
	explicit IF_CMPInst(Type::TypeID type) : Instruction(IF_CMPInstID, type) {}
	virtual IF_CMPInst* to_IF_CMPInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class TABLESWITCHInst : public EndInst {
private:
	s4 tablelow;
	s4 tablehigh;
public:
	/// wrapper for type safety
	struct LOW {
		s4 low;
		explicit LOW(s4 low) : low(low) {}
	};
	/// wrapper for type safety
	struct HIGH {
		s4 high;
		explicit HIGH(s4 high) : high(high) {}
	};
	/// constructor
	explicit TABLESWITCHInst(BeginInst *begin, Value* S1, LOW low, HIGH high)
			: EndInst(TABLESWITCHInstID, begin), tablelow(low.low), tablehigh(high.high) {
		assert(tablehigh >= tablelow);
		append_op(S1);
		set_type(Type::IntTypeID);
	}

	s4 get_low() const { return tablelow; }
	s4 get_high() const { return tablehigh; }
	virtual TABLESWITCHInst* to_TABLESWITCHInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }

	virtual bool verify() const {
		if (tablehigh < tablelow ) {
			ERROR_MSG("TABLESWITCH verification error","tablehigh ("<<tablehigh
				<< ") is greater then tablelow ("<< tablelow << ")");
			return false;
		}
		if (succ_size() != std::size_t(tablehigh - tablelow + 1 + 1)) {
			ERROR_MSG("TABLESWITCH verification error","Number of successors (" << succ_size()
				<< ") not equal to targets (" << tablehigh - tablelow +1 << ")");
			return false;
		}
		return Instruction::verify();
	}
};

class LOOKUPSWITCHInst : public EndInst {
public:
	typedef alloc::vector<s4>::type MatchTy;
	typedef MatchTy::iterator match_iterator;
private:
	s4 lookupcount;
	MatchTy matches;
public:
	/// wrapper for type safety
	struct MATCH {
		s4 match;
		explicit MATCH(s4 match) : match(match) {}
	};
	explicit LOOKUPSWITCHInst(BeginInst *begin, Value* S1, s4 lookupcount)
			: EndInst(LOOKUPSWITCHInstID, begin), lookupcount(lookupcount), matches(lookupcount) {
		append_op(S1);
		set_type(Type::IntTypeID);
	}
	virtual LOOKUPSWITCHInst* to_LOOKUPSWITCHInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
	void set_match(s4 index, MATCH match) {
		assert(index >= 0 && index < lookupcount);
		matches[index] = match.match;
	}
	s4 get_match(s4 index) {
		return matches[index];
	}
	match_iterator match_begin() {
		return matches.begin();
	}
	match_iterator match_end() {
		return matches.end();
	}
	virtual bool verify() const {
		if ( lookupcount < 0) {
			ERROR_MSG("LOOKUPSWITCH verification error","Lookup count is negative ("
				<< lookupcount << ")");
			return false;
		}
		if (succ_size() != std::size_t(lookupcount + 1)) {
			ERROR_MSG("LOOKUPSWITCH verification error","Number of successors (" << succ_size()
				<< ") not equal to targets (" << lookupcount + 1 << ")");
			return false;
		}
		return Instruction::verify();
	}
};

class RETURNInst : public EndInst {
public:
	/// void return
	explicit RETURNInst(BeginInst *begin) : EndInst(RETURNInstID, begin) {}
	/// value return
	explicit RETURNInst(BeginInst *begin, Value* S1) : EndInst(RETURNInstID, begin) {
		append_op(S1);
		set_type(S1->get_type());
	}
	virtual RETURNInst* to_RETURNInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class THROWInst : public Instruction {
public:
	explicit THROWInst(Type::TypeID type) : Instruction(THROWInstID, type) {}
	virtual THROWInst* to_THROWInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class COPYInst : public Instruction {
public:
	explicit COPYInst(Type::TypeID type) : Instruction(COPYInstID, type) {}
	virtual COPYInst* to_COPYInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class MOVEInst : public Instruction {
public:
	explicit MOVEInst(Type::TypeID type) : Instruction(MOVEInstID, type) {}
	virtual MOVEInst* to_MOVEInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class GETEXCEPTIONInst : public Instruction {
public:
	explicit GETEXCEPTIONInst(Type::TypeID type) : Instruction(GETEXCEPTIONInstID, type) {}
	virtual GETEXCEPTIONInst* to_GETEXCEPTIONInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class PHIInst : public MultiOpInst {
public:
	explicit PHIInst(Type::TypeID type, BeginInst *begin) : MultiOpInst(PHIInstID, type) {
		append_dep(begin);
	}
	virtual PHIInst* to_PHIInst() { return this; }

	virtual BeginInst* get_BeginInst() const {
		BeginInst *begin = (*dep_begin())->to_BeginInst();
		assert(begin);
		return begin;
	}
	virtual bool is_floating() const { return false; }

	// exporting to the public
	using Instruction::append_op;
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class ContainerInst : public Instruction {
public:
	explicit ContainerInst(BeginInst *begin) : Instruction(ContainerInstID, Type::VoidTypeID) {
		append_dep(begin);
	}
	virtual ContainerInst* to_ContainerInst() { return this; }

	virtual BeginInst* get_BeginInst() const {
		BeginInst *begin = (*dep_begin())->to_BeginInst();
		assert(begin);
		return begin;
	}
	virtual bool is_floating() const { return false; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

/// Provides a mapping from HIR values to baseline IR variables.
///
/// In order to reconstruct the source state during on-stack replacement, it
/// is necessary to be able to reconstruct the machine-independent source state
/// from the current machine-dependent execution state. Due to the generality of
/// the baseline compiler's IR the on-stack replacement mechanism represents the
/// source state in terms of the baseline IR variables. Hence, it is necessary
/// to be able to map compiler2-specific HIR values to baseline IR variables. A
/// SourceStateInst provides such mapping information for a single program
/// location which is identified by the (machine-independent) ID of the
/// corresponding baseline IR instruction.
class SourceStateInst : public Instruction {
public:

	/// Maps a HIR value to a baseline IR javalocal index.
	struct Javalocal {

		/// The value that is mapped to a baseline IR javalocal index.
		Value *value;

		/// A baseline IR javalocal index.
		std::size_t index;

		/// Construct a Javalocal that maps @p value to @p index
		///
		/// @param value The Value that is mapped to @p index.
		/// @param index The javalocal index that corresponds to @p value.
		explicit Javalocal(Value *value, std::size_t index) : value(value), index(index) {}
	};

private:

	typedef alloc::vector<Javalocal>::type JavalocalListTy;
	typedef alloc::vector<Value*>::type StackvarListTy;
	typedef alloc::vector<Value*>::type ParamListTy;

	/// The program location corresponding to the provided mapping information.
	///
	/// The mappings of HIR values to baseline IR variables, which are given by
	/// this SourceStateInst, are bound to this exact program location. This
	/// location is given in terms of an ID of the corresponding baseline IR
	/// instruction.
	s4 source_location;

	/// The state of the enclosing method in case we are in an inlined region.
	SourceStateInst *parent;

	/// List of mappings from HIR values to baseline IR javalocal indices.
	JavalocalListTy javalocals;

	/// List of HIR values that correspond to baseline IR stack variables.
	StackvarListTy stackvars;

	/// List of HIR values that correspond to method parameters.
	ParamListTy params;

public:

	typedef JavalocalListTy::const_iterator const_javalocal_iterator;
	typedef StackvarListTy::const_iterator const_stackvar_iterator;
	typedef ParamListTy::const_iterator const_param_iterator;

	/// Construct a SourceStateInst.
	///
	/// @param source_location The ID of the baseline IR instruction at the
	///                        mapped program location.
	/// @param hir_location    The HIR instruction that corresponds to the
	///                        given @p source_location.
	explicit SourceStateInst(s4 source_location, Instruction *hir_location) :
			Instruction(SourceStateInstID, Type::VoidTypeID),
			source_location(source_location) {
		assert(!hir_location->is_floating());
		append_dep(hir_location);
	}

	virtual BeginInst* get_BeginInst() const {
		BeginInst *begin = (*dep_begin())->get_BeginInst();
		assert(begin);
		return begin;
	}

	/// Get the program location that corresponds to the given mappings.
	s4 get_source_location() const { return source_location; }

	/// The state of the enclosing method in case we are in an inlined region.
	SourceStateInst *get_parent() const { return parent; };

	/// Set state of the enclosing method in case we are in an inlined region.
	///
 	/// @param new_parent The corresponding source state.
	void set_parent(SourceStateInst *new_parent) { parent = new_parent; }

	/// Append a new mapping from a HIR Value to a baseline IR javalocal index.
	///
	/// @param local The Javalocal to append.
	void append_javalocal(Javalocal local) {
		append_op(local.value);
		javalocals.push_back(local);
	}

	/// Append a new value to corresponds to a baseline IR stack variable.
	///
	/// @param value The value to append.
	void append_stackvar(Value *value) {
		append_op(value);
		stackvars.push_back(value);
	}

	/// Append a new value to corresponds to a method parameter.
	///
	/// @param value The value to append.
	void append_param(Value *value) {
		append_op(value);
		params.push_back(value);
	}

	const_javalocal_iterator javalocal_begin() const { return javalocals.begin(); }
	const_javalocal_iterator javalocal_end()   const { return javalocals.end(); }

	const_stackvar_iterator stackvar_begin() const { return stackvars.begin(); }
	const_stackvar_iterator stackvar_end()   const { return stackvars.end(); }

	const_param_iterator param_begin() const { return params.begin(); }
	const_param_iterator param_end()   const { return params.end(); }

	virtual void replace_op(Value* v_old, Value* v_new) {
		Instruction::replace_op(v_old, v_new);
		std::replace(stackvars.begin(), stackvars.end(), v_old, v_new);
		std::replace(params.begin(), params.end(), v_old, v_new);
		for (JavalocalListTy::iterator i = javalocals.begin(), e = javalocals.end();
				i != e; i++) {
			Javalocal &local = *i;
			if (local.value == v_old) {
				local.value = v_new;
			}
		}
	}

	virtual SourceStateInst* to_SourceStateInst() { return this; }
	virtual bool is_floating() const { return false; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
	virtual bool verify() const { return true; }
};

/// A point where the method can be entered through on-stack replacement.
class ReplacementEntryInst : public Instruction {
public:

	/// Construct a ReplacementEntryInst.
	///
	/// @param begin        The corresponding BeginInst.
	/// @param source_state The corresponding source state.
	explicit ReplacementEntryInst(BeginInst *begin, SourceStateInst *source_state)
			: Instruction(ReplacementEntryInstID, Type::VoidTypeID) {
		assert(begin);
		assert(source_state);
		append_dep(begin);
		append_dep(source_state);
	}

	/// Get the BeginInst this ReplacementEntryInst is attached to.
	virtual BeginInst* get_BeginInst() const {
		assert(dep_size() >= 1);
		BeginInst *begin = (*dep_begin())->to_BeginInst();
		assert(begin);
		return begin;
	}

	/// Get the source state at this ReplacementEntryInst.
	///
	/// The source state is used to reconstruct the stack frame for the
	/// optimized code. Since optimized code is currently only entered at method
	/// entry, the information that is captured by the source state is
	/// sufficient for this task.
	SourceStateInst* get_source_state() const {
		assert(dep_size() >= 2);
		SourceStateInst *source_state = (*(++dep_begin()))->to_SourceStateInst();
		assert(source_state);
		return source_state;
	}

	virtual ReplacementEntryInst* to_ReplacementEntryInst() { return this; }
	virtual bool is_floating() const { return false; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

/// Represents a speculative assumption that has to be checked at run-time.
///
/// Optimizations that rely on speculative assumptions may generate
/// AssumptionInst in order to avoid the illegal execution of speculatively
/// transformed program regions. In case a speculation fails at run-time,
/// deoptimization will be triggered to transfer execution back to an
/// unoptimized version of this method.
class AssumptionInst : public Instruction, public SourceStateAwareInst {
private:

	/// The entry to the region of code that relies on this assumption.
	BeginInst *guarded_block;

public:

	/// Construct an AssumptionInst.
	///
	/// @param condition     A boolean HIR expression that encodes the
	///                      speculative assumption to check.
	/// @param guarded_block The entry to the region of the code that relies on
	///                      this assumption and thus needs to be guarded
	///                      against illegal execution.
	explicit AssumptionInst(Value *condition, BeginInst *guarded_block)
			: Instruction(AssumptionInstID, Type::VoidTypeID) {
		assert(condition);
		assert(condition->get_type() != Type::VoidTypeID);
		append_op(condition);
		this->guarded_block = guarded_block;
		guarded_block->append_dep(this);
	}

	/// Get the entry to the region of code that relies on this assumption.
	virtual BeginInst *get_guarded_block() const {
		return guarded_block;
	}

	virtual AssumptionInst* to_AssumptionInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
	virtual ~AssumptionInst() {};
};

/// Transfers execution back to an unoptimized version of the method.
class DeoptimizeInst : public EndInst, public SourceStateAwareInst {
public:

	/// Construct a DeoptimizeInst.
	///
	/// @param begin The corresponding BeginInst.
	explicit DeoptimizeInst(BeginInst *begin)
			: EndInst(DeoptimizeInstID, begin) {
		set_type(Type::VoidTypeID);
	}

	virtual bool is_floating() const { return false; }
	virtual DeoptimizeInst* to_DeoptimizeInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
	virtual ~DeoptimizeInst() {};
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_INSTRUCTIONS */


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
