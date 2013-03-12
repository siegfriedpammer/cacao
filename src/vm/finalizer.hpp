/* src/vm/finalizer.hpp - finalizer linked list and thread header

   Copyright (C) 1996-2011
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


#ifndef _FINALIZER_H
#define _FINALIZER_H

#include "config.h"

#ifdef __cplusplus

struct Finalizer {
	typedef void (*FinalizerFunc)(java_handle_t *h, void *data);
	static void *attach_custom_finalizer(java_handle_t *h, FinalizerFunc f, void *data);
	static void reinstall_custom_finalizer(java_handle_t *h);
};

extern "C" {
#endif

#include "vm/types.hpp"

#include "vm/global.h"


/* function prototypes ********************************************************/

bool finalizer_init(void);
bool finalizer_start_thread(void);
void finalizer_notify(void);
void finalizer_run(void *o, void *p);

#ifdef __cplusplus
}
#endif

#endif /* _FINALIZER_H */


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
