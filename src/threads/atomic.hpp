/* src/threads/atomic.hpp - atomic instructions

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


#ifndef _ATOMIC_HPP
#define _ATOMIC_HPP

#include "config.h"

#include <stdint.h>

#ifdef __cplusplus

class Atomic {
public:
	// Generic functions.
	static uint32_t generic_compare_and_swap(volatile uint32_t *p, uint32_t oldval, uint32_t newval);
	static uint64_t generic_compare_and_swap(volatile uint64_t *p, uint64_t oldval, uint64_t newval);
	static void*    generic_compare_and_swap(volatile void** p, void* oldval, void* newval);
	static void     generic_memory_barrier(void);

	// Machine dependent functions.
	static uint32_t compare_and_swap(volatile uint32_t *p, uint32_t oldval, uint32_t newval);
	static uint64_t compare_and_swap(volatile uint64_t *p, uint64_t oldval, uint64_t newval);
	static void*    compare_and_swap(volatile void** p, void* oldval, void* newval);
	static void     memory_barrier(void);
	static void     write_memory_barrier(void);
	static void     instruction_barrier(void);
};

// Include machine dependent implementation.
#include "md-atomic.hpp"

#else

// Legacy C interface.

uint32_t Atomic_compare_and_swap_32(volatile uint32_t *p, uint32_t oldval, uint32_t newval);
uint64_t Atomic_compare_and_swap_64(volatile uint64_t *p, uint64_t oldval, uint64_t newval);
void*    Atomic_compare_and_swap_ptr(volatile void** p, void* oldval, void* newval);
void     Atomic_memory_barrier(void);
void     Atomic_write_memory_barrier(void);
void     Atomic_instruction_barrier(void);

#endif

#endif // _ATOMIC_HPP


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
