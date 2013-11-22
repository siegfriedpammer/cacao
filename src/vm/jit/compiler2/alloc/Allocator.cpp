/* src/vm/jit/compiler2/alloc/Allocator.cpp - Custom allocator for container

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

#include "vm/jit/compiler2/alloc/Allocator.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace alloc {

STAT_REGISTER_GROUP(comp2_alloc_group,"Compiler2Allocator","Standard Library Allocator for compiler2")
STAT_REGISTER_GROUP_VAR_EXTERN(std::size_t,comp2_allocated,0,"total allocations","Number of byte allocated",comp2_alloc_group)
STAT_REGISTER_GROUP_VAR_EXTERN(std::size_t,comp2_deallocated,0,"total deallocations","Number of byte deallocated",comp2_alloc_group)
STAT_REGISTER_GROUP_VAR_EXTERN(std::size_t,comp2_max,0,"maximum memory","Maximum memory consumption",comp2_alloc_group)

std::size_t& current_heap_size() {
	static std::size_t t = 0;
	return t;
}

} // end namespace alloc
} // end namespace compiler2
} // end namespace jit
} // end namespace cacao


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
