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

namespace cacao {
namespace jit {
namespace compiler2 {

// forward declaration
class Register;
class BeginInst;
class LivetimeAnalysisPass;

/**
 * TODO: doc me!
 */
class LivetimeInterval {
public:
	typedef std::list<std::pair<unsigned,unsigned> > IntervalListTy;
	typedef IntervalListTy::const_iterator const_iterator;
private:
	IntervalListTy intervals;
	Register *reg;
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
public:
	LivetimeInterval() : intervals(), reg(NULL) {}
	void set_reg(Register* r) {
		reg = r;
	}
	Register* get_reg() const {
		return reg;
	}
	const_iterator begin() const {
		return intervals.begin();
	}
	const_iterator end() const {
		return intervals.end();
	}
	std::size_t size() const {
		return intervals.size();
	}
	unsigned get_start() const {
		assert(intervals.size()>0);
		return intervals.front().first;
	}
	unsigned get_end() const {
		assert(intervals.size()>0);
		return intervals.back().second;
	}
	/**
	 * Returns true if this interval is active at pos
	 */
	bool is_inactive(unsigned pos) const {
		for(const_iterator i = begin(), e = end(); i != e ; ++i) {
			if( pos < i->first) {
				return true;
			}
			if( pos < i->second) {
				return false;
			}
		}
		return true;
	}

	friend class LivetimeAnalysisPass;
};

inline OStream& operator<<(OStream &OS, const LivetimeInterval &lti) {
	OS << lti.get_reg();
	for(LivetimeInterval::const_iterator i = lti.begin(), e = lti.end();
			i != e ; ++i) {
		OS << " [" << i->first << "," << i->second << ")";
	}
	return OS;
}
inline OStream& operator<<(OStream &OS, const LivetimeInterval *lti) {
	if (!lti) {
		return OS << "(LivetimeInterval) NULL";
	}
	return OS << *lti;
}

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
public:
	static char ID;
	LivetimeAnalysisPass() : Pass() {}
	bool run(JITData &JD);
	PassUsage& get_PassUsage(PassUsage &PA) const;

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
