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

#include "vm/jit/compiler2/LivetimeAnalysisPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/BasicBlockSchedulingPass.hpp"
#include "vm/jit/compiler2/LoopPass.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/MachineOperand.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/LivetimeAnalysis"

namespace cacao {
namespace jit {
namespace compiler2 {

namespace {

MachineInstruction::successor_iterator
get_successor_begin(MachineBasicBlock* MBB) {
	return MBB->back()->successor_begin();
}

MachineInstruction::successor_iterator
get_successor_end(MachineBasicBlock* MBB) {
	return MBB->back()->successor_end();
}

} // end anonymous namespace

/**
 * @Cpp11 use std::function
 */
class LivetimeAnalysisPass::InsertPhiOperands : public std::unary_function<MachineBasicBlock*,void> {
private:
	/// Phi inserter
	struct PhiOperandInserter : public std::unary_function<MachinePhiInst*,void> {
		LiveInSetTy &live;
		std::size_t index;
		/// constructor
		PhiOperandInserter(LiveInSetTy &live, std::size_t index)
			: live(live), index(index) {}

		void operator()(MachinePhiInst* phi) {
			live.insert(phi->get(index).op);
		}
	};

	LiveInSetTy &live;
	MachineBasicBlock *current;
public:
	/// constructor
	InsertPhiOperands(LiveInSetTy &live, MachineBasicBlock *current)
		: live(live), current(current) {}

	void operator()(MachineBasicBlock* MBB) {
		std::for_each(MBB->phi_begin(), MBB->phi_end(),
			PhiOperandInserter(live,MBB->get_predecessor_index(current)));
	}
};

/**
 * @Cpp11 use std::function
 */
struct LivetimeAnalysisPass::UnionLiveIn : public std::unary_function<MachineBasicBlock*,void> {
	LiveInSetTy &live_set;
	LiveInMapTy &live_map;

	/// Constructor
	UnionLiveIn(LiveInSetTy &live_set, LiveInMapTy &live_map)
		: live_set(live_set), live_map(live_map) {}

	void operator()(MachineBasicBlock* MBB) {
		LiveInSetTy &other_set = live_map[MBB];
		live_set.insert(other_set.begin(), other_set.end());
	}
};


bool LivetimeAnalysisPass::run(JITData &JD) {
	MIS = get_Pass<MachineInstructionSchedulingPass>();
	//LoweringPass *LP = get_Pass<LoweringPass>();
	//LoopTree *LT = get_Pass<LoopPass>();
	// TODO use better data structor for the register set
	LiveInMapTy liveIn;

	// for all basic blocks in reverse order
	for (MachineInstructionSchedule::const_reverse_iterator i = MIS->rbegin(),
			e = MIS->rend(); i != e ; ++i) {
		MachineBasicBlock* BB = *i;
		LOG2("BasicBlock: " << *BB << nl);
		// get live set
		LiveInSetTy &live = liveIn[BB];

		// live == union of successor.liveIn for each successor of BB
		std::for_each(get_successor_begin(BB), get_successor_end(BB), UnionLiveIn(live,liveIn));

		// for each phi function of successors of BB
		#if 0
		for(MachineInstruction::successor_iterator i = get_successor_begin(BB),
				e = get_successor_end(BB); i != e; ++i) {
			MachineBasicBlock *succ_MBB = *i;
			std::size_t index = succ_MBB->get_predecessor_index(BB);
			assert_msg(index != succ_MBB->pred_size(),
				"BasicBlock: " << * BB << " not found in predecessor list of " << succ_MBB);
			for (MachineBasicBlock::const_phi_iterator i = succ_MBB->phi_begin(),
					e = succ_MBB->phi_end(); i != e ; ++i ) {
				MachinePhiInst *phi = *i;
				live.insert(phi->get(index).op);
			}
		}
		#else
		std::for_each(get_successor_begin(BB), get_successor_end(BB), InsertPhiOperands(live,BB));
		#endif
	}
#if 0
		BeginInst *BI = *i;
		assert(BI);

		MachineInstructionSchedule::MachineInstructionRangeTy range = MIS->get_range(BI);
		// times 2 to distinguish between uses and defs
		unsigned from = range.first * 2;
		unsigned to = range.second * 2;

		EndInst *EI = BI->get_EndInst();
		assert(EI);
		// for all successors
		LOG3("line " << setz(4) << from << " " << "Number of successors of " << BI << " " << EI->succ_size() << nl);
		for (EndInst::SuccessorListTy::const_iterator i = EI->succ_begin(),
				e = EI->succ_end(); i != e ; ++i) {
			BeginInst *succ = i->get();
			LOG3("line " << setz(4) << from << " " << "Successor of " << BI << ": " << succ << nl);
			std::set<Register*> &succ_live_in = liveIn[succ];
			// union of all successor liveIns
			liveIn[BI].insert(succ_live_in.begin(),succ_live_in.end());
			// add Phi operands
			int index = succ->get_predecessor_index(BI);
			assert(index != -1);
			LOG3("line " << setz(4) << from << " " << "Predecessor index of " << BI << " in " << succ << " is " << index << nl);
			LOG3("line " << setz(4) << from << " " << "Number of reverse dependency links of " << succ << " is " << succ->dep_size() << nl);
			for (Instruction::DepListTy::const_iterator i = succ->rdep_begin(),
					e = succ->rdep_end() ; i != e; ++i) {
				PHIInst *phi = (*i)->to_PHIInst();
				if (phi) {
					LOG3("line " << setz(4) << from << " " << "PHI of " << succ << " is " << phi << nl);
					const LoweredInstDAG *dag = LP->get_LoweredInstDAG(phi);
					assert(dag);
					MachineOperand* op = dag->get_operand((unsigned)index);
					Register *reg;
					if ( op &&  (reg = op->to_Register()) ) {
						liveIn[BI].insert(reg);
						LOG2("line " << setz(4) << from << " " << "adding " << reg << " to liveIn of " << BI << nl);
					}
				}
			}
		}

		// set initial interval
		LOG2("initial liveIn for " << BI << ": ");
		if (DEBUG_COND_N(2)) {
			print_container(out(), liveIn[BI].begin(), liveIn[BI].end()) << nl;
		}
		for (LiveInSetTy::const_iterator i = liveIn[BI].begin(), e = liveIn[BI].end();
				i != e ; ++i) {
			LOG2("adding live range for " << *i << " (" << from << "," << to << ")" << nl);
			lti_map[*i].add_range(from,to);
		}
		// for all machine instructions in the current basic block in reversed order
		MachineInstructionSchedule::MachineInstructionRangeTy bb_range = MIS->get_range(BI);
		unsigned bb_first = bb_range.first * 2;
		unsigned bb_last = (bb_range.second - 1) * 2;
		for (signed inst_lineno = bb_last, end_lineno = bb_first;
				inst_lineno >= end_lineno; inst_lineno-=2) {
			assert(inst_lineno >= 0);
			MachineInstruction *MI = MIS->get((unsigned)inst_lineno / 2);
			// output operand
			{
				MachineOperandDesc& MOD = MI->get_result();
				Register *reg;
				if ( MOD.op &&  (reg = MOD.op->to_Register()) ) {
					// phis are handled differently
					if (!MI->is_phi()) {
						LOG2("line " << setz(4) << inst_lineno << " " << "removing " << reg << " to liveIn of " << BI << nl);
						LOG2("line " << setz(4) << inst_lineno << " " << "setting live range from for " << reg << " to " << inst_lineno << nl);
						lti_map[reg].set_from_if_available(inst_lineno + 1,to);
						liveIn[BI].erase(reg);
						// we set defines to the next (empty line)
						lti_map[reg].add_def(inst_lineno + 1,MOD);
					} else {
						// we set phi defines to the first def line of the basic block
						lti_map[reg].add_def(from + 1,MOD);
					}
					if (reg->to_MachineRegister()) {
						// this is already in a MachineRegister -> fixed interval
						lti_map[reg].set_fixed();
					}
				}
			}
			// input operands
			for(MachineInstruction::operand_iterator i = MI->begin(),
					e = MI->end(); i != e ; ++i) {
				MachineOperandDesc& MOD = *i;
				Register *reg;
				if ( MOD.op &&  (reg = MOD.op->to_Register()) ) {
					// phis are handled differently
					if (!MI->is_phi()) {
						LOG2("line " << setz(4) << inst_lineno << " " << "adding " << reg << " to liveIn of " << BI << nl);
						LOG2("line " << setz(4) << inst_lineno << " " << "adding live range for " << reg << " (" << from << "," << inst_lineno+1 << ")" << nl);
						lti_map[reg].add_range(from,inst_lineno+1);
						liveIn[BI].insert(reg);
						lti_map[reg].add_use(inst_lineno,MOD);
					} else{
						lti_map[reg].add_use(from,MOD);
					}
					if (reg->to_MachineRegister()) {
						// this is already in a MachineRegister -> fixed interval
						lti_map[reg].set_fixed();
					}
				}
			}
			// set move hint
			if (MI->is_move()) {
				Register *dst = MI->get_result().op->to_Register();
				Register *src = MI->get(0).op->to_Register();
				if (src && dst) {
					LOG2("HINT: " << dst << ".hint = " << src << nl);
					lti_map[dst].set_hint(src);
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
				MachineOperand* op = dag->get_result()->get_result().op;
				Register *reg;
				if ( op &&  (reg = op->to_Register()) ) {
					liveIn[BI].erase(reg);
					LOG2("line " << setz(4) << from << " " << "removing " << reg << " to liveIn of " << BI << nl);
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
					unsigned loop_exit_to = MIS->get_range(loop_exit).second * 2;
					LOG2("adding live range for " << *i << " (" << from << "," << loop_exit_to << ")" << nl);
					lti_map[*i].add_range(from,loop_exit_to);
				}
			}
		}

	}
	// set reg reference and delete empty intervals
	for (iterator i = lti_map.begin(),
			e = lti_map.end(); i != e ; ) {
		LivetimeInterval &lti = i->second;
		if (lti.size() == 0) {
			LOG2(BoldGreen << "info: " << reset_color <<
				"removing empty livetime interval: " << lti << nl);
			/// @Cpp11 erase returns an interator to the next element in C++11
			iterator del = i;
			++i;
			lti_map.erase(del);
		} else {
			lti.set_Register(i->first);
			++i;
		}
	}
	// debugging
	if (DEBUG_COND) {
		for (LiveInMapTy::const_iterator i = liveIn.begin(), e = liveIn.end();
				i != e ; ++i) {
			LOG("LiveIn for " << i->first << ": ");
			print_container(out(), i->second.begin(), i->second.end()) << nl;
		}

		LOG("Livetime Interval(s)" << nl);
		for (LivetimeIntervalMapTy::const_iterator i = lti_map.begin(),
				e = lti_map.end(); i != e ; ++i) {
			LOG(i->second << nl);
		}

	}
	DEBUG(print(dbg()));
#endif
	return true;
}

OStream& LivetimeAnalysisPass::print(OStream& OS) const {
#if 0
	OS << nl << "Liveinterval Table:" << nl;
	OS << "Line: ";

	for (LivetimeIntervalMapTy::const_iterator i = lti_map.begin(),
			e = lti_map.end(); i != e ; ++i) {
		Register *reg = i->first;
		// this is hardcoded -> do better
		if (reg->to_MachineRegister()) {
			OS << setw(7) << reg;
		} else {
			OS << setw(6) << reg;
		}
		OS << '|';
	}

	OS << "  Instructions" << nl;
	for (BasicBlockSchedule::const_bb_iterator i = BS->bb_begin(),
			e = BS->bb_end(); i != e ; ++i) {
		BeginInst *BI = *i;
		assert(BI);
		OS << BoldWhite << "BasicBlock: " << BI << reset_color << nl;
		MachineInstructionSchedule::MachineInstructionRangeTy range = MIS->get_range(BI);
		assert(range.first != range.second);
		for (unsigned pos = range.first*2, e = range.second*2; pos < e; ) {
			MachineInstruction *MI = MIS->get(pos/2);
			assert(MI);

			// print use and def lines
			for (int i = 0; i <2; ++i) {
				// print lineno
				OS << "  ";
				OS << setz(4) << pos;
				// print intervals
				for (LivetimeIntervalMapTy::const_iterator ii = lti_map.begin(),
						e = lti_map.end(); ii != e ; ++ii) {
					const LivetimeInterval *lti = &ii->second;
					// print cell entry!
					if (lti->is_unhandled(pos)) {
						OS << "       ";
					}
					else {
						for (; lti ; lti = lti->get_next()) {
							if (lti->is_handled(pos)) {
								if (lti->get_next()) {
									continue;
								}
								OS << "       ";
								break;
							}
							if (lti->is_active(pos)) {
								// avtive
								if (lti->is_in_Register()) {
									// in Register
									if (lti->get_Register()->to_MachineRegister()) {
										OS << setw(6) << lti->get_Register();
									} else {
										OS << setw(5) << lti->get_Register();
									}
								} else {
									// stack slot
									ManagedStackSlot *slot = lti->get_ManagedStackSlot();
									OS << "  s" << setz(3) << slot->get_id();
								}
								if (lti->is_use(pos)) {
									OS << "U";
								} else if (lti->is_def(pos)) {
									OS << "D";
								} else {
									OS << " ";
								}
								break;
							}
							if (lti->is_inactive(pos)) {
								OS << "  ---  ";
								break;
							}
						}
					}
					OS << '|';
				}
				// print instructions
				if (i == 0) {
					OS << ": " << MI << nl;
				} else {
					OS << ": [rslt]" << nl;
				}
				++pos;
			}
		}
	}
#endif
	return OS;
}

bool LivetimeAnalysisPass::verify() const {
#if 0
	for (LivetimeIntervalMapTy::const_iterator i = lti_map.begin(),
			e = lti_map.end(); i != e ; ++i) {
		const LivetimeInterval &lti = i->second;
		signed old = -1;
		for (LivetimeInterval::const_use_iterator i = lti.use_begin(),
				e = lti.use_end(); i != e; ++i) {
			if ( old > (signed)i->first) {
				err() << "use not ordered " << old << " > " << i->first << nl;
				return false;
			}
			old = i->first;
		}
		old = -1;
		for (LivetimeInterval::const_def_iterator i = lti.def_begin(),
				e = lti.def_end(); i != e; ++i) {
			if ( old > (signed)i->first) {
				err() << "def not ordered " << old << " > " << i->first << nl;
				return false;
			}
			old = i->first;
		}
	}
#endif
	return true;
}
// pass usage
PassUsage& LivetimeAnalysisPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires(BasicBlockSchedulingPass::ID);
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
