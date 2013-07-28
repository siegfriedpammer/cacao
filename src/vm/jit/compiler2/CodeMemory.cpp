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

#if 0
s4 CodeFragment::get_offset(const BeginInst *BI) const {
	return parent->get_offset(BI,code_ptr + size);
}
s4 CodeFragment::get_offset(std::size_t index) const {
	return parent->get_offset(index,code_ptr + size);
}
#endif
//const s4 CodeMemory::INVALID_OFFSET = std::numeric_limits<s4>::max();

CodeMemory::CodeMemory() :
	cseg(this),
	dseg(this){
}

void CodeMemory::add_label(const BeginInst *BI) {
	LOG2("CodeMemory::add_label " << *BI <<nl);
	cseg.insert_tag(CSLabel(BI));
}
s4 CodeMemory::get_offset(CodeSegment::IdxTy to, CodeSegment::IdxTy from) const {
	// Note that from/to is swapped because CodeSegment is written upside down!
	return s4(from.idx) - s4(to.idx);
}
s4 CodeMemory::get_offset(DataSegment::IdxTy to, CodeSegment::IdxTy from) const {
	// Note that from is swapped because CodeSegment is written upside down!
	return s4(from.idx) - s4(cseg.get_next_index().idx) - s4(to.idx);
}
#if 0
s4 CodeMemory::get_offset(const BeginInst *BI, u1 *current_pos) const {
	LabelMapTy::const_iterator i = label_map.find(BI);
	if (i == label_map.end() ) {
		return INVALID_OFFSET;
	}
	#ifndef NDEBUG
	if (!cseg.contains_tag(CSLabel(BI))) {
		cseg.print(dbg());
		assert(cseg.contains_tag(CSLabel(BI)));
	}
	#endif
	// FIXME this is so not safe!
	s4 offset = s4(i->second - current_pos);

	// this should never happen
	assert(offset != INVALID_OFFSET);

	return offset;
}

s4 CodeMemory::get_offset(std::size_t index, u1 *current_pos) const {
	// FIXME this is so not safe!
	//s4 offset = s4(get_start() - current_pos - index);
	u1* start = get_start();
	std::ptrdiff_t offset = start - current_pos - index;
	LOG2("start:   " << hex << setz(16) << start << dec << nl);
	LOG2("current: " << hex << setz(16) << current_pos << dec << nl);
	LOG2("offset:  " << setw(16) << offset << nl);

	return offset;
}
#endif
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
#if 0
void CodeMemory::resolve_later(const BeginInst *BI,
		const MachineInstruction* MI, CodeFragment CM) {
	resolve_map.insert(std::make_pair(BI,std::make_pair(MI,CM)));
}

void CodeMemory::resolve() {
	LOG2("CodeMemory::resolve" << nl);
	for (ResolveLaterMapTy::iterator i = resolve_map.begin(),
			e = resolve_map.end(); i != e; ++i) {
		const BeginInst *BI = i->first;
		const MachineInstruction *MI = i->second.first;
		LOG2("resolve " << MI << " jump to " << BI << nl);
		CodeFragment &CF = i->second.second;
		MI->emit(CF);
	}
}

void CodeMemory::resolve_data() {
	LOG2("CodeMemory::resolve_data" << nl);
	for (ResolveDataMapTy::iterator i = resolve_data_map.begin(),
			e = resolve_data_map.end(); i != e; ++i) {
		std::size_t index = i->first;
		const MachineInstruction *MI = i->second.first;
		CodeFragment &CF = i->second.second;
		LOG2("resolve " << MI << " data to " << index << nl);
		MI->emit(CF);
	}
}
#endif

CodeFragment CodeMemory::get_CodeFragment(std::size_t size) {
	return cseg.get_Ref(size);
}

CodeFragment CodeMemory::get_aligned_CodeFragment(std::size_t size) {
	std::size_t nops = Target::alignment
		- (std::size_t(cseg.size() - size) % Target::alignment);
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
