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
#include "vm/jit/compiler2/LoweringPass.hpp"
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
#if 0
class UpdateCurrentVisitor : public MachineStubVisitor {
private:
	MachineBasicBlock *MBB;
public:
	UpdateCurrentVisitor(MachineBasicBlock *MBB) : MBB(MBB) {}

	using MachineStubVisitor::visit;

	virtual void visit(MachineJumpStub *MS) {
		MS->set_current(MBB);
	}
};
#endif

struct UpdatePhiOperand : public std::unary_function<MachinePhiInst*,void> {
	typedef std::map<Instruction*,MachineOperand*> InstMapTy;
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
	std::map<BeginInst*,MachineBasicBlock*> map;

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
	std::map<Instruction*,MachineOperand*> inst_map;

	// lower instructions
	// XXX ensure dominator ordering!
	for (std::map<BeginInst*,MachineBasicBlock*>::const_iterator i = map.begin(),
			e = map.end(); i != e; ++i) {
		BeginInst *BI = i->first;
		MachineBasicBlock *MBB = i->second;

		LoweringVisitor LV(BE,MBB,map,inst_map);

		for (InstructionSchedule<Instruction>::const_inst_iterator i = IS->inst_begin(BI),
				e = IS->inst_end(BI); i != e; ++i) {
			Instruction *I = *i;
			I->accept(LV);
		}

	}

	// fix phi instructions
	for (const_iterator i = begin(), e = end(); i != e; ++i) {
		MachineBasicBlock *MBB = *i;
		UpdatePhiOperand functor(inst_map);
		std::for_each(MBB->phi_begin(), MBB->phi_end(), functor);
	}
	return true;
}

// verify
bool MachineInstructionSchedulingPass::verify() const {
	#if 0
	for (MachineInstructionSchedule::const_iterator i = begin(), e = end();
			i != e; ++i) {
		MachineBasicBlock *MBB = *i;
		if(MBB->size() == 0) {
			ERROR_MSG("verification failed", "MachineBasicBlock ("
				<< *MBB << ") empty");
			return false;
		}
		#if 0
		MachineInstruction *front = MBB->front();
		// check for label
		if(!front->is_label()) {
			ERROR_MSG("verification failed", "first Instruction ("
				<< *front << ") not a label");
			return false;
		}
		#endif
		MachineInstruction *back = MBB->back();
		// check for end
		if(!back->is_end()) {
			ERROR_MSG("verification failed", "last Instruction ("
				<< *back << ") not a control flow transfer instruction");
			return false;
		}
		// check for stub
		for (MachineBasicBlock::const_iterator i = MBB->begin(), e = MBB->end();
				i != e ; ++i) {
			MachineInstruction *MI = *i;
			if(MI->is_stub()) {
				ERROR_MSG("stub Instruction",*MI);
				return false;
			}
			if(!MI->get_block()) {
				ERROR_MSG("No block", *MI);
				return false;
			}
		}
	}
	#endif
	return true;
}

// pass usage
PassUsage& MachineInstructionSchedulingPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires(BasicBlockSchedulingPass::ID);
	PU.add_requires(ListSchedulingPass::ID);
	PU.add_requires(LoweringPass::ID);
	return PU;
}

// the address of this variable is used to identify the pass
char MachineInstructionSchedulingPass::ID = 0;

// register pass
static PassRegistery<MachineInstructionSchedulingPass> X("MachineInstructionSchedulingPass");

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
