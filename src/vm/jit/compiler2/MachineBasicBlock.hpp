/* src/vm/jit/compiler2/MachineBasicBlock.hpp - MachineBasicBlock

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

#ifndef _JIT_COMPILER2_MACHINEBASICBLOCK
#define _JIT_COMPILER2_MACHINEBASICBLOCK

#include "vm/jit/compiler2/MachineInstructionSchedule.hpp"
#include "toolbox/ordered_list.hpp"

#include "toolbox/future.hpp" // for all_of, none_of
namespace cacao {

// forward declarations
class OStream;

namespace jit {
namespace compiler2 {

// forward declarations
class MachineInstruction;
class MachineInstructionSchedule;
class MachineOperand;
class MachinePhiInst;

class MIIterator {
	typedef ordered_list<MachineInstruction*>::iterator _iterator;
	typedef MBBIterator block_iterator;
	block_iterator block_it;
	_iterator it;
	static _iterator _end;
public:
	typedef _iterator::reference reference;
	typedef _iterator::pointer pointer;

	/// construct end element
	MIIterator(const block_iterator &block_it)
		: block_it(block_it), it(_end) {}
	/// constructor
	MIIterator(const block_iterator &block_it, const _iterator& it)
		: block_it(block_it), it(it) {}
	/// copy constructor
	MIIterator(const MIIterator& other)
		: block_it(other.block_it), it(other.it) {}
	/// copy assignment operator
	MIIterator& operator=(const MIIterator& other) {
		block_it = other.block_it;
		it = other.it;
		return *this;
	}
	MIIterator& operator++();
	MIIterator& operator--();
	MIIterator operator++(int) {
		MIIterator tmp(*this);
		operator++();
		return tmp;
	}
	MIIterator operator--(int) {
		MIIterator tmp(*this);
		operator--();
		return tmp;
	}
	bool operator==(const MIIterator& rhs) const {
		if (it == _end && rhs.it == _end)
			return true;
		if (block_it == rhs.block_it)
			return it == rhs.it;
		return false;
	}
	bool operator<( const MIIterator& rhs) const {
		if (it == _end)
			return false;
		if (rhs.it == _end)
			return true;
		if (block_it == rhs.block_it) {
			return it < rhs.it;
		}
		return block_it < rhs.block_it;
	}
	bool operator!=(const MIIterator& rhs) const { return !(*this == rhs); }
	bool operator>( const MIIterator& rhs) const { return rhs < *this; }
	reference       operator*()        { return *it; }
	const reference operator*()  const { return *it; }
	pointer         operator->()       { return &*it; }
	const pointer   operator->() const { return &*it; }

	bool is_end() const;

	friend class MachineBasicBlock;
};

inline bool operator<=(const MIIterator &lhs, const MIIterator& rhs) {
	return !(rhs < lhs);
}
inline bool operator>(const MIIterator &lhs, const MIIterator& rhs) {
	return !(lhs <= rhs);
}
inline bool operator>=(const MIIterator &lhs, const MIIterator& rhs) {
	return !(lhs < rhs);
}

/**
 * A basic block of (scheduled) machine instructions.
 *
 * A MachineBasicBlock contains an ordered collection of
 * MachineInstructions. These MachineInstructions can be accessed via
 * MIIterators. These "smart-iterators" are not only used for access
 * but also for ordering MachineInstructions.
 *
 * Once a MachineInstruction is added, the MachineBasicBlock takes over
 * the responsability for deleting it.
 *
 * @ingroup low-level-ir
 */
class MachineBasicBlock {
public:
	typedef ordered_list<MachineInstruction*>::iterator iterator;
	typedef ordered_list<MachineInstruction*>::const_iterator const_iterator;
	typedef ordered_list<MachineInstruction*>::reference reference;
	typedef ordered_list<MachineInstruction*>::const_reference const_reference;
	typedef ordered_list<MachineInstruction*>::pointer pointer;
	typedef ordered_list<MachineInstruction*>::const_pointer const_pointer;

	typedef ordered_list<MachineInstruction*>::reverse_iterator
		reverse_iterator;
	typedef ordered_list<MachineInstruction*>::const_reverse_iterator
		const_reverse_iterator;

	typedef std::list<MachinePhiInst*> PhiListTy;
	typedef PhiListTy::const_iterator const_phi_iterator;

	typedef std::vector<MachineBasicBlock*> PredListTy;
	typedef PredListTy::const_iterator const_pred_iterator;
	typedef PredListTy::iterator pred_iterator;

	/// construct an empty MachineBasicBlock
	MachineBasicBlock(const MBBIterator &my_it)
		: id(id_counter++),  my_it(my_it) {};
	/// checks if the basic block has no elements.
	bool empty() const;
	/// returns the number of elements
	std::size_t size() const;
	/// Appends the given element value to the end of the container.
	void push_back(MachineInstruction* value);
	/// inserts value to the beginning
	void push_front(MachineInstruction* value);
	/// inserts value before the element pointed to by pos
	void insert_before(iterator pos, MachineInstruction* value);
	/// inserts value after the element pointed to by pos
	void insert_after(iterator pos, MachineInstruction* value);
	/// inserts elements from range [first, last) before pos
	template<class InputIt>
	void insert_before(iterator pos, InputIt first, InputIt last);
	/// inserts elements from range [first, last) after pos
	template<class InputIt>
	void insert_after(iterator pos, InputIt first, InputIt last);
	/// erases element
	iterator erase(iterator pos);
	/// erases elements
	iterator erase(iterator first, iterator last);
	/// returns an iterator to the beginning
	iterator begin();
	/// returns an iterator to the end
	iterator end();
	/// returns an const iterator to the beginning
	const_iterator begin() const;
	/// returns an const iterator to the end
	const_iterator end() const;
	/// returns an reverse_iterator to the beginning
	reverse_iterator rbegin();
	/// returns an reverse_iterator to the end
	reverse_iterator rend();
	/// returns an const reverse_iterator to the beginning
	const_reverse_iterator rbegin() const;
	/// returns an const reverse_iterator to the end
	const_reverse_iterator rend() const;
	/// access the first element
	const_reference front() const;
	/// access the last element
	const_reference back() const;
	/// access the first element
	reference front();
	/// access the last element
	reference back();

	/// Appends the given element value to the list of phis.
	void insert_phi(MachinePhiInst* value);
	/// returns an const iterator to the phi instructions
	const_phi_iterator phi_begin() const;
	/// returns an const iterator to the phi instructions
	const_phi_iterator phi_end() const;
	/// returns the number of phi nodes
	std::size_t phi_size() const;

	/**
	 * Appends the given element value to the list of predecessors.
	 * @note Ensure that the PHI instructions are updated as well.
	 */
	void insert_pred(MachineBasicBlock* value);
	/// returns an const iterator to the predecessors
	const_pred_iterator pred_begin() const;
	/// returns an const iterator to the predecessors
	const_pred_iterator pred_end() const;
	/// returns an iterator to the predecessors
	pred_iterator pred_begin();
	/// returns an iterator to the predecessors
	pred_iterator pred_end();
	/// returns the number of predecessor nodes
	std::size_t pred_size() const;
	/// Get the i'th predecessor
	MachineBasicBlock* get_predecessor(std::size_t i) const;
	/**
	 * Get predecessor index
	 * @return the predecessor index of MBB or pred_size() if not found
	 */
	std::size_t get_predecessor_index(MachineBasicBlock* MBB) const;

	/// get a MIIterator form a iterator
	MIIterator convert(iterator pos);
	/// get a MIIterator form a iterator
	MIIterator convert(reverse_iterator pos);
	/// returns an MIIterator to the first element
	MIIterator mi_first();
	/// returns an MIIterator to the last element (included)
	MIIterator mi_last();
	/// get self iterator
	MBBIterator self_iterator() const;
	/// get parent
	MachineInstructionSchedule* get_parent() const;
	/// print
	OStream& print(OStream& OS) const;
	/// equality operator
	bool operator==(const MachineBasicBlock &other) const;
private:
	/// update instruction block
	void update(MachineInstruction *MI);

	static std::size_t id_counter;
	std::size_t id;
	MBBIterator my_it;
	/// empty constructor
	MachineBasicBlock() : id(id_counter++) {}
	ordered_list<MachineInstruction*> list;
	PhiListTy phi;
	PredListTy predecessors;

	friend class MBBBuilder;
	friend class MachineInstructionSchedule;
};

inline MIIterator& MIIterator::operator++() {
	++it;
	if (it == (*block_it)->end()) {
		// end of basic block
		++block_it;
		// Note: by convention MachineBasicBlock blocks are not
		// allowed to be empty. Therefor we can safely assume
		// that (*block_it)->begin() != (*block_it)->end().
		if (block_it == block_it.get_parent()->end()) {
			it = _end;
		}
		else {
			it = (*block_it)->begin();
		}
	}
	return *this;
}
inline MIIterator& MIIterator::operator--() {
	if (it == _end || it == (*block_it)->begin()) {
		// begin of basic block
		// XXX I think this can be removed. Iterating beyond begin
		// is undefined.
		if (block_it == block_it.get_parent()->begin()) {
			return *this;
		}
		--block_it;
		it = (*block_it)->end();
	}
	--it;
	return *this;
}

inline bool MIIterator::is_end() const {
	assert(*block_it);
	assert((*block_it)->get_parent());
	return *this == (*block_it)->get_parent()->mi_end();
}

OStream& operator<<(OStream&, const MIIterator&);

bool check_is_phi(MachineInstruction *value);

inline bool MachineBasicBlock::empty() const {
	return list.empty();
}
inline std::size_t MachineBasicBlock::size() const {
	return list.size();
}
inline void MachineBasicBlock::push_back(MachineInstruction* value) {
	assert(!check_is_phi(value));
	update(value);
	list.push_back(value);
}
inline void MachineBasicBlock::push_front(MachineInstruction* value) {
	assert(!check_is_phi(value));
	update(value);
	list.push_front(value);
}
inline void MachineBasicBlock::insert_before(iterator pos, MachineInstruction* value) {
	assert(!check_is_phi(value));
	update(value);
	list.insert(pos,value);
}
inline void MachineBasicBlock::insert_after(iterator pos, MachineInstruction* value) {
	assert(!check_is_phi(value));
	update(value);
	list.insert(++pos,value);
}
template<class InputIt>
inline void MachineBasicBlock::insert_before(iterator pos,
		InputIt first, InputIt last) {
	assert(none_of(first,last,check_is_phi));
	list.insert(pos,first,last);
	std::for_each(first,last,
		std::bind1st(std::mem_fun(&MachineBasicBlock::update), this));
}
template<class InputIt>
inline void MachineBasicBlock::insert_after(iterator pos,
		InputIt first, InputIt last) {
	assert(none_of(first,last,check_is_phi));
	list.insert(++pos,first,last);
	std::for_each(first,last,
		std::bind1st(std::mem_fun(&MachineBasicBlock::update), this));
}
inline MachineBasicBlock::iterator
MachineBasicBlock::erase(iterator pos) {
	return list.erase(pos);
}
inline MachineBasicBlock::iterator
MachineBasicBlock::erase(iterator first, iterator last) {
	return list.erase(first,last);
}
inline MachineBasicBlock::iterator MachineBasicBlock::begin() {
	return list.begin();
}
inline MachineBasicBlock::iterator MachineBasicBlock::end() {
	return list.end();
}
inline MachineBasicBlock::const_iterator MachineBasicBlock::begin() const {
	return list.begin();
}
inline MachineBasicBlock::const_iterator MachineBasicBlock::end() const {
	return list.end();
}
inline MachineBasicBlock::reverse_iterator MachineBasicBlock::rbegin() {
	return list.rbegin();
}
inline MachineBasicBlock::reverse_iterator MachineBasicBlock::rend() {
	return list.rend();
}
inline MachineBasicBlock::const_reverse_iterator MachineBasicBlock::rbegin() const {
	return list.rbegin();
}
inline MachineBasicBlock::const_reverse_iterator MachineBasicBlock::rend() const {
	return list.rend();
}
inline MachineBasicBlock::const_reference MachineBasicBlock::front() const {
	return list.front();
}
inline MachineBasicBlock::const_reference MachineBasicBlock::back() const {
	return list.back();
}
inline MachineBasicBlock::reference MachineBasicBlock::front() {
	return list.front();
}
inline MachineBasicBlock::reference MachineBasicBlock::back() {
	return list.back();
}
inline MIIterator MachineBasicBlock::convert(iterator pos) {
	if (pos == end()) {
		return ++convert(--pos);
	}
	return MIIterator(my_it,pos);
}
inline MIIterator MachineBasicBlock::convert(reverse_iterator pos) {
	return convert(--(pos.base()));
}
inline MIIterator MachineBasicBlock::mi_first() {
	assert(!empty());
	return convert(begin());
}
inline MIIterator MachineBasicBlock::mi_last() {
	assert(!empty());
	return convert(--end());
}
inline MBBIterator MachineBasicBlock::self_iterator() const {
	return my_it;
}
inline MachineInstructionSchedule* MachineBasicBlock::get_parent() const {
	return my_it.get_parent();
}

inline OStream& operator<<(OStream& OS, MachineBasicBlock& MBB) {
	return MBB.print(OS);
}
inline void MachineBasicBlock::insert_phi(MachinePhiInst* value) {
	phi.push_back(value);
}
inline MachineBasicBlock::const_phi_iterator MachineBasicBlock::phi_begin() const {
	return phi.begin();
}
inline MachineBasicBlock::const_phi_iterator MachineBasicBlock::phi_end() const {
	return phi.end();
}
inline std::size_t MachineBasicBlock::phi_size() const {
	return phi.size();
}

inline void MachineBasicBlock::insert_pred(MachineBasicBlock* value) {
	predecessors.push_back(value);
}
inline MachineBasicBlock::const_pred_iterator MachineBasicBlock::pred_begin() const {
	return predecessors.begin();
}
inline MachineBasicBlock::const_pred_iterator MachineBasicBlock::pred_end() const {
	return predecessors.end();
}
inline MachineBasicBlock::pred_iterator MachineBasicBlock::pred_begin() {
	return predecessors.begin();
}
inline MachineBasicBlock::pred_iterator MachineBasicBlock::pred_end() {
	return predecessors.end();
}
inline std::size_t MachineBasicBlock::pred_size() const {
	return predecessors.size();
}
inline MachineBasicBlock* MachineBasicBlock::get_predecessor(std::size_t i) const {
	assert(i < predecessors.size());
	return predecessors[i];
}
inline std::size_t
MachineBasicBlock::get_predecessor_index(MachineBasicBlock* MBB) const {
	std::size_t i = 0, e = pred_size();
	for (; i < e; ++i) {
		if (predecessors[i] == MBB) {
			return i;
		}
	}
	return i;
}

inline bool MachineBasicBlock::operator==(const MachineBasicBlock &other) const {
	//return this == &other;
	return id == other.id;
}

struct MoveEdgeFunctor : public std::unary_function<MachineInstruction*,void> {
	MachineBasicBlock &from;
	MachineBasicBlock &to;
	MoveEdgeFunctor(MachineBasicBlock &from, MachineBasicBlock &to)
		: from(from), to(to) {}
	void operator()(MachineInstruction* MI);
};

/**
 * Move instructions to another basic block.
 *
 * @note this also updates the predecessor list for the target basic blocks
 * of jump instructions. Example if a JUMP to BB_target is moved from
 * BB_from to BB_to than all occurrences of BB_from in BB_targets predecessor
 * list are replaced with BB_to.
 */
template <class InputIterator>
inline void move_instructions(InputIterator first, InputIterator last,
		MachineBasicBlock &from, MachineBasicBlock &to) {
	MoveEdgeFunctor Fn(from,to);
	std::for_each(first,last,Fn);
	to.insert_before(to.end(),first,last);
	from.erase(first,last);
}

MachinePhiInst* get_phi_from_operand(MachineBasicBlock *MBB,
	MachineOperand* op);


} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_MACHINEBASICBLOCK */


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
