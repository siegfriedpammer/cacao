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
#include "vm/jit/compiler2/Pass.hpp"
#include "toolbox/logging.hpp"
#include "vm/vm.hpp"

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
	for(ScheduleListTy::iterator i = schedule.begin(), e = schedule.end(); i != e; ++i) {
		Pass* P = get_initialized_Pass(*i);
		LOG("initialize: " << get_Pass_name(*i) << nl);
		P->initialize();
	}
}

void PassManager::runPasses(JITData &JD) {
	initializePasses();
	for(ScheduleListTy::iterator i = schedule.begin(), e = schedule.end(); i != e; ++i) {
		Pass* P = get_initialized_Pass(*i);
		LOG("start: " << get_Pass_name(*i) << nl);
		if (!P->run(JD)) {
			err() << bold << Red << "error" << reset_color << " during pass " << get_Pass_name(*i) << nl;
			os::abort("compiler2: error");
		}
		#ifndef NDEBUG
		LOG("verifying: " << get_Pass_name(*i) << nl);
		if (!P->verify()) {
			err() << bold << Red << "verification error" << reset_color << " during pass " << get_Pass_name(*i) << nl;
			os::abort("compiler2: error");
		}
		#endif
		LOG("finished: " << get_Pass_name(*i) << nl);
	}
	finalizePasses();
}

void PassManager::finalizePasses() {
	for(ScheduleListTy::iterator i = schedule.begin(), e = schedule.end(); i != e; ++i) {
		Pass* P = get_initialized_Pass(*i);
		LOG("finialize: " << get_Pass_name(*i) << nl);
		P->finalize();
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
