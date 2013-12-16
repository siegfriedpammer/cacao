/* src/vm/jit/compiler2/MachineInstructionSchedulingPass.cpp - MachineInstructionSchedulingPass

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

#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"

#include "vm/jit/compiler2/ListSchedulingPass.hpp"
#include "vm/jit/compiler2/BasicBlockSchedulingPass.hpp"
#include "vm/jit/compiler2/MachineBasicBlock.hpp"

#include "Target.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/MachineInstructionSchedulingPass"

namespace cacao {
namespace jit {
namespace compiler2 {

void MachineInstructionSchedulingPass::initialize() {
#if 0
	list.clear();
	map.clear();
#endif
}


namespace {

struct UpdatePhiOperand : public std::unary_function<MachinePhiInst*,void> {
	typedef alloc::map<Instruction*,MachineOperand*>::type InstMapTy;
	InstMapTy inst_map;
	/// constructor
	UpdatePhiOperand(InstMapTy &inst_map) : inst_map(inst_map) {}
	/// function operator
	virtual void operator()(MachinePhiInst* phi) const {
		PHIInst* phi_inst = phi->get_PHIInst();
		assert(phi_inst);
		Instruction::const_op_iterator op = phi_inst->op_begin();

		for (MachineInstruction::operand_iterator i = phi->begin(),
				e = phi->end(); i != e; ++i) {
			Instruction *I = (*op)->to_Instruction();
			assert(I);
			InstMapTy::const_iterator it = inst_map.find(I);
			assert(it != inst_map.end());
			// set operand
			i->op = it->second;
			++op;
		}
	}
};

} // end anonymous namespace

bool MachineInstructionSchedulingPass::run(JITData &JD) {
	BasicBlockSchedule *BS = get_Pass<BasicBlockSchedulingPass>();
	InstructionSchedule<Instruction> *IS = get_Pass<ListSchedulingPass>();
	alloc::map<BeginInst*,MachineBasicBlock*>::type map;

	// create machine basic blocks
	for (BasicBlockSchedule::const_bb_iterator i = BS->bb_begin(),
			e = BS->bb_end(); i != e ; ++i) {
		BeginInst *BI = *i;
		assert(BI);
		// create MachineBasicBlock
		MachineBasicBlock *MBB = *push_back(MBBBuilder());
		map[BI] = MBB;
	}

	Backend *BE = JD.get_Backend();
	alloc::map<Instruction*,MachineOperand*>::type inst_map;

	// lower instructions
	for (BasicBlockSchedule::const_bb_iterator i = BS->bb_begin(),
			e = BS->bb_end(); i != e ; ++i) {
		BeginInst *BI = *i;
		MachineBasicBlock *MBB = map[BI];

		LoweringVisitor LV(BE,MBB,map,inst_map);

		for (InstructionSchedule<Instruction>::const_inst_iterator i = IS->inst_begin(BI),
				e = IS->inst_end(BI); i != e; ++i) {
			Instruction *I = *i;
			LOG2("lower: " << *I << nl);
			I->accept(LV);
		}

		// fix predecessors
		for (BeginInst::const_pred_iterator i = BI->pred_begin(),
				e = BI->pred_end(); i != e; ++i) {
			BeginInst *pred = (*i)->get_BeginInst();
			alloc::map<BeginInst*,MachineBasicBlock*>::type::const_iterator it = map.find(pred);
			assert(it != map.end());
			MBB->insert_pred(it->second);
		}

	}

	for (const_iterator i = begin(), e = end(); i != e; ++i) {
		MachineBasicBlock *MBB = *i;
		LOG2("MBB: " << *MBB << nl);
		// fix phi instructions
		UpdatePhiOperand functor(inst_map);
		std::for_each(MBB->phi_begin(), MBB->phi_end(), functor);
		// split basic blocks
		for (MachineBasicBlock::iterator i = MBB->begin(), e = MBB->end();
				i != e; ) {
			MachineInstruction *MI = *i;
			++i;
			if (MI->is_jump() && i != e) {
				MachineBasicBlock *new_MBB = *insert_after(MBB->self_iterator(),MBBBuilder());
				assert(new_MBB);
				new_MBB->insert_pred(MBB);
				new_MBB->push_front(new MachineLabelInst());
				LOG2("new MBB: " << *new_MBB << nl);
				move_instructions(i,e,*MBB,*new_MBB);
				break;
			}
		}
	}
	return true;
}

// verify
bool MachineInstructionSchedulingPass::verify() const {
	for (MachineInstructionSchedule::const_iterator i = begin(), e = end();
			i != e; ++i) {
		MachineBasicBlock *MBB = *i;
		if(MBB->size() == 0) {
			ERROR_MSG("verification failed", "MachineBasicBlock ("
				<< *MBB << ") empty");
			return false;
		}
		MachineInstruction *front = MBB->front();
		// check for label
		if(!front->is_label()) {
			ERROR_MSG("verification failed", "first Instruction ("
				<< *front << ") not a label");
			return false;
		}
		MachineInstruction *back = MBB->back();
		// check for end
		if(!back->is_end()) {
			ERROR_MSG("verification failed", "last Instruction ("
				<< *back << ") not a control flow transfer instruction");
			return false;
		}

		std::size_t num_pred = MBB->pred_size();
		// check phis
		for (MachineBasicBlock::const_phi_iterator i = MBB->phi_begin(), e = MBB->phi_end();
				i != e; ++i) {
			MachinePhiInst *phi = *i;
			if (phi->op_size() != num_pred) {
				ERROR_MSG("verification failed", "Machine basic block (" << *MBB << ") has "
					<< num_pred << " predecessors but phi node (" << *phi << ") has " << phi->op_size());
				return false;
			}
		}

		for (MachineBasicBlock::const_iterator i = MBB->begin(), e = MBB->end();
				i != e ; ++i) {
			MachineInstruction *MI = *i;
			// check for block
			if(!MI->get_block()) {
				ERROR_MSG("No block", *MI);
				return false;
			}
		}
	}
	return true;
}

// pass usage
PassUsage& MachineInstructionSchedulingPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires<BasicBlockSchedulingPass>();
	PU.add_requires<ListSchedulingPass>();
	return PU;
}

// the address of this variable is used to identify the pass
char MachineInstructionSchedulingPass::ID = 0;

// register pass
static PassRegistry<MachineInstructionSchedulingPass> X("MachineInstructionSchedulingPass");

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
