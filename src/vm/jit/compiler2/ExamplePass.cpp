/* src/vm/jit/compiler2/ExamplePass.cpp - ExamplePass

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

#include "vm/jit/compiler2/ExamplePass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/Method.hpp"
#include "vm/jit/compiler2/InstructionMetaPass.hpp"
#include "toolbox/logging.hpp"

// define name for debugging (see logging.hpp)
#define DEBUG_NAME "compiler2/ExamplePass"

namespace cacao {
namespace jit {
namespace compiler2 {

bool ExamplePass::run(JITData &JD) {
	#if defined(ENABLE_LOGGING)
	Method *M = JD.get_Method();
	// print all instructions (in an arbitrary sequence)
	for (Method::const_iterator i = M->begin(), e = M->end(); i != e; ++i) {
		Instruction *I = *i;
		LOG(*I << nl);
	}
	#endif
	return true;
}

// pass usage
PassUsage& ExamplePass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires<InstructionMetaPass>();
	return PU;
}

// the address of this variable is used to identify the pass
char ExamplePass::ID = 0;

// register pass
static PassRegistry<ExamplePass> X("ExamplePass");

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao


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
