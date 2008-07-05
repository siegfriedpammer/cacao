/* src/vm/jit/sparc64/atomic.hpp - SPARC64 atomic instructions

   Copyright (C) 2008
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


#ifndef _MD_ATOMIC_HPP
#define _MD_ATOMIC_HPP

#include "config.h"

#include <stdint.h>

#include "threads/atomic.hpp"


/**
 * An atomic compare and swap for 32-bit integer values.
 *
 * @param p      Pointer to memory address.
 * @param oldval Old value to be expected.
 * @param newval New value to be stored.
 *
 * @return value of the memory location before the store
 */
inline static uint32_t Atomic_compare_and_swap_32(volatile uint32_t *p, uint32_t oldval, uint32_t newval)
{
#if 0
	// This one should be correct.
	uint32_t result;

	__asm__ __volatile__ (
		"    mov %3,%0       \n"
		"    cas [%4],%2,%0  \n"
		: "=&r" (result), "=m" (*p) 
		: "r" (oldval), "r" (newval), "r" (p));

	return result;
#else
	return Atomic_generic_compare_and_swap_32(p, oldval, newval);
#endif
}


/**
 * An atomic compare and swap for 64-bit integer values.
 *
 * @param p      Pointer to memory address.
 * @param oldval Old value to be expected.
 * @param newval New value to be stored.
 *
 * @return value of the memory location before the store
 */
inline static uint64_t Atomic_compare_and_swap_64(volatile uint64_t *p, uint64_t oldval, uint64_t newval)
{
	uint64_t result;

	__asm__ __volatile__ (
		"    mov %3,%0        \n"
		"    casx [%4],%2,%0  \n"
		: "=&r" (result), "=m" (*p) 
		: "r" (oldval), "r" (newval), "r" (p));

	return result;
}


/**
 * An atomic compare and swap for pointer values.
 *
 * @param p      Pointer to memory address.
 * @param oldval Old value to be expected.
 * @param newval New value to be stored.
 *
 * @return value of the memory location before the store
 */
inline static void* Atomic_compare_and_swap_ptr(volatile void** p, void* oldval, void* newval)
{
	return (void*) Atomic_compare_and_swap_64((volatile uint64_t*) p, (uint64_t) oldval, (uint64_t) newval);
}


/**
 * A memory barrier.
 */
inline static void Atomic_memory_barrier(void)
{
	__asm__ __volatile__ ("membar 0x0F" : : : "memory" );
}


#define STORE_ORDER_BARRIER() __asm__ __volatile__ ("wmb" : : : "memory");
#define MEMORY_BARRIER_AFTER_ATOMIC() __asm__ __volatile__ ("mb" : : : "memory");

#endif // _MD_ATOMIC_HPP


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
