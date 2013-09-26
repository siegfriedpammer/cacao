/* src/vm/jit/compiler2/SpillAllAllocatorPass.hpp - SpillAllAllocatorPass

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

#ifndef _JIT_COMPILER2_SPILLALLALLOCATORPASS
#define _JIT_COMPILER2_SPILLALLALLOCATORPASS

#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/LivetimeAnalysisPass.hpp"

#include <list>
#include <queue>
#include <deque>

namespace cacao {
namespace jit {
namespace compiler2 {

class MachineInstructionSchedule;
class MachineInstruction;
class Backend;

struct MyStartComparator {
	bool operator()(const LivetimeInterval *lhs, const LivetimeInterval* rhs) {
		#if 0
		if(lhs->get_start() > rhs->get_start()) {
			return true;
		}
		#endif
		return false;
	}
};

/**
 * Spill All Allocator
 *
 * This allocator spills all variables onto the stack
 */
class SpillAllAllocatorPass : public Pass {
private:
	typedef std::list<MachineInstruction*> MoveListTy;
	typedef std::map<std::pair<BeginInst*,BeginInst*>,MoveListTy> MoveMapTy;
	typedef std::priority_queue<LivetimeInterval*,std::deque<LivetimeInterval*>, MyStartComparator> UnhandledSetTy;
	typedef std::list<LivetimeInterval*> InactiveSetTy;
	typedef std::list<LivetimeInterval*> ActiveSetTy;
	typedef std::list<LivetimeInterval*> HandledSetTy;

	UnhandledSetTy unhandled;
	HandledSetTy handled;

	#if 0
	LivetimeAnalysisPass *LA;
	MachineInstructionSchedule *MIS;
	Backend *backend;
	JITData *jd;
	#endif

public:
	static char ID;
	SpillAllAllocatorPass() : Pass() {}
	virtual bool run(JITData &JD);
	virtual PassUsage& get_PassUsage(PassUsage &PA) const;
	virtual bool verify() const;
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_SPILLALLALLOCATORPASS */


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
