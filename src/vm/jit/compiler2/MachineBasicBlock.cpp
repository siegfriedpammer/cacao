/* src/vm/jit/compiler2/MachineBasicBlock.cpp - MachineBasicBlock

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

#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MachineInstructions.hpp"
#include "vm/jit/compiler2/Backend.hpp"
#include "toolbox/OStream.hpp"

#include "toolbox/logging.hpp"
#define DEBUG_NAME "compiler2/MachineBasicBlock"

namespace cacao {
namespace jit {
namespace compiler2 {

MIIterator::_iterator MIIterator::_end;

std::size_t MachineBasicBlock::id_counter = 0;

OStream& MachineBasicBlock::print(OStream& OS) const {
	return OS << "MBB:" << setz(4) << id;
}

void MachineBasicBlock::update(MachineInstruction *MI) {
	MI->set_block(this);
}

void MoveEdgeFunctor::operator()(MachineInstruction* MI) {
	if (MI->is_jump()) {
		for (MachineInstruction::const_successor_iterator i = MI->successor_begin(),
				e = MI->successor_end(); i != e; ++i) {
			MachineBasicBlock *MBB = *i;
			for (MachineBasicBlock::pred_iterator i = MBB->pred_begin(),
					e = MBB->pred_end(); i != e; ++i) {
				if (**i == from) {
					LOG("Instruction " << *MI << " moved from " << from
						<< " to " << to << " predecessor of MBB: " << *MBB << nl);
					*i = &to;
				}
			}
		}
	}
}

bool check_is_phi(MachineInstruction *value) {
	return value->is_phi();
}

OStream& operator<<(OStream &OS, const MIIterator &it) {
	if (it.is_end())
		return OS << "MIIterator end";
	return OS << *it;
}

namespace {

struct TargetOperandEqualPred
		: public std::unary_function<MachineInstruction*,bool> {
	MachineOperand *op;
	/// constructor
	TargetOperandEqualPred(MachineOperand *op) : op(op) {}
	/// function call operand
	bool operator()(MachineInstruction* MI) {
		return MI->get_result().op == op;
	}
};

} // end anonymous namespace

MachinePhiInst* get_phi_from_operand(MachineBasicBlock *MBB,
		MachineOperand* op) {
	MachineBasicBlock::const_phi_iterator it
		= std::find_if(MBB->phi_begin(), MBB->phi_end(), TargetOperandEqualPred(op));
	if (it != MBB->phi_end())
		return *it;
	return NULL;
}

MachineBasicBlock* get_edge_block(MachineBasicBlock *from, MachineBasicBlock *to,
		Backend *backend) {
	assert(from->get_parent() == to->get_parent());
	MachineInstructionSchedule *MIS = from->get_parent();
	// create new block
	MachineBasicBlock *MBB = *MIS->insert_after(from->self_iterator(),MBBBuilder());
	MBB->push_front(new MachineLabelInst());
	MBB->push_back(backend->create_Jump(to));
	// fix links (from -> new)
	{
		MachineInstruction *jump = from->back();
		MachineInstruction::successor_iterator i =
			std::find(jump->successor_begin(),jump->successor_end(), to);
		assert(i != jump->successor_end());
		*i = MBB;
	}
	// fix links (new -> to)
	{
		MachineBasicBlock::pred_iterator i =
			std::find(to->pred_begin(), to->pred_end(),from);
		assert(i!= to->pred_end());
		*i = MBB;
	}
	return MBB;
}

MIIterator get_edge_iterator(MachineBasicBlock *from, MachineBasicBlock *to,
		Backend *backend) {
	MachineInstruction *jump = from->back();
	// sanity checks
	assert(std::find(jump->successor_begin(),jump->successor_end(), to)
		!= jump->successor_end());
	assert(std::find(to->pred_begin(), to->pred_end(),from) != to->pred_end());

	return get_edge_block(from,to,backend)->mi_last();
}

#if 0
std::insert_iterator<MachineBasicBlock> get_edge_inserter(
		MachineBasicBlock *from, MachineBasicBlock *to) {
	MIIterator last = get_edge_iterator(from,to);
	return std::inserter(*last.block_it(), last);
}
#endif


MIIterator insert_before(MIIterator pos, MachineInstruction* value) {
	MachineBasicBlock *MBB = (*pos)->get_block();
	return MBB->convert(MBB->insert_before(MachineBasicBlock::convert(pos), value));
}
MIIterator insert_after(MIIterator pos, MachineInstruction* value) {
	MachineBasicBlock *MBB = (*pos)->get_block();
	return MBB->convert(MBB->insert_after(MachineBasicBlock::convert(pos), value));
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
