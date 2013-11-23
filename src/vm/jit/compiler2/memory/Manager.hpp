/* src/vm/jit/compiler2/memory/MemoryManager.hpp - Custom new/delete handler

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

#ifndef _JIT_COMPILER2_MEMORY_MANAGER
#define _JIT_COMPILER2_MEMORY_MANAGER


#include <new>
#include "vm/statistics.hpp"

#if defined(ENABLE_STATISTICS)
#define _MEMORY_MANAGER_ENABLE_DETAILED_STATS
#endif

#if defined(_MEMORY_MANAGER_ENABLE_DETAILED_STATS)
#include "future/unordered_map.hpp"
#endif

namespace cacao {
namespace jit {
namespace compiler2 {
namespace memory {

STAT_DECLARE_VAR(std::size_t,comp2_allocated,0)

#if defined(_MEMORY_MANAGER_ENABLE_DETAILED_STATS)

STAT_DECLARE_VAR(std::size_t,comp2_deallocated,0)
STAT_DECLARE_VAR(std::size_t,comp2_max,0)

unordered_map<void*,std::size_t>& mem_map();

std::size_t& current_heap_size();


#define _MY_STAT(x) do { x; } while(0)
#else
#define _MY_STAT(x) /* nothing */
#endif


#if defined(ENABLE_STATISTICS)
inline void stat_new(std::size_t size, void* p) {
	STATISTICS(comp2_allocated+=size);
	_MY_STAT(current_heap_size()+=size);
	_MY_STAT(comp2_max.max(current_heap_size()));
	_MY_STAT(mem_map()[p] = size);
}
inline void stat_delete(void* p) {
	#if defined(_MEMORY_MANAGER_ENABLE_DETAILED_STATS)
	std::size_t size = mem_map()[p];
	current_heap_size() -= size;
	comp2_deallocated += size;
	#endif
}
#endif

/**
 * Custom new/delete handler mixin.
 *
 * @note See @cite Meyers2005 (Item 49-52) for more info.
 */
template<typename T>
class ManagerMixin {
public:
	/// normal new
	static void* operator new(std::size_t size) throw(std::bad_alloc) {
		void *p = ::operator new(size);
		STATISTICS(stat_new(size,p));
		return p;
	}
	/// normal delete
	static void operator delete(void *pMemory) throw() {
		::operator delete(pMemory);
		STATISTICS(stat_delete(pMemory));
	}

	/// placement new
	static void* operator new(std::size_t size, void *ptr) throw() {
		void *p = ::operator new(size, ptr);
		STATISTICS(stat_new(size,p));
		return p;
	}
	/// placement delete
	static void operator delete(void *pMemory, void *ptr) throw() {
		::operator delete(pMemory, ptr);
		STATISTICS(stat_delete(pMemory));
	}

	/// nothrow new
	static void* operator new(std::size_t size, const std::nothrow_t& nt) throw() {
		void *p = ::operator new(size, nt);
		STATISTICS(stat_new(size,p));
		return p;
	}
	/// nothrow delete
	static void operator delete(void *pMemory, const std::nothrow_t&) throw() {
		::operator delete(pMemory);
		STATISTICS(stat_delete(pMemory));
	}

	///////////////////
	// array new/delete
	///////////////////

	/// normal new[]
	static void* operator new[](std::size_t size) throw(std::bad_alloc) {
		void *p = ::operator new[](size);
		STATISTICS(stat_new(size,p));
		return p;
	}
	/// normal delete[]
	static void operator delete[](void *pMemory) throw() {
		::operator delete[](pMemory);
		STATISTICS(stat_delete(pMemory));
	}

	/// placement new[]
	static void* operator new[](std::size_t size, void *ptr) throw() {
		void *p = ::operator new[](size, ptr);
		STATISTICS(stat_new(size,p));
		return p;
	}
	/// placement delete[]
	static void operator delete[](void *pMemory, void *ptr) throw() {
		::operator delete[](pMemory, ptr);
		STATISTICS(stat_delete(pMemory));
	}

	/// nothrow new[]
	static void* operator new[](std::size_t size, const std::nothrow_t& nt) throw() {
		void *p = ::operator new[](size, nt);
		STATISTICS(stat_new(size,p));
		return p;
	}
	/// nothrow delete[]
	static void operator delete[](void *pMemory, const std::nothrow_t&) throw() {
		::operator delete[](pMemory);
		STATISTICS(stat_delete(pMemory));
	}

};

#undef _MY_STAT

} // end namespace memory
} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_MEMORY_MANAGER */


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
