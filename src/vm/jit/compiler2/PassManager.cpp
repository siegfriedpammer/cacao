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
#include "toolbox/Option.hpp"
#include "vm/vm.hpp"
#include "vm/jit/compiler2/alloc/set.hpp"
#include "vm/jit/compiler2/alloc/list.hpp"
#include "vm/jit/compiler2/alloc/deque.hpp"

#include "vm/jit/compiler2/ParserPass.hpp"
#include "vm/jit/compiler2/StackAnalysisPass.hpp"
#include "vm/jit/compiler2/VerifierPass.hpp"
#include "vm/jit/compiler2/CFGConstructionPass.hpp"
#include "vm/jit/compiler2/SSAConstructionPass.hpp"
#include "vm/jit/compiler2/CFGMetaPass.hpp"
#include "vm/jit/compiler2/LoopPass.hpp"
#include "vm/jit/compiler2/InstructionMetaPass.hpp"
#include "vm/jit/compiler2/ConstantPropagationPass.hpp"
#include "vm/jit/compiler2/GlobalValueNumberingPass.hpp"
#include "vm/jit/compiler2/DeadCodeEliminationPass.hpp"
#include "vm/jit/compiler2/DominatorPass.hpp"
#include "vm/jit/compiler2/ScheduleEarlyPass.hpp"
#include "vm/jit/compiler2/ScheduleLatePass.hpp"
#include "vm/jit/compiler2/ScheduleClickPass.hpp"
#include "vm/jit/compiler2/ListSchedulingPass.hpp"
#include "vm/jit/compiler2/NullCheckEliminationPass.hpp"
#include "vm/jit/compiler2/SourceStateAttachmentPass.hpp"
#include "vm/jit/compiler2/BasicBlockSchedulingPass.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/MachineInstructionPrinterPass.hpp"
#include "vm/jit/compiler2/MachineLoopPass.hpp"
#include "vm/jit/compiler2/ReversePostOrderPass.hpp"
#include "vm/jit/compiler2/PhiLiftingPass.hpp"
#include "vm/jit/compiler2/PhiCoalescingPass.hpp"
#include "vm/jit/compiler2/lsra/NewLivetimeAnalysisPass.hpp"
#include "vm/jit/compiler2/lsra/LoopPressurePass.hpp"
#include "vm/jit/compiler2/MachineDominatorPass.hpp"
#include "vm/jit/compiler2/lsra/NewSpillPass.hpp"
#include "vm/jit/compiler2/lsra/RegisterAssignmentPass.hpp"
#include "vm/jit/compiler2/RegisterAllocatorPass.hpp"
#include "vm/jit/compiler2/SSADeconstructionPass.hpp"
#include "vm/jit/compiler2/CodeGenPass.hpp"
#include "vm/jit/compiler2/DisassemblerPass.hpp"

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

namespace {
namespace option {
	Option<bool> print_pass_dependencies("PrintPassDependencies","compiler2: print pass dependencies",false,::cacao::option::xx_root());
}
} // end anonymous namespace


PassUPtrTy PassManager::create_Pass(PassInfo::IDTy ID) const {
	PassInfo *PI = registered_passes()[ID];
	assert(PI && "Pass not registered");

	// This should be the only place where a Pass is constructed!
	PassUPtrTy pass(PI->create_Pass());

	#if ENABLE_RT_TIMING
	auto iter = pass_timers.find(ID);
	if (iter == pass_timers.end()) {
		RTTimer &timer = pass_timers[ID];
		new (&timer) RTTimer(PI->get_name(),PI->get_name(),compiler2_rtgroup());
	}
	#endif

	return pass;
}

// Since passes do NOT guarantee that they can be reused cleanly
// we create new pass instances if they occur more than once in the schedule
PassUPtrTy& PassRunner::get_Pass(PassInfo::IDTy ID) {
	auto P = PassManager::get().create_Pass(ID);
	P->set_PassRunner(this);

	passes[ID] = std::move(P);
	result_ready[ID] = false;

	return passes[ID];
}


void PassRunner::runPasses(JITData &JD) {
	LOG("runPasses" << nl);

	auto& PS = PassManager::get();
	for (auto i = PS.schedule_begin(), e = PS.schedule_end(); i != e; ++i) {
		PassInfo::IDTy id = *i;
		result_ready[id] = false;
		auto&& P = get_Pass(id);
		
		#if ENABLE_RT_TIMING
		PassTimerMap::iterator f = pass_timers.find(id);
		assert(f != pass_timers.end());
		RTTimer &timer = f->second;
		timer.start();
		#endif
		
		LOG("initialize: " << PS.get_Pass_name(id) << nl);
		P->initialize();
		LOG("start: " << PS.get_Pass_name(id) << nl);
		if (!P->run(JD)) {
			err() << bold << Red << "error" << reset_color << " during pass " << PS.get_Pass_name(id) << nl;
			os::abort("compiler2: error");
		}
		// invalidating results
		PassUsage PU;
		PU = P->get_PassUsage(PU);
		for (PassUsage::const_iterator i = PU.destroys_begin(), e = PU.destroys_end();
				i != e; ++i) {
			result_ready[*i] = false;
			LOG("mark invalid" << PS.get_Pass_name(*i) << nl);
		}
		#ifndef NDEBUG
		LOG("verifying: " << PS.get_Pass_name(id) << nl);
		if (!P->verify()) {
			ERROR_MSG(bold << Red << "verification error" << reset_color, " during pass " << PS.get_Pass_name(id) << nl);
			// os::abort("compiler2: error");
			throw std::runtime_error("Verification error! (logs should have more info)");
		}
		#endif
		result_ready[id] = true;
		LOG("finialize: " << PS.get_Pass_name(id) << nl);
		P->finalize();
		#if ENABLE_RT_TIMING
		timer.stop();
		#endif
	}
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
struct ContainsFn<alloc::unordered_set<ValueType>,ValueType> {
	bool operator()(const typename alloc::unordered_set<ValueType>::type &c, const ValueType &val) {
		return c.find(val) != c.end();
	}
};

template <class Container, class ValueType>
inline bool contains(const Container &c, const ValueType &val) {
	return ContainsFn<Container,ValueType>()(c,val);
}

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

} // end anonymous namespace

void PassManager::schedulePasses() {
#if defined(ENABLE_LOGGING) || !defined(NDEBUG)
	// XXX for debugging only, to be removed
	latest = this;
#endif

	if (option::print_pass_dependencies) {
		print_PassDependencyGraph();
	}

	/// @todo Currently we hardcode the pass schedule since we can not model some
	///       dependencies. (E.g. pass does not change CFG, only changes instructions)
	///       This leads to unnecessary Pass re-runs.
	///       Either we hardcode the pass schedule here, which might get complicated if certain
	///       analysises *might* be re-run depending on some other passes.
	///       Or we extend the Pass scheduler with finer control (see llvm pass manager for what a full fledged one can do) 

	/// @todo Add most of the printer passes if they are enabled

	add<ParserPass>();
	add<StackAnalysisPass>();
	add<VerifierPass>();
	add<CFGConstructionPass>();
	add<SSAConstructionPass>();
	add<CFGMetaPass>();
	add<LoopPass>();
	add<InstructionMetaPass>();
	//add<ConstantPropagationPass>();
	//add<GlobalValueNumberingPass>();
	//add<DeadCodeEliminationPass>();
	add<DominatorPass>();
	add<ScheduleEarlyPass>();
	add<ScheduleLatePass>();
	add<ScheduleClickPass>();
	add<ListSchedulingPass>();
	add<NullCheckEliminationPass>();
	add<SourceStateAttachmentPass>();
	add<BasicBlockSchedulingPass>();
	add<MachineInstructionSchedulingPass>();

	if (MachineInstructionPrinterPass::enabled) {
		add<MachineInstructionPrinterPass>();
	}

	add<MachineLoopPass>();
	add<ReversePostOrderPass>();
	//add<PhiLiftingPass>();

	if (MachineInstructionPrinterPass::enabled) {
		add<MachineInstructionPrinterPass>();
	}

	add<NewLivetimeAnalysisPass>();
	//add<PhiCoalescingPass>();
	add<LoopPressurePass>();
	add<MachineDominatorPass>();
	add<NewSpillPass>();

	if (MachineInstructionPrinterPass::enabled) {
		add<MachineInstructionPrinterPass>();
	}

	add<NewLivetimeAnalysisPass>();
	add<RegisterAssignmentPass>();
	add<RegisterAllocatorPass>();
	add<SSADeconstructionPass>();

	if (MachineInstructionPrinterPass::enabled) {
		add<MachineInstructionPrinterPass>();
	}

	add<CodeGenPass>();

	if (DisassemblerPass::enabled) {
		add<DisassemblerPass>();
	}

	/*
	alloc::deque<PassInfo::IDTy>::type unhandled;
	alloc::unordered_set<PassInfo::IDTy>::type ready;
	alloc::list<PassInfo::IDTy>::type stack;
	ScheduleListTy new_schedule;

	for (const auto& id_to_info : registered_passes()) {
		PassInfo::IDTy id = id_to_info.first;
		auto pass = create_Pass(id);

		// Only schedule passes that want to be run
		if (!pass->is_enabled()) {
			continue;
		}

		if (pass->force_scheduling()) {
			schedule.push_back(id);
		}

		populate_passusage(pass, id);
		add_run_before(id);
		add_schedule_before(id);
	}

	populate_reverse_require_map();
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
		for (const auto& id : schedule) {
			LOG2("    " << get_Pass_name(id) << " id: " << id << nl);
		}
		LOG2("new Schedule:" << nl);
		for (const auto& id : new_schedule) {
			LOG2("    " << get_Pass_name(id) << " id: " << id << nl);
		}
	}
	schedule = new_schedule;
	*/
}

void PassManager::populate_passusage(const PassUPtrTy& pass, PassInfo::IDTy id) {
	PassUsage &PA = pu_map[id];
	pass->get_PassUsage(PA);
}

void PassManager::populate_reverse_require_map() {
	std::for_each(pu_map.begin(), pu_map.end(), [&](auto& id_to_passusage) {
		auto id = id_to_passusage.first;
		auto& pass_usage = id_to_passusage.second;

		std::for_each(pass_usage.requires_begin(), pass_usage.requires_end(),
					  [&](auto requires_id) {
			reverse_require_map[requires_id].insert(id);
		});
	});
}

void PassManager::add_run_before(PassInfo::IDTy id) {
	PassUsage& PA = pu_map[id];
	std::for_each(PA.run_before_begin(),PA.run_before_end(),[&](PassInfo::IDTy before) {
		pu_map[before].add_requires(id);
	});
}

void PassManager::add_schedule_before(PassInfo::IDTy id) {
	PassUsage& PA = pu_map[id];
	std::for_each(PA.schedule_before_begin(),PA.schedule_before_end(),[&](PassInfo::IDTy before) {
		pu_map[before].add_schedule_after(id);
	});
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
