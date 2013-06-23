/* src/vm/jit/compiler2/LivetimeAnalysisPass.hpp - LivetimeAnalysisPass

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

#ifndef _JIT_COMPILER2_LIVETIMEANALYSISPASS
#define _JIT_COMPILER2_LIVETIMEANALYSISPASS

#include "vm/jit/compiler2/Pass.hpp"

#include <map>
#include <set>
#include <list>

#include <climits>

namespace cacao {
namespace jit {
namespace compiler2 {

// forward declaration
class Register;
class BeginInst;
class LivetimeAnalysisPass;
class MachineOperand;
class MachineOperandDesc;
class StackSlotManager;
class ManagedStackSlot;
class BasicBlockSchedule;
class MachineInstructionSchedule;

/**
 * TODO: doc me!
 */
class LivetimeInterval {
public:
	typedef std::list<std::pair<unsigned,unsigned> > IntervalListTy;
	typedef IntervalListTy::const_iterator const_iterator;
	typedef IntervalListTy::iterator iterator;
	/**
	 * @Cpp11 this could be changed to std::set where erase returns an
	 * iterator.
	 */
	typedef std::pair<unsigned,MachineOperandDesc*> UseDefEntryTy;
	typedef std::list<UseDefEntryTy> UseDefTy;
	typedef UseDefTy::const_iterator const_use_iterator;
	typedef UseDefTy::const_iterator const_def_iterator;
	typedef UseDefTy::iterator use_iterator;
	typedef UseDefTy::iterator def_iterator;
private:
	IntervalListTy intervals;
	MachineOperand* operand;         ///< store for the interval
	UseDefTy uses;
	UseDefTy defs;
	bool fixed_interval;
	LivetimeInterval *next_split;    ///< if splitted, this points to the next iterval
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
	static bool compare_usedef(UseDefEntryTy &lhs, UseDefEntryTy &rhs) {
		return lhs.first < rhs.first;
	}
public:
	LivetimeInterval() : intervals(), operand(NULL), uses(), defs(),
			fixed_interval(false), next_split(NULL), hint(NULL) {}

	void set_Register(Register* r);
	Register* get_Register() const;
	void set_ManagedStackSlot(ManagedStackSlot* s);
	ManagedStackSlot* get_ManagedStackSlot() const;
	bool is_in_Register() const;

	bool is_fixed_interval()       const { return fixed_interval; }

	const_iterator begin()         const { return intervals.begin(); }
	const_iterator end()           const { return intervals.end(); }
	std::size_t size()             const { return intervals.size(); }

	UseDefEntryTy use_front()      const { return uses.front(); }
	const_use_iterator use_begin() const { return uses.begin(); }
	const_use_iterator use_end()   const { return uses.end(); }
	std::size_t use_size()         const { return uses.size(); }

	UseDefEntryTy def_front()      const { return defs.front(); }
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
		return pos < get_start();
	}
	bool is_handled(unsigned pos) const {
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
		assert(pos <= get_end());
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

	friend class LivetimeAnalysisPass;
};

inline OStream& operator<<(OStream &OS, const LivetimeInterval &lti);
inline OStream& operator<<(OStream &OS, const LivetimeInterval *lti) {
	if (!lti) {
		return OS << "(LivetimeInterval) NULL";
	}
	return OS << *lti;
}
inline OStream& operator<<(OStream &OS, const std::pair<unsigned,MachineOperandDesc*> &usedef);

/**
 * LivetimeAnalysisPass
 *
 * Based on the approach from "Linear scan register allocation on SSA form"
 * by Wimmer and Franz @cite Wimmer2010.
 */
class LivetimeAnalysisPass : public Pass {
public:
	typedef std::map<Register*,LivetimeInterval> LivetimeIntervalMapTy;
	typedef LivetimeIntervalMapTy::const_iterator const_iterator;
	typedef LivetimeIntervalMapTy::iterator iterator;
private:
	typedef std::set<Register*> LiveInSetTy;
	typedef std::map<BeginInst*,LiveInSetTy> LiveInMapTy;

	LivetimeIntervalMapTy lti_map;
	BasicBlockSchedule *BS;
	MachineInstructionSchedule *MIS;
public:
	static char ID;
	LivetimeAnalysisPass() : Pass() {}
	bool run(JITData &JD);
	PassUsage& get_PassUsage(PassUsage &PA) const;

	bool verify() const;

	const_iterator begin() const {
		return lti_map.begin();
	}
	const_iterator end() const {
		return lti_map.end();
	}
	iterator begin() {
		return lti_map.begin();
	}
	iterator end() {
		return lti_map.end();
	}
	std::size_t size() const {
		return lti_map.size();
	}
	LivetimeInterval* get(Register* reg) {
		iterator i = lti_map.find(reg);
		if ( i == lti_map.end()) {
			return NULL;
		}
		return &(i->second);
	}
	OStream& print(OStream& OS) const;
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_LIVETIMEANALYSISPASS */


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
