/* src/vm/jit/compiler2/lsra/LogHelper.hpp - LogHelper

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

#ifndef _JIT_COMPILER2_LSRA_LOGHELPER
#define _JIT_COMPILER2_LSRA_LOGHELPER

#include <iterator>

#define LOG_NAMED_PTR_CONTAINER_N(n, prefix, container)                                            \
	do {                                                                                           \
		if (DEBUG_COND_N(n)) {                                                                     \
			LOG_N(n, prefix);                                                                      \
			using std::begin;                                                                      \
			using std::end;                                                                        \
			DEBUG_N(n, print_ptr_container(dbg(), begin(container), end(container)));              \
			LOG_N(n, nl);                                                                          \
		}                                                                                          \
	} while (0)

#define LOG_NAMED_PTR_CONTAINER(prefix, container) LOG_NAMED_PTR_CONTAINER_N(0, prefix, container)
#define LOG1_NAMED_PTR_CONTAINER(prefix, container) LOG_NAMED_PTR_CONTAINER_N(1, prefix, container)
#define LOG2_NAMED_PTR_CONTAINER(prefix, container) LOG_NAMED_PTR_CONTAINER_N(2, prefix, container)
#define LOG3_NAMED_PTR_CONTAINER(prefix, container) LOG_NAMED_PTR_CONTAINER_N(3, prefix, container)

#define LOG_NAMED_CONTAINER_N(n, prefix, container)                                                \
	do {                                                                                           \
		if (DEBUG_COND_N(n)) {                                                                     \
			LOG_N(n, prefix);                                                                      \
			using std::cbegin;                                                                     \
			using std::cend;                                                                       \
			DEBUG_N(n, print_container(dbg(), cbegin(container), cend(container)));                \
			LOG_N(n, nl);                                                                          \
		}                                                                                          \
	} while (0)

#define LOG_NAMED_CONTAINER(prefix, container) LOG_NAMED_CONTAINER_N(0, prefix, container)
#define LOG1_NAMED_CONTAINER(prefix, container) LOG_NAMED_CONTAINER_N(1, prefix, container)
#define LOG2_NAMED_CONTAINER(prefix, container) LOG_NAMED_CONTAINER_N(2, prefix, container)
#define LOG3_NAMED_CONTAINER(prefix, container) LOG_NAMED_CONTAINER_N(3, prefix, container)

#endif /* _JIT_COMPILER2_LSRA_LOGHELPER */

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
