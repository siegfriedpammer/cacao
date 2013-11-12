/* src/vm/jit/compiler2/ListSchedulingPass.cpp - ListSchedulingPass

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

#include "vm/jit/compiler2/ListSchedulingPass.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/ScheduleClickPass.hpp"

#include "toolbox/logging.hpp"

#include <set>
#include <map>
#include <list>
#include <queue>
#include <deque>

#define DEBUG_NAME "compiler2/ListScheduling"

namespace cacao {
namespace jit {
namespace compiler2 {

namespace {
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
		// BeginInst always first!
		if (lhs->to_BeginInst()) return false;
		if (rhs->to_BeginInst()) return true;
		// EndInst always last!
		if (lhs->to_EndInst()) return true;
		if (rhs->to_EndInst()) return false;
		// PHIs right after BeginInst
		if (lhs->to_PHIInst()) return false;
		if (rhs->to_PHIInst()) return true;
		// LOADInst first
		if (lhs->to_LOADInst()) return false;
		if (rhs->to_LOADInst()) return true;
		// prioritize instruction with fewer users in the current bb
		return users(lhs) > users(rhs);
	}
};

typedef std::priority_queue<Instruction*,std::deque<Instruction*>,MyComparator> PriorityQueueTy;

struct FindLeader2 : public std::unary_function<Value*,void> {
	std::set<Instruction*> &scheduled;
	GlobalSchedule *sched;
	Instruction *I;
	bool &leader;
	/// constructor
	FindLeader2(std::set<Instruction*> &scheduled, GlobalSchedule *sched, Instruction *I, bool &leader)
		: scheduled(scheduled), sched(sched), I(I), leader(leader) {}
	/// function call operator
	void operator()(Value *value) {
		Instruction *op = value->to_Instruction();
		assert(op);
		if (op && sched->get(I) == sched->get(op) && scheduled.find(op) == scheduled.end() ) {
			leader = false;
		}
	}
};

struct FindLeader : public std::unary_function<Instruction*,void> {
	std::set<Instruction*> &scheduled;
	GlobalSchedule *sched;
	PriorityQueueTy &ready;
	BeginInst *BI;
	/// constructor
	FindLeader(std::set<Instruction*> &scheduled, GlobalSchedule *sched, PriorityQueueTy &ready, BeginInst *BI)
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


void ListSchedulingPass::schedule(BeginInst *BI) {
	// reference to the instruction list of the current basic block
	InstructionListTy &inst_list = map[BI];
	// set of already scheduled instructions
	std::set<Instruction*> scheduled;
	// queue of ready instructions
	MyComparator comp = MyComparator(sched);
	PriorityQueueTy ready(comp);
	// Begin is always the first instruction
	ready.push(BI);
	for(GlobalSchedule::const_inst_iterator i = sched->inst_begin(BI),
			e = sched->inst_end(BI); i != e; ++i) {
		Instruction *I = *i;
		// BI is already in the queue
		if (I->to_BeginInst() == BI)
			continue;
		//LOG("Instruction: " << I << nl);
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
}

bool ListSchedulingPass::run(JITData &JD) {
	M = JD.get_Method();
	sched = get_Pass<ScheduleClickPass>();
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

PassUsage& ListSchedulingPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires<ScheduleClickPass>();
	return PU;
}
// the address of this variable is used to identify the pass
char ListSchedulingPass::ID = 0;

// registrate Pass
static PassRegistry<ListSchedulingPass> X("ListSchedulingPass");

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
