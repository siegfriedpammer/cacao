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

#include "vm/jit/compiler2/PassUsage.hpp"

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
	using ScheduleListTy = std::vector<PassInfo::IDTy>;
	using PassInfoMapTy = alloc::unordered_map<PassInfo::IDTy, PassInfo*>::type;
	using ArtifactInfoMapTy = alloc::unordered_map<ArtifactInfo::IDTy, ArtifactInfo*>::type;
	using PassUsageMapTy = std::unordered_map<PassInfo::IDTy, PassUsage>;
private:

	PassInfoMapTy registered_passes;
	ArtifactInfoMapTy registered_artifacts;

	/**
	 * This is the pass schedule. A pass may occur more than once.
	 */
	ScheduleListTy schedule;

	bool passes_are_scheduled;

	PassUsageMapTy passusage_map;

	/**
	 * What artifact is provided by what pass?
	 * This map holds the answer
	 */
	alloc::unordered_map<ArtifactInfo::IDTy, PassInfo::IDTy>::type provided_by_map;

	void schedulePasses();
	PassUPtrTy create_Pass(PassInfo::IDTy ID) const;

	explicit PassManager() : passes_are_scheduled(false) {}

	// This template is used for creating the fixed pass schedule and
	// can be removed as soon as the pass scheduler is implemented in way that works better
	template <typename PassName>
	PassManager& add()
	{
		schedule.push_back(PassName::template ID<PassName>());
		return *this;
	}

public:
	static PassManager& get() {
		// C++11 ensures that the initialization for local static variables is thread-safe
		static PassManager instance;
		return instance;
	}

	/**
	 * DO NOT CALL THIS MANUALLY. ONLY INVOKE VIA PassRegistry.
	 */
	void register_Pass(PassInfo *PI) {
		MYLOG("PassManager::register_Pass: " << PI->get_name() << nl);
		registered_passes.insert(std::make_pair(PI->ID,PI));
	}

	/**
	 * DO NOT CALL THIS MANUALLY. ONLY INVOKE VIA ArtifactRegistry.
	 */
	void register_Artifact(ArtifactInfo *AI) {
		MYLOG("PassManager::register_Artifact: " << AI->get_name() << nl);
		registered_artifacts.insert(std::make_pair(AI->AID, AI));
	}

	const char * get_Pass_name(PassInfo::IDTy ID) {
		PassInfo *PI = registered_passes[ID];
		assert(PI && "Pass not registered");
		return PI->get_name();
	}

	const char * get_Artifact_name(ArtifactInfo::IDTy ID) {
		ArtifactInfo *AI = registered_artifacts[ID];
		assert(AI && "Artifact not registered");
		return AI->get_name();
	}

	ScheduleListTy::const_iterator schedule_begin() {
		if (!passes_are_scheduled) {
			schedulePasses();
			passes_are_scheduled = true;
		}
		return schedule.begin(); 
	}
	ScheduleListTy::const_iterator schedule_end() const { return schedule.end(); }

	PassInfoMapTy::const_iterator registered_begin() const { return registered_passes.begin(); }
	PassInfoMapTy::const_iterator registered_end() const { return registered_passes.end(); }

	ArtifactInfoMapTy::const_iterator registered_artifacts_begin() const { return registered_artifacts.begin(); }
	ArtifactInfoMapTy::const_iterator registered_artifacts_end() const { return registered_artifacts.end(); }
	
	friend class PassRunner;
	friend class PassScheduler;
};


/**
 * Each instance of PassRunner represents a single run of the compiler2.
 *
 * The PassRunner owns all the pass instances for its corresponding run.
 */
class PassRunner {
public:
	using PassMapTy = std::unordered_map<PassInfo::IDTy,PassUPtrTy>;
	using ArtifactReadyMapTy = alloc::unordered_map<ArtifactInfo::IDTy,bool>::type;
protected:
	/**
	 * Stores pass instances so other passes can retrieve
	 * their results. This map owns all contained passes.
	 */
	PassMapTy passes;
	
	/**
	 * Map of ready results
	 */
	ArtifactReadyMapTy result_ready;

	PassUPtrTy& get_Pass(PassInfo::IDTy ID);

	template<class ArtifactClass>
	ArtifactClass* get_Artifact() {
		auto artifact_id = ArtifactClass::template AID<ArtifactClass>();

		assert_msg(result_ready[artifact_id], "artifact "
			<< PassManager::get().get_Artifact_name(artifact_id) << " is not ready!");
		
		auto pass_id = PassManager::get().provided_by_map[artifact_id];
		return (ArtifactClass*) passes[pass_id]->provide_Artifact(artifact_id);
	}

public:
	/**
	 * run passes
	 */
	virtual void runPasses(JITData &JD);

	friend class Pass;
	friend class JsonGraphPrinter;
};

template<class _PassClass>
Pass *call_ctor() { return new _PassClass(); }

template <class _PassClass, 
          typename = typename std::enable_if<std::is_base_of<Pass, _PassClass>::value>::type>
struct PassRegistry : public PassInfo {
	PassRegistry(const char * name) : PassInfo(name, _PassClass::template ID<_PassClass>(), (PassInfo::ConstructorTy)call_ctor<_PassClass>) {
		PassManager::get().register_Pass(this);
	}
};

template<typename _ArtifactClass>
Artifact *call_actor() { return new _ArtifactClass(); }

template<typename _ArtifactClass,
         typename = typename std::enable_if<std::is_base_of<Artifact, _ArtifactClass>::value>::type>
struct ArtifactRegistry : public ArtifactInfo {
	ArtifactRegistry(const char * name) : ArtifactInfo(name, _ArtifactClass::template AID<_ArtifactClass>(), (ArtifactInfo::ConstructorTy)call_actor<_ArtifactClass>) {
		PassManager::get().register_Artifact(this);
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
