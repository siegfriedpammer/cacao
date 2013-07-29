/* src/vm/jit/compiler2/CodeMemory.cpp - CodeMemory

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

#include "vm/jit/compiler2/CodeMemory.hpp"
#include "vm/jit/compiler2/MachineInstruction.hpp"
#include "mm/dumpmemory.hpp"

#include "toolbox/logging.hpp"
#include "Target.hpp"

#include <map>
#include <functional>

#define DEBUG_NAME "compiler2/CodeMemory"

#define MCODEINITSIZE (1<<15)       /* 32 Kbyte code area initialization size */

namespace cacao {
namespace jit {
namespace compiler2 {

s4 CodeMemory::get_offset(CodeSegment::IdxTy to, CodeSegment::IdxTy from) const {
	// Note that from/to is swapped because CodeSegment is written upside down!
	return s4(from.idx) - s4(to.idx);
}

s4 CodeMemory::get_offset(DataSegment::IdxTy to, CodeSegment::IdxTy from) const {
	// Note that from is swapped because CodeSegment is written upside down!
	return -(
		s4(dseg.size()) - s4(from.idx)
		+
		s4(cseg.size()) - 1 - s4(to.idx));
}

void CodeMemory::require_linking(const MachineInstruction* MI, CodeFragment CF) {
	LOG2("LinkMeLater: " << MI << nl);
	linklist.push_back(std::make_pair(MI,CF));
}

namespace {

/// @Cpp11 use std::function
struct LinkMeClass : std::unary_function<void, CodeMemory::ResolvePointTy&> {
    void operator()(CodeMemory::ResolvePointTy &link_me) {
		const MachineInstruction *MI = link_me.first;
		CodeFragment &CF = link_me.second;
		LOG2("LinkMe " << MI << nl);
		MI->link(CF);
	}
} LinkMe;

} // end anonymous namespace

void CodeMemory::link() {
	std::for_each(linklist.begin(),linklist.end(),LinkMe);
}

CodeFragment CodeMemory::get_CodeFragment(std::size_t size) {
	return cseg.get_Ref(size);
}

CodeFragment CodeMemory::get_aligned_CodeFragment(std::size_t size) {
	std::size_t nops = (cseg.size() + size) % Target::alignment;
	return cseg.get_Ref(size + nops);
}

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
