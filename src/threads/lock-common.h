/* src/threads/lock-common.h - common stuff of lock implementation

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


#ifndef _LOCK_COMMON_H
#define _LOCK_COMMON_H

#include "config.h"

#include "vm/types.h"

#include "vm/global.h"

#if defined(ENABLE_THREADS)
# include "threads/posix/lock.h"
#else
# include "threads/none/lock.h"
#endif


/* only define the following stuff with thread enabled ************************/

#if defined(ENABLE_THREADS)

/* functions ******************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

void lock_init(void);

void lock_init_object_lock(java_object_t *);

ptrint lock_pre_compute_thinlock(s4 index);

bool lock_monitor_enter(java_handle_t *);
bool lock_monitor_exit(java_handle_t *);

#define LOCK_monitor_enter    (functionptr) lock_monitor_enter
#define LOCK_monitor_exit     (functionptr) lock_monitor_exit

bool lock_is_held_by_current_thread(java_handle_t *o);

void lock_wait_for_object(java_handle_t *o, s8 millis, s4 nanos);
void lock_notify_object(java_handle_t *o);
void lock_notify_all_object(java_handle_t *o);

#ifdef __cplusplus
}
#endif

#endif /* ENABLE_THREADS */

#endif /* _LOCK_COMMON_H */


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
