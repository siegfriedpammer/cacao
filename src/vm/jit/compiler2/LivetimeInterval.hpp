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

#include <map>
#include <set>
#include <list>

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
		Pseudo
	};

	/// constructor
	UseDef(Type type, MIIterator it) : type(type), it(it) {}
	/// Is a def position
	bool is_def() const { return type == Def; }
	/// Is a use position
	bool is_use() const { return type == Use; }
	/** Is a pseudo use position.
	 * A pseudo use position is used to mark backedges.
	 */
	bool is_pseudo_use() const { return type == Pseudo; }

	/// Get instruction iterator
	MIIterator get_iterator() const { return it; }
private:
	/// Type
	Type type;
	/// Iterator pointing to the instruction
	MIIterator it;
};

struct LivetimeRange {
	UseDef start;
	UseDef end;
	LivetimeRange(const UseDef &start, const UseDef &end)
		: start(start), end(end) {}
};


class LivetimeInterval {
public:
	typedef std::list<LivetimeRange> IntervalListTy;
	typedef IntervalListTy::const_iterator const_iterator;
	typedef IntervalListTy::iterator iterator;

	enum State {
		Unhandled, ///< Interval no yet started
		Active,    ///< Interval is live
		Inactive,  ///< Interval is inactive
		Handled    ///< Interval is finished
	};
	/// constructor
	LivetimeInterval(MachineOperand*);
	/// copy constructor
	LivetimeInterval(const LivetimeInterval &other);
	/// copy assignment operator
	LivetimeInterval& operator=(const LivetimeInterval &other);

	/// A range the range [first, last] to the interval
	void add_range(UseDef first, UseDef last);
	void set_from(UseDef from);
	State get_State(MIIterator pos) const;
	#if 0
	bool is_unhandled(MIIterator pos) const;
	bool is_handled(MIIterator pos) const;
	bool is_active(MIIterator pos) const;
	bool is_inactive(MIIterator pos) const;
	#endif
	/// Is use position
	bool is_use_at(MIIterator pos) const;
	/// Is def position
	bool is_def_at(MIIterator pos) const;
	/// Get the current store of the interval
	MachineOperand* get_operand() const;
	/// Set the current store of the interval
	void set_operand(MachineOperand* op);
	/**
	 * Get the initial operand. This is needed to identify phi instructions
	 * as they do not have an MIIterator associated with them.
	 */
	MachineOperand* get_init_operand() const;

	const_iterator begin() const;
	const_iterator end() const;
	std::size_t size() const;
	bool empty() const;
	LivetimeRange front() const;
	LivetimeRange back() const;
private:
	shared_ptr<LivetimeIntervalImpl> pimpl;
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

	void insert_usedef(const UseDef &usedef) {
		if (usedef.is_use())
			uses.insert(usedef);
		if (usedef.is_def())
			defs.insert(usedef);
	}
	#if 0
	UseDefTy usedefs;
	//bool fixed_interval;
	Register *hint;                  ///< Register hint
	void add_range(unsigned from, unsigned to) {
		if (intervals.size() > 0) {
			if (intervals.begin()->first == to) {
				// merge intervals
				intervals.begin()->first = from;
				return;
			}
			if (intervals.begin()->first <= from && intervals.begin()->second >= to) {
				// already covered
				return;
			}
		}
		// new interval
		intervals.push_front(std::make_pair(from,to));
	}
	void set_from(unsigned from) {
		assert(intervals.size() > 0);
		intervals.begin()->first = from;
	}
	void set_from_if_available(unsigned from, unsigned to) {
		if (intervals.size() > 0) {
			intervals.begin()->first = from;
		} else {
			add_range(from,to);
		}
	}
	static bool compare_usedef(UseDef &lhs, UseDef &rhs) {
		return lhs.first < rhs.first;
	}
	#endif
public:
	/// construtor
	LivetimeIntervalImpl(MachineOperand* op): operand(op), init_operand(op) {}
	/**
	 * A range the range [first, last] to the interval
	 */
	void add_range(UseDef first, UseDef last);
	void set_from(UseDef from);
	State get_State(MIIterator pos) const;
	bool is_use_at(MIIterator pos) const;
	bool is_def_at(MIIterator pos) const;
	MachineOperand* get_operand() const { return operand; }
	void set_operand(MachineOperand* op) { operand = op; }
	MachineOperand* get_init_operand() const { return init_operand; }

	#if 0
	LivetimeInterval() : intervals(), operand(NULL), uses(), defs(),
			fixed_interval(false), next_split(NULL), hint(NULL) {}

	void set_Register(Register* r);
	Register* get_Register() const;
	void set_ManagedStackSlot(ManagedStackSlot* s);
	ManagedStackSlot* get_ManagedStackSlot() const;
	bool is_in_Register() const;
	bool is_in_StackSlot() const;

	Type::TypeID get_type() const;

	bool is_fixed_interval()       const { return fixed_interval; }
#endif
	const_iterator begin()         const { return intervals.begin(); }
	const_iterator end()           const { return intervals.end(); }
	std::size_t size()             const { return intervals.size(); }
	bool empty()                   const { return intervals.empty(); }
	LivetimeRange front()          const { return intervals.front(); }
	LivetimeRange back()           const { return intervals.back(); }

#if 0
	UseDef use_front()      const { return uses.front(); }
	const_use_iterator use_begin() const { return uses.begin(); }
	const_use_iterator use_end()   const { return uses.end(); }
	std::size_t use_size()         const { return uses.size(); }

	UseDef def_front()      const { return defs.front(); }
	const_def_iterator def_begin() const { return defs.begin(); }
	const_def_iterator def_end()   const { return defs.end(); }
	std::size_t def_size()         const { return defs.size(); }

	unsigned get_start() const {
		assert(intervals.size()>0);
		return intervals.front().first;
	}
	unsigned get_end() const {
		assert(intervals.size()>0);
		return intervals.back().second;
	}
	void add_use(unsigned use, MachineOperandDesc &MOD) {
		add_use(std::make_pair(use,&MOD));
	}
	void add_def(unsigned def, MachineOperandDesc &MOD) {
		add_def(std::make_pair(def,&MOD));
	}
	void add_use(const std::pair<unsigned,MachineOperandDesc*> use) {
		uses.push_front(use);
		uses.sort(compare_usedef);
	}
	void add_def(const std::pair<unsigned,MachineOperandDesc*> def) {
		defs.push_front(def);
		defs.sort(compare_usedef);
	}
	void set_fixed() { fixed_interval = true; }
	/**
	 * Returns true if this interval is active at pos
	 */

	LivetimeInterval* get_next() const {
		return next_split;
	}

	bool is_unhandled(unsigned pos) const {
		if (intervals.size() == 0) return false;
		return pos < get_start();
	}
	bool is_handled(unsigned pos) const {
		if (intervals.size() == 0) return true;
		return pos >= get_end();
	}
	bool is_active(unsigned pos) const {
		for(const_iterator i = begin(), e = end(); i != e ; ++i) {
			if( pos < i->first) {
				return false;
			}
			if( pos < i->second) {
				return true;
			}
		}
		return false;
	}
	bool is_inactive(unsigned pos) const {
		return !is_active(pos);
	}
	bool is_def(unsigned pos) const {
		for (const_def_iterator i = def_begin(), e = def_end(); i != e; ++i) {
			if (i->first == pos) {
				return true;
			}
			if (i->first > pos) {
				return false;
			}
		}
		return false;
	}

	bool is_use(unsigned pos) const {
		for (const_use_iterator i = use_begin(), e = use_end(); i != e; ++i) {
			if (i->first == pos) {
				return true;
			}
			if (i->first > pos) {
				return false;
			}
		}
		return false;
	}

	bool is_split_of(const LivetimeInterval &LI) const {
		for( LivetimeInterval *lti = LI.get_next(); lti; lti = lti->get_next()) {
			if (lti == this) {
				return true;
			}
		}
		return false;
	}

	signed intersects(const LivetimeInterval &lti) const {
		for(const_iterator a_i = begin(), b_i = lti.begin(),
				a_e = end(), b_e = lti.end() ; a_i != a_e && b_i != b_e ; ) {
			unsigned a_start = a_i->first;
			unsigned a_end   = a_i->second;
			unsigned b_start = b_i->first;
			unsigned b_end   = b_i->second;

			if (b_start >= a_end) {
				++a_i;
				continue;
			}
			if (a_start >= b_end) {
				++b_i;
				continue;
			}
			return std::max(a_start,b_start);
		}
		return -1;
	}

	void set_hint(Register *reg) { hint = reg; }
	Register* get_hint() const { return hint; }

	signed next_usedef_after(unsigned pos) const {
		//assert(pos <= get_end());
		if (pos > get_end()) {
			return -1;
		}
		signed next_use = -1;
		for (const_use_iterator i = use_begin(), e = use_end(); i != e; ++i) {
			if (i->first >= pos) {
				next_use = i->first;
				break;
			}
		}
		for (const_def_iterator i = def_begin(), e = def_end(); i != e; ++i) {
			if (i->first >= pos) {
				if (next_use == -1)
					return (signed)i->first;
				return std::min((signed)i->first,next_use);
			}
		}
		if (next_use == -1) {
			// no use def after pos _but_ interval still live -> loop interval
			// so the next use is the backedge
			return get_end() ;
		}
		return next_use;
	}

	/**
	 * split this interval at position pos and return a new interval
	 */
	LivetimeInterval* split(unsigned pos);
	LivetimeInterval* split(unsigned pos, StackSlotManager *SSM);
#endif
	friend class LivetimeAnalysisPass;
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
inline void LivetimeInterval::set_from(UseDef from) {
	pimpl->set_from(from);
}
inline LivetimeInterval::State LivetimeInterval::get_State(MIIterator pos) const {
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
inline MachineOperand* LivetimeInterval::get_init_operand() const {
	return pimpl->get_init_operand();
}
inline void LivetimeInterval::set_operand(MachineOperand* op) {
	pimpl->set_operand(op);
}

MIIterator next_intersection(const LivetimeInterval &a,
	const LivetimeInterval &b, MIIterator pos, MIIterator end);

/// less then operator
inline bool operator<(const UseDef& lhs,const UseDef& rhs) {
	return lhs.get_iterator() < rhs.get_iterator();
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
