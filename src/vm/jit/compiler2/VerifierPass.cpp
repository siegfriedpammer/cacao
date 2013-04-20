/* src/vm/jit/compiler2/VerifierPass.cpp - VerifierPass

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

#include "vm/jit/compiler2/VerifierPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/verify/typecheck.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

bool VerifierPass::run(JITData &JD) {
	if (JITDATA_HAS_FLAG_VERIFY(JD.jitdata())) {
		//DEBUG_JIT_COMPILEVERBOSE("Typechecking: ");

		/* call typecheck pass */
		if (!typecheck(JD.jitdata())) {
			//DEBUG_JIT_COMPILEVERBOSE("Exception while typechecking: ");

			return false;
		}

		//DEBUG_JIT_COMPILEVERBOSE("Typechecking done: ");
	}
	return true;
}


// the address of this variable is used to identify the pass
char VerifierPass::ID = 0;

// registrate Pass
static PassRegistery<VerifierPass> X("VerifierPass");

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
