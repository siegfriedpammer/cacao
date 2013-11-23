/* src/vm/jit/compiler2/Value.hpp - Value

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

#ifndef _JIT_COMPILER2_VALUE
#define _JIT_COMPILER2_VALUE

#include "vm/jit/compiler2/Type.hpp"

#include <cstddef>
#include "vm/jit/compiler2/alloc/list.hpp"
#include <algorithm>

#include <cassert>

namespace cacao {

class OStream;

namespace jit {
namespace compiler2 {

class Instruction;

class Value {
public:
	typedef alloc::list<Instruction*>::type UserListTy;
protected:
	Type::TypeID type;
	UserListTy user_list;
	void append_user(Instruction* I) {
		assert(I);
		user_list.push_back(I);
	}
	OStream& print_users(OStream &OS) const;
public:
	Value(Type::TypeID type) : type(type) {}
	Type::TypeID get_type() const { return type; } ///< get the value type of the instruction

	virtual ~Value();

	UserListTy::const_iterator user_begin() const { return user_list.begin(); }
	UserListTy::const_iterator user_end()   const { return user_list.end(); }
	size_t user_size() const { return user_list.size(); }

	/**
	 * Replace this Value this the new one in all users.
	 */
	void replace_value(Value *v);

	virtual Instruction* to_Instruction() { return NULL; }

	/// print
	virtual OStream& print(OStream &OS) const;

	// we need this to access append_user
	friend class Instruction;
};

inline OStream& operator<<(OStream &OS, const Value &V) {
	return V.print(OS);
}
OStream& operator<<(OStream &OS, const Value *V);

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_VALUE */


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
