/* src/vm/jit/compiler2/PassManager.hpp - PassManager

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

#ifndef _JIT_COMPILER2_PASSMANAGER
#define _JIT_COMPILER2_PASSMANAGER

#include <memory>
#include <unordered_map>

#include "vm/jit/compiler2/alloc/vector.hpp"
#include "vm/jit/compiler2/alloc/unordered_set.hpp"
#include "vm/jit/compiler2/alloc/unordered_map.hpp"

#include "toolbox/logging.hpp"

// NOTE: the register stuff can not use LOG* macros because comman line
// arguments are not parsed at registration
#if 0
#define MYLOG(EXPR) do { dbg() << EXPR; } while(0)
#else
#define MYLOG(EXPR)
#endif

namespace cacao {
namespace jit {
namespace compiler2 {

class JITData;
class Pass;
class PassRunner;

using PassUPtrTy = std::unique_ptr<Pass>;

class PassInfo {
public:
	using IDTy = uint32_t;
	using ConstructorTy = Pass* (*)();
private:
	const char *const name;
	/// Constructor function pointer
	ConstructorTy ctor;
public:
	PassInfo::IDTy const ID;
	PassInfo(const char* name, PassInfo::IDTy ID,  ConstructorTy ctor) : name(name),  ctor(ctor), ID(ID) {}
	const char* get_name() const {
		return name;
	}
	Pass* create_Pass() const {
		return ctor();
	}
};


/**
 * Manages pass registry and scheduling.
 *
 * PassManager is implemented as a singleton. It holds a list of registered passes
 * and knows how to construct them.
 *
 * Passes are scheduled the first time
 * the schedule is accessed and the schedule does not change after that.
 *
 * Running passes and propagating results between passes
 * is handled by the PassRunner.
 */
class PassManager {
public:
	using ScheduleListTy = alloc::vector<PassInfo::IDTy>::type;
	using PassInfoMapTy = alloc::unordered_map<PassInfo::IDTy, PassInfo*>::type;
private:
	/**
	 * This is the pass schedule. A pass may occur more than once.
	 */
	ScheduleListTy schedule;

	bool passes_are_scheduled;

	static PassInfoMapTy &registered_passes() {
		static PassInfoMapTy registered_passes;
		return registered_passes;
	}

	void schedulePasses();
	PassUPtrTy create_Pass(PassInfo::IDTy ID) const;

	explicit PassManager() : passes_are_scheduled(false) {}

public:
	static PassManager& get() {
		// C++11 ensures that the initialization for local static variables is thread-safe
		static PassManager instance;
		return instance;
	}

	/**
	 * DO NOT CALL THIS MANUALLY. ONLY INVOKE VIA RegisterPass.
	 */
	static void register_Pass(PassInfo *PI) {
		MYLOG("PassManager::register_Pass: " << PI->get_name() << nl);
		registered_passes().insert(std::make_pair(PI->ID,PI));
	}

	const char * get_Pass_name(PassInfo::IDTy ID) {
		PassInfo *PI = registered_passes()[ID];
		assert(PI && "Pass not registered");
		return PI->get_name();
	}

	ScheduleListTy::const_iterator schedule_begin() {
		if (!passes_are_scheduled) {
			schedulePasses();
			passes_are_scheduled = true;
		}
		return schedule.begin(); 
	}
	ScheduleListTy::const_iterator schedule_end() const { return schedule.end(); }

	PassInfoMapTy::const_iterator registered_begin() const { return registered_passes().begin(); }
	PassInfoMapTy::const_iterator registered_end() const { return registered_passes().end(); }
	
	friend class PassRunner;
};


/**
 * Each instance of PassRunner represents a single run of the compiler2.
 *
 * The PassRunner owns all the pass instances for its corresponding run.
 */
class PassRunner {
public:
	using PassMapTy = std::unordered_map<PassInfo::IDTy,PassUPtrTy>;
	using ResultReadyMapTy = alloc::unordered_map<PassInfo::IDTy,bool>::type;
private:
	/**
	 * Stores pass instances so other passes can retrieve
	 * their results. This map owns all contained passes.
	 */
	PassMapTy passes;
	
	/**
	 * Map of ready results
	 */
	ResultReadyMapTy result_ready;

	PassUPtrTy& get_Pass(PassInfo::IDTy ID);

	template<class _PassClass>
	_PassClass* get_Pass_result() {
		auto pass_id = _PassClass::template ID<_PassClass>();

		assert_msg(result_ready[pass_id], "result for "
		  << PassManager::get().get_Pass_name(pass_id) << " is not ready!");
		return (_PassClass*) passes[pass_id].get();
	}

public:
	/**
	 * run passes
	 */
	void runPasses(JITData &JD);

	friend class Pass;
};

template<class _PassClass>
Pass *call_ctor() { return new _PassClass(); }

template <class _PassClass>
struct PassRegistry : public PassInfo {
	PassRegistry(const char * name) : PassInfo(name, _PassClass::template ID<_PassClass>(), (PassInfo::ConstructorTy)call_ctor<_PassClass>) {
		PassManager::register_Pass(this);
	}
};

#undef MYLOG

} // end namespace cacao
} // end namespace jit
} // end namespace compiler2

#endif /* _JIT_COMPILER2_PASSMANAGER */


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
