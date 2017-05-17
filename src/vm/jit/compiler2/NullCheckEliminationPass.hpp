/* src/vm/jit/compiler2/NullCheckEliminationPass.hpp - NullCheckEliminationPass

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

#ifndef _JIT_COMPILER2_NULLCHECKELIMINATIONPASS
#define _JIT_COMPILER2_NULLCHECKELIMINATIONPASS

#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/alloc/unordered_map.hpp"

#include <boost/dynamic_bitset.hpp>

MM_MAKE_NAME(NullCheckEliminationPass)

namespace cacao {
namespace jit {
namespace compiler2 {

class Method;
class Value;
class Instruction;
class BeginInst;

/**
 * NullCheckEliminationPass
 */
class NullCheckEliminationPass : public Pass, public memory::ManagerMixin<NullCheckEliminationPass> {
private:

	Method *M;

	/**
	 * Maps each object reference to a unique bitvector position.
	 */
	alloc::unordered_map<Instruction*,int>::type bitpositions;

	/**
	 *
	 */
	alloc::unordered_map<BeginInst*, boost::dynamic_bitset<>>::type non_null_references_at_exit;

	/**
	 * Holds for each BeginInst the local analysis state in the form of a
	 * bitvector.
	 */
	alloc::unordered_map<BeginInst*, boost::dynamic_bitset<>>::type non_null_references_at_entry;

	bool is_trivially_non_null(Value *objectref);

	void map_referenes_to_bitpositions();

	void prepare_bitvectors();

	void perform_null_check_elimination();

#ifdef ENABLE_LOGGING
	void print_final_results();
#endif

public:

	NullCheckEliminationPass() : Pass() {}
	virtual bool run(JITData &JD);
	virtual PassUsage& get_PassUsage(PassUsage &PU) const;
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_NULLCHECKELIMINATIONPASS */


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
