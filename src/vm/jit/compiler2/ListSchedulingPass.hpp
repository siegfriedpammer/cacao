/* src/vm/jit/compiler2/ListSchedulingPass.hpp - ListSchedulingPass

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

#ifndef _JIT_COMPILER2_LISTSCHEDULINGPASS
#define _JIT_COMPILER2_LISTSCHEDULINGPASS

#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/InstructionSchedule.hpp"

MM_MAKE_NAME(ListSchedulingPass)

namespace cacao {
namespace jit {
namespace compiler2 {

class GlobalSchedule;
class Method;

/**
 * ListSchedulingPass
 *
 * Constructs an instruction schedule for each basic block based on a
 * classical list scheduling algorithm.
 */
class ListSchedulingPass : public Pass,
						   public Artifact,
						   public memory::ManagerMixin<ListSchedulingPass>,
						   public InstructionSchedule<Instruction> {
private:

	/**
	 * The Method to process.
	 */
	Method *M;

	/**
	 * The current GlobalSchedule of the processed Method.
	 */
	GlobalSchedule *GS;

	/**
	 * Schedule the instructions within the block that starts at @p begin.
	 */
	void schedule(BeginInst *begin);

public:

	ListSchedulingPass() : Pass() {}
	bool run(JITData &JD) override;
	bool verify() const override;
	PassUsage& get_PassUsage(PassUsage &PU) const override;

	ListSchedulingPass* provide_Artifact(ArtifactInfo::IDTy) override {
		return this;
	}
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_LISTSCHEDULINGPASS */


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
