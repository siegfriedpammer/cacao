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

#include "vm/jit/compiler2/alloc/list.hpp"
#include "vm/jit/compiler2/alloc/queue.hpp"
#include "vm/jit/compiler2/alloc/deque.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

class MachineInstructionSchedule;
class MachineInstruction;
class Backend;

struct Move {
	MachineOperand *from;
	MachineOperand *to;
	Move* dep;
	bool scheduled;
	Move(MachineOperand *from, MachineOperand *to) : from(from), to(to), dep(NULL), scheduled(false) {}
	bool is_scheduled() const { return scheduled; }
};
struct Edge {
	MachineBasicBlock *predecessor;
	MachineBasicBlock *successor;
	Edge(MachineBasicBlock *predecessor, MachineBasicBlock *successor) :
		predecessor(predecessor), successor(successor) {}
};

inline bool operator<(const Edge &lhs, const Edge &rhs) {
	if (lhs.predecessor < rhs.predecessor) return true;
	if (lhs.predecessor > rhs.predecessor) return false;
	return lhs.successor < rhs.successor;
}

typedef alloc::list<Move>::type MoveMapTy;
typedef alloc::map<Edge,MoveMapTy>::type EdgeMoveMapTy;
typedef alloc::map<MachineBasicBlock*, alloc::list<LivetimeInterval>::type >::type BBtoLTI_Map;


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
public:
	struct StartComparator {
		bool operator()(const LivetimeInterval &lhs, const LivetimeInterval &rhs);
	};

	typedef alloc::list<MachineInstruction*>::type MoveListTy;
	//typedef alloc::map<std::pair<BeginInst*,BeginInst*>,MoveListTy>::type MoveMapTy;
	typedef alloc::priority_queue<LivetimeInterval,alloc::deque<LivetimeInterval>::type, StartComparator>::type UnhandledSetTy;
	typedef alloc::list<LivetimeInterval>::type InactiveSetTy;
	typedef alloc::list<LivetimeInterval>::type ActiveSetTy;
	typedef alloc::list<LivetimeInterval>::type HandledSetTy;
private:

	UnhandledSetTy unhandled;
	ActiveSetTy active;
	InactiveSetTy inactive;
	HandledSetTy handled;

	LivetimeAnalysisPass *LA;
	MachineInstructionSchedule *MIS;
	Backend *backend;
	JITData *jd;

	bool try_allocate_free(LivetimeInterval& current, UseDef pos);
	bool allocate_blocked(LivetimeInterval& current);
	bool allocate_unhandled();
	bool resolve();
	bool reg_alloc_resolve_block(MIIterator first, MIIterator last);
	bool order_and_insert_move(EdgeMoveMapTy::value_type &entry, BBtoLTI_Map &bb2lti_map);
	//bool order_and_insert_move(MachineBasicBlock *predecessor, MachineBasicBlock *successor,
	//		MoveMapTy &move_map);
public:
	static char ID;
	LinearScanAllocatorPass() : Pass() {}
	virtual void initialize();
	virtual bool run(JITData &JD);
	virtual PassUsage& get_PassUsage(PassUsage &PA) const;
	virtual bool verify() const;
};


/**
 * Second LSRA Pass. For the time being perform LSRA a second time to allocate resolution vars.
 */
class LinearScanAllocator2Pass : public LinearScanAllocatorPass {
public:
	static char ID;
	virtual PassUsage& get_PassUsage(PassUsage &PA) const;
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
