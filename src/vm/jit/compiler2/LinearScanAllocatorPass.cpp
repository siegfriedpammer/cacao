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

#include <climits>
#include <queue>
#include <deque>

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/LinearScanAllocator"

namespace cacao {
namespace jit {
namespace compiler2 {

namespace {
struct StartComparator {
	bool operator()(const LivetimeInterval *lhs, const LivetimeInterval* rhs) {
		if(lhs->get_start() > rhs->get_start()) {
			return true;
		}
		return false;
	}
};

template <typename Key, typename T>
bool max_value_comparator(const std::pair<Key,T> &lhs, const std::pair<Key,T> &rhs) {
	return lhs.second < rhs.second;
}

} // end anonymous namespace
typedef std::priority_queue<LivetimeInterval*,std::deque<LivetimeInterval*>, StartComparator> UnhandledSetTy;
typedef std::list<LivetimeInterval*> HandledSetTy;

inline bool LinearScanAllocatorPass::try_allocate_free_reg(JITData &JD, LivetimeInterval *current) {
	Backend* backend = JD.get_Backend();
	RegisterFile* reg_file = backend->get_RegisterFile();
	assert(reg_file);

	std::map<MachineRegister*,unsigned> free_until_pos;
	LOG2("try_allocate_free_reg (current=" << current << ")" << nl);

	// set all registers to free
	for(RegisterFile::const_iterator i = reg_file->begin(), e = reg_file->end();
			i != e ; ++i) {
		MachineRegister *reg = *i;
		free_until_pos[reg] = UINT_MAX;
	}
	// set active registes to occupied
	for (ActiveSetTy::const_iterator i = active.begin(), e = active.end();
			i != e ; ++i) {
		MachineRegister *reg = (*i)->get_reg()->to_MachineRegister();
		// all active intervals must be assigned to machine registers
		if (DEBUG_COND && !reg) {
			ABORT_MSG("Interval " << current << " not in a machine reg " << (*i)->get_reg(), "Not yet supported!");
		}
		assert(reg);
		// reg not free!
		free_until_pos[reg] = 0;
	}
	// all intervals in inactive intersection with current
	for (InactiveSetTy::const_iterator i = inactive.begin(), e = inactive.end();
			i != e ; ++i) {
		LivetimeInterval *inact = *i;
		assert(inact);
		signed intersection = current->intersects(*inact);
		if (intersection != -1) {
			LOG2("interval " << current << " intersects with " << inact << " (inactive) at " << intersection << nl);
			MachineRegister *reg = inact->get_reg()->to_MachineRegister();
			assert(reg);
			// reg not free!
			free_until_pos[reg] = intersection;
		}

	}
	// get the register with the maximum free until number
	std::map<MachineRegister*,unsigned>::const_iterator i = std::max_element(free_until_pos.begin(),
		free_until_pos.end(), max_value_comparator<MachineRegister*, unsigned>);
	assert(i != free_until_pos.end());
	MachineRegister *reg = i->first;
	unsigned free_pos = i->second;
	assert(reg);

	LOG("Selected Register: " << reg << " (free until: " << free_pos << ")" << nl);
	if ( free_pos == 0) {
		// no register available without spilling
		return false;
	}
	if ( current->get_end() <= free_pos ) {
		// register available for the whole interval. jackpot!
		current->set_reg(reg);
		return true;
	}
	// register available for the first part of the interval
	current->set_reg(reg);
	// split current before free_pos
	ABORT_MSG("Interval Splitting not yet supported!","");

	return true;
}


bool LinearScanAllocatorPass::run(JITData &JD) {
	LivetimeAnalysisPass *LA = get_Pass<LivetimeAnalysisPass>();
	// local
	UnhandledSetTy unhandled;
	HandledSetTy handled;

	for (LivetimeAnalysisPass::iterator i = LA->begin(), e = LA->end();
			i != e ; ++i) {
		LivetimeInterval &inter = i->second;
		unhandled.push(&inter);
	}
	while(!unhandled.empty()) {
		LivetimeInterval *current = unhandled.top();
		unhandled.pop();
		unsigned pos = current->get_start();
		LOG(BoldGreen << "pos:" << setw(4) << pos << " current: " << current << reset_color << nl);

		// check for intervals in active that are handled or inactive
		LOG2("check for intervals in active that are handled or inactive" << nl);
		for (ActiveSetTy::iterator i = active.begin(), e = active.end();
				i != e ; /* ++i */) {
			LivetimeInterval *act = *i;
			//LOG2("active LTI " << act << nl);
			if (act->get_end() <= pos) {
				LOG2("LTI " << act << " moved from active to handled" << nl);
				// add to handled
				handled.push_back(act);
				// remove from active
				i = active.erase(i);
			} else if(act->is_inactive(pos)) {
				LOG2("LTI " << act << " moved from active to inactive" << nl);
				// add to inactive
				inactive.push_back(act);
				// remove from active
				i = active.erase(i);
			} else {
				// NOTE: erase returns the next element so we can not increment
				// in the loop header!
				++i;
			}
		}
		// check for intervals in inactive that are handled or active
		LOG2("check for intervals in inactive that are handled or active" << nl);
		for (InactiveSetTy::iterator i = inactive.begin(), e = inactive.end();
				i != e ; /* ++i */) {
			LivetimeInterval *inact = *i;
			//LOG2("active LTI " << act << nl);
			if (inact->get_end() <= pos) {
				LOG2("LTI " << inact << " moved from inactive to handled" << nl);
				// add to handled
				handled.push_back(inact);
				// remove from inactive
				i = inactive.erase(i);
			} else if(!inact->is_inactive(pos)) {
				LOG2("LTI " << inact << " moved from inactive to active" << nl);
				// add to active
				active.push_back(inact);
				// remove from inactive
				i = inactive.erase(i);
			} else {
				// NOTE: erase returns the next element so we can not increment
				// in the loop header!
				++i;
			}
		}

		if (!current->get_reg()->to_MachineRegister()) {
			// Try to find a register
			if (try_allocate_free_reg(JD, current)) {
				// if allocation successful add current to active
				active.push_back(current);
			} else {
				ABORT_MSG("no free register available!","spilling not yet supported!");
			}
		} else {
			// preallocated
			active.push_back(current);
			LOG2("Preallocated: " << current << nl);
		}
	}
	// move all active/inactive to handled
	handled.insert(handled.end(),inactive.begin(),inactive.end());
	handled.insert(handled.end(),active.begin(),active.end());
	for (HandledSetTy::const_iterator i = handled.begin(), e = handled.end();
			i != e ; ++i) {
		LivetimeInterval *lti = *i;
		Register *reg = lti->get_reg();
		LOG("Assigned Interval " << lti << " to " << reg << nl );
		assert(reg->to_MachineRegister());
	}

	return true;
}

// pass usage
PassUsage& LinearScanAllocatorPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires(LivetimeAnalysisPass::ID);
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
