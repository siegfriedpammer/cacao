/* src/vm/jit/compiler2/PassUsage.hpp - PassUsage

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

#ifndef _JIT_COMPILER2_PASSUSAGE
#define _JIT_COMPILER2_PASSUSAGE

#include "vm/jit/compiler2/PassManager.hpp"

#include <set>

namespace cacao {
namespace jit {
namespace compiler2 {


/**
 * PassUsage
 * This is a Container to track dependencies between passes
 */
class PassUsage {
public:
	typedef std::set<PassInfo::IDTy> PIIDSet;
	typedef PIIDSet::const_iterator const_iterator;
private:
	PIIDSet requires;
	PIIDSet destroys;
	PIIDSet modifies;
public:
	PassUsage() {}
	void add_requires(char &ID) {
		requires.insert(&ID);
	}

	void add_destroys(char &ID) {
		destroys.insert(&ID);
	}

	void add_modifies(char &ID) {
		modifies.insert(&ID);
	}

	bool is_required(char &ID) const {
		return requires.find(&ID) != requires.end();
	}

	const_iterator destroys_begin() const { return destroys.begin(); }
	const_iterator destroys_end()   const { return destroys.end(); }

	const_iterator modifies_begin() const { return destroys.begin(); }
	const_iterator modifies_end()   const { return destroys.end(); }

	const_iterator requires_begin() const { return destroys.begin(); }
	const_iterator requires_end()   const { return destroys.end(); }
};


} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_PASSUSAGE */


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
