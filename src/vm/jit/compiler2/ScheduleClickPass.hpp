/* src/vm/jit/compiler2/ScheduleClickPass.hpp - ScheduleClickPass

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

#ifndef _JIT_COMPILER2_SCHEDULECLICKPASS
#define _JIT_COMPILER2_SCHEDULECLICKPASS

#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/GlobalSchedule.hpp"

MM_MAKE_NAME(ScheduleClickPass)

namespace cacao {
namespace jit {
namespace compiler2 {


class Method;
class Instruction;

/**
 * ScheduleClickPass
 *
 * Based on the algorithm in Click's Phd Thesis, Chapter 6 @cite ClickPHD.
 */
class ScheduleClickPass : public Pass, public memory::ManagerMixin<ScheduleClickPass>, public GlobalSchedule {
private:
	GlobalSchedule *late;
	Method *M;
public:
	ScheduleClickPass() : Pass() {}
	bool run(JITData &JD) override;
	PassUsage& get_PassUsage(PassUsage &PU) const override;
	bool verify() const override;

	ScheduleClickPass* provide_Artifact(ArtifactInfo::IDTy) override {
		return this;
	}
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_SCHEDULECLICKPASS */


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
