/* src/vm/jit/compiler2/BasicBlockSchedulingPass.hpp - BasicBlockSchedulingPass

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

#ifndef _JIT_COMPILER2_BASICBLOCKSCHEDULINGPASS
#define _JIT_COMPILER2_BASICBLOCKSCHEDULINGPASS

#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/BasicBlockSchedule.hpp"

MM_MAKE_NAME(BasicBlockSchedulingPass)

namespace cacao {
namespace jit {
namespace compiler2 {

// forward declaration
class Method;

/**
 * BasicBlockSchedulingPass
 * TODO: more info
 */
class BasicBlockSchedulingPass : public Pass, public memory::ManagerMixin<BasicBlockSchedulingPass>, public BasicBlockSchedule {
private:
	Method *M;
public:
	BasicBlockSchedulingPass() : Pass() {}
	bool run(JITData &JD) override;
	PassUsage& get_PassUsage(PassUsage &PA) const override;
	void initialize() override {
		clear();
	}
	bool verify() const override;

	BasicBlockSchedule* provide_Artifact(ArtifactInfo::IDTy) override {
		return this;
	}
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_BASICBLOCKSCHEDULINGPASS */


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
