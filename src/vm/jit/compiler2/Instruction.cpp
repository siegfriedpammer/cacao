/* src/vm/jit/compiler2/Instruction.cpp - Instruction

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

#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/Instructions.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/Instruction"

namespace cacao {
namespace jit {
namespace compiler2 {

int Instruction::id_counter = 0;

OStream& operator<<(OStream &OS, const Instruction *I) {
	if (!I)
		return OS << "Instruction is NULL";
	return OS << *I;
}
OStream& Instruction::print(OStream& OS) const {
	return OS << "[" << get_id() << ": " << get_name() << " ("
		<< get_type() << ")]";
}

Instruction::~Instruction() {
	LOG("deleting instruction: " << this << nl);
	#if 1
	// remove from users
	for( OperandListTy::iterator i = op_list.begin(), e = op_list.end(); i != e ; ++i) {
		Value *v = *i;
		//assert(v != (Value*)this);
		// might be a NULL operand
		//if (v) {
			v->user_remove(this);
		//}
	}
	for( DepListTy::iterator i = dep_list.begin(), e = dep_list.end(); i != e ; ++i) {
		Instruction *I = *i;
		I->reverse_dep_list.remove(this);
	}
	#endif
}
void Instruction::replace_op(Value* v_old, Value* v_new) {
	LOG("Instruction:::replace_op(this=" << this << ",v_old=" << v_old << ",v_new=" << v_new << ")" << nl );
	DEBUG(print_operands(dbg()));
	std::replace(op_list.begin(),op_list.end(),v_old,v_new);
	DEBUG(print_operands(dbg()));
	v_old->user_remove(this);
	if (v_new)
		v_new->append_user(this);
}
OStream& Instruction::print_operands(OStream &OS) {
	OS << "Operands of " << this << " (" << get_name() << ")" << nl ;
	for(OperandListTy::iterator i = op_list.begin(), e = op_list.end(); i != e; ++i) {
		OS << "OP: " << *i << nl;
	}
	return OS;
}
bool Instruction::verify() const {
	if (is_homogeneous()) {
		for(OperandListTy::const_iterator i = op_list.begin(), e = op_list.end(); i != e; ++i) {
			Value *V = *i;
			if (V->get_type() != get_type()) {
				LOG(Red << "Instruction verification error!" << reset_color << "\nThis type " << *this
					<< " is not equal  to operand (" << V << ") type " << V->get_type() << nl  );
				return false;
			}
		}
	}
	LOG ("Verifying deps for " << this << nl);
	for(auto dep_it = dep_list.begin(); dep_it != dep_list.end(); dep_it++){
		auto dep = *dep_it;
		auto correct_reverse_edge = std::find(dep->rdep_begin(), dep->rdep_end(), this) != dep->rdep_end();
		if (!correct_reverse_edge) {
			LOG(Red << "Instruction verification error!" << reset_color << nl <<
			 "Missing reverse dependency edge from " << dep << " to " << this);
			return false;
		}
	}
	LOG ("Verifying reverse deps for " << this << nl);
	for(auto rdep_it = reverse_dep_list.begin(); rdep_it != reverse_dep_list.end(); rdep_it++){
		auto rdep = *rdep_it;
		auto correct_reverse_edge = std::find(rdep->dep_begin(), rdep->dep_end(), this) != rdep->rdep_end();
		if (!correct_reverse_edge) {
			LOG(Red << "Instruction verification error!" << reset_color << nl <<
			 "Obsolete reverse dependency edge from " << this << " to " << rdep);
			return false;
		}
	}
	LOG ("Verifying ops for " << this << nl);
	for(auto op_it = op_list.begin(); op_it != op_list.end(); op_it++){
		auto op = *op_it;
		auto op_inst = op->to_Instruction();

		if(!op_inst)
			continue;

		auto correct_reverse_edge = std::find(op_inst->user_begin(), op_inst->user_end(), this) != op_inst->user_end();
		if (!correct_reverse_edge) {
			LOG(Red << "Instruction verification error!" << reset_color << nl <<
			 "Missing user edge from " << op_inst << " to " << this);
			return false;
		}
	}
	LOG ("Verifying users for " << this << nl);
	for(auto user_it = user_list.begin(); user_it != user_list.end(); user_it++){
		auto user = *user_it;
		auto correct_reverse_edge = std::find(user->op_begin(), user->op_end(), this) != user->op_end();
		if (!correct_reverse_edge) {
			LOG(Red << "Instruction verification error!" << reset_color << nl <<
			 "Obsolete op edge from " << this << " to " << user);
			return false;
		}
	}
	return true;
}


} // end namespace compiler2
} // end namespace jit
} // end namespace cacao


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
