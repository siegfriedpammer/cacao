/* src/vm/jit/compiler2/SpillAllAllocatorPass.cpp - SpillAllAllocatorPass

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

#include "vm/jit/compiler2/SpillAllAllocatorPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/LivetimeAnalysisPass.hpp"
#include "vm/jit/compiler2/MachineRegister.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/LoweringPass.hpp"
#include "vm/jit/compiler2/MachineInstructions.hpp"
#include "vm/jit/compiler2/ListSchedulingPass.hpp"
#include "vm/jit/compiler2/BasicBlockSchedulingPass.hpp"
#include "vm/jit/compiler2/ScheduleClickPass.hpp"
#include "vm/statistics.hpp"

#include <climits>

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/SpillAllAllocator"

STAT_REGISTER_GROUP(lsra_stat1,"LSRA","Linear Scan Register Allocator")
STAT_REGISTER_GROUP_VAR(unsigned,num_hints_followed1,0,"hints followed",
	"Number of Hints followed (i.e. no move)",lsra_stat1)

namespace cacao {
namespace jit {
namespace compiler2 {

namespace {

#if 0
MachineResource get_MachineResource_from_MachineRegister(const MachineRegister* reg) {
	return reg->get_MachineResource();
}
#endif
MachineRegister* get_MachineRegister_from_MachineResource(const MachineResource& res, Type::TypeID type) {
	return res.create_MachineRegister(type);
}

} // end anonymous namespace


MachineRegister* get_def_Register(Backend *backend, Type::TypeID type) {
	RegisterFile* reg_file = backend->get_RegisterFile(type);
	return get_MachineRegister_from_MachineResource(*reg_file->begin(),type);
}

MachineRegister* get_use_Register(Backend *backend, unsigned pos, Type::TypeID type) {
	RegisterFile* reg_file = backend->get_RegisterFile(type);
	assert( reg_file->size() >= pos + 1 );
	RegisterFile::const_iterator i = reg_file->begin();
	do {
		++i;
	} while (pos--);
	return get_MachineRegister_from_MachineResource(*i,type);
}

bool SpillAllAllocatorPass::run(JITData &JD) {
	LA = get_Pass<LivetimeAnalysisPass>();
	MIS = get_Pass<MachineInstructionSchedulingPass>();
	jd = &JD;
	backend = jd->get_Backend();

	for (LivetimeAnalysisPass::iterator i = LA->begin(), e = LA->end();
			i != e ; ++i) {
		LivetimeInterval &inter = i->second;
		unhandled.push(&inter);
	}
	while(!unhandled.empty()) {
		LivetimeInterval *current = unhandled.top();
		unhandled.pop();
		handled.push_back(current);
		unsigned pos = current->get_start();
		LOG(BoldGreen << "pos:" << setw(4) << pos << " current: " << current << reset_color << nl);

		if (!current->is_in_Register()) {
			// stackslot
			LOG2("Stackslot: " << current << nl);
			continue;
		}
		if (current->get_Register()->to_MachineRegister()) {
			// preallocated
			LOG2("Preallocated: " << current << nl);
			continue;
		}
		// get slot
		ManagedStackSlot *slot = jd->get_StackSlotManager()->create_ManagedStackSlot(current->get_type());
		assert(slot);
		current->set_ManagedStackSlot(slot);
	}

	// set src/dst operands
	for (HandledSetTy::const_iterator i = handled.begin(), e = handled.end();
			i != e ; ++i) {
		LivetimeInterval *lti = *i;
		if (lti->is_in_Register()) {
			Register *reg = lti->get_Register();
			assert(reg->to_MachineRegister());
			for (LivetimeInterval::const_def_iterator i = lti->def_begin(),
					e = lti->def_end(); i != e ; ++i) {
				MachineOperandDesc *MOD = i->second;
				MOD->op = lti->get_Register();
			}
			for (LivetimeInterval::const_use_iterator i = lti->use_begin(),
					e = lti->use_end(); i != e ; ++i) {
				MachineOperandDesc *MOD = i->second;
				MOD->op = lti->get_Register();
			}
		} 
		else if (lti->is_in_StackSlot()) {
			ManagedStackSlot *slot = lti->get_ManagedStackSlot();

			for (LivetimeInterval::const_def_iterator i = lti->def_begin(),
					e = lti->def_end(); i != e ; ++i) {
				MachineOperandDesc *MOD = i->second;
				unsigned pos = i->first;
				MOD->op = get_def_Register(backend,lti->get_type());
				MachineInstruction *move_to_stack = backend->create_Move(MOD->op,slot);
				LOG2("move to stack: " << move_to_stack << nl);
				move_to_stack->set_comment("move to stack");
				MIS->add_after(unsigned(pos/2),move_to_stack);
			}
			for (LivetimeInterval::const_use_iterator i = lti->use_begin(),
					e = lti->use_end(); i != e ; ++i) {
				MachineOperandDesc *MOD = i->second;
				unsigned pos = i->first;
				MOD->op = get_use_Register(backend,MOD->get_index(),lti->get_type());
				MachineInstruction *move_from_stack = backend->create_Move(slot,MOD->op);
				move_from_stack->set_comment("move from stack");
				LOG2("move from stack: " << move_from_stack << nl);
				MIS->add_before(unsigned(pos/2),move_from_stack);
			}
		}
		else {assert(0);}
	}

	DEBUG(LA->print(dbg()));
	// write back the spill/store instructions
	MIS->insert_added_instruction();
	return true;
}


bool SpillAllAllocatorPass::verify() const {
	for (MachineInstructionSchedule::const_iterator i = MIS->begin(),
			e = MIS->end(); i != e; ++i) {
		MachineInstruction *MI = *i;
		MachineOperand *op = MI->get_result().op;
		if (!op->is_StackSlot() &&
				op->is_Register() && !op->to_Register()->to_MachineRegister() ) {
			LOG("Not allocatd: " << MI << nl);
			return false;
		}

	}
	return true;
}

// pass usage
PassUsage& SpillAllAllocatorPass::get_PassUsage(PassUsage &PU) const {
	// requires
	PU.add_requires(LivetimeAnalysisPass::ID);
	PU.add_requires(MachineInstructionSchedulingPass::ID);
	PU.add_requires(LoweringPass::ID);
	PU.add_requires(ListSchedulingPass::ID);
	PU.add_requires(ScheduleClickPass::ID);
	// modified
	PU.add_modifies(LoweringPass::ID);
	PU.add_modifies(ListSchedulingPass::ID);
	PU.add_modifies(ScheduleClickPass::ID);
	// destroys
	PU.add_destroys(BasicBlockSchedulingPass::ID);
	PU.add_destroys(MachineInstructionSchedulingPass::ID);
	// not yet updated correctly (might be only changed)
	PU.add_destroys(LivetimeAnalysisPass::ID);
	return PU;
}

// the address of this variable is used to identify the pass
char SpillAllAllocatorPass::ID = 0;

// register pass
static PassRegistery<SpillAllAllocatorPass> X("SpillAllAllocatorPass");

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
