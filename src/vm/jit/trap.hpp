/* src/vm/jit/trap.hpp - hardware traps

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


#ifndef _TRAP_HPP
#define _TRAP_HPP

#include "config.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Include machine dependent trap stuff. */

#include "md-trap.h"


/* function prototypes ********************************************************/

void  trap_init(void);
void* trap_handle(int type, intptr_t val, void *pv, void *sp, void *ra, void *xpc, void *context);

#ifdef __cplusplus
}
#endif

#endif /* _TRAP_HPP */


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
