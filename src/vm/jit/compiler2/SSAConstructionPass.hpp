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
	std::map<BeginInst*,size_t> beginToIndex;
	std::vector<std::vector<Value*> > current_def;
	std::vector<bool> sealed_blocks;
	std::vector<Type::TypeID> var_type_tbl;
	inline void write_variable(size_t varindex, size_t bb, Value *V);
	inline Value* read_variable(size_t varindex, size_t bb);
	inline Value* read_variable_recursive(size_t varindex, size_t bb);
	inline Value* add_phi_operands(size_t varindex, PHIInst *phi);
	inline PHIInst* try_remove_trivial_phi(PHIInst *phi);
public:
	SSAConstructionPass(PassManager *PM) : Pass(PM) {}
	bool run(JITData &JD);
	const char* name() { return "SSAConstructionPass"; };
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
