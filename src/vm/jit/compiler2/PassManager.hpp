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

#include <vector>
#include <map>
#include <set>

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/PassManger"

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
 */
class PassManager {
private:
	typedef std::set<PassInfo::IDTy> PassListTy;
	typedef std::vector<PassInfo::IDTy> ScheduleListTy;
	typedef std::map<PassInfo::IDTy,Pass*> PassMapTy;
	/**
	 * This stores the initialized passes.
	 * Every Pass can only occure once.
	 */
	PassMapTy initialized_passes;
	/**
	 * This variable contains a schedule of the passes.
	 * A pass may occure more than once.
	 */
	ScheduleListTy schedule;
	/**
	 * The list of passed that should be performed
	 */
	PassListTy passes;

	static std::map<PassInfo::IDTy, PassInfo*> &registered_passes() {
		static std::map<PassInfo::IDTy, PassInfo*> registered_passes;
		return registered_passes;
	}

	Pass* get_initialized_Pass(PassInfo::IDTy ID);

	const char * get_Pass_name(PassInfo::IDTy ID) {
		PassInfo *PI = registered_passes()[ID];
		assert(PI && "Pass not registered");
		return PI->get_name();
	}
	template<class _PassClass>
	_PassClass* get_Pass_result() {
		return (_PassClass*)initialized_passes[&_PassClass::ID];
	}
public:
	PassManager() {
		dbg() << "PassManager::PassManager()" << nl;
	}

	~PassManager();

	/**
	 * DO NOT CALL THIS MANUALLY. ONLY INVOKE VIA RegisterPass.
	 */
	static void register_Pass(PassInfo *PI) {
		dbg() << "PassManager::register_Pass: " << PI->get_name() << nl;
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
	void add_Pass(PassInfo::IDTy ID) {
		assert(registered_passes()[ID] && "Pass not registered");
		passes.insert(ID);
		schedule.push_back(ID);
	}

	friend class Pass;

};

template<class _PassClass>
Pass *call_ctor() { return new _PassClass(); }

template <class _PassClass>
struct PassRegistery : public PassInfo {
	PassRegistery(const char * name) : PassInfo(name, &_PassClass::ID, (PassInfo::ConstructorTy)call_ctor<_PassClass>) {
		PassManager::register_Pass(this);
	}
};

#undef DEBUG_NAME

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
