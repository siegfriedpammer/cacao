/* src/vm/jit/compiler2/MachineInstructionSchedule.hpp - MachineInstructionSchedule

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

#ifndef _JIT_COMPILER2_MACHINEINSTRUCTIONSCHEDULE
#define _JIT_COMPILER2_MACHINEINSTRUCTIONSCHEDULE

#include "vm/jit/compiler2/MachineInstruction.hpp"

#include <map>
#include <list>
#include <vector>
#include <cassert>

namespace cacao {
namespace jit {
namespace compiler2 {

class BeginInst;
//class MachineInstruction;

/**
 * MachineInstructionSchedule
 * TODO: more info
 */
class MachineInstructionSchedule {
public:
	typedef std::vector<MachineInstruction*> MachineInstructionListTy;
	typedef std::pair<unsigned,unsigned> MachineInstructionRangeTy;
	typedef std::map<BeginInst*,MachineInstructionRangeTy> BasicBlockRangeTy;
	typedef MachineInstructionListTy::const_iterator const_iterator;
	typedef MachineInstructionListTy::const_reverse_iterator const_reverse_iterator;
protected:
	typedef std::map<unsigned,std::list<MachineInstruction*> > AddedInstListTy;
	MachineInstructionListTy list;
	BasicBlockRangeTy map;
	AddedInstListTy added_list;
public:
	MachineInstructionSchedule() {}
	MachineInstruction* operator[](const unsigned i) const {
		return get(i);
	}
	MachineInstruction* get(const unsigned i) const {
		assert(i < size());
		return list[i];
	}
	MachineInstructionRangeTy get_range(BeginInst* BI) const {
		BasicBlockRangeTy::const_iterator i = map.find(BI);
		if (i == map.end()) {
			return std::make_pair(0,0);
		}
		return i->second;
	}
	std::size_t size() const {
		return list.size();
	}
	const_iterator begin() const {
		return list.begin();
	}
	const_iterator end() const {
		return list.end();
	}
	const_reverse_iterator rbegin() const {
		return list.rbegin();
	}
	const_reverse_iterator rend() const {
		return list.rend();
	}
	void add_before(unsigned i, MachineInstruction *MI) {
		assert(MI);
		added_list[i].push_front(MI);
	}
	/**
	 * write the added instructions to the DAG
	 *
	 * @note This invalidates the schedule
	 */
	void insert_added_instruction() {
		for(unsigned i = 0, e = size(); i < e ; ++i) {
			AddedInstListTy::const_iterator added_it = added_list.find(i);
			MachineInstruction *MI = list[i];
			if (added_it != added_list.end()) {
				for (std::list<MachineInstruction*>::const_iterator i = added_it->second.begin(),
						e = added_it->second.end(); i != e; ++i) {
					MI->add_before(*i);
				}
			}
		}
	}
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_MACHINEINSTRUCTIONSCHEDULE */


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
