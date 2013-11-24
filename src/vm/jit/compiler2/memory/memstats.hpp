/* src/vm/jit/compiler2/memory/Memorymemstat.hpp - Custom new/delete handler statistics

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

#ifndef _JIT_COMPILER2_MEMORY_MEMSTAT
#define _JIT_COMPILER2_MEMORY_MEMSTAT

#include "config.h"

#if defined(ENABLE_STATISTICS)
#define ENABLE_MEMORY_MANAGER_STATISTICS
#endif

#if defined(ENABLE_MEMORY_MANAGER_STATISTICS)

#include "vm/statistics.hpp"
#include "future/unordered_map.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace memory {


STAT_DECLARE_VAR(std::size_t,comp2_allocated,0)
STAT_DECLARE_VAR(std::size_t,comp2_deallocated,0)
STAT_DECLARE_VAR(std::size_t,comp2_max,0)

unordered_map<void*,std::size_t>& mem_map();

std::size_t& current_heap_size();


template<typename T>
inline void stat_new(std::size_t size, void* p) {
	comp2_allocated+=size;
	current_heap_size()+=size;
	comp2_max.max(current_heap_size());
	mem_map()[p] = size;
}
template<typename T>
inline void stat_delete(void* p) {
	std::size_t size = mem_map()[p];
	current_heap_size() -= size;
	comp2_deallocated += size;
}


} // end namespace memory
} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#define MM_STAT(x) do { x; } while(0)
#else
#define MM_STAT(x) /* nothing */
#endif /* ENABLE_MEMORY_MANAGER_STATISTICS */

#endif /* _JIT_COMPILER2_MEMORY_MEMSTAT */


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
