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

// forward declaration
class Pass;

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
	PIIDSet run_before;
	PIIDSet run_after;

	bool is_required(char &ID) const {
		return requires.find(&ID) != requires.end();
	}

public:
	PassUsage() {}

	/**
	 * PassName is required
	 */
	template<class PassName>
	void add_requires() {
		requires.insert(&PassName::ID);
	}
	void add_requires(PassInfo::IDTy id) {
		requires.insert(id);
	}

	/**
	 * PassName will be invalidated
	 */
	template<class PassName>
	void add_destroys() {
		destroys.insert(&PassName::ID);
	}
	void add_destroys(PassInfo::IDTy id) {
		destroys.insert(id);
	}

	/**
	 * PassName will be modified
	 */
	template<class PassName>
	void add_modifies() {
		modifies.insert(&PassName::ID);
	}
	void add_modifies(PassInfo::IDTy id) {
		modifies.insert(id);
	}

	/**
	 * Run before PassName
	 */
	template<class PassName>
	void add_run_before() {
		run_before.insert(&PassName::ID);
	}
	void add_run_before(PassInfo::IDTy id) {
		run_before.insert(id);
	}

	/**
	 * Run after PassName.
	 *
	 * Like add_requires but does not rerun PassName if not up to date. Also,
	 * it is not allowed the use get_Pass<PassName>().
	 */
	template<class PassName>
	void add_run_after() {
		run_after.insert(&PassName::ID);
	}
	void add_run_after(PassInfo::IDTy id) {
		run_after.insert(id);
	}

	const_iterator destroys_begin() const { return destroys.begin(); }
	const_iterator destroys_end()   const { return destroys.end(); }

	const_iterator modifies_begin() const { return modifies.begin(); }
	const_iterator modifies_end()   const { return modifies.end(); }

	const_iterator requires_begin() const { return requires.begin(); }
	const_iterator requires_end()   const { return requires.end(); }

	const_iterator run_before_begin() const { return run_before.begin(); }
	const_iterator run_before_end()   const { return run_before.end(); }

	const_iterator run_after_begin() const { return run_after.begin(); }
	const_iterator run_after_end()   const { return run_after.end(); }

	friend class Pass;
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
