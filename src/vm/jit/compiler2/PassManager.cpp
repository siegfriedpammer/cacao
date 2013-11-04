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

#include <algorithm>

#define DEBUG_NAME "compiler2/PassManager"

namespace cacao {
namespace jit {
namespace compiler2 {

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

namespace {

template <class InputIterator, class ValueType>
inline bool contains(InputIterator begin, InputIterator end, const ValueType &val) {
	return std::find(begin,end,val) != end;
}

template <class Container, class ValueType>
inline bool contains(Container c, const ValueType &val) {
	return c.find(val) != c.end();
}


class PassScheduler {
private:
	std::deque<PassInfo::IDTy> &unhandled;
	std::set<PassInfo::IDTy> &ready;
	std::list<PassInfo::IDTy> &stack;
	PassManager::ScheduleListTy &new_schedule;
	std::map<PassInfo::IDTy,PassUsage> &pu_map;
public:
	/// constructor
	PassScheduler(std::deque<PassInfo::IDTy> &unhandled, std::set<PassInfo::IDTy> &ready,
		std::list<PassInfo::IDTy> &stack, PassManager::ScheduleListTy &new_schedule,
		std::map<PassInfo::IDTy,PassUsage> &pu_map)
			: unhandled(unhandled), ready(ready), stack(stack), new_schedule(new_schedule),
			pu_map(pu_map) {}
	/// call operator
	void operator()(PassInfo::IDTy id) {
		if (contains(ready,id)) return;
		stack.push_back(id);
		PassUsage &PU = pu_map[id];
		for (PassUsage::const_iterator i = PU.requires_begin(), e  = PU.requires_end();
				i != e ; ++i) {
			if (ready.find(*i) == ready.end()) {
				operator()(*i);
			}
		}
		// remove all destroyed passes from ready
		for (PassUsage::const_iterator i = PU.destroys_begin(), e  = PU.destroys_end();
				i != e ; ++i) {
			ready.erase(*i);
		}
		// dummy use
		unhandled.front();
		new_schedule.push_back(id);
		ready.insert(id);
		stack.pop_back();
	}
};

} // end anonymous namespace

void PassManager::schedulePasses() {
	std::deque<PassInfo::IDTy> unhandled;
	std::set<PassInfo::IDTy> ready;
	std::list<PassInfo::IDTy> stack;
	ScheduleListTy new_schedule;
	std::map<PassInfo::IDTy,PassUsage> pu_map;

	for (PassInfoMapTy::const_iterator i = registered_passes().begin(), e = registered_passes().end();
			i != e; ++i) {
		PassInfo::IDTy id = i->first;
		Pass *pass = get_initialized_Pass(id);
		PassUsage &PA = pu_map[id];
		pass->get_PassUsage(PA);
	}
	std::copy(schedule.begin(), schedule.end(), std::back_inserter(unhandled));

	PassScheduler scheduler(unhandled,ready,stack,new_schedule,pu_map);
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

void PassManager::runPasses(JITData &JD) {
	LOG("runPasses" << nl);
	print_PassDependencyGraph(*this);
	initializePasses();
	schedulePasses();
	for(ScheduleListTy::iterator i = schedule.begin(), e = schedule.end(); i != e; ++i) {
		PassInfo::IDTy id = *i;
		result_ready[id] = false;
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
	}
	finalizePasses();
}

void PassManager::finalizePasses() {
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
