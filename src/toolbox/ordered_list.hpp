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

// forward declaration
template<class T, class Allocator, class intern_list>
class _ordered_iterator;
template<class T, class Allocator, class intern_list>
class _ordered_const_iterator;

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
	struct Entry;
	typedef std::list<Entry,typename Allocator::template rebind<Entry>::other> intern_list;
	typedef typename intern_list::iterator intern_iterator;

	/// tagged list entry
	struct Entry {
		Entry(T value, std::size_t index) : value(value), index(index) {}
		T value;
		std::size_t index;
		bool operator==(const Entry& other) const {
			return index == other.index && value == other.value;
		}
	};
	/// Inserter
	struct Inserter {
		intern_list &list;
		intern_iterator pos;
		std::size_t index;
		Inserter(intern_list &list, intern_iterator pos)
			: list(list), pos(pos), index(pos->index) {}
		void operator()(T& value) {
			list.insert(pos, Entry(value,index++));
		}
	};
	/// Increment function struct
	struct IncrementEntry {
		void operator()(Entry& E) {
			++E.index;
		}
	};
	/// Increasing function struct
	struct IncreasingEntry {
		std::size_t index;
		IncreasingEntry(std::size_t index) : index(index) {}
		void operator()(Entry& E) {
			E.index = index++;
		}
	};
	/// Decrement function struct
	struct DecrementEntry {
		void operator()(Entry& E) {
			--E.index;
		}
	};
	/// internal storage
	intern_list list;

public:
	typedef T value_type;
	typedef Allocator allocator_type;
	typedef typename allocator_type::reference reference;
	typedef typename allocator_type::const_reference const_reference;
	typedef typename allocator_type::pointer pointer;
	typedef typename allocator_type::const_pointer const_pointer;
	typedef _ordered_iterator<T, Allocator, intern_list > iterator;
	typedef _ordered_const_iterator<T, Allocator, intern_list > const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

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
	iterator insert(iterator pos, const T& value);
	/// inserts elements from range [first, last) before the element pointed to by pos
	template<class InputIt>
	void insert(iterator pos, InputIt first, InputIt last);
	/// erases element
	iterator erase(iterator pos );
	/// erases elements
	iterator erase(iterator first, iterator last);
	/// returns an iterator to the beginning
	iterator begin();
	/// returns an iterator to the end
	iterator end();
	/// returns a const_iterator to the beginning
	const_iterator begin() const;
	/// returns a const_iterator to the end
	const_iterator end() const;
	/// returns a reverse iterator to the beginning
	reverse_iterator rbegin();
	/// returns a reverse iterator to the end
	reverse_iterator rend();
	/// returns a const reverse iterator to the beginning
	const_reverse_iterator rbegin() const;
	/// returns a const reverse iterator to the end
	const_reverse_iterator rend() const;
	/// access the first element
	const_reference front() const;
	/// access the last element
	const_reference back() const;
	/// access the first element
	reference front();
	/// access the last element
	reference back();
	/// removes all elements from the container
	void clear();
	/// exchanges the contents of the container with those of other
	void swap(ordered_list<T,Allocator> &other);

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


/**
 * ordered iterator
 */
template<class T, class Allocator, class intern_list>
class _ordered_iterator : public std::iterator<std::bidirectional_iterator_tag,T> {
public:
	typedef typename std::iterator<std::bidirectional_iterator_tag,T>
		::reference reference;
	typedef typename std::iterator<std::bidirectional_iterator_tag,T>
		::pointer pointer;
	_ordered_iterator() {}
	_ordered_iterator(typename intern_list::iterator it) : it(it) {}
	_ordered_iterator(const _ordered_iterator& other) : it(other.it) {}
	_ordered_iterator& operator++() {
		++it;
		return *this;
	}
	_ordered_iterator operator++(int) {
		_ordered_iterator tmp(*this);
		operator++();
		return tmp;
	}
	_ordered_iterator& operator--() {
		--it;
		return *this;
	}
	_ordered_iterator operator--(int) {
		_ordered_iterator tmp(*this);
		operator--();
		return tmp;
	}
	bool operator==(const _ordered_iterator& rhs) const { return it == rhs.it; }
	bool operator!=(const _ordered_iterator& rhs) const { return it != rhs.it; }
	bool operator<( const _ordered_iterator& rhs) const { return it->index < rhs.it->index; }
	bool operator>( const _ordered_iterator& rhs) const { return rhs < *this; }
	reference       operator*()        { return it->value; }
	const reference operator*()  const { return it->value; }
	pointer         operator->()       { return it->value; }
	const pointer   operator->() const { return it->value; }
private:
	typename intern_list::iterator it;
	friend class ordered_list<T,Allocator>;
	friend class _ordered_const_iterator<T,Allocator,intern_list>;
};

/**
 * ordered const_iterator
 */
template<class T, class Allocator, class intern_list>
class _ordered_const_iterator : public std::iterator<std::bidirectional_iterator_tag,const T> {
public:
	typedef typename std::iterator<std::bidirectional_iterator_tag,const T>
		::reference reference;
	typedef typename std::iterator<std::bidirectional_iterator_tag,const T>
		::pointer pointer;
	_ordered_const_iterator() {}
	_ordered_const_iterator(typename intern_list::const_iterator it) : it(it) {}
	_ordered_const_iterator(const _ordered_const_iterator& other) : it(other.it) {}
	// convert from non const version
	_ordered_const_iterator(const _ordered_iterator<T,Allocator,
			intern_list>& other) : it(other.it) {}
	_ordered_const_iterator& operator++() {
		++it;
		return *this;
	}
	_ordered_const_iterator operator++(int) {
		_ordered_const_iterator tmp(*this);
		operator++();
		return tmp;
	}
	_ordered_const_iterator& operator--() {
		--it;
		return *this;
	}
	_ordered_const_iterator operator--(int) {
		_ordered_const_iterator tmp(*this);
		operator--();
		return tmp;
	}
	bool operator==(const _ordered_const_iterator& rhs) const { return it == rhs.it; }
	bool operator!=(const _ordered_const_iterator& rhs) const { return it != rhs.it; }
	bool operator<( const _ordered_const_iterator& rhs) const { return it->index < rhs.it->index; }
	bool operator>( const _ordered_const_iterator& rhs) const { return rhs < *this; }
	reference       operator*()        { return it->value; }
	const reference operator*()  const { return it->value; }
	pointer         operator->()       { return it->value; }
	const pointer   operator->() const { return it->value; }
private:
	typename intern_list::const_iterator it;
	friend class ordered_list<T,Allocator>;
};

// implementation
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
inline typename ordered_list<T, Allocator>::iterator
ordered_list<T, Allocator>::insert(typename ordered_list<T, Allocator>::iterator pos,
		const T& value) {
	typename intern_list::iterator it = list.insert(pos.it,Entry(value,pos.it->index));
	std::for_each(pos.it,list.end(),IncrementEntry());
	return iterator(it);
}

template <class T, class Allocator>
template <class InputIt>
inline void ordered_list<T, Allocator>::insert(
		typename ordered_list<T, Allocator>::iterator pos,
		InputIt first, InputIt last) {
	typename intern_list::iterator it = pos.it;
	std::for_each(first,last,Inserter(list,pos.it));
	std::for_each(it,list.end(),IncreasingEntry((--it)->index+1));
}

template <class T, class Allocator>
inline typename ordered_list<T, Allocator>::iterator
ordered_list<T, Allocator>::erase(
		typename ordered_list<T, Allocator>::iterator pos) {
	typename intern_list::iterator it = list.erase(pos.it);
	std::for_each(it,list.end(),DecrementEntry());
	return it;
}

template <class T, class Allocator>
inline typename ordered_list<T, Allocator>::iterator
ordered_list<T, Allocator>::erase(
		typename ordered_list<T, Allocator>::iterator first,
		typename ordered_list<T, Allocator>::iterator last) {
	std::size_t index = first.it->index;
	typename intern_list::iterator it = list.erase(first.it,last.it);
	std::for_each(it,list.end(),IncreasingEntry(index));
	return it;
}

template <class T, class Allocator>
inline typename ordered_list<T, Allocator>::iterator ordered_list<T, Allocator>::begin() {
	return list.begin();
}

template <class T, class Allocator>
inline typename ordered_list<T, Allocator>::const_iterator
ordered_list<T, Allocator>::begin() const {
	return list.begin();
}

template <class T, class Allocator>
inline typename ordered_list<T, Allocator>::iterator ordered_list<T, Allocator>::end() {
	return list.end();
}

template <class T, class Allocator>
inline typename ordered_list<T, Allocator>::const_iterator
ordered_list<T, Allocator>::end() const {
	return list.end();
}

template <class T, class Allocator>
inline typename ordered_list<T, Allocator>::reverse_iterator
ordered_list<T, Allocator>::rbegin() {
	return list.rbegin();
}

template <class T, class Allocator>
inline typename ordered_list<T, Allocator>::reverse_iterator
ordered_list<T, Allocator>::rend() {
	return list.rend();
}

template <class T, class Allocator>
inline typename ordered_list<T, Allocator>::const_reverse_iterator
ordered_list<T, Allocator>::rbegin() const {
	return list.rbegin();
}

template <class T, class Allocator>
inline typename ordered_list<T, Allocator>::const_reverse_iterator
ordered_list<T, Allocator>::rend() const {
	return list.rend();
}

template <class T, class Allocator>
inline typename ordered_list<T, Allocator>::reference
ordered_list<T, Allocator>::front() {
	return list.front().value;
}
template <class T, class Allocator>
inline typename ordered_list<T, Allocator>::reference
ordered_list<T, Allocator>::back() {
	return list.back().value;
}
template <class T, class Allocator>
inline typename ordered_list<T, Allocator>::const_reference
ordered_list<T, Allocator>::front() const {
	return list.front().value;
}
template <class T, class Allocator>
inline typename ordered_list<T, Allocator>::const_reference
ordered_list<T, Allocator>::back() const {
	return list.back().value;
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
