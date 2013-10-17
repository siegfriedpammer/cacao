/* src/vm/jit/compiler2/LinearScanAllocatorPass.cpp - LinearScanAllocatorPass

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

#include "vm/jit/compiler2/LinearScanAllocatorPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/LivetimeAnalysisPass.hpp"
#include "vm/jit/compiler2/MachineRegister.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/MachineInstructions.hpp"
#include "vm/jit/compiler2/BasicBlockSchedulingPass.hpp"
#include "vm/statistics.hpp"

#include <climits>

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/LinearScanAllocator"

STAT_REGISTER_GROUP(lsra_stat,"LSRA","Linear Scan Register Allocator")
STAT_REGISTER_GROUP_VAR(unsigned,num_hints_followed,0,"hints followed",
	"Number of Hints followed (i.e. no move)",lsra_stat)

namespace cacao {
namespace jit {
namespace compiler2 {

bool LinearScanAllocatorPass::StartComparator::operator()(const LivetimeInterval &lhs,
		const LivetimeInterval &rhs) {
	return rhs.front().start < lhs.front().start;
}

namespace {
/**
 * @Cpp11 use std::function
 */
struct FreeUntilCompare: public std::binary_function<MachineOperand*,MachineOperand*,bool> {
	bool operator()(MachineOperand *lhs, MachineOperand *rhs) {
		bool r = lhs->aquivalence_less(*rhs);
		//LOG2("FreeUntilCompare: " << *lhs << " aquivalence_less " << *rhs << "=" << r << nl);
		return r;
	}
};

typedef std::map<MachineOperand*,UseDef,FreeUntilCompare> FreeUntilMap;

/**
 * @Cpp11 use std::function
 */
struct InitFreeUntilMap: public std::unary_function<MachineOperand*,void> {
	FreeUntilMap &free_until_pos;
	UseDef end;
	/// Constructor
	InitFreeUntilMap(FreeUntilMap &free_until_pos,
		UseDef end) : free_until_pos(free_until_pos), end(end) {}

	void operator()(MachineOperand *MO) {
		free_until_pos.insert(std::make_pair(MO,end));
	}
};


/**
 * @Cpp11 use std::function
 */
struct SetActive: public std::unary_function<LivetimeInterval&,void> {
	FreeUntilMap &free_until_pos;
	UseDef start;
	/// Constructor
	SetActive(FreeUntilMap &free_until_pos,
		UseDef start) : free_until_pos(free_until_pos), start(start) {}

	void operator()(LivetimeInterval& lti) {
		MachineOperand *MO = lti.get_operand();
		//LOG2("SetActive: " << lti << " operand: " << *MO << nl);
		FreeUntilMap::iterator i = free_until_pos.find(MO);
		if (i != free_until_pos.end()) {
			//LOG2("set to zero" << nl);
			LOG2("SetActive: " << lti << " operand: " << *MO << " to " << start << nl);
			i->second = start;
		}
	}
};


/**
 * @Cpp11 use std::function
 */
struct SetIntersection: public std::unary_function<LivetimeInterval&,void> {
	FreeUntilMap &free_until_pos;
	LivetimeInterval &current;
	UseDef pos;
	UseDef end;
	/// Constructor
	SetIntersection(FreeUntilMap &free_until_pos,
		LivetimeInterval &current, UseDef pos, UseDef end)
		: free_until_pos(free_until_pos), current(current), pos(pos), end(end) {}

	void operator()(LivetimeInterval& lti) {
		MachineOperand *MO = lti.get_operand();
		FreeUntilMap::iterator i = free_until_pos.find(MO);
		if (i != free_until_pos.end()) {
			UseDef inter = next_intersection(lti,current,pos,end);
			if (inter != end) {
				i->second = next_intersection(lti,current,pos,end);
				LOG2("SetIntersection: " << lti << " operand: " << *MO << " to " << i->second << nl);
			}
		}
	}
};


/**
 * @Cpp11 use std::function
 */
struct FreeUntilMaxCompare
		: public std::binary_function<const FreeUntilMap::value_type&, const FreeUntilMap::value_type&,bool> {
	bool operator()(const FreeUntilMap::value_type &lhs, const FreeUntilMap::value_type &rhs) {
		return lhs.second < rhs.second;
	}
};

/**
 * @Cpp11 use std::function
 */
struct SetUseDefOperand: public std::unary_function<const UseDef&,void> {
	MachineOperand *op;
	/// Constructor
	SetUseDefOperand(MachineOperand* op) : op(op) {}
	void operator()(const UseDef &usedef) {
		LOG2("  set def operand " << usedef << " to " << op << nl);
		usedef.get_operand()->op = op;
	}
};


MIIterator insert_move_before(MachineInstruction* move, UseDef usedef) {
	MIIterator free_until_pos_reg = usedef.get_iterator();
	while ((*free_until_pos_reg)->is_label()) {
		--free_until_pos_reg;
		assert((*free_until_pos_reg)->is_end());
		--free_until_pos_reg;
	}
	return insert_before(free_until_pos_reg, move);

}

} // end anonymous namespace

inline bool LinearScanAllocatorPass::try_allocate_free(LivetimeInterval &current, UseDef pos) {
	LOG2(Magenta << "try_allocate_free: " << current << reset_color << nl);
	// set free until pos of all physical operands to maximum
	FreeUntilMap free_until_pos;
	OperandFile op_file;
	backend->get_OperandFile(op_file,current.get_operand());
	// set to end
	std::for_each(op_file.begin(),op_file.end(),
		InitFreeUntilMap(free_until_pos, UseDef(UseDef::PseudoDef,MIS->mi_end())));

	// for each interval in active set free until pos to zero
	std::for_each(active.begin(), active.end(),
		SetActive(free_until_pos, UseDef(UseDef::PseudoUse,MIS->mi_begin())));

	// for each interval in inactive set free until pos to next intersection
	std::for_each(inactive.begin(), inactive.end(),
		SetIntersection(free_until_pos, current,pos,UseDef(UseDef::PseudoDef,MIS->mi_end())));

	LOG2("free_until_pos:" << nl);
	for (FreeUntilMap::iterator i = free_until_pos.begin(), e = free_until_pos.end(); i != e; ++i) {
		LOG2("  " << (i->first) << ": " << i->second << nl);
	}

	// XXX hints!
	// get operand with highest free until pos
	FreeUntilMap::value_type x = *std::max_element(free_until_pos.begin(), free_until_pos.end(), FreeUntilMaxCompare());
	LOG2("reg: " << x.first << " pos: " << x.second << nl);
	MachineOperand *reg = x.first;
	UseDef free_until_pos_reg = x.second;

	if (free_until_pos_reg == UseDef(UseDef::PseudoUse,MIS->mi_begin())) {
		// no register available without spilling
		return false;
	}
	if (current.back().end < free_until_pos_reg) {
		// register available for the whole interval
		current.set_operand(reg);
		LOG2("assigned operand: " << *reg << nl);
		return true;
	}
	// register available for the first part of the interval
	MachineOperand *tmp = new VirtualRegister(reg->get_type());
	MachineInstruction *move = backend->create_Move(reg,tmp);
	MIIterator split_pos = insert_move_before(move,free_until_pos_reg);
	assert_msg(pos.get_iterator() < split_pos, "pos: " << pos << " free_until_pos_reg "
		<< free_until_pos_reg << " split: " << split_pos);
	ABORT_MSG("Not yet implemented","splitting intervals not yet implemented");
	return true;
}

namespace {
/**
 * @Cpp11 use std::function
 */
struct SetNextUseActive: public std::unary_function<LivetimeInterval&,void> {
	FreeUntilMap &next_use_pos;
	UseDef pos;
	UseDef end;
	/// Constructor
	SetNextUseActive(FreeUntilMap &next_use_pos,
		UseDef pos, UseDef end) : next_use_pos(next_use_pos), pos(pos), end(end) {}

	void operator()(LivetimeInterval& lti) {
		MachineOperand *MO = lti.get_operand();
		//LOG2("SetActive: " << lti << " operand: " << *MO << nl);
		FreeUntilMap::iterator i = next_use_pos.find(MO);
		if (i != next_use_pos.end()) {
			//LOG2("set to zero" << nl);
			i->second = lti.next_usedef_after(pos,end);
			LOG2("SetNextUseActive: " << lti << " operand: " << *MO << " to " << i->second << nl);
		}
	}
};

/**
 * @Cpp11 use std::function
 */
struct SetNextUseInactive: public std::unary_function<LivetimeInterval&,void> {
	FreeUntilMap &free_until_pos;
	LivetimeInterval &current;
	UseDef pos;
	UseDef end;
	/// Constructor
	SetNextUseInactive(FreeUntilMap &free_until_pos,
		LivetimeInterval &current, UseDef pos, UseDef end)
		: free_until_pos(free_until_pos), current(current), pos(pos), end(end) {}

	void operator()(LivetimeInterval& lti) {
		MachineOperand *MO = lti.get_operand();
		FreeUntilMap::iterator i = free_until_pos.find(MO);
		if (i != free_until_pos.end() && next_intersection(lti,current,pos,end) != end) {
			i->second = lti.next_usedef_after(pos,end);
			LOG2("SetNextUseInactive: " << lti << " operand: " << *MO << " to " << i->second << nl);
		}
	}
};

inline MachineOperand* get_stackslot(Backend *backend, Type::TypeID type) {
	return backend->get_JITData()->get_StackSlotManager()->create_ManagedStackSlot(type);
}

void split_active_position(LivetimeInterval &lti, UseDef current_pos, UseDef next_use_pos, Backend *backend,
		LinearScanAllocatorPass::UnhandledSetTy &unhandled) {
	MachineOperand *MO = lti.get_operand();

	// move to stack
	MachineOperand *stackslot = get_stackslot(backend,MO->get_type());
	MachineInstruction *move_to_stack = backend->create_Move(MO,stackslot);
	MIIterator split_pos = insert_move_before(move_to_stack,current_pos);
	LivetimeInterval new_lti = lti.split_active(split_pos);
	// XXX should we add the stack interval?
	unhandled.push(new_lti);

	// move from stack
	if (!next_use_pos.is_pseudo()) {
		// if the next use position is no real use/def we can ignore it
		// the moves, if required, will be inserted by the resolution phase
		MachineOperand *vreg = new VirtualRegister(MO->get_type());
		MachineInstruction *move_from_stack = backend->create_Move(stackslot, vreg);
		MIIterator split_pos = insert_move_before(move_from_stack,next_use_pos);
		LivetimeInterval new_lti2 = new_lti.split_active(split_pos);
		unhandled.push(new_lti2);
	}
}

/**
 * @Cpp11 use std::function
 */
struct SplitActive: public std::unary_function<LivetimeInterval&,void> {
	MachineOperand *reg;
	UseDef pos;
	UseDef next_use_pos;
	Backend *backend;
	LinearScanAllocatorPass::UnhandledSetTy &unhandled;
	/// Constructor
	SplitActive(MachineOperand *reg , UseDef pos, UseDef next_use_pos, Backend *backend,
		LinearScanAllocatorPass::UnhandledSetTy &unhandled)
			: reg(reg), pos(pos), next_use_pos(next_use_pos), backend(backend), unhandled(unhandled) {}

	void operator()(LivetimeInterval& lti) {
		MachineOperand *MO = lti.get_operand();
		if (reg->aquivalent(*MO)) {
			split_active_position(lti,pos,next_use_pos,backend, unhandled);
		}
	}
};

/**
 * @Cpp11 use std::function
 */
struct SplitInactive: public std::unary_function<LivetimeInterval&,void> {
	MachineOperand *reg;
	UseDef pos;
	LinearScanAllocatorPass::UnhandledSetTy &unhandled;
	/// Constructor
	SplitInactive(MachineOperand *reg , UseDef pos,	LinearScanAllocatorPass::UnhandledSetTy &unhandled)
			: reg(reg), pos(pos), unhandled(unhandled) {}

	void operator()(LivetimeInterval& lti) {
		MachineOperand *MO = lti.get_operand();
		if (reg->aquivalent(*MO)) {
			//ABORT_MSG("Spill current not yet implemented","not yet implemented");
			LivetimeInterval new_lti = lti.split_inactive(pos, new VirtualRegister(reg->get_type()));
			assert(lti.front().start < new_lti.front().start);
			unhandled.push(new_lti);
		}
	}
};

inline void split_current(LivetimeInterval &current, Type::TypeID type, UseDef pos, UseDef end,
		LinearScanAllocatorPass::UnhandledSetTy &unhandled, Backend *backend) {
	LOG2("Spill current (" << current << ") at " << pos << nl);
	// create move
	MachineOperand *stackslot = get_stackslot(backend,type);
	// only create new register if there is a user
	if ( pos != end) {
		MachineOperand *vreg = new VirtualRegister(type);
		MachineInstruction *move_from_stack = backend->create_Move(stackslot,vreg);
		// split
		MIIterator split_pos = insert_move_before(move_from_stack,pos);
		LivetimeInterval new_lti = current.split_active(split_pos);
		unhandled.push(new_lti);
	}
	// set operand
	current.set_operand(stackslot);
}

} // end anonymous namespace

inline bool LinearScanAllocatorPass::allocate_blocked(LivetimeInterval &current) {
	LOG2(Blue << "allocate_blocked: " << current << reset_color << nl);
	// set free until pos of all physical operands to maximum
	FreeUntilMap next_use_pos;
	OperandFile op_file;
	backend->get_OperandFile(op_file,current.get_operand());
	// set to end
	std::for_each(op_file.begin(),op_file.end(),
		InitFreeUntilMap(next_use_pos, UseDef(UseDef::PseudoDef,MIS->mi_end())));

	// for each interval it in active set next use to next use after start of current
	std::for_each(active.begin(), active.end(),
		SetNextUseActive(next_use_pos, current.front().start, UseDef(UseDef::PseudoDef,MIS->mi_end())));

	// for each interval in inactive intersecting with current  set next pos to next use
	std::for_each(inactive.begin(), inactive.end(),
		SetNextUseInactive(next_use_pos, current, current.front().start, UseDef(UseDef::PseudoDef,MIS->mi_end())));

	LOG2("next_use_pos:" << nl);
	for (FreeUntilMap::iterator i = next_use_pos.begin(), e = next_use_pos.end(); i != e; ++i) {
		LOG2("  " << (i->first) << ": " << i->second << nl);
	}

	// get register with highest nextUsePos
	FreeUntilMap::value_type x = *std::max_element(next_use_pos.begin(), next_use_pos.end(), FreeUntilMaxCompare());
	MachineOperand *reg = x.first;
	UseDef next_use_pos_reg = x.second;
	LOG2("reg: " << *reg << " pos: " << next_use_pos_reg << nl);

	UseDef next_use_pos_current = current.next_usedef_after(current.front().start,
		UseDef(UseDef::PseudoDef,MIS->mi_end()));
	if (next_use_pos_reg < next_use_pos_current) {
		// all other intervals are used before current
		// so spill current
		split_current(current,current.get_operand()->get_type(), next_use_pos_current,
			UseDef(UseDef::PseudoDef,MIS->mi_end()), unhandled, backend);
		//ERROR_MSG("Spill current not yet implemented","not yet implemented");
		return true;
	}
	else {
		LOG2("Spill intervals blocking: " << *reg << nl);
		// spill intervals that currently block reg

		// split for each interval it in active that blocks reg (there can be only one)
		std::for_each(active.begin(), active.end(),
			SplitActive(reg, current.front().start, next_use_pos_reg, backend, unhandled));

		// spill each interval it in inactive for reg at the end of the livetime hole
		std::for_each(inactive.begin(), inactive.end(),
			SplitInactive(reg, current.front().start, unhandled));
		current.set_operand(reg);
		LOG2("assigned operand: " << *reg << nl);
		return true;
	}
	return false;
}

void LinearScanAllocatorPass::initialize() {
	while (!unhandled.empty()) {
		unhandled.pop();
	}
	handled.clear();
	inactive.clear();
	active.clear();
}

bool LinearScanAllocatorPass::run(JITData &JD) {
	LA = get_Pass<LivetimeAnalysisPass>();
	MIS = get_Pass<MachineInstructionSchedulingPass>();
	jd = &JD;
	backend = jd->get_Backend();

	for (LivetimeAnalysisPass::iterator i = LA->begin(), e = LA->end();
			i != e ; ++i) {
		LivetimeInterval &inter = i->second;
		unhandled.push(inter);
	}
	while(!unhandled.empty()) {
		LivetimeInterval current = unhandled.top();
		unhandled.pop();
		LOG(BoldCyan << "current: " << current << reset_color<< nl);
		UseDef pos = current.front().start;

		// check for intervals in active that are handled or inactive
		LOG2("check for intervals in active that are handled or inactive" << nl);
		for (ActiveSetTy::iterator i = active.begin(), e = active.end();
				i != e ; /* ++i */) {
			LivetimeInterval act = *i;
			switch (act.get_State(pos)) {
			case LivetimeInterval::Handled:
				LOG2("  LTI " << act << " moved from active to handled" << nl);
				handled.push_back(act);
				i = active.erase(i);
				break;
			case LivetimeInterval::Inactive:
				LOG2("  LTI " << act << " moved from active to inactive" << nl);
				inactive.push_back(act);
				i = active.erase(i);
				break;
			default:
				++i;
			}
		}
		// check for intervals in inactive that are handled or active
		LOG2("check for intervals in inactive that are handled or active" << nl);
		for (InactiveSetTy::iterator i = inactive.begin(), e = inactive.end();
				i != e ; /* ++i */) {
			LivetimeInterval inact = *i;
			switch (inact.get_State(pos)) {
			case LivetimeInterval::Handled:
				LOG2("  LTI " << inact << " moved from inactive to handled" << nl);
				handled.push_back(inact);
				i = inactive.erase(i);
				break;
			case LivetimeInterval::Active:
				LOG2("  LTI " << inact << " moved from inactive to active" << nl);
				active.push_back(inact);
				i = inactive.erase(i);
				break;
			default:
				++i;
			}
		}

		LOG2("active: ");
		DEBUG2(print_container(dbg(),active.begin(),active.end())<<nl);
		LOG2("inactive: ");
		DEBUG2(print_container(dbg(),inactive.begin(),inactive.end())<<nl);

		if (current.get_operand()->is_virtual()) {
			// Try to find a register
			if (!try_allocate_free(current,pos)) {
				if (!allocate_blocked(current)) {
					ERROR_MSG("Register allocation failed",
						"could not allocate register for " << current);
					return false;
				}
			}
		}
		else {
			// fixed interval
			for (ActiveSetTy::iterator i = active.begin(), e = active.end(); i != e ; ++i ) {
				LivetimeInterval act = *i;
				if (current.get_operand()->aquivalent(*act.get_operand())) {
					MachineInstruction *MI = *pos.get_iterator();
					if (!(MI->is_move() && MI->get_result().op->aquivalent(*MI->get(0).op))) {
						ERROR_MSG("Fixed Interval is blocked",
							"spilling not yet implemented " << current);
						return false;
					}
				}
			}
		}
		// add current to active
		active.push_back(current);
		LOG2("LTI " << current << " moved from unhandled to handled" << nl);
	}

	// move everything to handled
	for (ActiveSetTy::iterator i = active.begin(), e = active.end(); i != e ; /* ++i */) {
		handled.push_back(*i);
		i = active.erase(i);
	}
	for (InactiveSetTy::iterator i = inactive.begin(), e = inactive.end(); i != e ; /* ++i */) {
		handled.push_back(*i);
		i = inactive.erase(i);
	}

	// resolve
	resolve();

	// set operands
	for (HandledSetTy::const_iterator i = handled.begin(), e = handled.end(); i != e ; ++i) {
		LivetimeInterval lti = *i;
		LOG("Assigned Interval " << lti << " (init: " << *lti.get_init_operand() << ")" <<nl );
		std::for_each(lti.use_begin(), lti.use_end(), SetUseDefOperand(lti.get_operand()));
		std::for_each(lti.def_begin(), lti.def_end(), SetUseDefOperand(lti.get_operand()));
	}
	return true;
}

typedef std::map<MachineBasicBlock*, std::list<LivetimeInterval> > BBtoLTI_Map;

namespace {
/**
 * @Cpp11 use std::function
 */
template <class BinaryOperation>
class CfgEdgeFunctor : public std::unary_function<MachineBasicBlock*,void> {
private:
	BinaryOperation op;
public:
	/// Constructor
	CfgEdgeFunctor(BinaryOperation op) : op(op) {}
	void operator()(MachineBasicBlock *predecessor) {
		std::for_each(predecessor->back()->successor_begin(),
			predecessor->back()->successor_end(), std::bind1st(op,predecessor));
	}
};
/// Wrap class template argument.
template <class BinaryOperation>
CfgEdgeFunctor<BinaryOperation>	CfgEdge(BinaryOperation op) {
	return CfgEdgeFunctor<BinaryOperation>(op);
}

class CalcBBtoLTIMap : public std::unary_function<MachineBasicBlock*,void> {
private:
	LivetimeAnalysisPass* LA;
	BBtoLTI_Map &bb2lti_map;

	struct inner : public std::unary_function<LivetimeAnalysisPass::LivetimeIntervalMapTy::value_type,void> {
		MachineBasicBlock *MBB;
		BBtoLTI_Map &bb2lti_map;
		/// constructor
		inner(MachineBasicBlock *MBB, BBtoLTI_Map &bb2lti_map) : MBB(MBB), bb2lti_map(bb2lti_map) {}
		/// function call operator
		void operator()(argument_type lti_pair) {
			LivetimeInterval &lti = lti_pair.second;
			if (lti.get_State(MBB->mi_first()) == LivetimeInterval::Active) {
				bb2lti_map[MBB].push_back(lti);
			}
			else if (lti.has_next()) {
				operator()(argument_type(lti_pair.first,lti.get_next()));
			}
		}
	};
public:
	/// constructor
	CalcBBtoLTIMap(LivetimeAnalysisPass* LA, BBtoLTI_Map &bb2lti_map)
		: LA(LA), bb2lti_map(bb2lti_map) {}
	/// function call operator
	void operator()(MachineBasicBlock *MBB) {
		std::for_each(LA->begin(),LA->end(),inner(MBB,bb2lti_map));
	}
};
struct Move {
	MachineOperand *from;
	MachineOperand *to;
	Move* dep;
	bool scheduled;
	Move(MachineOperand *from, MachineOperand *to) : from(from), to(to), dep(NULL), scheduled(false) {}
	bool is_scheduled() const { return scheduled; }
};

#if 0
bool operator<(const Move &lhs, const Move &rhs) {
	if (lhs.from < rhs.from)
		return true;
	if (rhs.from < lhs.from)
		return false;
	return lhs.to < rhs.to;
}
#endif

typedef std::list<Move> MoveMapTy;

class ForEachLiveiterval : public std::unary_function<LivetimeInterval&,void> {
private:
	MachineBasicBlock *predecessor;
	MachineBasicBlock *successor;
	LivetimeAnalysisPass *LA;
	MoveMapTy &move_map;
public:
	/// constructor
	ForEachLiveiterval(MachineBasicBlock *predecessor, MachineBasicBlock *successor,
		LivetimeAnalysisPass *LA, MoveMapTy &move_map) :
		predecessor(predecessor), successor(successor), LA(LA), move_map(move_map) {}
	/// function call operator
	void operator()(LivetimeInterval& lti) const {
		LOG2("live: " << lti << " op: " << *lti.get_init_operand() << nl);

		MachineOperand *move_from;
		if (lti.front().start.get_iterator() == successor->mi_first()) {
			MachinePhiInst *phi = get_phi_from_operand(successor, lti.get_init_operand());
			assert(phi);
			LOG2("starts at successor begin: " << *phi << nl);
			std::size_t index = successor->get_predecessor_index(predecessor);
			MachineOperand *op = phi->get(index).op;
			if (op->is_Immediate()) {
				ABORT_MSG("PHI with immediate operand", "Can not yet handle phis with immediate operands");
				move_from = NULL;
			} else {
				move_from = LA->get(op).get_operand(predecessor->mi_last());
			}

		}
		else {
			move_from = lti.get_operand(predecessor->mi_last());
		}
		MachineOperand *move_to = lti.get_operand();
		if (!move_from->aquivalent(*move_to)) {
			move_map.push_back(Move(move_from,move_to));
		}
	}
};

struct MachineOperandCmp : public std::binary_function<MachineOperand*,MachineOperand*,bool> {
	bool operator()(MachineOperand *lhs, MachineOperand *rhs) const {
		return lhs->aquivalence_less(*rhs);
	}
};


inline OStream& operator<<(OStream &OS, const Move &move) {
	return OS << "move from " << *move.from << " to " << *move.to;
}

void schedule(std::list<MachineInstruction*> &scheduled, MoveMapTy &move_map,
		std::list<Move*> &queue, Backend *backend, std::list<Move*> &stack) {
	Move* node = stack.back();
	assert(!node->from->aquivalent(*node->to));
	LOG2("schedule pre: " << *node << nl);
	// already scheduled?
	if (node->is_scheduled()) {
		stack.pop_back();
		return;
	}
	if (node->dep && !node->dep->is_scheduled()) {
		if (std::find(stack.begin(),stack.end(),node->dep) == stack.end()) {
			LOG2("cycle detected!" << nl);
			MachineOperand *tmp = new VirtualRegister(node->to->get_type());
			move_map.push_back(Move(tmp, node->to));
			queue.push_back(&move_map.back());
			node->to = tmp;
			node->dep = NULL;
			ABORT_MSG("Cycle Detected!","No re-allocation strategy yet!");
		} else {
			stack.push_back(node->dep);
			schedule(scheduled, move_map, queue, backend, stack);
		}
	}
	LOG2("schedule post: " << *node << nl);
	stack.pop_back();
	scheduled.push_back(backend->create_Move(node->from,node->to));
	node->scheduled = true;
}

template <class OutputIterator>
struct RefToPointerInserterImpl: public std::unary_function<Move&,void> {
	OutputIterator i;
	/// constructor
	RefToPointerInserterImpl(OutputIterator i) : i(i) {}
	/// call operator
	void operator()(Move& move) {
		*i = &move;
	}
};
/// wrapper
template <class OutputIterator>
inline RefToPointerInserterImpl<OutputIterator> RefToPointerInserter(OutputIterator i) {
	return RefToPointerInserterImpl<OutputIterator>(i);
}

bool compare_moves(Move *lhs, Move *rhs) {
	if (!lhs->dep)
		return true;
	if (!rhs->dep)
		return false;
	return lhs < rhs;
}

void order_and_insert_move(MachineBasicBlock *predecessor, MachineBasicBlock *successor,
		Backend *backend, MoveMapTy &move_map) {

	if (move_map.empty()) return;
	// calculate data dependency graph (DDG)
	std::map<MachineOperand*,Move*,MachineOperandCmp> read_from;

	// create graph nodes
	for (MoveMapTy::iterator i = move_map.begin(), e = move_map.end(); i != e; ++i) {
		LOG2("need move from " << i->from << " to " << i->to << nl);
		read_from[i->from] = &*i;
	}
	// create graph edges
	for (MoveMapTy::iterator i = move_map.begin(), e = move_map.end(); i != e; ++i) {
		i->dep = read_from[i->to];
	}
	// nodes already scheduled
	std::list<MachineInstruction*> scheduled;
	std::list<Move*> stack;
	std::list<Move*> queue;
	std::for_each(move_map.begin(), move_map.end(), RefToPointerInserter(std::back_inserter(queue)));
	assert(move_map.size() == queue.size());

	while (!queue.empty()) {
		// sort and pop element
		queue.sort(compare_moves);
		stack.push_back(queue.front());
		queue.pop_front();
		// schedule
		schedule(scheduled, move_map, queue, backend, stack);
		assert(stack.empty());
	}

	std::copy(scheduled.begin(), scheduled.end(), get_edge_inserter(predecessor, successor));
}

class ForEachCfgEdge : public std::binary_function<MachineBasicBlock*,MachineBasicBlock*,void> {
private:
	BBtoLTI_Map &bb2lti_map;
	LivetimeAnalysisPass *LA;
	Backend *backend;

public:
	/// constructor
	ForEachCfgEdge(BBtoLTI_Map &bb2lti_map, LivetimeAnalysisPass *LA, Backend *backend)
		: bb2lti_map(bb2lti_map), LA(LA), backend(backend) {}
	/// function call operator
	void operator()(MachineBasicBlock *predecessor, MachineBasicBlock *successor) const {
		MoveMapTy move_map;
		LOG2("edge " << *predecessor << " -> " << *successor << nl);
		BBtoLTI_Map::mapped_type &lti_live_set = bb2lti_map[successor];
		// for each live interval live at successor
		std::for_each(lti_live_set.begin(), lti_live_set.end(),
			ForEachLiveiterval(predecessor,successor, LA, move_map));
		// order and insert move
		order_and_insert_move(predecessor, successor, backend, move_map);
	}
};

} // end anonymous namespace

void LinearScanAllocatorPass::resolve() {
	LOG(BoldGreen << "resolve: " << nl);
	// calculate basicblock to live interval map
	BBtoLTI_Map bb2lti_map;
	std::for_each(MIS->begin(), MIS->end(), CalcBBtoLTIMap(LA,bb2lti_map));

	if (DEBUG_COND_N(2))
	for (BBtoLTI_Map::const_iterator i = bb2lti_map.begin(), e = bb2lti_map.end(); i != e ; ++i) {
		LOG2("live at: " << *i->first << nl);
		for (BBtoLTI_Map::mapped_type::const_iterator ii = i->second.begin(), ee = i->second.end(); ii != ee ; ++ii) {
			LOG2("  " << *ii << nl);
		}
	}

	// for each cfg edge from predecessor to successor (implemented in ForEachCfgEdge)
	std::for_each(MIS->begin(), MIS->end(), CfgEdge(ForEachCfgEdge(bb2lti_map,LA,backend)));
	// remove all phis
	std::for_each(MIS->begin(), MIS->end(), std::mem_fun(&MachineBasicBlock::phi_clear));
}

bool LinearScanAllocatorPass::verify() const {
#if 0
	for (MachineInstructionSchedule::const_iterator i = MIS->begin(),
			e = MIS->end(); i != e; ++i) {
		MachineInstruction *MI = *i;
		MachineOperand *op = MI->get_result().op;
		if (!op->is_StackSlot() &&
				op->is_Register() && !op->to_Register()->to_MachineRegister() ) {
			LOG("Not allocatd: " << MI << nl);
			return false;
		}

	}
#endif
	return true;
}


// pass usage
PassUsage& LinearScanAllocatorPass::get_PassUsage(PassUsage &PU) const {
	// requires
	PU.add_requires(LivetimeAnalysisPass::ID);
	PU.add_requires(MachineInstructionSchedulingPass::ID);
	// modified
	PU.add_modifies(MachineInstructionSchedulingPass::ID);
	// destroys
	// not yet updated correctly (might be only changed)
	PU.add_destroys(LivetimeAnalysisPass::ID);
	return PU;
}

// the address of this variable is used to identify the pass
char LinearScanAllocatorPass::ID = 0;

// register pass
static PassRegistery<LinearScanAllocatorPass> X("LinearScanAllocatorPass");


} // end namespace compiler2
} // end namespace jit
} // end namespace cacao


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
