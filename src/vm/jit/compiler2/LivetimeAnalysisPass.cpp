/* src/vm/jit/compiler2/LivetimeAnalysisPass.cpp - LivetimeAnalysisPass

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

/*
 * The algorithm to callculate the livetime information is based on
 * Wimmer and Franz "Linear Scan Register Allocation on SSA Form",
 * 2010.
 *
 */

#include "vm/jit/compiler2/LivetimeAnalysisPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/ListSchedulingPass.hpp"
#include "vm/jit/compiler2/BasicBlockSchedulingPass.hpp"
#include "vm/jit/compiler2/LoweringPass.hpp"
#include "vm/jit/compiler2/LoopPass.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/livetimeanalysis"

namespace cacao {
namespace jit {
namespace compiler2 {

bool LivetimeAnalysisPass::run(JITData &JD) {
	BasicBlockSchedule *BS = get_Pass<BasicBlockSchedulingPass>();
	InstructionSchedule<Instruction> *IS = get_Pass<ListSchedulingPass>();
	LoweringPass *LP = get_Pass<LoweringPass>();
	LoopTree *LT = get_Pass<LoopPass>();
	// TODO use better data structor for the register set
	LiveInMapTy liveIn;

	// for all basic blocks in reverse order
	for (BasicBlockSchedule::const_reverse_bb_iterator i = BS->bb_rbegin(),
			e = BS->bb_rend(); i != e ; ++i) {
		BeginInst *BI = *i;
		assert(BI);
		EndInst *EI = BI->get_EndInst();
		assert(EI);
		// for all successors
		for (EndInst::SuccessorListTy::const_iterator i = EI->succ_begin(),
				e = EI->succ_end(); i != e ; ++i) {
			BeginInst *succ = *i;
			LOG3("Successor of " << BI << ": " << succ << nl);
			std::set<VirtualRegister*> &succ_live_in = liveIn[succ];
			// union of all successor liveIns
			liveIn[BI].insert(succ_live_in.begin(),succ_live_in.end());
			// add Phi operands
			int index = succ->get_predecessor_index(BI);
			assert(index != -1);
			LOG3("Predecessor index of " << BI << " in " << succ << " is " << index << nl);
			LOG3("Number of reverse dependency links of " << succ << " is " << succ->dep_size() << nl);
			for (Instruction::DepListTy::const_iterator i = succ->rdep_begin(),
					e = succ->rdep_end() ; i != e; ++i) {
				PHIInst *phi = (*i)->to_PHIInst();
				if (phi) {
					LOG3("PHI of " << succ << " is " << phi << nl);
					const LoweredInstDAG *dag = LP->get_LoweredInstDAG(phi);
					assert(dag);
					MachineOperand* op = dag->get_operand((unsigned)index);
					Register *reg;
					VirtualRegister *vreg;
					if ( op &&  (reg = op->to_Register()) && (vreg = reg->to_VirtualRegister()) ) {
						liveIn[BI].insert(vreg);
						LOG2("adding " << vreg << " to liveIn of " << BI << nl);
					}
				}
			}
		}
		// for all instructions in the current basic block in reversed order
		for (InstructionSchedule<Instruction>::const_reverse_inst_iterator i = IS->inst_rbegin(BI),
				e = IS->inst_rend(BI); i != e; ++i) {
			Instruction *I = *i;
			const LoweredInstDAG *dag = LP->get_LoweredInstDAG(I);
			assert(dag);
			for (unsigned i = 0, e = I->op_size(); i < e ; ++i) {
				MachineOperand* op = dag->get_operand(i);
				Register *reg;
				VirtualRegister *vreg;
				if ( op &&  (reg = op->to_Register()) && (vreg = reg->to_VirtualRegister()) ) {
					liveIn[BI].insert(vreg);
					LOG2("adding " << vreg << " to liveIn of " << BI << nl);
				}
			}
			MachineOperand* op = dag->get_result()->get_result();
			Register *reg;
			VirtualRegister *vreg;
			if ( op &&  (reg = op->to_Register()) && (vreg = reg->to_VirtualRegister()) ) {
				liveIn[BI].erase(vreg);
				LOG2("removing " << vreg << " to liveIn of " << BI << nl);
			}
		}
		// for each phi function
		for (Instruction::DepListTy::const_iterator i = BI->rdep_begin(),
				e = BI->rdep_end() ; i != e; ++i) {
			PHIInst *phi = (*i)->to_PHIInst();
			if (phi) {
				const LoweredInstDAG *dag = LP->get_LoweredInstDAG(phi);
				assert(dag);
				MachineOperand* op = dag->get_result()->get_result();
				Register *reg;
				VirtualRegister *vreg;
				if ( op &&  (reg = op->to_Register()) && (vreg = reg->to_VirtualRegister()) ) {
					liveIn[BI].erase(vreg);
					LOG2("removing " << vreg << " to liveIn of " << BI << nl);
				}
			}
		}
		// if the basicblock is a loop header
		Loop *loop = LT->get_Loop(BI);
		if (loop) {
			BeginInst *loop_exit = loop->get_exit();
			// for each vreg live so something with the range
		}

	}
	if (DEBUG_COND) {
		for (LiveInMapTy::const_iterator i = liveIn.begin(), e = liveIn.end();
				i != e ; ++i) {
			LOG("LiveIn for " << i->first << ": ");
			print_container(dbg(), i->second.begin(), i->second.end()) << nl;
		}
	}

	return true;
}

// pass usage
PassUsage& LivetimeAnalysisPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires(BasicBlockSchedulingPass::ID);
	PU.add_requires(ListSchedulingPass::ID);
	PU.add_requires(LoweringPass::ID);
	PU.add_requires(LoopPass::ID);
	return PU;
}

// the address of this variable is used to identify the pass
char LivetimeAnalysisPass::ID = 0;

// register pass
static PassRegistery<LivetimeAnalysisPass> X("LivetimeAnalysisPass");

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
