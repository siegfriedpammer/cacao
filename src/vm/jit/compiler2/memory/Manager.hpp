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
#include "vm/jit/compiler2/memory/memstats.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace memory {

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
		MM_STAT(stat_new<T>(size,p));
		return p;
	}
	/// normal delete
	static void operator delete(void *pMemory) throw() {
		::operator delete(pMemory);
		MM_STAT(stat_delete<T>(pMemory));
	}

	/// placement new
	static void* operator new(std::size_t size, void *ptr) throw() {
		void *p = ::operator new(size, ptr);
		MM_STAT(stat_new<T>(size,p));
		return p;
	}
	/// placement delete
	static void operator delete(void *pMemory, void *ptr) throw() {
		::operator delete(pMemory, ptr);
		MM_STAT(stat_delete<T>(pMemory));
	}

	/// nothrow new
	static void* operator new(std::size_t size, const std::nothrow_t& nt) throw() {
		void *p = ::operator new(size, nt);
		MM_STAT(stat_new<T>(size,p));
		return p;
	}
	/// nothrow delete
	static void operator delete(void *pMemory, const std::nothrow_t&) throw() {
		::operator delete(pMemory);
		MM_STAT(stat_delete<T>(pMemory));
	}

	///////////////////
	// array new/delete
	///////////////////

	/// normal new[]
	static void* operator new[](std::size_t size) throw(std::bad_alloc) {
		void *p = ::operator new[](size);
		MM_STAT(stat_new<T>(size,p));
		return p;
	}
	/// normal delete[]
	static void operator delete[](void *pMemory) throw() {
		::operator delete[](pMemory);
		MM_STAT(stat_delete<T>(pMemory));
	}

	/// placement new[]
	static void* operator new[](std::size_t size, void *ptr) throw() {
		void *p = ::operator new[](size, ptr);
		MM_STAT(stat_new<T>(size,p));
		return p;
	}
	/// placement delete[]
	static void operator delete[](void *pMemory, void *ptr) throw() {
		::operator delete[](pMemory, ptr);
		MM_STAT(stat_delete<T>(pMemory));
	}

	/// nothrow new[]
	static void* operator new[](std::size_t size, const std::nothrow_t& nt) throw() {
		void *p = ::operator new[](size, nt);
		MM_STAT(stat_new<T>(size,p));
		return p;
	}
	/// nothrow delete[]
	static void operator delete[](void *pMemory, const std::nothrow_t&) throw() {
		::operator delete[](pMemory);
		MM_STAT(stat_delete<T>(pMemory));
	}

};

#undef MM_STAT

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
