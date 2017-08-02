/* src/vm/jit/i386/patcher.hpp - architecture specific code patching stuff

   Copyright (C) 1996-2017
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


#ifndef _PATCHER_HPP
#define _PATCHER_HPP

#include "vm/jit/patcher-common.hpp"

/* function prototypes ********************************************************/


bool patcher_getfield(patchref_t *pr);
#define PATCHER_getfield (functionptr) patcher_getfield

bool patcher_putfield(patchref_t *pr);
#define PATCHER_putfield (functionptr) patcher_putfield

bool patcher_putfieldconst(patchref_t *pr);
#define PATCHER_putfieldconst (functionptr) patcher_putfieldconst

bool patcher_aconst(patchref_t *pr);
#define PATCHER_aconst (functionptr) patcher_aconst

bool patcher_builtin_multianewarray(patchref_t *pr);
#define PATCHER_builtin_multianewarray (functionptr) patcher_builtin_multianewarray

bool patcher_builtin_arraycheckcast(patchref_t *pr);
#define PATCHER_builtin_arraycheckcast (functionptr) patcher_builtin_arraycheckcast

bool patcher_checkcast_instanceof_flags(patchref_t *pr);
#define PATCHER_checkcast_instanceof_flags (functionptr) patcher_checkcast_instanceof_flags

bool patcher_checkcast_class(patchref_t *pr);
#define PATCHER_checkcast_class (functionptr) patcher_checkcast_class

bool patcher_instanceof_class(patchref_t *pr);
#define PATCHER_instanceof_class (functionptr) patcher_instanceof_class

#endif // _PATCHER_HPP


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
