/* src/vm/jit/compiler2/CodeMemory.hpp - CodeMemory

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

#ifndef _JIT_COMPILER2_CODEMEMORY
#define _JIT_COMPILER2_CODEMEMORY

#include "vm/jit/compiler2/CodeSegment.hpp"
#include "vm/jit/compiler2/DataSegment.hpp"
#include "vm/types.hpp"

#include "vm/jit/compiler2/alloc/map.hpp"
#include "vm/jit/compiler2/alloc/vector.hpp"
#include <cassert>

namespace cacao {
namespace jit {
namespace compiler2 {

// forward declarations
class MachineInstruction;

/**
 * CodeMemory
 */
class CodeMemory {
public:
	typedef std::pair<const MachineInstruction*,CodeFragment> ResolvePointTy;
private:
	typedef alloc::list<ResolvePointTy>::type LinkListTy;

	LinkListTy linklist;            ///< instructions that require linking

	CodeSegment cseg;               ///< code segment
	DataSegment dseg;               ///< data segment

public:
	/// constructor
	CodeMemory() : cseg(this), dseg(this) {}

	/// get CodeSegment
	const CodeSegment& get_CodeSegment() const { return cseg; }
	/// get CodeSegment
	CodeSegment& get_CodeSegment()             { return cseg; }
	/// get DataSegment
	const DataSegment& get_DataSegment() const { return dseg; }
	/// get DataSegment
	DataSegment& get_DataSegment()             { return dseg; }

	s4 get_offset(CodeSegment::IdxTy to, CodeSegment::IdxTy from) const;
	s4 get_offset(CodeSegment::IdxTy to, CodeFragment &CF) const {
		return get_offset(to, CF.get_end());
	}
	s4 get_offset(CodeSegment::IdxTy to) const {
		return get_offset(to, cseg.get_following_index());
	}
	/**
	 * @deprecated this should be moved to FixedCodeMemory or so
	 */
	s4 get_offset(DataSegment::IdxTy to, CodeSegment::IdxTy from) const;
	s4 get_offset(DataSegment::IdxTy to, CodeFragment &CF) const {
		return get_offset(to, CF.get_end());
	}
	/// Add a MachineInstruction that require linking
	void require_linking(const MachineInstruction*, CodeFragment CF);

	/**
	 * Link instructions.
	 *
	 * @deprecated this should be moved to FixedCodeMemory or so
	 */
	void link();

	/**
	 * get a code fragment
	 */
	CodeFragment get_CodeFragment(std::size_t size);

	/**
	 * get an aligned code fragment
	 *
	 * @return A CodeFragment aligend to Target::alignment.
	 */
	CodeFragment get_aligned_CodeFragment(std::size_t size);

};


} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_CODEMEMORY */


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
