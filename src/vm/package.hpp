/* src/vm/package.hpp - Java boot-package functions

   Copyright (C) 2007, 2008
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


#ifndef _VM_PACKAGE_HPP
#define _VM_PACKAGE_HPP

#include "config.h"

#include <stdint.h>

#include "vm/utf8.h"


#ifdef __cplusplus

/**
 *
 */
class Package {
public:
	static void initialize();
	/* static void add(java_handle_t *packagename); */
	static void add(utf *packagename);
	/* static java_handle_t* find(java_handle_t *packagename); */
	static utf* find(utf *packagename);
};

#else

/* Legacy C interface *********************************************************/

typedef struct Package Package;

void Package_initialize();
void Package_add(utf* packagename);
utf* Package_find(utf* packagename);

#endif

#endif /* _VM_PACKAGE_HPP */


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
