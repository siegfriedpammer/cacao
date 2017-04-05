/* src/vm/jit/compiler2/SSAConstructionPass.hpp - SSAConstructionPass

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

#ifndef _JIT_COMPILER2_SSACONSTRUCTIONPASS
#define _JIT_COMPILER2_SSACONSTRUCTIONPASS

#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/jit.hpp"
#include "vm/jit/compiler2/alloc/unordered_map.hpp"

#include "vm/jit/compiler2/alloc/vector.hpp"

MM_MAKE_NAME(SSAConstructionPass)

namespace cacao {
namespace jit {
namespace compiler2 {

class InVarPhis;

/**
 * SSAConstructionPass
 *
 * This Pass constructs the compiler2 specific SSA based IR from
 * the ICMD_* style IR used in the baseline compiler.
 *
 * The approach is based on "Simple and Efficient Construction of
 * Static Singe Assignment Form" by Braun et al. 2013 @cite SSAsimple2013.
 */
class SSAConstructionPass : public Pass, public memory::ManagerMixin<SSAConstructionPass> {
private:
	Method *M;
	jitdata *jd;
	alloc::vector<BeginInst*>::type BB;
	alloc::unordered_map<BeginInst*,size_t>::type beginToIndex;
	alloc::vector<alloc::vector<Value*>::type >::type current_def;
	// TODO may be changed to std::vector<unordered_map<int,PHIInst*> > incomplete_phi;
	alloc::vector<alloc::vector<PHIInst*>::type >::type incomplete_phi;
	// TODO this should be merged with incomplete_phi
	alloc::vector<alloc::list<InVarPhis*>::type >::type incomplete_in_phi;

	alloc::vector<bool>::type sealed_blocks;
	alloc::vector<bool>::type filled_blocks;

	/// Used to remember which basicblocks have already been visited.
	///
	/// This vector is indexed by basicblock indices (i.e. by basicblock::nr).
	alloc::vector<bool>::type visited_blocks;

	/// Indicates whether IR construction should be skipped for a basicblock.
	///
	/// This vector is indexed by basicblock indices (i.e. by basicblock::nr).
	///
	/// @see SSAConstructionPass::deoptimize()
	alloc::vector<bool>::type skipped_blocks;

	alloc::vector<Type::TypeID>::type var_type_tbl;
	void write_variable(size_t varindex, size_t bb, Value *V);
	Value* read_variable_recursive(size_t varindex, size_t bb);
	Value* add_phi_operands(size_t varindex, PHIInst *phi);
	Value* try_remove_trivial_phi(PHIInst *phi);
	void seal_block(size_t bb);
	bool try_seal_block(basicblock *bb);
	void print_current_def() const;

	/// Check whether all predecessor blocks of @p bb have been skipped.
	///
	/// @param bb The block to check.
	///
	/// @return True if all predecessors have been skipped, false otherwise.
	bool skipped_all_predecessors(basicblock *bb);

	/// Remove unreachable BeginInsts and their corresponding EndInsts.
	void remove_unreachable_blocks();

	/// Helper function to terminate a block with a DeoptimizeInst.
	///
	/// Assigns a new DeoptimizeInst to the BeginInst that corresponds to the
	/// given @p bbindex. Additionally the corresponding skipped_blocks flag
	/// is set to true in order to signal that IR construction should be skipped
	/// for the rest of this block.
	///
	/// @param bbindex The corresponding basicblock index.
	void deoptimize(int bbindex);

	void install_javalocal_dependencies(SourceStateInst *source_state,
			s4 *javalocals, basicblock *bb);

	void install_stackvar_dependencies(SourceStateInst *source_state,
		s4 *stack, s4 stackdepth, s4 paramcount, basicblock *bb);

	SourceStateInst *record_source_state(Instruction *I, instruction *iptr,
		basicblock *bb, s4 *javalocals, s4 *stack, s4 stackdepth);

	/// Creates a CONSTInst of type @p type from the s2 operand of @p iptr.
	///
	/// @param iptr The corresponding baseline IR instruction.
	/// @param type The corresponding type.
	///
	/// @return The new CONSTInst.
	CONSTInst *parse_s2_constant(instruction *iptr, Type::TypeID type);

public:
	Value* read_variable(size_t varindex, size_t bb);
	static char ID;
	SSAConstructionPass() : Pass() {}
	virtual bool run(JITData &JD);
	virtual bool verify() const;
	virtual PassUsage& get_PassUsage(PassUsage &PU) const;
};

} // end namespace cacao
} // end namespace jit
} // end namespace compiler2

#endif /* _JIT_COMPILER2_SSACONSTRUCTIONPASS */


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
