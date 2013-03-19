/* src/toolbox/Debug.hpp - core debugging facilities

   Copyright (C) 1996-2005, 2006, 2007, 2008
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

#ifndef DEBUG_HPP_
#define DEBUG_HPP_ 1

#include "config.h"

#ifndef NDEBUG

#include <cstring>
#include <cassert>

#ifdef __cplusplus

namespace cacao {

struct Debug {
	/// True if we should print a prefix
	///
	/// Can be set using the -XX:+DebugPrefix command line flag
	/// @default false
	static bool prefix_enabled;

	/// True if we should print a the thread id
	///
	/// Can be set using the -XX:+DebugPrintThread command line flag
	/// @default false
	static bool thread_enabled;

	/// Set the name of system you are interested in debugging
	///
	/// can be conviently be set via the command line flag -XX:DebugName
	static void set_current_system(const char *system);

	/// Debugging of a sub system is enabled if it's name is a valid prefix
	/// of the currently set system's name (as set via set_current_system)
	static bool is_debugging_enabled(const char *system, size_t sz);

	inline static bool is_debugging_enabled(const char *system) {
		return is_debugging_enabled(system, ::std::strlen(system));
	}
};

/// This macro executes STMT iff debugging of sub system DBG_NAME is enabled
#define DEBUG_WITH_NAME(DBG_NAME, STMT)                      \
	do {                                                     \
		if (cacao::Debug::is_debugging_enabled(DBG_NAME)) {  \
			STMT;                                            \
		}                                                    \
	} while (0)

/// Execute debug statements in your current module.
///
/// To use this macro you must define the macro DEBUG_NAME to the
/// name of your current module (should be a string literal.
/// Never do this in a header!
#define DEBUG(STMT) DEBUG_WITH_NAME(DEBUG_NAME, STMT)

} // end namespace cacao

#endif // end __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

void debug_set_current_system(const char *);

int  debug_is_debugging_enabled(const char *);

#ifdef __cplusplus
}
#endif

#else

#define DEBUG_WITH_NAME(DBG_NAME, STMT) do { } while(0)
#define DEBUG(STMT)                     DEBUG_WITH_NAME(DEBUG_NAME, STMT)

#endif // end NDEBUG

#endif // DEBUG_HPP_

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
