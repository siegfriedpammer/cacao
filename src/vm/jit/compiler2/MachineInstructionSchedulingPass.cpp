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

#include "vm/jit/compiler2/BasicBlockSchedulingPass.hpp"
#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/ConstantPropagationPass.hpp"

#include "vm/jit/compiler2/Matcher.hpp"

#include "vm/jit/compiler2/alloc/queue.hpp"
#include "vm/jit/compiler2/alloc/deque.hpp"

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

// LIST SCHEDULING ADDITIONAL STUFF IN ANONYMOUS NAMESPACE

class MyComparator {
private:
	const GlobalSchedule* sched;
	unsigned users(Instruction *I) const {
		unsigned users = 0;
		for (Instruction::UserListTy::const_iterator i = I->user_begin(),
				e = I->user_end(); i != e; ++i) {
			Instruction *user = *i;
			// only instructions in this basic block
			if ( sched->get(user) == sched->get(I)) {
				users++;
			}
		}
		return users;
	}
public:
	MyComparator(const GlobalSchedule* sched) : sched(sched) {}
	bool operator() (Instruction* lhs, Instruction* rhs) const {
		if (lhs->get_opcode() != rhs->get_opcode()) {
			// BeginInst always first!
			if (lhs->to_BeginInst()) return false;
			if (rhs->to_BeginInst()) return true;
			// EndInst always last!
			if (rhs->to_EndInst()) return false;
			if (lhs->to_EndInst()) return true;
			// PHIs right after BeginInst
			if (lhs->to_PHIInst()) return false;
			if (rhs->to_PHIInst()) return true;
			// LOADInst first
			if (lhs->to_LOADInst()) return false;
			if (rhs->to_LOADInst()) return true;
		}
		// prioritize instruction with fewer users in the current bb
		unsigned lhs_user = users(lhs);
		unsigned rhs_user = users(rhs);
		if (lhs_user == rhs_user) {
			return InstPtrLess()(rhs,lhs);
		}
		return lhs_user > rhs_user;
	}
};

typedef alloc::priority_queue<Instruction*,alloc::deque<Instruction*>::type,MyComparator>::type PriorityQueueTy;

struct FindLeader2 : public std::unary_function<Value*,void> {
	alloc::set<Instruction*>::type &scheduled;
	GlobalSchedule *sched;
	Instruction *I;
	bool &leader;
	/// constructor
	FindLeader2(alloc::set<Instruction*>::type &scheduled, GlobalSchedule *sched, Instruction *I, bool &leader)
		: scheduled(scheduled), sched(sched), I(I), leader(leader) {}
	/// function call operator
	void operator()(Value *value) {
		Instruction *op = value->to_Instruction();
		assert(op);
		if (op != I && sched->get(I) == sched->get(op) && scheduled.find(op) == scheduled.end() ) {
			leader = false;
		}
	}
};

struct FindLeader : public std::unary_function<Instruction*,void> {
	alloc::set<Instruction*>::type &scheduled;
	GlobalSchedule *sched;
	PriorityQueueTy &ready;
	BeginInst *BI;
	/// constructor
	FindLeader(alloc::set<Instruction*>::type &scheduled, GlobalSchedule *sched, PriorityQueueTy &ready, BeginInst *BI)
		: scheduled(scheduled), sched(sched), ready(ready), BI(BI) {}

	/// function call operator
	void operator()(Instruction* I) {
		// only instructions in this basic block
		if ( sched->get(I) != sched->get(BI)) {
			return;
		}
		bool leader = true;
		// for each operand
		std::for_each(I->op_begin(), I->op_end(), FindLeader2(scheduled, sched, I, leader));
		// for each dependency
		std::for_each(I->dep_begin(), I->dep_end(), FindLeader2(scheduled, sched, I, leader));
		if (leader) {
			LOG("leader: " << I << nl);
			ready.push(I);
		}
	}
};

} // end anonymous namespace

bool MachineInstructionSchedulingPass::run(JITData &JD) {
	BasicBlockSchedule *BS = get_Pass<BasicBlockSchedulingPass>();
	GlobalSchedule* GS = get_Pass<ScheduleClickPass>();

#if !PATTERN_MATCHING
	IS = shared_ptr<ListSchedulingPass>(new ListSchedulingPass(GS));
	IS->run(JD);
#endif


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
		LoweringVisitor LV(BE,MBB,map,inst_map,this);

#if !PATTERN_MATCHING
		LOG2("lowering for BB using list sched " << BI << nl);
		for (InstructionSchedule<Instruction>::const_inst_iterator i = IS->inst_begin(BI),
				e = IS->inst_end(BI); i != e; ++i) {
			Instruction *I = *i;
			LOG2("lower: " << *I << nl);
			I->accept(LV, true);
		}
#else
		LOG2("Lowering with Pattern Matching " << BI << nl);
		Matcher M(GS, BI, &LV);
		M.run();
#endif
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

#if !PATTERN_MATCHING
	// call List Scheduling verify first
	IS->verify();
#endif

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
	PU.add_requires<ScheduleClickPass>();
	return PU;
}

// register pass
static PassRegistry<MachineInstructionSchedulingPass> X("MachineInstructionSchedulingPass");

// LIST SCHEDULING PART MOVED HERE

void MachineInstructionSchedulingPass::ListSchedulingPass::schedule(BeginInst *BI) {
	// reference to the instruction list of the current basic block
	InstructionListTy &inst_list = map[BI];
	// set of already scheduled instructions
	alloc::set<Instruction*>::type scheduled;
	// queue of ready instructions
	MyComparator comp = MyComparator(sched);
	PriorityQueueTy ready(comp);
	// Begin is always the first instruction
	ready.push(BI);
	LOG3(Cyan<<"BI: " << BI << reset_color << nl);
	for(GlobalSchedule::const_inst_iterator i = sched->inst_begin(BI),
			e = sched->inst_end(BI); i != e; ++i) {
		Instruction *I = *i;
		// BI is already in the queue
		if (I->to_BeginInst() == BI)
			continue;
		LOG3("Instruction: " << I << nl);
		//fill_ready(sched, ready, I);
		bool leader = true;

		LOG3("schedule(initial leader): " << I << " #op " << I->op_size()  << " #dep " << I->dep_size() << nl);
		// for each operand
		std::for_each(I->op_begin(), I->op_end(), FindLeader2(scheduled, sched, I, leader));
		// for each dependency
		std::for_each(I->dep_begin(), I->dep_end(), FindLeader2(scheduled, sched, I, leader));

		if (leader) {
			LOG("initial leader: " << I << nl);
			ready.push(I);
		}
	}

	while (!ready.empty()) {
		Instruction *I = ready.top();
		ready.pop();
		if (scheduled.find(I) != scheduled.end()){
			continue;
		}
		inst_list.push_back(I);
		LOG("insert: " << I << nl);
		scheduled.insert(I);
		// for all users
		std::for_each(I->user_begin(), I->user_end(), FindLeader(scheduled,sched,ready,BI));
		// for all dependant instruction
		std::for_each(I->rdep_begin(), I->rdep_end(), FindLeader(scheduled,sched,ready,BI));
	}
	#if defined(ENABLE_LOGGING)
	for(const_inst_iterator i = inst_begin(BI), e = inst_end(BI); i != e ; ++i) {
		Instruction *I = *i;
		LOG(I<<nl);
	}
	#endif
	assert( std::distance(inst_begin(BI),inst_end(BI)) == std::distance(sched->inst_begin(BI),sched->inst_end(BI)) );
}

bool MachineInstructionSchedulingPass::ListSchedulingPass::run(JITData &JD) {
	M = JD.get_Method();
	for (Method::const_bb_iterator i = M->bb_begin(), e = M->bb_end() ; i != e ; ++i) {
		BeginInst *BI = *i;
		LOG("ListScheduling: " << BI << nl);
		schedule(BI);
	}
	#if defined(ENABLE_LOGGING)
	LOG("Schedule:" << nl);
	for (Method::const_bb_iterator i = M->bb_begin(), e = M->bb_end() ; i != e ; ++i) {
		BeginInst *BI = *i;
		for(const_inst_iterator i = inst_begin(BI), e = inst_end(BI); i != e ; ++i) {
			Instruction *I = *i;
			LOG(I<<nl);
		}
	}
	#endif
	return true;
}

bool MachineInstructionSchedulingPass::ListSchedulingPass::verify() const {
	for (Method::const_iterator i = M->begin(), e = M->end(); i!=e; ++i) {
		Instruction *I = *i;
		bool found = false;
		for (Method::const_bb_iterator i = M->bb_begin(), e = M->bb_end(); i!=e; ++i) {
			BeginInst *BI = *i;
			if (std::find(inst_begin(BI),inst_end(BI),I) != inst_end(BI)) {
				found = true;
			}
		}
		if (!found) {
			ERROR_MSG("Instruction not Scheduled!","Instruction: " << I);
			return false;
		}
	}
	return true;
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
