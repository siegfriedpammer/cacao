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
#include "future/unordered_map.hpp"

#include <vector>

namespace cacao {
namespace jit {
namespace compiler2 {

/**
 * SSAConstructionPass
 *
 * This Pass constructs the compiler2 specific SSA based IR from
 * the ICMD_* style IR used in the baseline compiler.
 *
 * The approach is based on "Simple and Efficient Construction of
 * Static Singe Assignment Form" by Braun et al. 2013 @cite SSAsimple2013.
 */
class SSAConstructionPass : public Pass {
private:
	Method *M;
	std::vector<BeginInst*> BB;
	unordered_map<BeginInst*,size_t> beginToIndex;
	std::vector<std::vector<Value*> > current_def;
	// TODO may be changed to std::vector<unordered_map<int,PHIInst*> > incomplete_phi;
	std::vector<std::vector<PHIInst*> > incomplete_phi;
	std::vector<bool> sealed_blocks;
	//
	std::vector<bool> filled_blocks;
	std::vector<Type::TypeID> var_type_tbl;
	void write_variable(size_t varindex, size_t bb, Value *V);
	Value* read_variable(size_t varindex, size_t bb);
	Value* read_variable_recursive(size_t varindex, size_t bb);
	Value* add_phi_operands(size_t varindex, PHIInst *phi);
	Value* try_remove_trivial_phi(PHIInst *phi);
	void seal_block(size_t bb);
	bool try_seal_block(basicblock *bb);
	void print_current_def() const;
public:
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
