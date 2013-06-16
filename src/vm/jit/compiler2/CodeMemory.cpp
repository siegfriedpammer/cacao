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
#include "mm/dumpmemory.hpp"

#include <map>

#define MCODEINITSIZE (1<<15)       /* 32 Kbyte code area initialization size */

namespace cacao {
namespace jit {
namespace compiler2 {

//const s4 CodeMemory::INVALID_OFFSET = std::numeric_limits<s4>::max();

CodeMemory::CodeMemory() {
	mcodebase    = (u1*) DumpMemory::allocate(MCODEINITSIZE);
	mcodeend     = mcodebase + MCODEINITSIZE;
	mcodesize    = MCODEINITSIZE;
	//mcodeptr     = mcodebase;
	mcodeptr     = mcodeend;
}

void CodeMemory::add_label(const BeginInst *BI) {
	label_map.insert(std::make_pair(BI,mcodeptr));
}

s4 CodeMemory::get_offset(const BeginInst *BI) const {
	LabelMapTy::const_iterator i = label_map.find(BI);
	if (i == label_map.end() ) {
		return INVALID_OFFSET;
	}
	s4 offset = s4(i->second - mcodeptr);

	// this should never happen
	assert(offset == INVALID_OFFSET);

	return offset;
}
void CodeMemory::resolve_later(const BeginInst *BI, CodeFragment CM) {
	resolve_map.insert(std::make_pair(BI,CM));
}

CodeFragment CodeMemory::get_CodeFragment(unsigned size) {
	assert((mcodeptr - size) >= mcodebase);
	mcodeptr -= size;
	return CodeFragment(mcodeptr, size);
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
