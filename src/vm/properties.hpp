/* src/vm/properties.hpp - handling commandline properties

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


#ifndef _PROPERTIES_HPP
#define _PROPERTIES_HPP

#include "config.h"

#include <stdint.h>

#ifdef __cplusplus

#include <map>

#include "vm/os.hpp"


class ltstr {
public:
	bool operator()(const char* s1, const char* s2) const
	{
		return os::strcmp(s1, s2) < 0;
	}
};


/**
 * Commandline properties.
 */
class Properties {
private:
	std::map<const char*, const char*, ltstr> _properties;

private:
	// Don't allow to copy the properties.
	Properties(const Properties&);

public:
	Properties();

	// Static function.
	static void put(java_handle_t* p, const char* key, const char* value);

	void        put(const char* key, const char* value);
	const char* get(const char* key);
	void        fill(java_handle_t* p);
#if !defined(NDEBUG)
	void        dump();
#endif
};

#endif

#endif // _PROPERTIES_HPP


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
