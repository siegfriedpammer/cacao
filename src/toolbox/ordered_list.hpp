/* src/toolbox/ordered_list.hpp - ordered list

   Copyright (C) 1996-2013
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


#ifndef ORDEREDLIST_HPP_
#define ORDEREDLIST_HPP_ 1

#include <list>
#include <algorithm>

namespace cacao {

/**
 * An ordered_list is an indexed sequence container. It shares most
 * characteristics with a std::list with the difference that its iterators are
 * more powerful. In addition the features of bidirectional iterators the total
 * ordering relation is supported (hence the name ordered_list).
 *
 * Like std::list it supports constant time insertion and removal of elements
 * anywhere. Additionally those operations do not invalidate iterators.
 *
 * ordered_list meets the requirement of Container, AllocatorAwareContainer,
 * SequenceContainer, ReversibleContainer.
 *
 * @todo not all methods are implemented yet!
 */
template <class T,class Allocator = std::allocator<T> >
class ordered_list {
private:
	/// tagged list entry
	struct Entry {
		Entry(T value, std::size_t index) : value(value), index(index) {}
		T value;
		std::size_t index;
		bool operator==(const Entry& other) const {
			return index == other.index && value == other.value;
		}
	};
	/// Increment function struct
	struct IncrementEntry {
		void operator()(Entry& E) {
			++E.index;;
		}
	};
	/// internal storage
	std::list<Entry> list;

	typedef typename std::list<Entry>::iterator intern_iterator;
public:
	typedef T value_type;
	typedef Allocator allocator_type;
	typedef typename allocator_type::reference reference;
	typedef typename allocator_type::const_reference const_reference;
	typedef typename allocator_type::pointer pointer;
	typedef typename allocator_type::const_pointer const_pointer;
	class iterator;

	/// construct an empty MachineBasicBlock
	ordered_list() {}
	/// copy constructor
	ordered_list(const ordered_list<T,Allocator> &other) : list(other.list) {}
	/// copy assignment operator
	ordered_list& operator=(const ordered_list<T,Allocator> &other);
	/// checks if the container has no elements.
	bool empty() const;
	/// returns the number of elements
	std::size_t size() const;
	/// returns the maximum possible number of elements
	std::size_t max_size() const;
	/// Appends the given element value to the end of the container.
	void push_back(const T& value);
	/// inserts value to the beginning
	void push_front(const T& value);
	/// inserts value before the element pointed to by pos
	void insert(iterator pos, const T& value);
	/// returns an iterator to the beginning
	iterator begin();
	/// returns an iterator to the end
	iterator end();
	/// removes all elements from the container
	void clear();
	/// exchanges the contents of the container with those of other
	void swap(ordered_list<T,Allocator> &other);

	/// iterator
	class iterator : public std::iterator<std::bidirectional_iterator_tag,T> {
	public:
		iterator() {}
		iterator(intern_iterator it) : it(it) {}
		iterator(const iterator& other) : it(other.it) {}
		iterator& operator++() {
			++it;
			return *this;
		}
		iterator operator++(int) {
			iterator tmp(*this);
			operator++();
			return tmp;
		}
		iterator& operator--() {
			--it;
			return *this;
		}
		iterator operator--(int) {
			iterator tmp(*this);
			operator--();
			return tmp;
		}
		bool operator==(const iterator& rhs) const { return it == rhs.it; }
		bool operator!=(const iterator& rhs) const { return it != rhs.it; }
		bool operator<( const iterator& rhs) const { return it->index < rhs.it->index; }
		bool operator>( const iterator& rhs) const { return rhs < *this; }
		T& operator*()        { return it->value; }
		const T& operator*()  const { return it->value; }
		T* operator->()       { return it->value; }
		const T* operator->() const { return it->value; }

	private:
		intern_iterator it;
		friend class ordered_list<T,Allocator>;
	};

	// friends
	template<class _T,class _Allocator>
	friend bool operator==(const ordered_list<_T,_Allocator>& lhs,
						   const ordered_list<_T,_Allocator>& rhs);
	template<class _T,class _Allocator>
	friend bool operator==(const typename ordered_list<_T,_Allocator>::Entry& lhs,
						   const typename ordered_list<_T,_Allocator>::Entry& rhs);

};

/// equality
template< class T, class Allocator>
inline bool operator==(const ordered_list<T,Allocator>& lhs,
                       const ordered_list<T,Allocator>& rhs) {
	return lhs.list == rhs.list;
}

/// inequality
template< class T, class Allocator>
inline bool operator!=(const ordered_list<T,Allocator>& lhs,
                       const ordered_list<T,Allocator>& rhs) {
	return !(lhs == rhs);
}

// implementations
template <class T, class Allocator>
inline ordered_list<T,Allocator>&
ordered_list<T,Allocator>::operator=(const ordered_list<T,Allocator> &other) {
	list = other.list;
	return *this;
}

template <class T, class Allocator>
inline bool ordered_list<T, Allocator>::empty() const {
	return list.empty();
}

template <class T, class Allocator>
inline std::size_t ordered_list<T, Allocator>::size() const {
	return list.size();
}

template <class T, class Allocator>
inline std::size_t ordered_list<T, Allocator>::max_size() const {
	return list.max_size();
}

template <class T, class Allocator>
inline void ordered_list<T, Allocator>::push_back(const T& value) {
	list.push_back(Entry(value,size()));
}

template <class T, class Allocator>
inline void ordered_list<T, Allocator>::push_front(const T& value) {
	std::for_each(list.begin(),list.end(),IncrementEntry());
	list.push_front(Entry(value,0));
}

template <class T, class Allocator>
inline void ordered_list<T, Allocator>::insert(typename ordered_list<T, Allocator>::iterator pos, const T& value) {
	list.insert(pos.it,Entry(value,pos.it->index));
	std::for_each(pos.it,list.end(),IncrementEntry());
}

template <class T, class Allocator>
inline typename ordered_list<T, Allocator>::iterator ordered_list<T, Allocator>::begin() {
	return list.begin();
}

template <class T, class Allocator>
inline typename ordered_list<T, Allocator>::iterator ordered_list<T, Allocator>::end() {
	return list.end();
}

template <class T, class Allocator>
inline void ordered_list<T, Allocator>::clear() {
	return list.clear();
}

template <class T, class Allocator>
inline void ordered_list<T, Allocator>::swap(ordered_list<T,Allocator> &other) {
	list.swap(other.list);
}

} // end namespace cacao

#endif // ORDEREDLIST_HPP


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
