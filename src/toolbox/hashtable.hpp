/* src/toolbox/hashtable.hpp - hashtable classes

   Copyright (C) 2009, 2011
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO
   Copyright (C) 2009 Theobroma Systems Ltd.

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


#ifndef _HASHTABLE_HPP
#define _HASHTABLE_HPP

#include "config.h"

#if __cplusplus

#include <tr1/unordered_map>


/**
 * Default hashing function for hashtable implementation.
 */
template<class T> class Hasher {
	// XXX Implement me!
};


/**
 * Hashtable implementation.
 */
template<class Key, class T,
		class Hash = Hasher<Key>,
		class Pred = std::equal_to<Key> >
class Hashtable :
		protected std::tr1::unordered_map<Key,T,Hash,Pred> {
public:
	// Constructor.
	Hashtable(size_t n) : std::tr1::unordered_map<Key,T,Hash,Pred>(n) {}

	// Make iterator of TR1 unordered map visible.
	using std::tr1::unordered_map<Key,T,Hash,Pred>::iterator;

	// Make functions of TR1 unordered map visible.
	using std::tr1::unordered_map<Key,T,Hash,Pred>::end;
	using std::tr1::unordered_map<Key,T,Hash,Pred>::find;
	using std::tr1::unordered_map<Key,T,Hash,Pred>::insert;
};


// Required by LockableHashtable.
#include "threads/mutex.hpp"


/**
 * Hashtable implementation with a Mutex.
 */
template<class Key, class T,
		class Hash = Hasher<Key>,
		class Pred = std::equal_to<Key> >
class LockableHashtable :
		public Hashtable<Key,T,Hash,Pred>,
		public Mutex {
public:
	// Constructor.
	LockableHashtable(size_t n) : Hashtable<Key,T,Hash,Pred>(n) {}
};

#endif

#endif /* _HASHTABLE_HPP */


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
