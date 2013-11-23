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


class PassInfo {
public:
	typedef void * IDTy;
	typedef Pass* (*ConstructorTy)();
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
 * Manage the execution of compiler passes
 *
 * @todo handle modified (timestamp?)
 * @todo conditionally reevaluate
 */
class PassManager {
public:
	typedef alloc::unordered_set<PassInfo::IDTy>::type PassListTy;
	typedef alloc::vector<PassInfo::IDTy>::type ScheduleListTy;
	typedef alloc::unordered_map<PassInfo::IDTy,Pass*>::type PassMapTy;
	typedef alloc::unordered_map<PassInfo::IDTy,bool>::type ResultReadyMapTy;
	typedef alloc::unordered_map<PassInfo::IDTy, PassInfo*>::type PassInfoMapTy;
private:
	/**
	 * This stores the initialized passes.
	 * Every Pass can only occur once.
	 */
	PassMapTy initialized_passes;
	/**
	 * This variable contains a schedule of the passes.
	 * A pass may occur more than once.
	 */
	ScheduleListTy schedule;
	/**
	 * The list of passed that should be performed
	 */
	PassListTy passes;
	/**
	 * Map of ready results
	 */
	ResultReadyMapTy result_ready;

	static PassInfoMapTy &registered_passes() {
		static PassInfoMapTy registered_passes;
		return registered_passes;
	}

	Pass* get_initialized_Pass(PassInfo::IDTy ID);

	template<class _PassClass>
	_PassClass* get_Pass_result() {
		assert_msg(result_ready[&_PassClass::ID], "result for "
		  << get_Pass_name(&_PassClass::ID) << " is not ready!");
		return (_PassClass*)initialized_passes[&_PassClass::ID];
	}
	void schedulePasses();
public:
	const char * get_Pass_name(PassInfo::IDTy ID) {
		PassInfo *PI = registered_passes()[ID];
		assert(PI && "Pass not registered");
		return PI->get_name();
	}
	PassManager() {
		MYLOG("PassManager::PassManager()" << nl);
	}

	~PassManager();

	/**
	 * DO NOT CALL THIS MANUALLY. ONLY INVOKE VIA RegisterPass.
	 */
	static void register_Pass(PassInfo *PI) {
		MYLOG("PassManager::register_Pass: " << PI->get_name() << nl);
		registered_passes().insert(std::make_pair(PI->ID,PI));
	}

	/**
	 * run pass initializers
	 */
	void initializePasses();

	/**
	 * run passes
	 */
	void runPasses(JITData &JD);

	/**
	 * run pass finalizers
	 */
	void finalizePasses();

	/**
	 * add a compiler pass
	 */
	template<class _PassClass>
	void add_Pass() {
		PassInfo::IDTy ID = &_PassClass::ID;
		assert(registered_passes()[ID] && "Pass not registered");
		passes.insert(ID);
		schedule.push_back(ID);
	}

	PassMapTy::const_iterator initialized_begin() const { return initialized_passes.begin(); }
	PassMapTy::const_iterator initialized_end() const { return initialized_passes.end(); }
	PassInfoMapTy::const_iterator registered_begin() const { return registered_passes().begin(); }
	PassInfoMapTy::const_iterator registered_end() const { return registered_passes().end(); }

	friend class Pass;

};

template<class _PassClass>
Pass *call_ctor() { return new _PassClass(); }

template <class _PassClass>
struct PassRegistry : public PassInfo {
	PassRegistry(const char * name) : PassInfo(name, &_PassClass::ID, (PassInfo::ConstructorTy)call_ctor<_PassClass>) {
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
