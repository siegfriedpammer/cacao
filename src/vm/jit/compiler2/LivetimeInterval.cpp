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

void LivetimeIntervalImpl::add_range(UseDef first, UseDef last) {
	//LOG("first " << **first.get_iterator() << " last " << **last.get_iterator() << nl);
	assert(!(last < first));

	insert_usedef(first);
	insert_usedef(last);

	LivetimeRange range(first,last);
	for (IntervalListTy::iterator i = intervals.begin(), e = intervals.end();
			i != e; ) {
		LivetimeRange &current = *i;
		if (range.end < current.start) {
			break;
		}
		if (current.end < range.start) {
			++i;
			continue;
		}
		if (current.start <= range.start) {
			range.start = current.start;
		}
		if (range.end < current.end) {
			range.end = current.end;
		}
		i = intervals.erase(i);
	}
	// new interval
	intervals.push_front(range);
}

void LivetimeIntervalImpl::set_from(UseDef from, UseDef to) {
	if(intervals.empty()) {
		add_range(from, to);
	} else {
		insert_usedef(from);
		intervals.front().start = from;
	}
}

LivetimeIntervalImpl::State LivetimeIntervalImpl::get_State(UseDef pos) const {
	State state = LivetimeInterval::Unhandled;

	if (!empty() && back().end < pos) {
		return LivetimeInterval::Handled;
	}
	for (const_iterator i = begin(), e = end(); i != e; ++i) {
		if (pos < i->start)
			break;
		if (pos <= i->end) {
			return LivetimeInterval::Active;
		}
		state = LivetimeInterval::Inactive;
	}
	return state;
}

LivetimeIntervalImpl::State LivetimeIntervalImpl::get_State(MIIterator pos) const {
	State state = LivetimeInterval::Unhandled;

	if (!empty() && back().end.get_iterator() < pos) {
		return LivetimeInterval::Handled;
	}
	for (const_iterator i = begin(), e = end(); i != e; ++i) {
		if (pos < i->start.get_iterator())
			break;
		if (pos <= i->end.get_iterator()) {
			return LivetimeInterval::Active;
		}
		state = LivetimeInterval::Inactive;
	}
	return state;
}

bool LivetimeIntervalImpl::is_use_at(MIIterator pos) const {
	return uses.find(UseDef(UseDef::Pseudo,pos)) != uses.end();
}

bool LivetimeIntervalImpl::is_def_at(MIIterator pos) const {
	return defs.find(UseDef(UseDef::Pseudo,pos)) != defs.end();
}


UseDef next_intersection(const LivetimeInterval &a,
		const LivetimeInterval &b, UseDef pos, UseDef end) {
	for(LivetimeInterval::const_iterator a_i = a.begin(), b_i = b.begin(),
			a_e = a.end(), b_e = b.end() ; a_i != a_e && b_i != b_e ; ) {

		if (a_i->end < b_i->start) {
			++a_i;
			continue;
		}
		if (b_i->end < a_i->start) {
			++b_i;
			continue;
		}
		return std::max(a_i->start,b_i->start);
	}
	return end;
}
namespace {
OStream& operator<<(OStream &OS, const LivetimeIntervalImpl &lti) {
	return OS << "LivetimeIntervalImpl (" << lti.front().start << ") in " << *lti.get_operand();
}
} // end anonymous namespace

MachineOperand* LivetimeIntervalImpl::get_operand(MIIterator pos) const {
	//LOG2("get_operand(this:" << *this << " pos:" << pos << ")" <<nl);
	if (back().end.get_iterator() < pos) {
		LivetimeInterval lti_next = get_next();
		assert(lti_next.pimpl);
		return lti_next.get_operand(pos);
	}
	assert(!(pos < front().start.get_iterator()));
	return get_operand();
}

UseDef LivetimeIntervalImpl::next_usedef_after(UseDef pos) const {
	// search use
	for (const_use_iterator i = use_begin(), e = use_end(); i != e; ++i) {
		if (pos < *i) {
			UseDef use = *i;
			// search def
			for (const_def_iterator i = def_begin(), e = def_end(); i != e; ++i) {
				if (pos < *i) {
					if (*i < use)
						return *i;
					break;
				}
			}
			return use;
		}
	}
	// possible because phi functions do not have use sites
	return back().end;
}

inline void LivetimeIntervalImpl::move_use_def(LivetimeIntervalImpl *from, LivetimeIntervalImpl *to, UseDef pos) {
	// copy uses
	LOG2("copy uses" << nl);
	{
		const_use_iterator i = from->use_begin(), e = from->use_end();
		for (; i!= e && *i < pos; ++i) {
			LOG2("  keep " << *i << nl);
		}
		while(i != e) {
			const_use_iterator tmp = i++;
			LOG2("  move " << *tmp << nl);
			to->uses.insert(*tmp);
			from->uses.erase(tmp);
		}
	}
	LOG2("copy defs" << nl);
	// copy defs
	{
		const_def_iterator i = from->def_begin(), e = from->def_end();
		for (; i!= e && *i < pos; ++i) {
			LOG2("  keep " << *i << nl);
		}
		while(i != e) {
			const_def_iterator tmp = i++;
			LOG2("  move " << *tmp << nl);
			to->defs.insert(*tmp);
			from->defs.erase(tmp);
		}
	}
}


LivetimeInterval LivetimeIntervalImpl::split_active(MIIterator pos) {
	MachineInstruction *MI = *pos;
	assert(!has_next());
	assert(get_State(pos) == LivetimeInterval::Active);
	assert(MI->op_size() == 1);
	assert(MI->is_move());

	UseDef use(UseDef::Use, pos, &MI->get(0));
	UseDef def(UseDef::Def, pos, &MI->get_result());

	LOG2("Split " << *this << " at " << pos << nl);
	LOG2("new use: " << use << nl);
	LOG2("new def: " << def << nl);

	// new iterval
	next = new LivetimeInterval(get_init_operand());
	LivetimeInterval &lti = *next;
	lti.set_operand(def.get_operand()->op);

	// copy uses defs
	move_use_def(this, lti.pimpl.get(), def);

	LOG2("copy ranges" << nl);
	// copy ranges
	iterator i = intervals.begin(), e = intervals.end();
	for (; i != e && i->end < def; ++i) {
		LOG2("  keep " << *i << nl);
	}
	assert(i != e);
	assert(i->start < def);

	// split range
	iterator tmp = i++;
	intervals.insert(tmp,LivetimeRange(tmp->start,use));
	insert_usedef(use);
	lti.pimpl->intervals.push_front(LivetimeRange(def,tmp->end));
	lti.pimpl->insert_usedef(def);
	intervals.erase(tmp);

	while(i != e) {
		iterator tmp = i++;
		LOG2("  move " << *tmp << nl);
		lti.pimpl->intervals.push_back(*tmp);
		intervals.erase(tmp);
	}

	return lti;
}

LivetimeInterval LivetimeIntervalImpl::split_inactive(UseDef pos, MachineOperand* MO) {
	assert(!has_next());
	assert(get_State(pos) == LivetimeInterval::Inactive);

	LOG2("Split " << *this << " at " << pos << nl);

	next = new LivetimeInterval(get_init_operand());
	LivetimeInterval &lti = *next;
	lti.set_operand(MO);

	// copy uses defs
	move_use_def(this, lti.pimpl.get(), pos);

	LOG2("copy ranges" << nl);
	// copy ranges
	iterator i = intervals.begin(), e = intervals.end();
	for (; i != e && i->start < pos; ++i) {
		LOG2("  keep " << *i << nl);
	}
	assert(i != e);

	while(i != e) {
		iterator tmp = i++;
		LOG2("  move " << *tmp << nl);
		lti.pimpl->intervals.push_back(*tmp);
		intervals.erase(tmp);
	}
	assert(lti.front().start.is_pseudo_use());
	assert((*lti.front().start.get_iterator())->is_label());
	// set the start of the livetime interval to the end of the livetime hole
	lti.set_from(UseDef(UseDef::Pseudo,--lti.front().start.get_iterator()),lti.front().end);
	assert((*lti.front().start.get_iterator())->is_end());

	return lti;
}
#if 0
void LivetimeIntervalImpl::set_Register(Register* r) {
	operand = r;
}

Register* LivetimeIntervalImpl::get_Register() const {
	Register *r = operand->to_Register();
	assert(r);
	return r;
}
void LivetimeIntervalImpl::set_ManagedStackSlot(ManagedStackSlot* s) {
	operand = s;
}

ManagedStackSlot* LivetimeIntervalImpl::get_ManagedStackSlot() const {
	ManagedStackSlot *s = operand->to_ManagedStackSlot();
	return s;
}

bool LivetimeIntervalImpl::is_in_Register() const {
	if(operand)
		return operand->to_Register() != NULL;
	return false;
}
bool LivetimeIntervalImpl::is_in_StackSlot() const {
	if(operand)
		return operand->to_ManagedStackSlot() != NULL;
	return false;
}
Type::TypeID LivetimeIntervalImpl::get_type() const {
	if (!operand) return Type::VoidTypeID;
	return operand->get_type();
}

LivetimeIntervalImpl* LivetimeIntervalImpl::split(unsigned pos, StackSlotManager *SSM) {
	//sanity checks:
	assert(pos >= get_start() && pos < get_end());
	// NOTE Note that the end of an interval shall be a use position
	// (even) (what about for phi input intervals input for phi?).
	// in special cases (eg fixed intervals) this is not the case
	// -> force it
	//pos -= (pos & 1);

	LivetimeIntervalImpl *stack_interval =  NULL;
	LivetimeIntervalImpl *lti = NULL;
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
				lti = new LivetimeIntervalImpl();
				lti->add_range(next_usedef,end);
			//}
			++i;
			// create stack interval
			ManagedStackSlot *slot = SSM->create_ManagedStackSlot(get_type());
			stack_interval = new LivetimeIntervalImpl();
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
		lti = new LivetimeIntervalImpl();
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
	return OS << "LivetimeInterval (" << lti.front().start << ") in "
		<< *lti.get_operand() << " (init:"  << *lti.get_init_operand() << ")";
}
OStream& operator<<(OStream &OS, const LivetimeInterval *lti) {
	if (!lti) {
		return OS << "(LivetimeInterval) NULL";
	}
	return OS << *lti;
}

OStream& operator<<(OStream &OS, const UseDef &usedef) {
	if (usedef.is_use()) OS << "Use";
	if (usedef.is_def()) OS << "Def";
	if (usedef.is_pseudo_use()) OS << "PseudoUse";
	return OS << " " << usedef.get_iterator();
}
OStream& operator<<(OStream &OS, const LivetimeRange &range) {
	return OS << "LivetimeRange " << range.start << " - " << range.end;
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
