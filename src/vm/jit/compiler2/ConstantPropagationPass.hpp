/* src/vm/jit/compiler2/ConstantPropagationPass.hpp - ConstantPropagationPass

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

#ifndef _JIT_COMPILER2_CONSTANTPROPAGATIONPASS
#define _JIT_COMPILER2_CONSTANTPROPAGATIONPASS

#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/alloc/unordered_map.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "toolbox/Option.hpp"

#include <list>

namespace cacao {
namespace jit {
namespace compiler2 {

// forward declaration
class Instruction;

/**
 * ConstantPropagationPass
 *
 * This optimization pass is a combined version of constant folding (a.k.a
 * constant expression evaluation) and constant propagation, based on the
 * algorithm in @cite ReisingerBScThesis. Analysis and program transformations
 * in the course of this optimization are targeted at the high-level
 * intermediate representation of the compiler2. It evalutes operations at
 * compile-time, whose operands are all constants and propagates the results
 * of these evaluations as far as possible through the program.
 */
class ConstantPropagationPass : public Pass {
private:
	typedef alloc::unordered_map<Instruction*,bool>::type InstBoolMapTy;
	typedef alloc::unordered_map<Instruction*,std::size_t>::type InstIntMapTy;
	typedef std::list<Instruction*> WorkListTy;

	/**
	 * This work list is used by the algorithm to store the instructions which
	 * have to be reconsidered. at the beginning it therefore contains all
	 * instructions.
	 */
	WorkListTy workList;
	
	/**
	 * will be used to look up whether an instruction is currently contained in the
	 * worklist to avoid inserting an instruction which is already in the list
	 */
	InstBoolMapTy inWorkList;
	
	/**
	 * used to track for each instruction the number of its operands which are
	 * already known to be constant
	 */
	InstIntMapTy constantOperands;

	void replace_by_constant(Instruction *isnt, CONSTInst *c, Method *M);
	void propagate(Instruction *inst);

public:
	static Option<bool> enabled;
	ConstantPropagationPass() : Pass() {}
	virtual bool run(JITData &JD);
	virtual PassUsage& get_PassUsage(PassUsage &PA) const;

	virtual bool is_enabled() const {
		return ConstantPropagationPass::enabled;
	}
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_CONSTANTPROPAGATIONPASS */


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
