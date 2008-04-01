/* src/threads/threadlist.h - different thread-lists

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


#ifndef _THREADLIST_H
#define _THREADLIST_H

#include "config.h"

#include <stdint.h>

#include "threads/thread.h"


/* function prototypes ********************************************************/

void          threadlist_init(void);

void          threadlist_add(threadobject *t);
void          threadlist_remove(threadobject *t);
threadobject *threadlist_first(void);
threadobject *threadlist_next(threadobject *t);

void          threadlist_free_add(threadobject *t);
void          threadlist_free_remove(threadobject *t);
threadobject *threadlist_free_first(void);

int           threadlist_get_non_daemons(void);

void          threadlist_index_add(int index);
int           threadlist_get_free_index(void);

/* implementation specific functions */

void          threadlist_impl_init(void);

void          threadlist_lock(void);
void          threadlist_unlock(void);

#endif /* _THREADLIST_H */


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
