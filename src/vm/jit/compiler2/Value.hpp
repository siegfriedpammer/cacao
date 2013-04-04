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

#include <cstddef>
#include <list>
#include <algorithm>

#include "toolbox/logging.hpp"

#include <cassert>

namespace cacao {
namespace jit {
namespace compiler2 {

class Instruction;

#define DEBUG_NAME "compiler2/Value"
class Value {
public:
	typedef std::list<Instruction*> UserListTy;
protected:
	UserListTy user_list;
	void append_user(Instruction* I) {
		assert(I);
		user_list.push_back(I);
	}
	void remove_user(Instruction* I) {
		LOG("Value::remove_user(this=" << this << ",I=" << I << ")" << nl );
		user_list.remove(I);
		#if 0
		UserListTy::iterator f = user_list.find(I);
		if (f != user_list.end()) {
			user_list.erase(f);
		}
		user_list.erase(std::find(user_list.begin(),user_list.end(),I));
		#endif
	}
public:

	virtual ~Value();

	UserListTy::const_iterator user_begin() const { return user_list.begin(); }
	UserListTy::const_iterator user_end()   const { return user_list.end(); }
	size_t user_size() const { return user_list.size(); }

	/**
	 * Replace this Value this the new one in all users.
	 */
	void replace_value(Value *v);

	virtual Instruction* to_Instruction() { return NULL; }

	// we need this to access append_user
	friend class Instruction;
};

#undef DEBUG_NAME

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
