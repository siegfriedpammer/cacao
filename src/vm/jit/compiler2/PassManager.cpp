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

PassManager::~PassManager() {
	// delete all passes
	for(PassList::iterator i = passes.begin(), e = passes.end(); i != e; ++i) {
		Pass* P = *i;
		delete P;
	}
}

void PassManager::initializePasses() {
	for(PassList::iterator i = passes.begin(), e = passes.end(); i != e; ++i) {
		Pass* P = *i;
		LOG("initialize: " << P->name() << nl);
		P->initialize();
	}
}

void PassManager::runPasses(JITData &JD) {
	initializePasses();
	for(PassList::iterator i = passes.begin(), e = passes.end(); i != e; ++i) {
		Pass* P = *i;
		LOG("start: " << P->name() << nl);
		if (!P->run(JD)) {
			err() << bold << Red << "error" << reset_color << " during pass " << P->name() << nl;
			vm_abort("compiler2: error");
		}
		LOG("finished: " << P->name() << nl);
	}
	finalizePasses();
}

void PassManager::finalizePasses() {
	for(PassList::iterator i = passes.begin(), e = passes.end(); i != e; ++i) {
		Pass* P = *i;
		LOG("finialize: " << P->name() << nl);
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
