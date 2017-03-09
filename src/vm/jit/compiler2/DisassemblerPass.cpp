/* src/vm/jit/compiler2/DisassemblerPass.cpp - DisassemblerPass

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

#include "vm/jit/compiler2/DisassemblerPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/CodeGenPass.hpp"

#include "vm/jit/jit.hpp"
#include "vm/jit/disass.hpp"
#include "vm/jit/code.hpp"


#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/DisassemblerPass"

namespace cacao {
namespace jit {
namespace compiler2 {

Option<bool> DisassemblerPass::enabled("DisassemblerPass","compiler2: enable DisassemblerPass",false,::cacao::option::xx_root());

bool DisassemblerPass::run(JITData &JD) {
#if defined(ENABLE_DISASSEMBLER)
	codeinfo *cd = JD.get_jitdata()->code;
	u1 *start = cd->entrypoint;
#if 0
	u1 *end = cd->mcode + cd->mcodelength;

	LOG2("DisassemblerPass: start: " << start << " end " << end << nl);

	disassemble(start, end);
#else
	CodeGenPass *CG = get_Pass<CodeGenPass>();
	for (CodeGenPass::BasicBlockMap::const_iterator i = CG->begin(), e = CG->end(); i != e ; ++i) {
		u1 *end = start + i->second;
		dbg() << BoldWhite << *i->first << " " << reset_color;
		print_ptr_container(dbg(),i->first->pred_begin(),i->first->pred_end()) << nl;
		//dbg() << hex << start << " - " << end << dec << " size: " << i->second<< nl;
		for (; start < end; )
			start = disassinstr(start);
		start = end;
	}
#endif

#endif
	return true;
}

PassUsage& DisassemblerPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires<CodeGenPass>();
	return PU;
}

// the address of this variable is used to identify the pass
char DisassemblerPass::ID = 0;

// register pass
static PassRegistry<DisassemblerPass> X("DisassemblerPass");

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
