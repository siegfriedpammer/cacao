/* src/vm/jit/compiler2/BasicBlockSchedulingPass.cpp - BasicBlockSchedulingPass

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

#include "vm/jit/compiler2/BasicBlockSchedulingPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/LoopPass.hpp"
#include "vm/jit/compiler2/DominatorPass.hpp"
#include "toolbox/logging.hpp"

#include "vm/jit/compiler2/alloc/map.hpp"
#include "vm/jit/compiler2/alloc/set.hpp"
#include "vm/jit/compiler2/alloc/list.hpp"
#include "vm/jit/compiler2/alloc/deque.hpp"

#define DEBUG_NAME "compiler2/BasicBlockSchedulingPass"

#define VERIFY_LOOP_PROPERTY 1
#define VERIFY_DOM_PROPERTY 1

namespace cacao {
namespace jit {
namespace compiler2 {


namespace {
	struct DomComparator : std::binary_function<BeginInst*, BeginInst*, bool>{
		DominatorTree *DT;
		DomComparator(DominatorTree *DT) : DT(DT) {}
		bool operator()(BeginInst *lhs, BeginInst *rhs) {
			return DT->dominates(lhs,rhs);
		}
	};
} // anonymous namespace


class LoopScheduler {
private:
	// XXX use unordered map and hash
	typedef alloc::list<BeginInst*>::type BeginListTy;
	typedef alloc::unordered_set<BeginInst*>::type BeginSetTy;
	typedef alloc::unordered_set<Loop*>::type LoopSetTy;
	typedef alloc::map<BeginInst*,BeginSetTy>::type DomSuccMapTy;
	typedef alloc::map<Loop*,BeginSetTy>::type LoopMapTy;
	typedef alloc::set<BeginInst*>::type BeginSchedMapTy;

	LoopMapTy loopmap;
	DomSuccMapTy dommap;
	BeginListTy bb_sched;
	DominatorTree *DT;
	LoopTree *LT;

	bool is_scheduled(BeginInst* BI) const {
		return std::find(bb_sched.begin(),bb_sched.end(), BI) != bb_sched.end();
	}

	void schedule_loop(Loop* loop) {
		LOG3(Magenta << "schedule_loop: " << loop << reset_color << nl);
		BeginSetTy unsched(loopmap[loop]);
		LoopSetTy subloops;
		if (loop) {
			subloops.insert(loop->loop_begin(),loop->loop_end());
		}
		else {
			subloops.insert(LT->loop_begin(),LT->loop_end());
		}
		while ( !unsched.empty() || !subloops.empty()) {
			// try schedule loop bbs
			bool changed = true;
			while (changed) {
				changed = false;
				for(BeginSetTy::iterator i = unsched.begin(), e = unsched.end() ; i != e; ++i) {
					BeginInst *BI = *i;
					BeginInst *idom = DT->get_idominator(BI);
					if ( !idom || is_scheduled(idom)) {
						// idom scheduled
						bb_sched.push_back(BI);
						unsched.erase(i);
						LOG3(Green <<"  scheduled: " << BI << reset_color<<nl);
						changed=true;
						break;
					} else {
						LOG3(Red <<"  not scheduled: " << BI << reset_color << " idom: " << idom <<nl);
					}
				}
			}
			assert(unsched.empty() || !subloops.empty());
			for(LoopSetTy::iterator i = subloops.begin() , e = subloops.end(); i != e; ++i) {
				Loop *subloop = *i;
				if (is_scheduled(DT->get_idominator(subloop->get_header()))){
					LOG3("schedule subloop of " << loop << nl);
					schedule_loop(*i);
					subloops.erase(i);
					break;
				}
			}
		}
		LOG3("finished: " << loop << nl);
	}
public:
	/// constructor
	LoopScheduler(Method *M, DominatorTree *DT, LoopTree *LT) : DT(DT), LT(LT) {
		// fill maps
		for(Method::const_bb_iterator i = M->bb_begin(), e = M->bb_end() ; i != e; ++i) {
			BeginInst *BI = *i;
			dommap[DT->get_idominator(BI)].insert(BI);
			loopmap[LT->get_Loop(BI)].insert(BI);
		}
		// start scheduling
		schedule_loop(NULL);
	}
	BeginListTy::const_iterator begin() const { return bb_sched.begin(); }
	BeginListTy::const_iterator end()   const { return bb_sched.end(); }
};

bool BasicBlockSchedulingPass::run(JITData &JD) {
	M = JD.get_Method();
	DominatorTree *DT = get_Pass<DominatorPass>();
	LoopTree *LT = get_Pass<LoopPass>();

	// schedule loops
	LoopScheduler loop_scheduler(M,DT,LT);

	insert(begin(),loop_scheduler.begin(),loop_scheduler.end());

	#if 0
	// random order
	alloc::deque<BeginInst*>::type bbs(M->bb_begin(),M->bb_end());
	std::remove(bbs.begin(),bbs.end(),M->get_init_bb());
	std::random_shuffle(bbs.begin(),bbs.end());
	bbs.push_front(M->get_init_bb());
	insert(begin(),bbs.begin(),bbs.end());
	#endif

	if (DEBUG_COND) {
		LOG("BasicBlockSchedule:" << nl);
		for (const_bb_iterator i = bb_begin(), e = bb_end(); i != e; ++i) {
			LOG(*i << nl);
		}
	}
	return true;
}

namespace {
#if VERIFY_LOOP_PROPERTY
// push loops recursively in reverse order
void push_loops_inbetween(alloc::list<Loop*>::type &active, Loop* inner, Loop* outer) {
	if (inner == outer) return;
	push_loops_inbetween(active,inner->get_parent(),outer);
	active.push_back(inner);
}
#endif
} // anonymous namespace


namespace tree {

template <>
inline Loop* get_parent(Loop* a) {
	return a->get_parent();
}

template <>
inline Loop* get_root() {
	return NULL;
}

} // end namespace tree

bool BasicBlockSchedulingPass::verify() const {
	// check init basic block
	if (*bb_begin() != M->get_init_bb()) {
		ERROR_MSG("Schedule does not start with init basic block!",
			"Init basic block is " << M->get_init_bb()
			<< " but schedule starts with " << *bb_begin());
		return false;
	}
	// check if ordering (obsolete)
	#if 0
	for (unsigned i = 0, e = bb_list.size() ; i < e ; ++i) {
		BeginInst *BI = bb_list[i];
		assert(BI);
		EndInst *EI = BI->get_EndInst();
		if (EI->to_IFInst()) {
			EndInst::SuccessorListTy::const_iterator it = EI->succ_begin();
			++it;
			assert(it != EI->succ_end());
			assert(i+1 < e);
			if (it->get() != bb_list[i+1]) {
				return false;
			}
		}
	}
	#endif
	#if VERIFY_DOM_PROPERTY
	// check dominator property
	DominatorTree *DT = get_Pass<DominatorPass>();
	alloc::set<BeginInst*>::type handled;
	for (BasicBlockSchedule::const_bb_iterator i = bb_begin(), e = bb_end();
			i !=e ; ++i) {
		BeginInst *idom = DT->get_idominator(*i);
		if (idom) {
			if (handled.find(idom) == handled.end()) {
				ERROR_MSG("Dominator property violated!","immediate dominator (" << *idom
					<< ") of " << **i << "not already scheduled" );
				return false;
			}
		}
		handled.insert(*i);
	}
	#endif
	// check contiguous loop property
	#if VERIFY_LOOP_PROPERTY
	LoopTree *LT = get_Pass<LoopPass>();
	alloc::set<Loop*>::type finished;
	alloc::list<Loop*>::type active;
	// sentinel
	active.push_back(NULL);
	for (BasicBlockSchedule::const_bb_iterator i = bb_begin(), e = bb_end();
			i !=e ; ++i) {
		BeginInst *BI = *i;
		Loop *loop = LT->get_Loop(BI);
		Loop *top = active.back();

		if ( loop && finished.find(loop) != finished.end()) {
				ERROR_MSG("Loop property violated!","Loop " << *loop
					<< " already finished but " << *BI << " occurred" );
				return false;
		}

		if (loop == top) {
			// same loop
			continue;
		}
		// new loop(s)
		// find least common ancestor
		Loop *lca = tree::find_least_common_ancestor(loop, top);
		while (top != lca) {
			assert(!active.empty());
			finished.insert(top);
			active.pop_back();
			top = active.back();
		}
		push_loops_inbetween(active,loop,lca);
	}
	#endif
	return true;
}

// pass usage
PassUsage& BasicBlockSchedulingPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires<DominatorPass>();
	PU.add_requires<LoopPass>();
	return PU;
}

// the address of this variable is used to identify the pass
char BasicBlockSchedulingPass::ID = 0;

// register pass
static PassRegistry<BasicBlockSchedulingPass> X("BasicBlockSchedulingPass");

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
