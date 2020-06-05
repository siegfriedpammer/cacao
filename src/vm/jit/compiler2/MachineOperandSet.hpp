/* src/vm/jit/compiler2/MachineOperandSet.hpp - MachineOperandSet

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

#ifndef _JIT_COMPILER2_MACHINEOPERANDSET
#define _JIT_COMPILER2_MACHINEOPERANDSET

#include <utility>

#include <boost/dynamic_bitset.hpp>

#include "vm/jit/compiler2/MachineOperand.hpp"
#include "vm/jit/compiler2/memory/Manager.hpp"
#include "vm/types.hpp"

MM_MAKE_NAME(OperandSet)

namespace cacao {
namespace jit {
namespace compiler2 {

class MachineOperandFactory;

/// OperandList is just an alias for a std::vector,
/// mainly used for OperandSets that need to be sorted
using OperandList = std::vector<MachineOperand*>;

/**
 * OperandSets are basically bitsets mapped on top of
 * all operands created by a MachineOperandFactory.
 *
 * The factory is backed by a vector of MachineOperands. If a operand
 * is in a set, the bit corresponding to the index in the vector is true.
 *
 * Mainly used for fast "set" operations (union, intersection, etc.)
 *
 * OperandSet also provides iterators that behave according to ForwardIter.
 *
 * Important, since this is a set, it can NOT be sorted.
 */
class OperandSet : public memory::ManagerMixin<OperandSet> {
public:
	using size_t = boost::dynamic_bitset<>::size_type;

	OperandSet() = delete;
	OperandSet(const OperandSet&) = default;
	OperandSet(OperandSet&&) = default;
	OperandSet& operator=(const OperandSet&) = default;

	template <typename Value, typename Set>
	class Iterator {
	public:
		// ForwardIter must be DefaultConstructible
		Iterator() : Iterator(nullptr) {}
		Iterator(const Iterator&) = default;
		Iterator(Iterator&&) = default;
		Iterator& operator=(const Iterator&) = default;

		// iterator traits
		using difference_type = long;
		using value_type = Value;
		using pointer = Value*;
		using reference = Value&;
		using iterator_category = std::forward_iterator_tag;

		/// Iterators need to be swappable
		void swap(Iterator& other);

		reference operator*();
		pointer operator->();
		Iterator& operator++();
		Iterator operator++(int);

		bool operator==(const Iterator&);
		bool operator!=(const Iterator&);

	private:
		Iterator(Set set, size_t index = boost::dynamic_bitset<>::npos) : set(set), index(index) {}
		Set set;

		size_t index;

		friend class OperandSet;
	};

	using const_iterator = Iterator<const MachineOperand, const OperandSet*>;
	using iterator = Iterator<MachineOperand, OperandSet*>;

	/// Returns the number of operands that "could" be in this set.
	/// That is, the number of operands in the factory at the time when
	/// this set was created.
	size_t max_size() const { return set.size(); }

	/// Returns the number of operands currently in the set
	size_t size() const { return set.count(); }

	/// Returns true, iff no operands are in the set
	bool empty() const { return set.none(); }

	/// Adds a operand to the set
	void add(const MachineOperand*);

	/// Removes a operand from the set
	void remove(const MachineOperand*);

	/// Returns true, iff the operand is in the set
	bool contains(const MachineOperand*) const;

	/// Removes all operands from the set
	void clear();

	/// Provide swap function
	void swap(OperandSet& other);

	/// Set difference, every element in other is removed from *this
	OperandSet& operator-=(const OperandSet& other);

	/// Set union, adds all elements in other to *this
	OperandSet& operator|=(const OperandSet& other);

	/// Set intersection, removes all elements that are not in other
	OperandSet& operator&=(const OperandSet& other);

	/// Returns a new OperandSet that is the set difference of lhs and rhs
	friend OperandSet operator-(const OperandSet& lhs, const OperandSet& rhs)
	{
		OperandSet result(lhs);
		result -= rhs;
		return result;
	}

	/// Returns a new OperandSet that is the set union of lhs and rhs
	friend OperandSet operator|(const OperandSet& lhs, const OperandSet& rhs)
	{
		OperandSet result(lhs);
		result |= rhs;
		return result;
	}

	/// Returns a new OperandSet that is the set intersection of lhs and rhs
	friend OperandSet operator&(const OperandSet& lhs, const OperandSet& rhs)
	{
		OperandSet result(lhs);
		result &= rhs;
		return result;
	}

	const_iterator cbegin() const { return const_iterator(this, set.find_first()); }
	const_iterator cend() const { return const_iterator(this); }

	iterator begin() { return iterator(this, set.find_first()); }
	iterator end() { return iterator(this); }
	const_iterator begin() const { return cbegin(); }
	const_iterator end() const { return cend(); }

	std::unique_ptr<OperandList> ToList();
	OperandSet& operator=(const OperandList&);


private:
	OperandSet(const MachineOperandFactory* MOF, size_t size) : set(size), MOF(MOF) {}

	boost::dynamic_bitset<> set;
	const MachineOperandFactory* MOF;

	MachineOperand* get(size_t index);
	const MachineOperand* get(size_t index) const;

	template <typename Value, typename Set>
	friend class Iterator;
	friend class MachineOperandFactory;
	template <typename Q>
	friend class ExtendedOperandSet;
};

/**
 * Class that embeds a OperandSet itself, but also associates a user-provided
 * type with each MachineOperand in the set.
 */
template <typename T>
class ExtendedOperandSet : public memory::ManagerMixin<OperandSet> {
public:
	ExtendedOperandSet() = delete;
	ExtendedOperandSet(const ExtendedOperandSet&) = default;
	ExtendedOperandSet(ExtendedOperandSet&&) = default;
	ExtendedOperandSet& operator=(const ExtendedOperandSet&) = default;

	void swap(ExtendedOperandSet<T>& other);

	void insert(const MachineOperand& operand, const T& value)
	{
		assert(!operand_set.contains(&operand));

		insert_or_update(operand, value);
	}

	void insert_or_update(const MachineOperand& operand, const T& value)
	{
		if (!operand_set.contains(&operand)) {
			operand_set.add(&operand);
		}
		data[operand.get_id()] = value;
	}

	T& operator[](const MachineOperand& operand)
	{
		assert(contains(operand));
		return data[operand.get_id()];
	}

	const T& operator[](const MachineOperand& operand) const
	{
		assert(contains(operand));
		return data[operand.get_id()];
	}

	void remove(const MachineOperand& operand) { operand_set.remove(&operand); }

	void remove(const OperandSet& other)
	{
		assert(operand_set.max_size() == other.max_size());
		operand_set -= other;
	}

	bool contains(const MachineOperand& operand) const { return operand_set.contains(&operand); }

	void clear() { operand_set.clear(); }

	using const_iterator = OperandSet::const_iterator;
	using iterator = OperandSet::iterator;

	const_iterator cbegin() const { return operand_set.cbegin(); }
	const_iterator cend() const { return operand_set.cend(); }

	iterator begin() { return operand_set.begin(); }
	iterator end() { return operand_set.end(); }

	const_iterator begin() const { return cbegin(); }
	const_iterator end() const { return cend(); }

	/// Returns the underlying OperandSet
	const OperandSet& GetOperandSet() const { return operand_set; }

private:
	ExtendedOperandSet(const MachineOperandFactory* MOF, OperandSet::size_t size)
	    : MOF(MOF), operand_set(MOF, size), data(size)
	{
	}

	const MachineOperandFactory* MOF;
	OperandSet operand_set;
	std::vector<T> data;

	friend class MachineOperandFactory;
};

inline void swap(OperandSet& lhs, OperandSet& rhs)
{
	lhs.swap(rhs);
}

template <typename Value, typename Set>
void swap(OperandSet::Iterator<Value, Set>& lhs, OperandSet::Iterator<Value, Set>& rhs)
{
	lhs.swap(rhs);
}

template <typename V, typename S>
void OperandSet::Iterator<V, S>::swap(OperandSet::Iterator<V, S>& other)
{
	assert(set == other.set && "Swapping iterators from differenct containers is not supported!");
	using std::swap;
	swap(index, other.index);
}

template <typename V, typename S>
typename OperandSet::Iterator<V, S>::reference OperandSet::Iterator<V, S>::operator*()
{
	assert(index >= 0 && index < set->max_size() && "De-referencing past-the-end iterator!");
	return *set->get(index);
}

template <typename V, typename S>
typename OperandSet::Iterator<V, S>::pointer OperandSet::Iterator<V, S>::operator->()
{
	assert(index >= 0 && index < set->max_size() && "De-referencing past-the-end iterator!");
	return set->get(index);
}

template <typename V, typename S>
OperandSet::Iterator<V, S>& OperandSet::Iterator<V, S>::operator++()
{
	assert(index < set->max_size() && "Incrementing end-iterator not allowed!");
	index = set->set.find_next(index);
	return *this;
}

template <typename V, typename S>
OperandSet::Iterator<V, S> OperandSet::Iterator<V, S>::operator++(int)
{
	assert(index < set->max_size() && "Incrementing end-iterator not allowed!");
	OperandSet::Iterator<V, S> return_iter = *this;
	++(*this);
	return return_iter;
}

template <typename V, typename S>
bool OperandSet::Iterator<V, S>::operator==(const OperandSet::Iterator<V, S>& other)
{
	return index == other.index;
}

template <typename V, typename S>
bool OperandSet::Iterator<V, S>::operator!=(const OperandSet::Iterator<V, S>& other)
{
	return !(*this == other);
}

//
// ExtendedOperandSet template methods
//

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_MACHINEOPERANDSET */

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
