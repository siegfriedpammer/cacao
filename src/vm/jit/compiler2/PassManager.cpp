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
#include "vm/jit/compiler2/PassScheduler.hpp"
#include "vm/jit/compiler2/JsonGraphPrinter.hpp"
#include "toolbox/logging.hpp"
#include "toolbox/Option.hpp"
#include "vm/vm.hpp"
#include "vm/jit/compiler2/alloc/set.hpp"
#include "vm/jit/compiler2/alloc/list.hpp"
#include "vm/jit/compiler2/alloc/deque.hpp"
#include "vm/jit/compiler2/treescan/LogHelper.hpp"


#include "vm/rt-timing.hpp"

#include <algorithm>

#define DEBUG_NAME "compiler2/PassManager"

namespace cacao {
namespace jit {
namespace compiler2 {

#if ENABLE_RT_TIMING
namespace {
RT_REGISTER_GROUP(compiler2_rtgroup,"compiler2","compiler2 pass pipeline")

typedef alloc::unordered_map<PassInfo::IDTy,RTTimer>::type PassTimerMap;
PassTimerMap pass_timers;

} // end anonymous namespace
#endif

namespace {
namespace option {
	Option<bool> print_pass_dependencies("PrintPassDependencies","compiler2: print pass dependencies",false,::cacao::option::xx_root());
	Option<bool> dump_method_json("DumpMethodJson", "compiler2: dumps all the information for our debug tool in JSON format",false,::cacao::option::xx_root());

}
} // end anonymous namespace


PassUPtrTy PassManager::create_Pass(PassInfo::IDTy ID) const {
	PassInfo *PI = registered_passes.find(ID)->second;
	assert(PI && "Pass not registered");

	// This should be the only place where a Pass is constructed!
	PassUPtrTy pass(PI->create_Pass());

	#if ENABLE_RT_TIMING
	auto iter = pass_timers.find(ID);
	if (iter == pass_timers.end()) {
		RTTimer &timer = pass_timers[ID];
		new (&timer) RTTimer("compiler2",PI->get_name(),compiler2_rtgroup());
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

	return passes[ID];
}

void PassRunner::runPasses(JITData &JD) {
	auto& PS = PassManager::get();
	PS.schedule_begin(); // ensure the schedule is created
	auto last_pass = *(std::prev(PS.schedule_end()));	
	runPassesUntil(JD, last_pass);
}

void PassRunner::runPassesUntil(JITData &JD, PassInfo::IDTy last_pass) {
	auto& PS = PassManager::get();	
	LOG("runPasses until " << PS.get_Pass_name(last_pass) << nl);

	JsonGraphPrinter graphPrinter(this);
	if (option::dump_method_json) {
		graphPrinter.initialize(JD);
	}
	
	for (auto i = PS.schedule_begin(), e = PS.schedule_end(); i != e; ++i) {
		PassInfo::IDTy id = *i;
		auto& pa = PS.passusage_map[id];
		std::for_each(pa.provides_begin(), pa.provides_end(), [&](auto artifact_id) {
			result_ready[artifact_id] = false;
		});
		
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

		#ifndef NDEBUG
		LOG("verifying: " << PS.get_Pass_name(id) << nl);
		if (!P->verify()) {
			LOG(bold << Red << "verification error" << reset_color << " during pass " << PS.get_Pass_name(id) << nl);
			// os::abort("compiler2: error");
			throw std::runtime_error("Verification error! (logs should have more info)");
		}
		#endif
		std::for_each(pa.provides_begin(), pa.provides_end(), [&](auto artifact_id) {
			result_ready[artifact_id] = true;
		});
		LOG("finialize: " << PS.get_Pass_name(id) << nl);
		P->finalize();
		
		#if ENABLE_RT_TIMING
		timer.stop();
		if (option::dump_method_json) {
			auto ts = timer.time();
			graphPrinter.printPass(JD, P.get(), PS.get_Pass_name(id), ts.count());
		}
		#else
		if (option::dump_method_json) {
			graphPrinter.printPass(JD, P.get(), PS.get_Pass_name(id), 0);
		}
		#endif

		if(id == last_pass){
			LOG("Last pass " << PS.get_Pass_name(last_pass) << " reached." << nl);
			break;
		}
	}

	if (option::dump_method_json) {
		graphPrinter.close();
	}
	
	// assert(false && "Only one method gets compiled!");
}

void PassManager::schedulePasses() {
	if (option::print_pass_dependencies) {
		print_PassDependencyGraph();
	}

	PassIDSetTy enabled_passes;

	for (const auto& id_to_info : registered_passes) {
		PassInfo::IDTy id = id_to_info.first;
		auto pass = create_Pass(id);

		// Only schedule passes that want to be run
		if (!pass->is_enabled()) {
			continue;
		}

		enabled_passes.insert(id);
		
		auto& pa = passusage_map[id];
		pass->get_PassUsage(pa);
	}

	// Prepare provided_by_map, since we have all the passusages now
	for (const auto& id_to_usage : passusage_map) {
		auto id = id_to_usage.first;
		auto& pa = id_to_usage.second;

		std::for_each(pa.provides_begin(), pa.provides_end(), [&](auto artifact_id) {
			provided_by_map[artifact_id] = id;
		});
	}

	schedule = GetPassSchedule(enabled_passes, passusage_map);
	
	if (DEBUG_COND_N(2)) {
		LOG2("Registered passes: \n");
		for (const auto& p : registered_passes) {
			LOG2("    " << p.first << ": " << p.second->get_name() << nl);
		}
		LOG2("Regsitered artifacts: \n");
		for (const auto& p : registered_artifacts) {
			LOG2("    " << p.first << ": " << p.second->get_name() << nl);
		}
		LOG2("Schedule:" << nl);
		for (const auto& id : schedule) {
			LOG2("    " << get_Pass_name(id) << " id: " << id << nl);
		}
	}
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
