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

/**
 * @file
 *
 * The algorithm to callculate the livetime information is based on
 * Wimmer and Franz "Linear Scan Register Allocation on SSA Form", 2010.
 * @cite Wimmer2010
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

#define DEBUG_NAME "compiler2/LivetimeAnalysis"

namespace cacao {
namespace jit {
namespace compiler2 {
////////////////////// LivetimeInterval

LivetimeInterval* LivetimeInterval::split(unsigned pos) {
	//sanity checks:
	if (pos <= get_start() || pos >= get_end()) {
		// no need to split!?
		return NULL;
	}
	LivetimeInterval *lti = new LivetimeInterval();
	// copy intervals
	iterator i = intervals.begin();
	iterator e = intervals.end();
	for( ; i != e ; ++i) {
		if (i->first >= pos) {
			// livetime hole
			break;
		}
		if (i->second > pos) {
			// currently active
			unsigned end = i->second;
			i->second = pos;
			//lti->add_range(pos,end);
			// XXX Wow I am so not sure if this is correct:
			// If the new range will start at pos it will be
			// selected for reg allocation in the next iteration
			// and we end up in splitting another interval which
			// in ture creates a new interval starting at pos...
			// But if we let it start at the next usedef things look
			// better. Because of the phi functions we dont run into
			// troubles with backedges and loop time intervals,
			// I guess...
			signed next_usedef = next_usedef_after(pos);
			assert(next_usedef != -1);
			lti->add_range(next_usedef,end);
			++i;
			break;
		}
	}
	// move all remaining intervals to the new lti
	while (i != e) {
		lti->intervals.push_back(std::make_pair(i->first,i->second));
		i = intervals.erase(i);
	}
	// copy uses
	for (use_iterator i = uses.begin(), e = uses.end(); i != e ; ) {
		if (*i >= pos) {
			lti->add_use(*i);
			i = uses.erase(i);
		} else {
			++i;
		}
	}
	// copy defs
	for (def_iterator i = defs.begin(), e = defs.end(); i != e ; ) {
		if (*i >= pos) {
			lti->add_def(*i);
			i = defs.erase(i);
		} else {
			++i;
		}
	}
	// TODO set reg!
	VirtualRegister *vreg = new VirtualRegister();
	lti->set_reg(vreg);
	// TODO set user at the end of this lti (store)
	// TODO set def for new lti (load)
	return lti;
}

inline OStream& operator<<(OStream &OS, const LivetimeInterval &lti) {
	OS << lti.get_reg();
	for(LivetimeInterval::const_iterator i = lti.begin(), e = lti.end();
			i != e ; ++i) {
		OS << " [" << i->first << "," << i->second << ")";
	}
	return OS;
}

////////////////////// LivetimeAnalysis

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
			// output operand
			{
				MachineOperand* op = MI->get_result();
				Register *reg;
				if ( op &&  (reg = op->to_Register()) ) {
					// phis are handled differently
					if (!MI->is_phi()) {
						LOG2("line " << setw(4) << fillzero << inst_lineno << " " << "removing " << reg << " to liveIn of " << BI << nl);
						LOG2("line " << setw(4) << fillzero << inst_lineno << " " << "setting live range from for " << reg << " to " << inst_lineno << nl);
						lti_map[reg].set_from_if_available(inst_lineno,to);
						liveIn[BI].erase(reg);
					}
					lti_map[reg].add_def(inst_lineno);
					if (reg->to_MachineRegister()) {
						// this is already in a MachineRegister -> fixed interval
						lti_map[reg].set_fixed();
					}
				}
			}
			// input operands
			for(MachineInstruction::operand_iterator i = MI->begin(),
					e = MI->end(); i != e ; ++i) {
				MachineOperand* op = *i;
				Register *reg;
				if ( op &&  (reg = op->to_Register()) ) {
					// phis are handled differently
					if (!MI->is_phi()) {
						LOG2("line " << setw(4) << fillzero << inst_lineno << " " << "adding " << reg << " to liveIn of " << BI << nl);
						LOG2("line " << setw(4) << fillzero << inst_lineno << " " << "adding live range for " << reg << " (" << from << "," << inst_lineno << ")" << nl);
						lti_map[reg].add_range(from,inst_lineno);
						liveIn[BI].insert(reg);
					}
					lti_map[reg].add_use(inst_lineno);
					if (reg->to_MachineRegister()) {
						// this is already in a MachineRegister -> fixed interval
						lti_map[reg].set_fixed();
					}
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
			LOG(i->second);
			dbg() << "  use: ";
			print_container(dbg(),i->second.use_begin(),i->second.use_end());
			dbg() << "  def: ";
			print_container(dbg(),i->second.def_begin(),i->second.def_end());
			dbg() << nl;
		}

	}

	return true;
}

bool LivetimeAnalysisPass::verify() const {
	for (LivetimeIntervalMapTy::const_iterator i = lti_map.begin(),
			e = lti_map.end(); i != e ; ++i) {
		const LivetimeInterval &lti = i->second;
		signed old = -1;
		for (LivetimeInterval::const_use_iterator i = lti.use_begin(),
				e = lti.use_end(); i != e; ++i) {
			if ( old >= (signed)*i) {
				err() << old << " >= " << (signed)*i << nl;
				return false;
			}
			old = *i;
		}
		old = -1;
		for (LivetimeInterval::const_def_iterator i = lti.def_begin(),
				e = lti.def_end(); i != e; ++i) {
			if ( old >= (signed)*i) {
				err() << old << " >= " << (signed)*i << nl;
				return false;
			}
			old = *i;
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
