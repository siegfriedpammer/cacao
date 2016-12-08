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

class CHECKNULLInst : public Instruction {
public:
	explicit CHECKNULLInst(Type::TypeID type) : Instruction(CHECKNULLInstID, type) {}
	virtual CHECKNULLInst* to_CHECKNULLInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
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
	bool resolved;
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
	explicit CONSTInst(void *anyptr, Type::ReferenceType, bool resolved) : Instruction(CONSTInstID, Type::ReferenceTypeID), resolved(resolved) {
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
	void* get_Anyptr() const {
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

	bool is_resolved() const { return resolved; }
};

class GETFIELDInst : public Instruction {
private:
	constant_FMIref *fmiref;
	bool resolved;
public:
	/**
	 * @param resolved This _should_ not change during compilation
	 */
	explicit GETFIELDInst(Type::TypeID type, constant_FMIref *fmiref, bool resolved,
			BeginInst *begin, Instruction *state_change, Value *s1)
			: Instruction(GETFIELDInstID, type), fmiref(fmiref), resolved(resolved) {
		append_dep(begin);
		append_dep(state_change);
		append_op(s1);
	}
	virtual BeginInst* get_BeginInst() const {
		BeginInst *begin = dep_front()->to_BeginInst();
		assert(begin);
		return begin;
	}
	virtual bool has_side_effects() const { return true; }
	virtual bool is_floating() const { return false; }
	virtual bool is_homogeneous() const { return false; }
	virtual GETFIELDInst* to_GETFIELDInst() { return this; }
	bool is_resolved() const { return resolved; }
	constant_FMIref* get_fmiref() const { return fmiref; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class PUTFIELDInst : public Instruction {
private:
	constant_FMIref *fmiref;
	bool resolved;
public:
	/**
	 * @param resolved This _should_ not change during compilation
	 */
	explicit PUTFIELDInst(Value *s1, Value *s2, constant_FMIref *fmiref, bool resolved,
			BeginInst* begin, Instruction *state_change)
			: Instruction(PUTFIELDInstID, s2->get_type()), fmiref(fmiref), resolved(resolved) {
		append_op(s1);
		append_op(s2);
		append_dep(begin);
		//append_dep(state_change);
	}
	virtual BeginInst* get_BeginInst() const {
		BeginInst *begin = (*dep_begin())->to_BeginInst();
		assert(begin);
		return begin;
	}
	virtual bool has_side_effects() const { return true; }
	virtual bool is_floating() const { return false; }
	virtual bool is_homogeneous() const { return false; }
	virtual PUTFIELDInst* to_PUTFIELDInst() { return this; }
	bool is_resolved() const { return resolved; }
	constant_FMIref* get_fmiref() const { return fmiref; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class PUTSTATICInst : public Instruction {
private:
	constant_FMIref *fmiref;
	bool resolved;
public:
	/**
	 * @param resolved This _should_ not change during compilation
	 */
	explicit PUTSTATICInst(Value *value, constant_FMIref *fmiref, bool resolved,
			BeginInst* begin, Instruction *state_change)
			: Instruction(PUTSTATICInstID, value->get_type()), fmiref(fmiref), resolved(resolved) {
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
	bool is_resolved() const { return resolved; }
	constant_FMIref* get_fmiref() const { return fmiref; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class GETSTATICInst : public Instruction {
private:
	constant_FMIref *fmiref;
	bool resolved;
public:
	/**
	 * @param resolved This _should_ not change during compilation
	 */
	explicit GETSTATICInst(Type::TypeID type, constant_FMIref *fmiref, bool resolved,
			BeginInst *begin, Instruction *state_change)
			: Instruction(GETSTATICInstID, type), fmiref(fmiref), resolved(resolved) {
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
	bool is_resolved() const { return resolved; }
	constant_FMIref* get_fmiref() const { return fmiref; }
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

class ARRAYBOUNDSCHECKInst : public BinaryInst {
public:
	explicit ARRAYBOUNDSCHECKInst(Type::TypeID type, Value* ref, Value* index)
			: BinaryInst(ARRAYBOUNDSCHECKInstID, Type::VoidTypeID, ref, index) {
		assert(ref->get_type() == Type::ReferenceTypeID);
		assert(index->get_type() == Type::IntTypeID);
	}
	virtual ARRAYBOUNDSCHECKInst* to_ARRAYBOUNDSCHECKInst() { return this; }
	virtual bool is_homogeneous() const { return false; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
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

class BUILTINInst : public MultiOpInst {
private:
	MethodDescriptor MD;
	constant_FMIref *fmiref;
	bool resolved;
	builtintable_entry *bte;
public:
	explicit BUILTINInst(Type::TypeID type, unsigned size,
			constant_FMIref *fmiref, builtintable_entry* bte, bool resolved, BeginInst *begin, Instruction *state_change)
			: MultiOpInst(BUILTINInstID, type), MD(size),
			fmiref(fmiref), resolved(resolved), bte(bte) {
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
	bool is_resolved() const { return resolved; }
	constant_FMIref* get_fmiref() const { return fmiref; }
	builtintable_entry* get_bte() const { return bte; }
	virtual BUILTINInst* to_BUILTINInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};


class INVOKEVIRTUALInst : public Instruction {
public:
	explicit INVOKEVIRTUALInst(Type::TypeID type) : Instruction(INVOKEVIRTUALInstID, type) {}
	virtual INVOKEVIRTUALInst* to_INVOKEVIRTUALInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class INVOKESPECIALInst : public MultiOpInst {
private:
	MethodDescriptor MD;
	constant_FMIref *fmiref;
	bool resolved;
public:
	explicit INVOKESPECIALInst(Type::TypeID type, unsigned size,
			constant_FMIref *fmiref, bool resolved, BeginInst *begin, Instruction *state_change)
			: MultiOpInst(INVOKESPECIALInstID, type), MD(size),
			fmiref(fmiref), resolved(resolved) {
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
	bool is_resolved() const { return resolved; }
	constant_FMIref* get_fmiref() const { return fmiref; }
	virtual INVOKESPECIALInst* to_INVOKESPECIALInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class INVOKESTATICInst : public MultiOpInst {
private:
	MethodDescriptor MD;
	constant_FMIref *fmiref;
	bool resolved;
public:
	explicit INVOKESTATICInst(Type::TypeID type, unsigned size,
			constant_FMIref *fmiref, bool resolved, BeginInst *begin, Instruction *state_change)
			: MultiOpInst(INVOKESTATICInstID, type), MD(size),
			fmiref(fmiref), resolved(resolved) {
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
	bool is_resolved() const { return resolved; }
	constant_FMIref* get_fmiref() const { return fmiref; }
	virtual INVOKESTATICInst* to_INVOKESTATICInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { v.visit(this, copyOperands); }
};

class INVOKEINTERFACEInst : public Instruction {
public:
	explicit INVOKEINTERFACEInst(Type::TypeID type) : Instruction(INVOKEINTERFACEInstID, type) {}
	virtual INVOKEINTERFACEInst* to_INVOKEINTERFACEInst() { return this; }
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
