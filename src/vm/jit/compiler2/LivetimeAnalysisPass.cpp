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
#include "vm/jit/compiler2/BasicBlockSchedulingPass.hpp"
#include "vm/jit/compiler2/LoweringPass.hpp"
#include "vm/jit/compiler2/LoopPass.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/livetimeanalysis"

namespace cacao {
namespace jit {
namespace compiler2 {

bool LivetimeAnalysisPass::run(JITData &JD) {
	BasicBlockSchedule *BS = get_Pass<BasicBlockSchedulingPass>();
	LoweringPass *LP = get_Pass<LoweringPass>();
	LoopTree *LT = get_Pass<LoopPass>();
	MachineInstructionSchedule *MIS = get_Pass<MachineInstructionSchedulingPass>();
	// TODO use better data structor for the register set
	LiveInMapTy liveIn;

	// for all basic blocks in reverse order
	for (BasicBlockSchedule::const_reverse_bb_iterator i = BS->bb_rbegin(),
			e = BS->bb_rend(); i != e ; ++i) {
		BeginInst *BI = *i;
		assert(BI);

		MachineInstructionSchedule::MachineInstructionRangeTy range = MIS->get_range(BI);
		unsigned from = range.first;
		unsigned to = range.second;

		EndInst *EI = BI->get_EndInst();
		assert(EI);
		// for all successors
		LOG3("line " << setw(4) << fillzero << from << " " << "Number of successors of " << BI << " " << EI->succ_size() << nl);
		for (EndInst::SuccessorListTy::const_iterator i = EI->succ_begin(),
				e = EI->succ_end(); i != e ; ++i) {
			BeginInst *succ = *i;
			LOG3("line " << setw(4) << fillzero << from << " " << "Successor of " << BI << ": " << succ << nl);
			std::set<Register*> &succ_live_in = liveIn[succ];
			// union of all successor liveIns
			liveIn[BI].insert(succ_live_in.begin(),succ_live_in.end());
			// add Phi operands
			int index = succ->get_predecessor_index(BI);
			assert(index != -1);
			LOG3("line " << setw(4) << fillzero << from << " " << "Predecessor index of " << BI << " in " << succ << " is " << index << nl);
			LOG3("line " << setw(4) << fillzero << from << " " << "Number of reverse dependency links of " << succ << " is " << succ->dep_size() << nl);
			for (Instruction::DepListTy::const_iterator i = succ->rdep_begin(),
					e = succ->rdep_end() ; i != e; ++i) {
				PHIInst *phi = (*i)->to_PHIInst();
				if (phi) {
					LOG3("line " << setw(4) << fillzero << from << " " << "PHI of " << succ << " is " << phi << nl);
					const LoweredInstDAG *dag = LP->get_LoweredInstDAG(phi);
					assert(dag);
					MachineOperand* op = dag->get_operand((unsigned)index);
					Register *reg;
					if ( op &&  (reg = op->to_Register()) ) {
						liveIn[BI].insert(reg);
						LOG2("line " << setw(4) << fillzero << from << " " << "adding " << reg << " to liveIn of " << BI << nl);
					}
				}
			}
		}

		// set initial interval
		LOG2("initial liveIn for " << BI << ": ");
		if (DEBUG_COND_N(2)) {
			print_container(dbg(), liveIn[BI].begin(), liveIn[BI].end()) << nl;
		}
		for (LiveInSetTy::const_iterator i = liveIn[BI].begin(), e = liveIn[BI].end();
				i != e ; ++i) {
			LOG2("adding live range for " << *i << " (" << from << "," << to << ")" << nl);
			lti_map[*i].add_range(from,to);
		}
		// for all machine instructions in the current basic block in reversed order
		MachineInstructionSchedule::MachineInstructionRangeTy bb_range = MIS->get_range(BI);
		for (signed inst_lineno = bb_range.second - 1, end_lineno = bb_range.first;
				inst_lineno >= end_lineno; --inst_lineno) {
			assert(inst_lineno >= 0);
			MachineInstruction *MI = MIS->get((unsigned)inst_lineno);
			if (MI->is_phi()) {
				// phis are handled differently
				continue;
			}
			// output operand
			{
				MachineOperand* op = MI->get_result();
				Register *reg;
				if ( op &&  (reg = op->to_Register()) ) {
					LOG2("line " << setw(4) << fillzero << inst_lineno << " " << "removing " << reg << " to liveIn of " << BI << nl);
					LOG2("line " << setw(4) << fillzero << inst_lineno << " " << "setting live range from for " << reg << " to " << inst_lineno << nl);
					lti_map[reg].set_from_if_available(inst_lineno,to);
					liveIn[BI].erase(reg);
				}
			}
			// input operands
			for(MachineInstruction::operand_iterator i = MI->begin(),
					e = MI->end(); i != e ; ++i) {
				MachineOperand* op = *i;
				Register *reg;
				if ( op &&  (reg = op->to_Register()) ) {
					LOG2("line " << setw(4) << fillzero << inst_lineno << " " << "adding " << reg << " to liveIn of " << BI << nl);
					LOG2("line " << setw(4) << fillzero << inst_lineno << " " << "adding live range for " << reg << " (" << from << "," << inst_lineno << ")" << nl);
					lti_map[reg].add_range(from,inst_lineno);
					liveIn[BI].insert(reg);
				}
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
				if ( op &&  (reg = op->to_Register()) ) {
					liveIn[BI].erase(reg);
					LOG2("line " << setw(4) << fillzero << from << " " << "removing " << reg << " to liveIn of " << BI << nl);
				}
			}
		}
		// if the basicblock is a loop header
		if (LT->is_loop_header(BI)) {
			LoopTree::ConstLoopIteratorPair it_pair = LT->get_Loops_from_header(BI);
			// for all loops with loop header BI
			for (LoopTree::const_loop_iterator i = it_pair.first, e = it_pair.second;
				i != e; ++i) {
				Loop *loop = *i;
				BeginInst *loop_exit = loop->get_exit();
				assert(loop_exit);
				LOG2("Loop " << loop_exit << " -> " << BI << "(" << loop << ")" << nl);
				// for each vreg live so something with the range
				for (LiveInSetTy::const_iterator i = liveIn[BI].begin(), e = liveIn[BI].end();
						i != e ; ++i) {
					unsigned loop_exit_to = MIS->get_range(loop_exit).second;
					LOG2("adding live range for " << *i << " (" << from << "," << loop_exit_to << ")" << nl);
					lti_map[*i].add_range(from,loop_exit_to);
				}
			}
		}

	}
	// set reg reference
	for (LivetimeIntervalMapTy::iterator i = lti_map.begin(),
			e = lti_map.end(); i != e ; ++i) {
		LivetimeInterval &lti = i->second;
		lti.set_reg(i->first);
	}
	// debugging
	if (DEBUG_COND) {
		for (LiveInMapTy::const_iterator i = liveIn.begin(), e = liveIn.end();
				i != e ; ++i) {
			LOG("LiveIn for " << i->first << ": ");
			print_container(dbg(), i->second.begin(), i->second.end()) << nl;
		}

		LOG("Livetime Interval(s)" << nl);
		for (LivetimeIntervalMapTy::const_iterator i = lti_map.begin(),
				e = lti_map.end(); i != e ; ++i) {
			LOG(i->second << nl);
		}

	}

	return true;
}

// pass usage
PassUsage& LivetimeAnalysisPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires(BasicBlockSchedulingPass::ID);
	PU.add_requires(LoweringPass::ID);
	PU.add_requires(LoopPass::ID);
	PU.add_requires(MachineInstructionSchedulingPass::ID);
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
