/* src/vm/jit/compiler2/LinearScanAllocatorPass.hpp - LinearScanAllocatorPass

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

#ifndef _JIT_COMPILER2_LINEARSCANALLOCATORPASS
#define _JIT_COMPILER2_LINEARSCANALLOCATORPASS

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

namespace {
struct StartComparator {
	bool operator()(const LivetimeInterval *lhs, const LivetimeInterval* rhs) {
		if(lhs->get_start() > rhs->get_start()) {
			return true;
		}
		return false;
	}
};
} // end anonymous namespace

/**
 * Linear Scan Allocator
 *
 * It supports interval splitting, register hints and fixed intervals
 * and makes use of livetime holes.
 *
 * Based on the approach from "Optimized Interval Splitting in a Linear Scan
 * Register Allocator" by Wimmer and Moessenboeck @cite Wimmer2005 with
 * the adoptions from "Linear scan register allocation on SSA form"
 * by Wimmer and Franz @cite Wimmer2010.
 * See also Wimmer's Masters Thesis @cite WimmerMScThesis.
 */
class LinearScanAllocatorPass : public Pass {
private:
	typedef std::list<MachineInstruction*> MoveListTy;
	typedef std::map<std::pair<BeginInst*,BeginInst*>,MoveListTy> MoveMapTy;
	typedef std::priority_queue<LivetimeInterval*,std::deque<LivetimeInterval*>, StartComparator> UnhandledSetTy;
	typedef std::list<LivetimeInterval*> InactiveSetTy;
	typedef std::list<LivetimeInterval*> ActiveSetTy;
	typedef std::list<LivetimeInterval*> HandledSetTy;

	UnhandledSetTy unhandled;
	ActiveSetTy active;
	InactiveSetTy inactive;
	HandledSetTy handled;

	LivetimeAnalysisPass *LA;
	MachineInstructionSchedule *MIS;
	Backend *backend;
	JITData *jd;

	bool try_allocate_free_reg(LivetimeInterval* current);
	bool allocate_blocked_reg(LivetimeInterval* current);
	void split_blocking_ltis(LivetimeInterval* current);
	void split(LivetimeInterval *lti, unsigned pos);
public:
	static char ID;
	LinearScanAllocatorPass() : Pass() {}
	virtual bool run(JITData &JD);
	virtual PassUsage& get_PassUsage(PassUsage &PA) const;
	virtual bool verify() const;
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_LINEARSCANALLOCATORPASS */


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
