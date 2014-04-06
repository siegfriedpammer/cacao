/* src/toolbox/Debug.cpp - core debugging facilities

   Copyright (C) 1996-2013
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

#include "toolbox/Debug.hpp"

// TODO conditional Makefile.am
#ifdef ENABLE_LOGGING

cacao::Option<const char*> cacao::Debug::debugname("DebugName","Name of the subsystem to debug", NULL, cacao::option::xx_root());

using namespace cacao;

bool cacao::Debug::prefix_enabled = false;
bool cacao::Debug::thread_enabled = false;
unsigned int cacao::Debug::verbose = 0;

bool cacao::Debug::is_debugging_enabled(const char *name, size_t sz) {
	const char* current_system_name = debugname.get();
	if (!current_system_name) {
		return false;
	}
	size_t current_system_name_size = std::strlen(current_system_name);
	return (current_system_name != NULL)    &&
	       (current_system_name_size <= sz) &&
	       (std::strncmp(name, current_system_name, current_system_name_size) == 0);
}

#endif

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
