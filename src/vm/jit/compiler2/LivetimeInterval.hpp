/* src/vm/jit/compiler2/LivetimeInterval.hpp - LivetimeInterval

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

#ifndef _JIT_COMPILER2_LIVETIMEINTERVAL
#define _JIT_COMPILER2_LIVETIMEINTERVAL

#include "vm/jit/compiler2/Type.hpp"
#include "vm/jit/compiler2/MachineBasicBlock.hpp"

#include "future/memory.hpp"            // for cacao::shared_ptr

#include "vm/jit/compiler2/alloc/map.hpp"
#include "vm/jit/compiler2/alloc/set.hpp"
#include "vm/jit/compiler2/alloc/list.hpp"

#include <climits>

namespace cacao {
namespace jit {
namespace compiler2 {

// forward declaration
class LivetimeIntervalImpl;
class BeginInst;
class LivetimeAnalysisPass;
class Register;
class MachineOperand;
class MachineOperandDesc;
class StackSlotManager;
class ManagedStackSlot;
class BasicBlockSchedule;
class MachineInstructionSchedule;

class UseDef {
public:
	enum Type {
		Use,
		Def,
		PseudoUse,
		PseudoDef
	};

	/// constructor
	UseDef(Type type, MIIterator it, MachineOperandDesc *op = NULL) : type(type), it(it), op(op) {
		assert(type == PseudoUse || type == PseudoDef || op);
	}
	/// Is a def position
	bool is_def() const { return type == Def; }
	/// Is a use position
	bool is_use() const { return type == Use; }
	/// Is a pseudo def position
	bool is_pseudo_def() const { return type == PseudoDef; }
	/// Is a pseudo use position
	bool is_pseudo_use() const { return type == PseudoUse; }
	/// Is a pseudo use/def
	bool is_pseudo() const { return is_pseudo_use() || is_pseudo_def(); }


	/// Get instruction iterator
	MIIterator get_iterator() const { return it; }
	/// Get operand descriptor
	MachineOperandDesc* get_operand() const { return op; }
private:
	/// Type
	Type type;
	/// Iterator pointing to the instruction
	MIIterator it;
	/// Operand
	MachineOperandDesc *op;
};

struct LivetimeRange {
	UseDef start;
	UseDef end;
	LivetimeRange(const UseDef &start, const UseDef &end)
		: start(start), end(end) {}
};


class LivetimeInterval {
public:
	typedef alloc::list<LivetimeRange>::type IntervalListTy;
	typedef IntervalListTy::const_iterator const_iterator;
	typedef IntervalListTy::iterator iterator;

	typedef std::multiset<UseDef> UseListTy;
	typedef std::multiset<UseDef> DefListTy;
	typedef UseListTy::const_iterator const_use_iterator;
	typedef DefListTy::const_iterator const_def_iterator;
	typedef UseListTy::iterator use_iterator;
	typedef DefListTy::iterator def_iterator;

	enum State {
		Unhandled, ///< Interval no yet started
		Active,    ///< Interval is live
		Inactive,  ///< Interval is inactive
		Handled    ///< Interval is finished
	};
	/// constructor
	explicit LivetimeInterval(MachineOperand*);
	/// copy constructor
	LivetimeInterval(const LivetimeInterval &other);
	/// copy assignment operator
	LivetimeInterval& operator=(const LivetimeInterval &other);

	/// A range the range [first, last] to the interval
	void add_range(UseDef first, UseDef last);
	/// Set from. If no interval available add range from to.
	void set_from(UseDef from, UseDef to);
	State get_State(MIIterator pos) const;
	State get_State(UseDef pos) const;
	/// Is use position
	bool is_use_at(MIIterator pos) const;
	/// Is def position
	bool is_def_at(MIIterator pos) const;
	/// Get the current store of the interval
	MachineOperand* get_operand() const;
	/// Get the store of the interval at position pos
	MachineOperand* get_operand(MIIterator pos) const;
	/// Set the current store of the interval
	void set_operand(MachineOperand* op);
	/// Get the hint for the interval
	MachineOperand* get_hint() const;
	/// Set the hit for the interval
	void set_hint(MachineOperand* op);
	/**
	 * Get the initial operand. This is needed to identify phi instructions
	 * as they do not have an MIIterator associated with them.
	 */
	MachineOperand* get_init_operand() const;

	/**
	 * Split interval at active pos.
	 * @param pos Must be a move/copy instruction with one input and one output
	 */
	LivetimeInterval split_active(MIIterator pos);
	/**
	 * Split interval at inactive pos.
	 */
	LivetimeInterval split_inactive(UseDef pos, MachineOperand* MO);
	/**
	 * Split a phi input operand
	 */
	LivetimeInterval split_phi_active(MIIterator pos, MachineOperand* MO);

	/// get next use def after pos (if not found return end)
	UseDef next_usedef_after(UseDef pos, UseDef end) const;

	/// get next split interval
	LivetimeInterval get_next() const;
	bool has_next() const;

	const_iterator begin() const;
	const_iterator end() const;
	std::size_t size() const;
	bool empty() const;
	LivetimeRange front() const;
	LivetimeRange back() const;

	// uses
	const_use_iterator use_begin() const;
	const_use_iterator use_end() const;
	std::size_t use_size() const;
	// defs
	const_def_iterator def_begin() const;
	const_def_iterator def_end() const;
	std::size_t def_size() const;
private:
	shared_ptr<LivetimeIntervalImpl> pimpl;
	friend class LivetimeIntervalImpl;
};

class LivetimeIntervalImpl {
public:
	typedef LivetimeInterval::IntervalListTy IntervalListTy;
	typedef LivetimeInterval::const_iterator const_iterator;
	typedef LivetimeInterval::iterator iterator;

	typedef LivetimeInterval::State State;
/**
 * TODO: doc me!
 */

	/**
	 * @Cpp11 this could be changed to std::set where erase returns an
	 * iterator.
	 */
	typedef std::multiset<UseDef> UseListTy;
	typedef std::multiset<UseDef> DefListTy;
	typedef UseListTy::const_iterator const_use_iterator;
	typedef DefListTy::const_iterator const_def_iterator;
	typedef UseListTy::iterator use_iterator;
	typedef DefListTy::iterator def_iterator;
private:
	IntervalListTy intervals;
	UseListTy uses;
	DefListTy defs;
	MachineOperand* operand;         ///< store for the interval
	MachineOperand* init_operand;    ///< initial operand for the interval
	MachineOperand* hint;            ///< hint for the interval
	LivetimeInterval *next;

	void insert_usedef(const UseDef &usedef) {
		if (usedef.is_use())
			uses.insert(usedef);
		if (usedef.is_def())
			defs.insert(usedef);
	}
	static void move_use_def(LivetimeIntervalImpl *from, LivetimeIntervalImpl *to, UseDef pos);
public:
	/// construtor
	LivetimeIntervalImpl(MachineOperand* op): operand(op), init_operand(op), hint(NULL), next(NULL) {}
	/**
	 * A range the range [first, last] to the interval
	 */
	void add_range(UseDef first, UseDef last);
	void set_from(UseDef from, UseDef to);
	State get_State(MIIterator pos) const;
	State get_State(UseDef pos) const;
	bool is_use_at(MIIterator pos) const;
	bool is_def_at(MIIterator pos) const;
	MachineOperand* get_operand() const { return operand; }
	void set_operand(MachineOperand* op) { operand = op; }
	MachineOperand* get_init_operand() const { return init_operand; }
	MachineOperand* get_hint() const { return hint; }
	void set_hint(MachineOperand* op) { hint = op; }
	LivetimeInterval get_next() const { assert(has_next()); return *next; }
	bool has_next() const { return bool(next); }
	LivetimeInterval split_active(MIIterator pos);
	LivetimeInterval split_inactive(UseDef pos, MachineOperand* MO);
	LivetimeInterval split_phi_active(MIIterator pos, MachineOperand* MO);

	MachineOperand* get_operand(MIIterator pos) const;
	UseDef next_usedef_after(UseDef,UseDef) const;

	const_iterator begin()         const { return intervals.begin(); }
	const_iterator end()           const { return intervals.end(); }
	std::size_t size()             const { return intervals.size(); }
	bool empty()                   const { return intervals.empty(); }
	LivetimeRange front()          const { return intervals.front(); }
	LivetimeRange back()           const { return intervals.back(); }

	const_use_iterator use_begin() const { return uses.begin(); }
	const_use_iterator use_end()   const { return uses.end(); }
	std::size_t use_size()         const { return uses.size(); }

	const_def_iterator def_begin() const { return defs.begin(); }
	const_def_iterator def_end()   const { return defs.end(); }
	std::size_t def_size()         const { return defs.size(); }
};

// LivetimeInterval

inline LivetimeInterval::LivetimeInterval(MachineOperand* op) : pimpl(new LivetimeIntervalImpl(op)){}
inline LivetimeInterval::LivetimeInterval(const LivetimeInterval &other) : pimpl(other.pimpl) {}
inline LivetimeInterval& LivetimeInterval::operator=(const LivetimeInterval &other) {
	pimpl = other.pimpl;
	return *this;
}
inline void LivetimeInterval::add_range(UseDef first, UseDef last) {
	pimpl->add_range(first, last);
}
inline void LivetimeInterval::set_from(UseDef from, UseDef to) {
	pimpl->set_from(from, to);
}
inline LivetimeInterval::State LivetimeInterval::get_State(MIIterator pos) const {
	return pimpl->get_State(pos);
}
inline LivetimeInterval::State LivetimeInterval::get_State(UseDef pos) const {
	return pimpl->get_State(pos);
}
inline bool LivetimeInterval::is_use_at(MIIterator pos) const {
	return pimpl->is_use_at(pos);
}
inline bool LivetimeInterval::is_def_at(MIIterator pos) const {
	return pimpl->is_def_at(pos);
}

inline LivetimeInterval::const_iterator LivetimeInterval::begin() const {
	return pimpl->begin();
}
inline LivetimeInterval::const_iterator LivetimeInterval::end() const {
	return pimpl->end();
}
inline std::size_t LivetimeInterval::size() const {
	return pimpl->size();
}
inline bool LivetimeInterval::empty() const {
	return pimpl->empty();
}
inline LivetimeRange LivetimeInterval::front() const {
	return pimpl->front();
}
inline LivetimeRange LivetimeInterval::back() const {
	return pimpl->back();
}
inline MachineOperand* LivetimeInterval::get_operand() const {
	return pimpl->get_operand();
}
inline MachineOperand* LivetimeInterval::get_operand(MIIterator pos) const {
	return pimpl->get_operand(pos);
}
inline MachineOperand* LivetimeInterval::get_init_operand() const {
	return pimpl->get_init_operand();
}
inline void LivetimeInterval::set_operand(MachineOperand* op) {
	pimpl->set_operand(op);
}
inline MachineOperand* LivetimeInterval::get_hint() const {
	return pimpl->get_hint();
}
inline void LivetimeInterval::set_hint(MachineOperand* op) {
	pimpl->set_hint(op);
}
inline UseDef LivetimeInterval::next_usedef_after(UseDef pos, UseDef end) const {
	return pimpl->next_usedef_after(pos,end);
}
inline LivetimeInterval LivetimeInterval::split_active(MIIterator pos) {
	return pimpl->split_active(pos);
}
inline LivetimeInterval LivetimeInterval::split_inactive(UseDef pos, MachineOperand* MO) {
	return pimpl->split_inactive(pos, MO);
}
inline LivetimeInterval LivetimeInterval::split_phi_active(MIIterator pos, MachineOperand* MO) {
	return pimpl->split_phi_active(pos, MO);
}
inline LivetimeInterval LivetimeInterval::get_next() const {
	return pimpl->get_next();
}
inline bool LivetimeInterval::has_next() const {
	return pimpl->has_next();
}

// uses
inline LivetimeInterval::const_use_iterator LivetimeInterval::use_begin() const {
	return pimpl->use_begin();
}
inline LivetimeInterval::const_use_iterator LivetimeInterval::use_end() const {
	return pimpl->use_end();
}
inline std::size_t LivetimeInterval::use_size() const {
	return pimpl->use_size();
}
// defs
inline LivetimeInterval::const_def_iterator LivetimeInterval::def_begin() const {
	return pimpl->def_begin();
}
inline LivetimeInterval::const_def_iterator LivetimeInterval::def_end() const {
	return pimpl->def_end();
}
inline std::size_t LivetimeInterval::def_size() const {
	return pimpl->def_size();
}

UseDef next_intersection(const LivetimeInterval &a,
	const LivetimeInterval &b, UseDef pos, UseDef end);

/// less then operator
inline bool operator<(const UseDef& lhs,const UseDef& rhs) {
	if (lhs.get_iterator() < rhs.get_iterator() )
		return true;
	if (rhs.get_iterator() < lhs.get_iterator() )
		return false;
	// iterators are equal
	if ( (lhs.is_use() || lhs.is_pseudo_use() ) && (rhs.is_def() || rhs.is_pseudo_def() ) )
		return true;
	return false;
}

inline bool operator!=(const UseDef& lhs,const UseDef& rhs) {
	return (lhs < rhs || rhs < lhs);
}
inline bool operator==(const UseDef& lhs,const UseDef& rhs) {
	return !(lhs != rhs);
}
inline bool operator<=(const UseDef& lhs,const UseDef& rhs) {
	return lhs < rhs || lhs == rhs;
}

OStream& operator<<(OStream &OS, const LivetimeInterval &lti);
OStream& operator<<(OStream &OS, const LivetimeInterval *lti);

OStream& operator<<(OStream &OS, const UseDef &usedef);
OStream& operator<<(OStream &OS, const LivetimeRange &range);

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_LIVETIMEINTERVAL */


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
