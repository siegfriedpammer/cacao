/* src/vm/jit/allocator/simplereg.hpp - register allocator header

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


#ifndef SIMPLE_REG_HPP_
#define SIMPLE_REG_HPP_ 1

#include "config.h"
#include "vm/types.hpp"

#include "arch.hpp"

#include "vm/jit/codegen-common.hpp"
#include "vm/jit/jit.hpp"
#include "vm/jit/inline/inline.hpp"


/* function prototypes ********************************************************/

#ifdef __cplusplus
extern "C" {
#endif

bool regalloc(jitdata *jd);

#if defined(ENABLE_STATISTICS)
void simplereg_make_statistics(jitdata *jd);
#endif

#ifdef __cplusplus
}
#endif

#endif // SIMPLE_REG_HPP_


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
 */
