/* src/toolbox/intern_table.hpp - Intern table header

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

#ifndef INTERN_TABLE_HPP_
#define INTERN_TABLE_HPP_ 1

#include "config.h"

#include <cassert>
#include <functional>

#include "threads/thread.hpp"
#include "threads/mutex.hpp"

/* InternTable *****************************************************************

	A specialized hash map that allows for parallel access by multiple threads.
	This is achieved by splitting up the table into multiple segments. Each
	segment is a separate hash table protected by it's own lock.
	We need no separate global lock for accessing segments because there is a
	fixed number of segments.

	Every segment itself is a simple hash table with open addressing.

	TEMPLAT PARAMETERS:
		T ..................... The type of element stored in the table.
		                        Must have a default and a copy constructor and
		                        must be assignable.
		                        Should be small, a pointer is optimal.
		Hash .................. The hash function to use.
		                        The hash of an element may never change.
		Eq .................... The equality predicate to use.
		concurrency_factor .... The number of segments to use. It is the maximum
		                        number of threads that can access the table
		                        concurrently. Must be a power of two.

	NOTE:
		If we don't wan't any concurrency we can use an intern table with a
		concurrency factor of 1, this means the table has exactly one global
		lock without any overhead.

		The design of the hash table is taken from Doug Lea's ConcurrentHashMap
		from OpenJDK, but the implementation of the segments is simpler
		because we do not need to support concurrent reads.

		The design of the segment internal hash table is taken from CPython.
		As presented in the book 'Beautiful Code'

*******************************************************************************/

// A helper for selecting which segment to use for a given object.
// The top log2( concurrency_factor ) bits of the hash are used
// as an index into the segment array.
// Thus it is important to have a hash function that does not leave
// the top bits empty or always the same.
//
// Partial template specialization allows us to eliminate any
// selection overhead for a concurrency factor of 1.
//
// TODO: replace with simple C++11 constexpr
template<size_t concurrency_factor>
struct SegmentSelector;

template<>
struct SegmentSelector<1> {
	static size_t segment_index_for_hash(size_t hash) {
		// with a concurreny_factor of 1 there is only 1 segment.
		return 0;
	}
};
template<>
struct SegmentSelector<4> {
	static size_t segment_index_for_hash(size_t hash) {
		return (hash >> 30) & 3;
	}
};
template<>
struct SegmentSelector<8> {
	static size_t segment_index_for_hash(size_t hash) {
		return (hash >> 29) & 7;
	}
};

template<class T, class _Hash, class _Eq, size_t concurrency_factor=4>
class InternTable {
	public:
		InternTable() {
			init(0.66, 2048);
		}
		InternTable(size_t initial_capacity) {
			init(0.66, initial_capacity);
		}
		InternTable(float load_factor, size_t initial_capacity) {
			init(load_factor, initial_capacity);
		}

		/* intern(const T& t) **************************************************

			IN:
				t ... a value to intern
			OUT:
				either the same as the input or, if the table already contains
				a value that is equal to t (checked via _Eq) that interned
				value.

		***********************************************************************/

		const T& intern(const T& t) {
			return intern(t,t);
		}

		/* intern **************************************************************

			Check if a value with a given key is present, and if not lazily
			create a new T and store it in the table.

			TEMPLATE PARAMETERS:
				Key ..... Uniquely identifies a T, _Eq must be able to compare
				          a T and a Key.
				Thunk ... A type that can be implicitly converted to T.
			IN:
				key ..... key of the value to intern.
				thunk ... if no T equal to key is present, thunk is converted
				          to a T and that is stored in the table.
			OUT:
				A value that is equal to key (checked via _Eq).

		***********************************************************************/

		template<typename Key, typename Thunk>
		const T& intern(const Key& key, const Thunk& thunk) {
			size_t hash = hasher(key);

			Segment& segment = segment_for_hash(hash);

			MutexLocker lock(segment.mutex);

			segment.rehash_if_full();

			return segment.intern(key, hash, thunk);
		}
	private:
		struct Entry {
			Entry() : hash( 0 ) {}

			bool unused() { return hash == 0; }
			bool used()   { return hash != 0; }

        	size_t hash; /* cached hash of the payload */
			T      elem; /* the payload */
		};

		struct Segment {
			Segment() : entries(0) {}
			~Segment() { delete[] entries; }

			void init(size_t capacity, float load_factor) {
				this->entries     = new Entry[capacity];
				this->capacity    = capacity;
				this->count       = 0;
				this->threshold   = capacity * load_factor;
				this->load_factor = load_factor;
			}

			template<typename Key, typename Thunk>
			const T& intern(const Key& key, size_t hash, const Thunk& thunk) {
				// we use a hash of zero to signal an empty entry,
				// just map 0 to another value.
				if (hash == 0) hash = 1;

				// find entry with same content as t, or a free entry
				size_t index   = hash;
				size_t perturb = hash;
				Entry *e;

				while (1) {
					e = get_entry(index);

					if (e->unused()) {
						// we found a free slot -> the element is new, insert it
						e->hash = hash;
						e->elem = thunk;
						count++;
						return e->elem;
					} else if ((e->hash == hash) && eq(e->elem, key)) {
						// the element is already in the table
						return e->elem;
					} else {
						// collision, keep searching
						index   = (5 * index) + 1 + perturb;
						perturb >>= 5;
					}
				}
			}

			/* resize the table and rehash all entries */
			void rehash_if_full() {
				if (count < threshold) return;

				Entry *old_entries = entries;
				Entry *it          = entries;
				Entry *end         = it + capacity;

				// resize table
				capacity  = capacity * 2;
				entries   = new Entry[capacity];
				threshold = capacity * load_factor;

				// re-insert all entries
				for ( ; it != end; it++ ) {
					if (it->used()) intern(it->elem, it->hash, it->elem);
				}

				// free old table
				delete[] old_entries;
			}

			/* HELPERS */

			/* get entry at (index % table size) */
			Entry* get_entry(size_t idx) {
				return entries + (idx & (capacity - 1));
			}

			/* FIELDS */

			Entry *entries;     /* the table of entries */
			float  load_factor; /* how full the segment may get before
			                       resizing*/
			size_t capacity;    /* size of the segment table */
			size_t count;       /* numer of elements stored in the
			                       segment */
			size_t threshold;   /* table will be resized when count gets
			                       greater than threshold */
			_Eq     eq;         /* used to compare entries for equality */
			Mutex   mutex;     /* for locking this segment */
		};

		// TODO: replace with C++11 delegating constructor
		void init(float load_factor, size_t initial_capacity) {
			assert(load_factor      > 0);
			assert(load_factor      < 1);
			assert(initial_capacity > 0);

			// evenly divide capacity among segments
			size_t cap = divide_rounding_up(initial_capacity,
			                                concurrency_factor);

			for ( size_t i = 0; i < concurrency_factor; ++i ) {
				segments[i].init( cap, load_factor );
			}
		}

		Segment& segment_for_hash(size_t hash) {
			typedef SegmentSelector<concurrency_factor> Selector;

			return segments[Selector::segment_index_for_hash( hash )];
		}

		static size_t divide_rounding_up(size_t a, size_t b) {
			return (a + b - 1) / b;
		}

		Segment segments[concurrency_factor]; /* the sub-hashtables */
		_Hash hasher;                         /* the hash function  */
};

#endif // INTERN_TABLE_HPP_

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
