/* src/vm/jit/compiler2/MethodC2.hpp - Method

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

#ifndef _JIT_COMPILER2_METHOD
#define _JIT_COMPILER2_METHOD

#include <cstddef>

#include "vm/jit/compiler2/alloc/vector.hpp"
#include "vm/jit/compiler2/alloc/list.hpp"

#include "vm/method.hpp"

// forware declaration
struct methodinfo;
class Utf8String;

namespace cacao {
class OStream;
}

namespace cacao {
namespace jit {
namespace compiler2 {

// forware declaration
class Instruction;
class BeginInst;
class MethodDescriptor;

template <typename _NodeType>
struct edge {
	_NodeType predecessor;
	_NodeType successsor;
	edge(_NodeType p, _NodeType s) : predecessor(p), successsor(s) {}
};

template <typename _NodeType>
edge<_NodeType> make_edge (_NodeType x, _NodeType y) {
	return ( edge<_NodeType>(x,y) );
}

class Method {
public:
	typedef alloc::list<Instruction*>::type InstructionListTy;
	typedef alloc::list<BeginInst*>::type BBListTy;
	typedef InstructionListTy::iterator iterator;
	typedef InstructionListTy::const_iterator const_iterator;
	typedef BBListTy::iterator bb_iterator;
	typedef BBListTy::const_iterator const_bb_iterator;
private:
	/**
	 * This is were the instructions live.
	 */
	InstructionListTy inst_list;
	BBListTy bb_list;
	BeginInst* init_bb;
	methodinfo *method;
	MethodDescriptor* method_desc;
	Utf8String &class_name_utf8;
	Utf8String &method_name_utf8;
	Utf8String &method_desc_utf8;

	/**
	 * Line number of the current instruction during SSA construction.
	 * Everytime add_Instruction is called, the line number of the
	 * Instruction is set to the value of current_line.
	 */
	u2 current_line = 0;

public:
	Method(methodinfo *m);
	~Method();

	const Utf8String& get_name_utf8() const { return method_name_utf8; }
	const Utf8String& get_class_name_utf8() const { return class_name_utf8; }
	const Utf8String& get_desc_utf8() const { return method_desc_utf8; }

	bool is_static() const { return method->flags & ACC_STATIC; }

	/**
	 * Add instructions to a Method.
	 *
	 * Instructions added via this method will be deleted by ~Method.
	 * use remove_Instruction() to delete them manually.
	 */
	void add_Instruction(Instruction* I);

	/**
	 * Remove an Instruction for a Method.
	 *
	 * The constructor will be called
	 */
	void remove_Instruction(Instruction* I);

	/**
	 * Replaces the instructions of the method, with the instructions in the
	 * given range [first, last).
	 *
	 * Instructions added via this method will be deleted by ~Method.
	 * use remove_Instruction() to delete them manually.
	 */
	template <class InputIterator>
	void replace_instruction_list(InputIterator first, InputIterator last) {
		inst_list.assign(first, last);
	}

	/**
	 * Add a BeginInst.
	 *
	 * The Instruction will be added using add_Instruction().
	 */
	void add_bb(BeginInst *bi);

	/**
	 * Remove BeginInst.
	 *
	 * The Instruction will be removed using remove_Instruction().
	 */
	void remove_bb(BeginInst *bi);

	void set_init_bb(BeginInst *bi) {
		init_bb = bi;
	}

	BeginInst* get_init_bb() const {
		return init_bb;
	}

	const_iterator begin() const {
		return inst_list.begin();
	}

	const_iterator end() const {
		return inst_list.end();
	}

	size_t size() const {
		return inst_list.size();
	}

	const_bb_iterator bb_begin() const {
		return bb_list.begin();
	}

	const_bb_iterator bb_end() const {
		return bb_list.end();
	}

	size_t bb_size() const {
		return bb_list.size();
	}

	/**
	 * Get the MethodDescriptor
	 */
	const MethodDescriptor& get_MethodDescriptor() const {
		return *method_desc;
	}

	void clear_schedule() const;

	/**
	 * Get a BeginInst representing an edge
	 */
	BeginInst* get_edge_block(BeginInst* pred, BeginInst* succ);

	void set_current_line(u2 value) { current_line = value; }
};

OStream& operator<<(OStream &OS, const Method &M);

} // end namespace cacao
} // end namespace jit
} // end namespace compiler2

#endif /* _JIT_COMPILER2_METHOD */


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
