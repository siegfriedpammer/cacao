/* src/vm/jit/compiler2/Instruction.hpp - Instruction

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

#ifndef _JIT_COMPILER2_INSTRUCTION
#define _JIT_COMPILER2_INSTRUCTION

#include <algorithm>

#include <cstddef>

#include <unordered_set>
#include <vector>
#include <list>

#include "Type.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

class Instruction {
public:

	typedef std::unordered_set<Instruction*> UserListTy;


	typedef std::vector<Instruction*> OperandListTy;
	typedef std::list<Instruction*> DepListTy;

	typedef OperandListTy::iterator op_iterator;
	typedef DepListTy::iterator dep_iterator;
	typedef OperandListTy::const_iterator const_op_iterator;
	typedef DepListTy::const_iterator const_dep_iterator;

	enum InstID {

#include "InstructionIDGen.inc"

		NoInstID
	};
private:
	Type::TypeID type;

	UserListTy user_list;


	OperandListTy op_list;
	DepListTy dep_list;
	DepListTy reverse_dep_list;


protected:
	const InstID opcode;
	//Method* method;
	const int id;
	Instruction* begin;

public:
	static int id_counter;
	void append_op(Instruction* v) {
		op_list.push_back(v);
		v->append_user(this);
	}

	void append_user(Instruction* I) {
		assert(I);
		user_list.insert(I);
	}

	size_t user_size() const { return user_list.size(); }

	void replace_op(Instruction* v_old, Instruction* v_new);

	void set_type(Type::TypeID t) {
		type = t;
	}
	Type::TypeID get_type() const { return type; }

public:
	explicit Instruction(InstID opcode, Type::TypeID type, Instruction* begin = NULL)
			: /*Value(type), */opcode(opcode), id(id_counter++), begin(begin) {}

	//virtual ~Instruction();

	int get_id() const { return id; } ///< return a unique identifier for this instruction
	InstID get_opcode() const { return opcode; } ///< return the opcode of the instruction

	//void set_Method(Method* M) { method = M; }
	//Method* get_Method() const { return method; }

	const_op_iterator op_begin() const { return op_list.begin(); }
	const_op_iterator op_end()   const { return op_list.end(); }
	Instruction* op_front() const { return op_list.front(); }
	Instruction* op_back() const { return op_list.back(); }
	size_t op_size() const { return op_list.size(); }
	Instruction* get_operand(size_t i) {
		return op_list[i];
	}
	int get_operand_index(Instruction *op) const {
		for (int i = 0, e = op_list.size(); i < e; ++i) {
			if (op == op_list[i])
				return i;
		}
		return -1;
	}

	void append_dep(Instruction* I) {
		assert(I);
		dep_list.push_back(I);
		I->reverse_dep_list.push_back(this);
	}
	
	void remove_dep(Instruction* I) {
		assert(I);
		dep_list.remove(I);
		I->reverse_dep_list.remove(this);
	}


	UserListTy::const_iterator user_begin() const { return user_list.begin(); }
	UserListTy::const_iterator user_end()   const { return user_list.end(); }

	/// check if the instruction is in a correct state
	//bool verify() const override;

	const_dep_iterator dep_begin() const { return dep_list.begin(); }
	const_dep_iterator dep_end()   const { return dep_list.end(); }
	Instruction* dep_front() const { return dep_list.front(); }
	Instruction* dep_back() const { return dep_list.back(); }
	size_t dep_size() const { return dep_list.size(); }
	const_dep_iterator rdep_begin() const { return reverse_dep_list.begin(); }
	const_dep_iterator rdep_end()   const { return reverse_dep_list.end(); }
	size_t rdep_size() const { return reverse_dep_list.size(); }

	/**
	 * Get the corresponding BeginInst.
	 *
	 * BeginInst are used to mark control flow joins (aka the start of a basic block).
	 * @return The directly dominating BeginInst. NULL if there is none (eg. several
	 *         cadidates or dead code).
	 */
	virtual Instruction *get_BeginInst() const { return begin; }
	virtual bool set_BeginInst(Instruction *b) {
		if (is_floating()) {
			begin = b;
			return true;
		}
		assert(0 && "Trying to set BeginInst of a non floating instruction");
		return false;
	}
	/// True if the instruction has a homogeneous signature.
	/// (i.e. all operands and the result have the same type)
	virtual bool is_homogeneous() const { return true; }
	/// True if the instruction has no fixed control dependencies
	virtual bool is_floating() const { return true; }
	/// True the instruction has side effects
	virtual bool has_side_effects() const { return false; }
	/// True if the instruction is an arithmetic instruction
	virtual bool is_arithmetic() const { return false; }
	/// True if the operands of the instruction are commutable
	virtual bool is_commutable() const { return false; }

	// casting functions
	virtual Instruction*          to_Instruction()          { return this; }

// include to_XXXInst()'s
//#include "vm/jit/compiler2/InstructionToInstGen.inc"

// 	const char* get_name() const {
// 		switch (opcode) {
// // include switch cases
// #include "vm/jit/compiler2/InstructionNameSwitchGen.inc"

// 			case NoInstID:               return "NoInst";
// 		}
// 		return "Unknown Instruction";
// 	}

	/// Visitor
	//virtual void accept(InstructionVisitor& v) = 0;

	// needed to access replace_op in replace_value
	//friend class Value;
};

int Instruction::id_counter = 0;

/**
 * Less comparator for Instruction pointers.
 *
 * Use for std::set, std::map.
 */
/*struct InstPtrLess :
public std::binary_function<const Instruction*, const Instruction*, bool> {
	bool operator()(const Instruction *lhs, const Instruction *rhs) const {
		return lhs->get_id() < rhs->get_id();
	}
};*/

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#undef DEBUG_NAME

#endif /* _JIT_COMPILER2_INSTRUCTION */


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
