/* src/vm/jit/compiler2/PassManager.cpp - PassManager

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

#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/PassDependencyGraphPrinter.hpp"
#include "vm/jit/compiler2/Pass.hpp"
#include "toolbox/logging.hpp"
#include "vm/vm.hpp"
#include "vm/jit/compiler2/alloc/set.hpp"
#include "vm/jit/compiler2/alloc/list.hpp"
#include "vm/jit/compiler2/alloc/deque.hpp"

#include "vm/rt-timing.hpp"

#include <algorithm>

#define DEBUG_NAME "compiler2/PassManager"

namespace cacao {
namespace jit {
namespace compiler2 {

#if ENABLE_RT_TIMING
namespace {
RT_REGISTER_GROUP(compiler2_rtgroup,"compiler2-pipeline","compiler2 pass pipeline")

typedef alloc::unordered_map<PassInfo::IDTy,RTTimer>::type PassTimerMap;
PassTimerMap pass_timers;

} // end anonymous namespace
#endif

Pass* PassManager::get_initialized_Pass(PassInfo::IDTy ID) {
	Pass *P = initialized_passes[ID];
	if (!P) {
		PassInfo *PI = registered_passes()[ID];
		assert(PI && "Pass not registered");
		// This should be the only place where a Pass is constructed!
		P = PI->create_Pass();
		P->set_PassManager(this);
		initialized_passes[ID] = P;
		result_ready[ID] = false;
		#if ENABLE_RT_TIMING
		RTTimer &timer = pass_timers[ID];
		new (&timer) RTTimer(PI->get_name(),PI->get_name(),compiler2_rtgroup());
		#endif
	}
	return P;
}

PassManager::~PassManager() {
	// delete all passes
	for(PassMapTy::iterator i = initialized_passes.begin(), e = initialized_passes.end(); i != e; ++i) {
		Pass* P = i->second;
		delete P;
	}
}

void PassManager::initializePasses() {
}

void PassManager::runPasses(JITData &JD) {
	LOG("runPasses" << nl);
	print_PassDependencyGraph(*this);
	initializePasses();
	schedulePasses();
	for(ScheduleListTy::iterator i = schedule.begin(), e = schedule.end(); i != e; ++i) {
		PassInfo::IDTy id = *i;
		result_ready[id] = false;
		#if ENABLE_RT_TIMING
		PassTimerMap::iterator f = pass_timers.find(id);
		assert(f != pass_timers.end());
		RTTimer &timer = f->second;
		timer.start();
		#endif
		Pass* P = get_initialized_Pass(id);
		LOG("initialize: " << get_Pass_name(id) << nl);
		P->initialize();
		LOG("start: " << get_Pass_name(id) << nl);
		if (!P->run(JD)) {
			err() << bold << Red << "error" << reset_color << " during pass " << get_Pass_name(id) << nl;
			os::abort("compiler2: error");
		}
		// invalidating results
		PassUsage PU;
		PU = P->get_PassUsage(PU);
		for (PassUsage::const_iterator i = PU.destroys_begin(), e = PU.destroys_end();
				i != e; ++i) {
			result_ready[*i] = false;
			LOG("mark invalid" << get_Pass_name(*i) << nl);
		}
		#ifndef NDEBUG
		LOG("verifying: " << get_Pass_name(id) << nl);
		if (!P->verify()) {
			err() << bold << Red << "verification error" << reset_color << " during pass " << get_Pass_name(id) << nl;
			os::abort("compiler2: error");
		}
		#endif
		result_ready[id] = true;
		LOG("finialize: " << get_Pass_name(id) << nl);
		P->finalize();
		#if ENABLE_RT_TIMING
		timer.stop();
		#endif
	}
	finalizePasses();
}

void PassManager::finalizePasses() {
}


// begin Pass scheduler

#undef DEBUG_NAME
#define DEBUG_NAME "compiler2/PassManager/Scheduler"
namespace {

#if defined(ENABLE_LOGGING) || !defined(NDEBUG)
// XXX for debugging only, to be removed
PassManager* latest = NULL;
// XXX for debugging only, to be removed
const char* get_Pass_name(PassInfo::IDTy id) {
	if (!latest) return "PassManager not available";
	return latest->get_Pass_name(id);
}
#endif // defined(ENABLE_LOGGING) || !defined(NDEBUG)

template <class InputIterator, class ValueType>
inline bool contains(InputIterator begin, InputIterator end, const ValueType &val) {
	return std::find(begin,end,val) != end;
}

template <class Container, class ValueType>
struct ContainsFn {
	bool operator()(const Container &c, const ValueType &val) {
		return contains(c.begin(),c.end(),val);
	}
};

template <class ValueType>
struct ContainsFn<typename alloc::set<ValueType>::type,ValueType> {
	bool operator()(const typename alloc::set<ValueType>::type &c, const ValueType &val) {
		return c.find(val) != c.end();
	}
};

template <class ValueType>
struct ContainsFn<unordered_set<ValueType>,ValueType> {
	bool operator()(const typename alloc::unordered_set<ValueType>::type &c, const ValueType &val) {
		return c.find(val) != c.end();
	}
};

template <class Container, class ValueType>
inline bool contains(const Container &c, const ValueType &val) {
	return ContainsFn<Container,ValueType>()(c,val);
}


typedef alloc::unordered_map<PassInfo::IDTy,PassUsage>::type ID2PUTy;
typedef alloc::unordered_map<PassInfo::IDTy,alloc::unordered_set<PassInfo::IDTy>::type >::type ID2MapTy;

struct Invalidate :
public std::unary_function<PassInfo::IDTy,void> {
	ID2MapTy &reverse_require_map;
	alloc::unordered_set<PassInfo::IDTy>::type &ready;
	// constructor
	Invalidate(ID2MapTy &reverse_require_map, alloc::unordered_set<PassInfo::IDTy>::type &ready)
		: reverse_require_map(reverse_require_map), ready(ready) {}
	// function call operator
	void operator()(PassInfo::IDTy id) {
		if (ready.erase(id)) {
			LOG3("  invalidated: " << get_Pass_name(id) << nl);
			std::for_each(reverse_require_map[id].begin(),reverse_require_map[id].end(),*this);
		}
	}
};

class PassScheduler {
private:
	alloc::deque<PassInfo::IDTy>::type &unhandled;
	alloc::unordered_set<PassInfo::IDTy>::type &ready;
	alloc::list<PassInfo::IDTy>::type &stack;
	PassManager::ScheduleListTy &new_schedule;
	ID2PUTy &pu_map;
	ID2MapTy &reverse_require_map;
public:
	/// constructor
	PassScheduler(alloc::deque<PassInfo::IDTy>::type &unhandled, alloc::unordered_set<PassInfo::IDTy>::type &ready,
		alloc::list<PassInfo::IDTy>::type &stack, PassManager::ScheduleListTy &new_schedule,
		ID2PUTy &pu_map, ID2MapTy &reverse_require_map)
			: unhandled(unhandled), ready(ready), stack(stack), new_schedule(new_schedule),
			pu_map(pu_map), reverse_require_map(reverse_require_map) {}
	/// call operator
	void operator()(PassInfo::IDTy id) {
		if (contains(ready,id)) return;
		#ifndef NDEBUG
			if (contains(stack,id)) {
				ABORT_MSG("PassManager: dependency cycle detected",
					"Pass " << get_Pass_name(id) << " already stacked for scheduling!");
			}
		#endif
		stack.push_back(id);
		PassUsage &PU = pu_map[id];
		LOG3("prescheduled: " << get_Pass_name(id) << nl);
		bool fixpoint;
		do {
			fixpoint = true;
			// schedule schedule_after
			for (PassUsage::const_iterator i = PU.schedule_after_begin(), e  = PU.schedule_after_end();
					i != e ; ++i) {
				LOG3(" schedule_after: " << get_Pass_name(*i) << nl);
				if (std::find(new_schedule.rbegin(),new_schedule.rend(),*i) == new_schedule.rend()) {
					operator()(*i);
					fixpoint = false;
				}
			}
			// schedule requires
			for (PassUsage::const_iterator i = PU.requires_begin(), e  = PU.requires_end();
					i != e ; ++i) {
				LOG3(" requires: " << get_Pass_name(*i) << nl);
				if (ready.find(*i) == ready.end()) {
					operator()(*i);
					fixpoint = false;
				}
			}
		} while(!fixpoint);

		LOG3("scheduled: " << get_Pass_name(id) << nl);
		// remove all destroyed passes from ready
		std::for_each(PU.destroys_begin(),PU.destroys_end(),Invalidate(reverse_require_map,ready));
		// remove all passed depending on modified from ready
		for (PassUsage::const_iterator i = PU.modifies_begin(), e  = PU.modifies_end();
				i != e ; ++i) {
			if (ready.find(*i) != ready.end()) {
				LOG3(" modifies: " << get_Pass_name(*i) << nl);
				std::for_each(reverse_require_map[*i].begin(),reverse_require_map[*i].end(),
					Invalidate(reverse_require_map,ready));
			}
		}
		// dummy use
		unhandled.front();
		new_schedule.push_back(id);
		ready.insert(id);
		stack.pop_back();
	}
};

struct RunBefore : public std::unary_function<PassInfo::IDTy,void> {
	ID2PUTy &pu_map;
	PassInfo::IDTy id;

	RunBefore(ID2PUTy &pu_map, PassInfo::IDTy id)
		: pu_map(pu_map), id(id) {}

	void operator()(PassInfo::IDTy before) {
		//pu_map[before].add_schedule_after(id);
		pu_map[before].add_requires(id);
	}
};

struct ScheduleBefore : public std::unary_function<PassInfo::IDTy,void> {
	ID2PUTy &pu_map;
	PassInfo::IDTy id;

	ScheduleBefore(ID2PUTy &pu_map, PassInfo::IDTy id)
		: pu_map(pu_map), id(id) {}

	void operator()(PassInfo::IDTy before) {
		pu_map[before].add_schedule_after(id);
	}
};

struct AddReverseRequire : public std::unary_function<PassInfo::IDTy,void> {
	ID2MapTy &map;
	PassInfo::IDTy id;

	AddReverseRequire(ID2MapTy &map, PassInfo::IDTy id)
		: map(map), id(id) {}

	void operator()(PassInfo::IDTy required_by) {
		map[required_by].insert(id);
	}
};

struct ReverseRequire :
public std::unary_function<ID2PUTy::value_type,void> {
	ID2MapTy &map;
	// constructor
	ReverseRequire(ID2MapTy &map) : map(map) {}
	// function call operator
	void operator()(argument_type pair) {
		std::for_each(pair.second.requires_begin(), pair.second.requires_end(),AddReverseRequire(map,pair.first));
	}
};

} // end anonymous namespace

void PassManager::schedulePasses() {
#if defined(ENABLE_LOGGING) || !defined(NDEBUG)
	// XXX for debugging only, to be removed
	latest = this;
#endif

	alloc::deque<PassInfo::IDTy>::type unhandled;
	alloc::unordered_set<PassInfo::IDTy>::type ready;
	alloc::list<PassInfo::IDTy>::type stack;
	ScheduleListTy new_schedule;
	ID2PUTy pu_map;
	ID2MapTy reverse_require_map;

	for (PassInfoMapTy::const_iterator i = registered_passes().begin(), e = registered_passes().end();
			i != e; ++i) {
		PassInfo::IDTy id = i->first;
		Pass *pass = get_initialized_Pass(id);
		PassUsage &PA = pu_map[id];
		pass->get_PassUsage(PA);
		// add run before
		std::for_each(PA.run_before_begin(),PA.run_before_end(),RunBefore(pu_map,id));
		// add schedule before
		std::for_each(PA.schedule_before_begin(),PA.schedule_before_end(),ScheduleBefore(pu_map,id));
	}
	// fill reverser required map
	std::for_each(pu_map.begin(),pu_map.end(),ReverseRequire(reverse_require_map));
	//
	std::copy(schedule.begin(), schedule.end(), std::back_inserter(unhandled));

	PassScheduler scheduler(unhandled,ready,stack,new_schedule,pu_map,reverse_require_map);
	while (!unhandled.empty()) {
		PassInfo::IDTy id = unhandled.front();
		unhandled.pop_front();
		// schedule
		scheduler(id);
		assert(stack.empty());
	}

	if (DEBUG_COND_N(2)) {
		LOG2("old Schedule:" << nl);
		for (ScheduleListTy::const_iterator i = schedule.begin(),
				e = schedule.end(); i != e ; ++i) {
			LOG2("    " << get_Pass_name(*i) << " id: " << *i << nl);
		}
		LOG2("new Schedule:" << nl);
		for (ScheduleListTy::const_iterator i = new_schedule.begin(),
				e = new_schedule.end(); i != e ; ++i) {
			LOG2("    " << get_Pass_name(*i) << " id: " << *i << nl);
		}
	}
	schedule = new_schedule;
}
} // end namespace cacao
} // end namespace jit
} // end namespace compiler2

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
