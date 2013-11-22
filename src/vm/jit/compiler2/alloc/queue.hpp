/* src/vm/jit/compiler2/alloc/queue.hpp - custom allocator queue

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

#ifndef _JIT_COMPILER2_ALLOC_QUEUE
#define _JIT_COMPILER2_ALLOC_QUEUE

#include "vm/jit/compiler2/alloc/deque.hpp"
#include <queue>

namespace cacao {
namespace jit {
namespace compiler2 {
namespace alloc {

template<class T, class Container = typename alloc::deque<T>::type>
struct queue {
	typedef std::queue<T,Container> type;
};

template<
	class T, 
	class Container = typename alloc::deque<T>::type,
	class Compare = std::less<typename Container::value_type>
> struct priority_queue {
	typedef std::priority_queue<T,Container,Compare> type;
};

} // end namespace alloc
} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_ALLOC_QUEUE */


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
