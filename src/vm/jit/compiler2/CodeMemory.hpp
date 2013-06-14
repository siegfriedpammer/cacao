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

#ifndef _JIT_COMPILER2_CODEMEMERY
#define _JIT_COMPILER2_CODEMEMERY

#include "vm/types.hpp"

#include <map>
#include <cassert>

namespace cacao {
namespace jit {
namespace compiler2 {

// forward declarations
class BeginInst;
class CodeMemory;

/**
 * Code fregments
 *
 * Code fregments are used to write instructions to the CodeMemory. It abstracts
 * the fact that code memory is written upside down and achitecure dependent
 * properties such as endianess.
 *
 * It can be used just like an u1 array.
 */
class CodeFragment {
private:
	u1 *code_ptr;
	unsigned size;
	CodeFragment(u1 *code_ptr,unsigned size) : code_ptr(code_ptr), size(size) {}
public:
	// Copy Constructor
	CodeFragment(const CodeFragment &other) : code_ptr(other.code_ptr), size(other.size) {}
	// Copy assignment operator
	CodeFragment& operator=(const CodeFragment &other) {
		code_ptr = other.code_ptr;
		size = other.size;
		return *this;
	}
	u1& operator[](unsigned i) {
		assert(i < size);
		return *(code_ptr + i);
	}
	friend class CodeMemory;
};

/**
 * CodeMemory
 */
class CodeMemory {
private:
	u1             *mcodebase;      ///< base pointer of code area
	u1             *mcodeend;       ///< pointer to end of code area
	s4              mcodesize;      ///< complete size of code area (bytes)
	u1             *mcodeptr;       ///< code generation pointer

	std::map<BeginInst*,u1*> label_map; ///< label map
public:
	CodeMemory() ;
	void add_Label(BeginInst *BI);

	/**
	 * get a code fragment
	 */
	CodeFragment get_Fragment(unsigned size);

	/**
	 * get the start address of the code memory
	 */
	u1* get_start() const { return mcodeptr; }
	/**
	 * get the end address of the code memory
	 */
	u1* get_end() const { return mcodeend; }
};


} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_CODEMEMERY */


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
