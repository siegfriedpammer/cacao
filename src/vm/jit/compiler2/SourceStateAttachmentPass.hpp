/* src/vm/jit/compiler2/SourceStateAttachmentPass.hpp - SourceStateAttachmentPass

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

#ifndef _JIT_COMPILER2_SOURCESTATEATTACHMENTPASS
#define _JIT_COMPILER2_SOURCESTATEATTACHMENTPASS

#include "vm/jit/compiler2/Pass.hpp"

MM_MAKE_NAME(SourceStateAttachmentPass)

namespace cacao {
namespace jit {
namespace compiler2 {

class Method;
class GlobalSchedule;
class SourceStateInst;
class BeginInst;
class DeoptInst;

/**
 * SourceStateAttachmentPass
 *
 * TODO more info
 */
class SourceStateAttachmentPass : public Pass, public memory::ManagerMixin<SourceStateAttachmentPass> {
private:
	Method *M;
	GlobalSchedule *schedule;

	SourceStateInst *find_nearest_dominating_source_state(BeginInst *block);
	void attach_source_state(DeoptInst *deopt);

public:
	static char ID;
	SourceStateAttachmentPass() : Pass() {}
	virtual bool run(JITData &JD);
	virtual PassUsage& get_PassUsage(PassUsage &PU) const;
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_SOURCESTATEATTACHMENTPASS */


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
