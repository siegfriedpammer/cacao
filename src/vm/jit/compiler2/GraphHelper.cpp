/* src/vm/jit/compiler2/GraphHelper.cpp - GraphHelper

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


#include "vm/jit/compiler2/GraphHelper.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MachineInstruction.hpp"

#include <cassert>

#define DEBUG_NAME "compiler2/GraphHelper"

namespace cacao {
namespace jit {
namespace compiler2 {

// specialization for BeginInst
template <>
DFSTraversal<BeginInst>::NodeListTy& DFSTraversal<BeginInst>::successor(BeginInst *v, NodeListTy& list) {
	EndInst *ve = v->get_EndInst();
	assert(ve);
	if (ve->to_IFInst()) {
		// the first successor is usually the jump target, the second the fall-through block
		assert(ve->succ_size() == 2);
		for (EndInst::SuccessorListTy::const_reverse_iterator i = ve->succ_rbegin(),
				e = ve->succ_rend(); i != e; ++i) {
			list.insert(i->get());
		}
	} else {
		for (EndInst::SuccessorListTy::const_iterator i = ve->succ_begin(),
				e = ve->succ_end(); i != e; ++i) {
			list.insert(i->get());
		}
	}
	return list;
}

template <>
int DFSTraversal<BeginInst>::num_nodes(BeginInst *v) const {
	assert(v->get_Method());
	return v->get_Method()->bb_size();
}

// specialization for MachineBasicBlock
template <>
DFSTraversal<MachineBasicBlock>::NodeListTy& DFSTraversal<MachineBasicBlock>::successor(MachineBasicBlock *v, NodeListTy& list) {
	MachineInstruction *last = v->back();
	std::copy(last->successor_begin(),last->successor_end(),std::inserter(list,list.begin()));
	return list;
}

template <>
int DFSTraversal<MachineBasicBlock>::num_nodes(MachineBasicBlock *v) const {
	return v->get_parent()->size();
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
