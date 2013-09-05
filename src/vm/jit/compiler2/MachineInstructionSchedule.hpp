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
#include "toolbox/future.hpp"

#include <map>
#include <list>
#include <vector>
#include <cassert>

namespace cacao {
namespace jit {
namespace compiler2 {
/**
 * @defgroup low-level-ir Low Level IR
 * @ingroup compiler2
 *
 * @{
 */

///////////////////

class BeginInst;
//class MachineInstruction;

///////////////////

// forward declarations
class ScheduledMachineInstruction;
class MachineBasicBlock;
class MachineInstructionSchedule;

class MachineBasicBlockImpl;

struct MIEntry {
	MIEntry(MachineInstruction* inst, std::size_t index) : inst(inst), index(index) {}
	MachineInstruction* inst;
	std::size_t index;
};

/**
 * A MachineInstruction which is part of a schedule.
 *
 */
class ScheduledMachineInstruction : public std::iterator<std::bidirectional_iterator_tag,
															MachineInstruction*> {
private:
	typedef std::list<MIEntry>::iterator intern_iterator;
public:
	ScheduledMachineInstruction() {}
	ScheduledMachineInstruction(intern_iterator it) : it(it) {}
	ScheduledMachineInstruction(const ScheduledMachineInstruction& other) : it(other.it) {}
	ScheduledMachineInstruction& operator++() {
		++it;
		return *this;
	}
	ScheduledMachineInstruction operator++(int) {
		ScheduledMachineInstruction tmp(*this);
		operator++();
		return tmp;
	}
	ScheduledMachineInstruction& operator--() {
		--it;
		return *this;
	}
	ScheduledMachineInstruction operator--(int) {
		ScheduledMachineInstruction tmp(*this);
		operator--();
		return tmp;
	}
	bool operator==(const ScheduledMachineInstruction& rhs) const { return it == rhs.it; }
	bool operator!=(const ScheduledMachineInstruction& rhs) const { return it != rhs.it; }
	bool operator<( const ScheduledMachineInstruction& rhs) const { return it->index < rhs.it->index; }
	bool operator>( const ScheduledMachineInstruction& rhs) const { return rhs < *this; }
	MachineInstruction*& operator*() { return it->inst; }
	MachineInstruction*  operator*() const { return it->inst; }
	MachineInstruction*& operator->() { return it->inst; }
	MachineInstruction*  operator->() const { return it->inst; }

private:
	intern_iterator it;
	friend class MachineBasicBlockImpl;
};

/**
 * A basic block of (scheduled) machine instructions.
 *
 * A MachineBasicBlock contains an ordered collection of
 * ScheduledMachineInstructions.
 */
class MachineBasicBlock {
public:
	MachineBasicBlock();
	/// returns the number of elements
	std::size_t size() const;
	/// Appends the given element value to the end of the container.
	void push_back(MachineInstruction* value);
	/// inserts value to the beginning
	void push_front(MachineInstruction* value);
	/// inserts value before the element pointed to by pos
	void insert_before(ScheduledMachineInstruction pos, MachineInstruction* value);
	/// inserts value after the element pointed to by pos
	void insert_after(ScheduledMachineInstruction pos, MachineInstruction* value);
	/// returns an iterator to the beginning
	ScheduledMachineInstruction begin();
	/// returns an iterator to the end
	ScheduledMachineInstruction end();
private:
	MachineBasicBlock(MachineBasicBlockImpl* pImpl) : pImpl(pImpl) {}
	shared_ptr<MachineBasicBlockImpl> pImpl;
};

/**
 * A machine instruction schedule.
 */
class NewMachineInstructionSchedule {
};

// end in group llir
/// @}

class MachineBasicBlockImpl {
private:
	std::list<MIEntry> list;

	struct IncrementMIEntry {
		void operator()(MIEntry& MIE) {
			++MIE.index;;
		}
	};
public:
	/// returns the number of elements
	std::size_t size() const {
		return list.size();
	}
	/// Appends the given element value to the end of the container.
	void push_back(MachineInstruction* value) {
		list.push_back(MIEntry(value,size()));
	}
	/// inserts value to the beginning
	void push_front(MachineInstruction* value) {
		std::for_each(list.begin(),list.end(),IncrementMIEntry());
		list.push_front(MIEntry(value,0));
	}

	/// inserts value before the element pointed to by pos
	void insert_before(ScheduledMachineInstruction pos, MachineInstruction* value) {
		list.insert(pos.it,MIEntry(value,pos.it->index));
		std::for_each(pos.it,list.end(),IncrementMIEntry());
	}
	/// inserts value after the element pointed to by pos
	void insert_after(ScheduledMachineInstruction pos, MachineInstruction* value) {
		insert_before(++pos,value);
	}
	/// returns an iterator to the beginning
	ScheduledMachineInstruction begin() {
		return list.begin();
	}
	/// returns an iterator to the end
	ScheduledMachineInstruction end() {
		return list.end();
	}
};

// MachineBasicBlock definitions

inline MachineBasicBlock::MachineBasicBlock() : pImpl(new MachineBasicBlockImpl) {}

inline std::size_t MachineBasicBlock::size() const {
	return pImpl->size();
}
inline void MachineBasicBlock::push_back(MachineInstruction* value) {
	pImpl->push_back(value);
}
inline void MachineBasicBlock::push_front(MachineInstruction* value) {
	pImpl->push_front(value);
}
inline void MachineBasicBlock::insert_before(ScheduledMachineInstruction pos, MachineInstruction* value) {
	pImpl->insert_before(pos,value);
}
inline void MachineBasicBlock::insert_after(ScheduledMachineInstruction pos, MachineInstruction* value) {
	pImpl->insert_after(pos,value);
}
ScheduledMachineInstruction MachineBasicBlock::begin() {
	return pImpl->begin();
}
ScheduledMachineInstruction MachineBasicBlock::end() {
	return pImpl->end();
}

/**
 * A machine instruction schedule.
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
	void add_after(unsigned i, MachineInstruction *MI) {
		assert(MI);
		added_list[i].push_back(MI);
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
