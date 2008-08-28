/* src/mm/dumpmemory.hpp - dump memory management

   Copyright (C) 1996-2005, 2006, 2007, 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO
   Copyright (C) 2008 Theobroma Systems Ltd.

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


#ifndef _DUMPMEMORY_HPP
#define _DUMPMEMORY_HPP

#include "config.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus

#include <cstddef>
#include <list>
#include <vector>
#include <stdio.h> // REMOVEME


// Forward declaration.
class DumpMemoryArea;
class DumpMemoryBlock;


/**
 * Thread-local dump memory structure.
 */
class DumpMemory {
private:
	size_t                     _size;  ///< Size of the dump areas in this dump memory.
	size_t                     _used;  ///< Used memory in this dump memory.
	std::list<DumpMemoryArea*> _areas; ///< Pointer to the current dump area.

#if 0
#if defined(ENABLE_MEMCHECK)
	dump_allocation_t *allocations;       /* list of allocations in this area */
#endif
#endif

public:
	DumpMemory();
	~DumpMemory();

	static inline DumpMemory* get_current();
	static inline void*       allocate(size_t size);

	inline void   add_size(size_t size) { _size += size; }

	inline size_t get_size() const { return _size; }
	inline size_t get_used() const { return _used; }

	inline DumpMemoryArea* get_current_area() const;

	static void* reallocate(void* src, size_t len1, size_t len2);

	void  add_area(DumpMemoryArea* dma);
	void  remove_area(DumpMemoryArea* dma);
};


/**
 * Dump memory area.
 */
class DumpMemoryArea {
private:
	size_t                        _size;   ///< Size of the current memory block.
	size_t                        _used;   ///< Used memory in the current memory block.
	std::vector<DumpMemoryBlock*> _blocks; ///< List of memory blocks in this area.

public:
	DumpMemoryArea(size_t size = 0);
	~DumpMemoryArea();

	inline size_t get_size() const { return _size; }
	inline size_t get_used() const { return _used; }

	// Inline functions.
	inline void*            allocate(size_t size);
	inline DumpMemoryBlock* get_current_block() const;

	DumpMemoryBlock* allocate_new_block(size_t size);
};


/**
 * Dump memory block.
 */
class DumpMemoryBlock {
private:
	static const size_t DEFAULT_SIZE = 2 << 13; // 2 * 8192 bytes

	size_t _size;  ///< Size of the current memory block.
	size_t _used;  ///< Used memory in the current memory block.
	void*  _block; ///< List of memory blocks in this area.

public:
	DumpMemoryBlock(size_t size = 0);
	~DumpMemoryBlock();

	inline size_t get_size() const { return _size; }
	inline size_t get_used() const { return _used; }
	inline size_t get_free() const { return _size - _used; }

	// Inline functions.
	inline void* allocate(size_t size);
};


/**
 * Allocator for the dump memory.
 */
template<class T> class DumpMemoryAllocator {
public:
	// Type definitions.
	typedef T              value_type;
	typedef T*             pointer;
	typedef const T*       const_pointer;
	typedef T&             reference;
	typedef const T&       const_reference;
	typedef std::size_t    size_type;
	typedef std::ptrdiff_t difference_type;

	// Rebind allocator to type U.
	template <class U> struct rebind {
		typedef DumpMemoryAllocator<U> other;
	};

	/* constructors and destructor
	 * - nothing to do because the allocator has no state
	 */
	DumpMemoryAllocator() throw() {
	}

	DumpMemoryAllocator(const DumpMemoryAllocator&) throw() {
	}

	template <class U> DumpMemoryAllocator(const DumpMemoryAllocator<U>&) throw() {
	}

	~DumpMemoryAllocator() throw() {
	}

	pointer allocate(size_type n, void* = 0) {
// 		printf("allocate: n=%d * %d\n", n, sizeof(T));
		return static_cast<pointer>(DumpMemory::allocate(n * sizeof(T)));
	}

	// Initialize elements of allocated storage p with value value.
	void construct(pointer p, const T& value) {
// 		printf("construct: p=%p, value=%p\n", (void*) p, (void*) value);
		// Initialize memory with placement new.
		new ((void*) p) T(value);
	}

	// Destroy elements of initialized storage p.
	void destroy(pointer p) {
// 		printf("destroy: p=%p\n", (void*) p);
		// Destroy objects by calling their destructor.
		p->~T();
	}

	void deallocate(pointer p, size_type n) {
// 		printf("deallocate: p=%p, n=%d\n", (void*) p, n);
		// We don't need to deallocate on dump memory.
	}
};


/* dump_allocation *************************************************************

   This struct is used to record dump memory allocations for ENABLE_MEMCHECK.

*******************************************************************************/

#if 0
#if defined(ENABLE_MEMCHECK)
typedef struct dump_allocation_t dump_allocation_t;

struct dump_allocation_t {
	dump_allocation_t *next;
	void              *mem;
	int32_t            used;
	int32_t            size;
};
#endif
#endif


// Includes.
#include "mm/memory.h"

#include "threads/thread.hpp"

#include "vm/options.h"

#if defined(ENABLE_STATISTICS)
# include "vm/statistics.h"
#endif


// Inline functions.

inline DumpMemory* DumpMemory::get_current()
{
	// Get the DumpMemory object of the current thread.
	threadobject* t = thread_get_current();
	DumpMemory* dm = t->_dumpmemory;
	return dm;
}

inline DumpMemoryArea* DumpMemory::get_current_area() const
{
	return _areas.back();
}

inline void* DumpMemory::allocate(size_t size)
{
	DumpMemory* dm = get_current();
	DumpMemoryArea* dma = dm->get_current_area();

	size_t alignedsize = size;

#if defined(ENABLE_MEMCHECK)
	alignedsize += 2 * MEMORY_CANARY_SIZE;
#endif

	// Align the allocation size.
	alignedsize = MEMORY_ALIGN(alignedsize, ALIGNSIZE);

	void* p = dma->allocate(alignedsize);

	// Increase the used count of the dump memory.
	dm->_used += alignedsize;

	return p;
}

inline DumpMemoryBlock* DumpMemoryArea::get_current_block() const
{
	return _blocks.empty() ? NULL : _blocks.back();
}

inline void* DumpMemoryArea::allocate(size_t size)
{
	DumpMemoryBlock* dmb = get_current_block();

	// Check if we have a memory block or have enough memory in the
	// current memory block.
	if (dmb == NULL || size > dmb->get_free()) {
		// No, allocate a new one.
		dmb = allocate_new_block(size);

		// Increase the size of the memory area.  We use get_size()
		// here because the default size is very likely to be bigger
		// than size.
		_size += dmb->get_size();
	}

	void* p = dmb->allocate(size);

	// Increase the used size of the memory area.
	_used += size;

	return p;
}

/**
 * Allocate memory in the current dump memory area.
 *
 * This function is a fast allocator suitable for scratch memory that
 * can be collectively freed when the current activity (eg. compiling)
 * is done.
 *
 * You cannot selectively free dump memory. Before you start
 * allocating it, you remember the current size returned by
 * `dumpmemory_marker`. Later, when you no longer need the memory,
 * call `dumpmemory_release` with the remembered size and all dump
 * memory allocated since the call to `dumpmemory_marker` will be
 * freed.
 *
 * @parm size Size of block to allocate in bytes. May be zero, in which case NULL is returned
 *
 * @return Pointer to allocated memory, or NULL iff size was zero.
 */
void* DumpMemoryBlock::allocate(size_t size)
{
#if defined(ENABLE_MEMCHECK)
	size_t origsize = size; /* needed for the canary system */
#endif

	if (size == 0)
		return NULL;

	// Sanity check.
	assert(size <= (_size - _used));

	// Calculate the memory address of the newly allocated memory.
	void* p = (void*) (((uint8_t*) _block) + _used);

#if defined(ENABLE_MEMCHECK)
	{
		dump_allocation_t *da = NEW(dump_allocation_t);
		uint8_t           *pm;
		int                i;

		/* add the allocation to our linked list of allocations */

		da->next = di->allocations;
		da->mem  = ((uint8_t *) p) + MEMORY_CANARY_SIZE;
		da->size = origsize;
		da->used = di->used;

		di->allocations = da;

		/* write the canaries */

		pm = (uint8_t *) p;

		for (i = 0; i < MEMORY_CANARY_SIZE; ++i)
			pm[i] = i + MEMORY_CANARY_FIRST_BYTE;

		pm = ((uint8_t *) da->mem) + da->size;

		for (i = 0; i < MEMORY_CANARY_SIZE; ++i)
			pm[i] = i + MEMORY_CANARY_FIRST_BYTE;

		/* make m point after the bottom canary */

		p = ((uint8_t *) p) + MEMORY_CANARY_SIZE;

		/* clear the memory */

		(void) os_memset(p, MEMORY_CLEAR_BYTE, da->size);
	}
#endif /* defined(ENABLE_MEMCHECK) */

	// Increase used memory block size by the allocated memory size.
	_used += size;

	return p;
}

#else

// Legacy C interface.

void* DumpMemory_allocate(size_t size);
void* DumpMemory_reallocate(void* src, size_t len1, size_t len2);

#define DNEW(type)                    ((type*) DumpMemory_allocate(sizeof(type)))
#define DMNEW(type,num)               ((type*) DumpMemory_allocate(sizeof(type) * (num)))
#define DMREALLOC(ptr,type,num1,num2) ((type*) DumpMemory_reallocate((ptr), sizeof(type) * (num1), sizeof(type) * (num2)))

#endif

#endif // _DUMPMEMORY_HPP


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
