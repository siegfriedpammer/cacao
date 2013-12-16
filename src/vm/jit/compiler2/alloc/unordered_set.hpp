/* src/vm/jit/compiler2/alloc/unordered_set.hpp - custom allocator unordered_set

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

#ifndef _JIT_COMPILER2_ALLOC_UNORDERED_SET
#define _JIT_COMPILER2_ALLOC_UNORDERED_SET

#include "vm/jit/compiler2/alloc/Allocator.hpp"
#include "future/unordered_set.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace alloc {

template<
	class Key,
	class Hash = cacao::hash<Key>,
	class KeyEqual = std::equal_to<Key>
> struct unordered_set {
	typedef cacao::unordered_set<Key, Hash, KeyEqual, Allocator<Key> > type;
};

template<
	class Key,
	class Hash = cacao::hash<Key>,
	class KeyEqual = std::equal_to<Key>
> struct unordered_multiset {
	typedef cacao::unordered_multiset<Key, Hash, KeyEqual, Allocator<Key> > type;
};

} // end namespace alloc
} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_ALLOC_UNORDERED_SET */


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
