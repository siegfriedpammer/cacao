/* src/vm/jit/compiler2/LivetimeInterval.cpp - LivetimeInterval

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

#include "vm/jit/compiler2/LivetimeInterval.hpp"
#include "vm/jit/compiler2/MachineInstruction.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/LivetimeInterval"

namespace cacao {
namespace jit {
namespace compiler2 {

void LivetimeInterval::add_range(UseDef first, UseDef last) {
	//LOG("first " << **first.get_iterator() << " last " << **last.get_iterator() << nl);
	assert(!(last < first));

	insert_usedef(first);
	insert_usedef(last);

	if (!intervals.empty()) {
		if (!(intervals.front().start < last) && !(last < intervals.front().start)) {
			// merge intervals
			intervals.front().start = first;
			return;
		}
		if (!(first < intervals.front().start) && !(intervals.front().end < last)) {
			// already covered
			return;
		}
		assert_msg(first < intervals.front().end,
			"For the time being ranges can only be added to the beginning.");
	}
	LivetimeRange range(first,last);
	// new interval
	intervals.push_front(range);
}

void LivetimeInterval::set_from(UseDef from) {
	assert(!intervals.empty());
	//assert_msg(!(intervals.front().start < from),
	//	**from.get_iterator() << " not lesser or equal then " << **intervals.front().start.get_iterator());

	insert_usedef(from);
	intervals.front().start = from;
}
LivetimeInterval::State LivetimeInterval::get_State(MIIterator pos) const {
	State state = Unhandled;

	if (!empty() && back().end.get_iterator() < pos) {
		return Handled;
	}
	for (const_iterator i = begin(), e = end(); i != e; ++i) {
		if (pos < i->start.get_iterator())
			break;
		if (pos <= i->end.get_iterator()) {
			return Active;
		}
		state = Inactive;
	}
	return state;
}
#if 0
void LivetimeInterval::set_Register(Register* r) {
	operand = r;
}

Register* LivetimeInterval::get_Register() const {
	Register *r = operand->to_Register();
	assert(r);
	return r;
}
void LivetimeInterval::set_ManagedStackSlot(ManagedStackSlot* s) {
	operand = s;
}

ManagedStackSlot* LivetimeInterval::get_ManagedStackSlot() const {
	ManagedStackSlot *s = operand->to_ManagedStackSlot();
	return s;
}

bool LivetimeInterval::is_in_Register() const {
	if(operand)
		return operand->to_Register() != NULL;
	return false;
}
bool LivetimeInterval::is_in_StackSlot() const {
	if(operand)
		return operand->to_ManagedStackSlot() != NULL;
	return false;
}
Type::TypeID LivetimeInterval::get_type() const {
	if (!operand) return Type::VoidTypeID;
	return operand->get_type();
}

LivetimeInterval* LivetimeInterval::split(unsigned pos, StackSlotManager *SSM) {
	//sanity checks:
	assert(pos >= get_start() && pos < get_end());
	// NOTE Note that the end of an interval shall be a use position
	// (even) (what about for phi input intervals input for phi?).
	// in special cases (eg fixed intervals) this is not the case
	// -> force it
	//pos -= (pos & 1);

	LivetimeInterval *stack_interval =  NULL;
	LivetimeInterval *lti = NULL;
	// copy intervals
	iterator i = intervals.begin();
	iterator e = intervals.end();
	for( ; i != e ; ++i) {
		if (i->first >= pos) {
			// livetime hole
			break;
		}
		if (i->second > pos) {
			signed next_usedef = next_usedef_after(pos);
			// currently active
			unsigned end = i->second;
			i->second = pos;
			//lti->add_range(pos,end);
			// XXX Wow I am so not sure if this is correct:
			// If the new range will start at pos it will be
			// selected for reg allocation in the next iteration
			// and we end up in splitting another interval which
			// in ture creates a new interval starting at pos...
			// But if we let it start at the next usedef things look
			// better. Because of the phi functions we dont run into
			// troubles with backedges and loop time intervals,
			// I guess...

			assert(next_usedef != -1);
			//if (next_usedef != end) {
				// ok it is not a loop pseudo use
				lti = new LivetimeInterval();
				lti->add_range(next_usedef,end);
			//}
			++i;
			// create stack interval
			ManagedStackSlot *slot = SSM->create_ManagedStackSlot(get_type());
			stack_interval = new LivetimeInterval();
			stack_interval->add_range(pos,next_usedef+1);
			stack_interval->set_ManagedStackSlot(slot);
			this->next_split = stack_interval;
			break;
		}
	}
	if (i == e && lti == NULL) {
		// already at the end
		return NULL;
	}
	if (lti == NULL) {
		lti = new LivetimeInterval();
	}
	if (stack_interval) {
		stack_interval->next_split = lti;
	} else {
		this->next_split = lti;
	}
	// move all remaining intervals to the new lti
	while (i != e) {
		lti->intervals.push_back(std::make_pair(i->first,i->second));
		i = intervals.erase(i);
	}
	// copy uses
	for (use_iterator i = uses.begin(), e = uses.end(); i != e ; ) {
		if (i->first >= pos) {
			lti->add_use(*i);
			i = uses.erase(i);
		} else {
			++i;
		}
	}
	// copy defs
	for (def_iterator i = defs.begin(), e = defs.end(); i != e ; ) {
		if (i->first >= pos) {
			lti->add_def(*i);
			i = defs.erase(i);
		} else {
			++i;
		}
	}
	// create new virtual register
	VirtualRegister *vreg = new VirtualRegister(get_type());
	lti->set_Register(vreg);
	// set hint to the current register
	assert(get_Register()->to_MachineRegister());
	lti->set_hint(get_Register());
	return lti;
}

#endif
OStream& operator<<(OStream &OS, const LivetimeInterval &lti) {
	return OS << "LivetimeInterval";
}
OStream& operator<<(OStream &OS, const LivetimeInterval *lti) {
	if (!lti) {
		return OS << "(LivetimeInterval) NULL";
	}
	return OS << *lti;
}

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
