/* src/vm/jit/compiler2/Value.cpp - Value

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

#include "vm/jit/compiler2/Value.hpp"
#include "vm/jit/compiler2/Instruction.hpp"
#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/Value"

namespace cacao {
namespace jit {
namespace compiler2 {

OStream& operator<<(OStream &OS, const Value *V) {
	if (!V)
		return OS << "Value NULL";
	return OS << *V;
}
OStream& Value::print(OStream &OS) const {
	OS << "[Value ";
	for(UserListTy::const_iterator i = user_list.begin(), e = user_list.end(); i != e; ++i) {
		Instruction *I = *i;
		OS << I->get_id() << " ";
	}
	return print_users(OS) << "]";
}
OStream& Value::print_users(OStream &OS) const {
	OS << "Users of " << this << nl ;
	for(UserListTy::const_iterator i = user_list.begin(), e = user_list.end(); i != e; ++i) {
		OS << "user: " << *i << nl;
	}
	return OS;
}
void Value::replace_value(Value *v) {
	LOG("Value::replace_value(this=" << this << ",v=" << v << ")" << nl );
	UserListTy::iterator i;
	UserListTy::iterator e = user_list.end();
	while ( (i = user_list.begin() ) != e ) {
		#ifndef NDEBUG
		size_t size = user_list.size();
		#endif
		DEBUG(print_users(dbg()));
		Instruction *I = *i;
		LOG("replacing value " << this << " with " << v << nl );
		assert(I);
		I->replace_op(this, v);
		assert( size > user_list.size());
	}
}
#if 0
void Value::remove_user(Instruction* I) {
	#if 0 & NDEBUG
	size_t size = user_list.size();
	#endif
	LOG("Value::remove_user(this=" << this << ",I=" << I << ")" << nl );
	user_list.remove(I);
	#if 0
	if (!( size > user_list.size())) {
		WARING_MSG("corrupt double link detected","Instruction " << I << " used " << this);
	}
	UserListTy::iterator f = user_list.find(I);
	if (f != user_list.end()) {
		user_list.erase(f);
	}
	user_list.erase(std::find(user_list.begin(),user_list.end(),I));
	#endif
}
#endif
Value::~Value()
{
#if 0
#ifndef NDEBUG
	size_t size = user_list.size();
#endif

	LOG("Value::~Value(this=" << this << ")" << nl );
	UserListTy::iterator i;
	UserListTy::iterator e = user_list.end();
	while ( (i = user_list.begin() ) != e ) {
		Instruction *I = *i;
		assert(I);
		I->replace_op(this, NULL);
		assert( --size == user_list.size());
	}
#endif
}

} // end namespace cacao
} // end namespace jit
} // end namespace compiler2


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
