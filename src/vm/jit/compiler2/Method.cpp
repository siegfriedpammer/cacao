/* src/vm/jit/compiler2/Method.cpp - Method

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

#include "vm/jit/compiler2/Method.hpp"
#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/MethodDescriptor.hpp"

#include "vm/method.hpp"
#include "vm/class.hpp"
#include "vm/descriptor.hpp"
#include "vm/jit/jit.hpp"

#include <cassert>
#include <algorithm>

namespace cacao {
namespace jit {
namespace compiler2 {

Method::Method(methodinfo *m) : class_name_utf8(m->clazz->name),
		method_name_utf8(m->name), method_desc_utf8(m->descriptor) {
	assert(m);
	methoddesc *md = m->parseddesc;
	assert(md);
	assert(md->paramtypes);
	method_desc = new MethodDescriptor(md->paramcount);
	MethodDescriptor &MD = *method_desc;


	for (int i = 0, slot = 0; i < md->paramcount; ++i) {
		int type = md->paramtypes[i].type;
		MD[i] = convert_var_type(type);
		/*
		int varindex = jd->local_map[slot * 5 + type];
		assert(varindex != UNUSED);
		*/

		switch (type) {
			case TYPE_LNG:
			case TYPE_DBL:
				slot +=2;
				break;
			default:
				++slot;
		}
	}
}

Method::~Method() {
	delete method_desc;
	for(InstructionListTy::iterator i = inst_list.begin(),
			e = inst_list.end(); i != e ; ++i) {
		Instruction *I = *i;
		//delete I;
	}
}

void Method::add_Instruction(Instruction* I) {
	assert(I);
	I->set_Method(this);
	assert(std::find(inst_list.begin(), inst_list.end(),I) == inst_list.end());
	inst_list.push_back(I);
}

void Method::remove_Instruction(Instruction* I) {
	inst_list.remove(I);
	#if 0
	std::replace(inst_list.begin(),	inst_list.end(),I,(Instruction*)0);
	#endif
	delete I;
}

void Method::add_bb(BeginInst *bi) {
	add_Instruction(bi);
	bb_list.push_back(bi);
}

void Method::remove_bb(BeginInst *bi) {
	bb_list.remove(bi);
	remove_Instruction(bi);
}

void Method::clear_schedule() const {
	// clear schedule
	for (InstructionListTy::const_iterator i = begin(),
			e = end() ; i != e ; ++i) {
		if ((*i)->is_floating()) {
			(*i)->set_BeginInst(NULL);
		}
	}
}

BeginInst* Method::get_edge_block(BeginInst* pred, BeginInst* succ) {
	assert(pred);
	assert(succ);
	int pred_idx = succ->get_predecessor_index(pred);
	int succ_idx = pred->get_successor_index(succ);
	assert(pred_idx != -1);
	assert(succ_idx != -1);
	// setpu new block
	BeginInst* BI = new BeginInst();
	EndInst* EI = new GOTOInst(BI);
	EI->succ_list.push_back(BeginInstRef(succ));
	BI->pred_list.push_back(pred);
	// insert new block
	succ->set_predecessor(pred_idx,BI);
	pred->set_successor(succ_idx,BI);
	// add to method
	add_bb(BI);
	add_Instruction(EI);

	return BI;
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

