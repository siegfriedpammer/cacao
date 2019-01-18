/* src/vm/jit/compiler2/alloc/map.hpp - custom allocator map

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

#ifndef _JIT_COMPILER2_ALLOC_MAP
#define _JIT_COMPILER2_ALLOC_MAP

#include "vm/jit/compiler2/alloc/Allocator.hpp"
#include <map>

namespace cacao {
namespace jit {
namespace compiler2 {
namespace alloc {

template<class Key, class T, class Compare = std::less<Key> >
struct map {
	typedef std::map<Key, T, Compare, Allocator<std::pair<Key const, T> > > type;
};

template<class Key, class T, class Compare = std::less<Key> >
struct multimap {
	typedef std::multimap<Key, T, Compare, Allocator<std::pair<Key, T> > > type;
};

} // end namespace alloc
} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_ALLOC_MAP */


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
