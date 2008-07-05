/* src/vm/jit/arm/atomic.hpp - ARM atomic instructions

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
	uint32_t result;
	uint32_t temp;

	/* TODO: improve this one! */
	__asm__ __volatile__ (
		"1:\t"
		"ldr   %0,[%2]\n\t"
		"cmp   %0,%4\n\t"
		"bne   2f\n\t"
		"swp   %1,%3,[%2]\n\t"
		"cmp   %1,%0\n\t"
		"swpne %0,%1,[%2]\n\t"
		"bne   1b\n\t"
		"2:"
		: "=&r" (result), "=&r" (temp)
		: "r" (p), "r" (newval), "r" (oldval)
		: "cc", "memory"
	);

	return result;
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
	return Atomic_generic_compare_and_swap_64(p, oldval, newval);
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
	return (void*) Atomic_compare_and_swap_32((volatile uint32_t*) p, (uint32_t) oldval, (uint32_t) newval);
}


/**
 * A memory barrier.
 */
inline static void Atomic_memory_barrier(void)
{
	__asm__ __volatile__ ("" : : : "memory");
}


#define STORE_ORDER_BARRIER() __asm__ __volatile__ ("" : : : "memory");
#define MEMORY_BARRIER_AFTER_ATOMIC() __asm__ __volatile__ ("" : : : "memory");

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
