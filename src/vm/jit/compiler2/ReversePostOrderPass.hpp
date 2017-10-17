/* src/vm/jit/compiler2/ReversePostOrderPass.hpp - ReversePostOrderPass

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

#ifndef _JIT_COMPILER2_REVERSEPOSTORDERPASS
#define _JIT_COMPILER2_REVERSEPOSTORDERPASS

#include <list>

#include "vm/jit/compiler2/Pass.hpp"

MM_MAKE_NAME(ReversePostOrderPass)

namespace cacao {
namespace jit {
namespace compiler2 {

class MachineBasicBlock;

/**
 * Builds the reverse post-order of MachineBasicBlocks from a given MachineInstructionSchedule.
 * Since the reverse post-order requires the MachineLoopPass, which also requires the MIS pass,
 * the reverse post-order is calculated in its own pass.
 */
class ReversePostOrderPass : public Pass, public memory::ManagerMixin<ReversePostOrderPass> {
public:
	ReversePostOrderPass() : Pass() {}
	virtual bool run(JITData &JD);
	virtual PassUsage& get_PassUsage(PassUsage &PU) const;

	auto begin() { return reverse_post_order.begin(); }
	auto end()   { return reverse_post_order.end(); }

	auto rbegin() { return reverse_post_order.rbegin(); }
	auto rend() { return reverse_post_order.rend(); }

private:
	std::list<MachineBasicBlock*> reverse_post_order;

	void build_reverse_post_order(MachineBasicBlock*);
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_REVERSEPOSTORDERPASS */


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
