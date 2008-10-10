/* src/threads/lock.hpp - lock implementation

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


#ifndef _LOCK_HPP
#define _LOCK_HPP

#include <stdint.h>

#include "native/llni.h"

#include "threads/mutex.hpp"

#include "toolbox/list.hpp"

#include "vm/global.h"


/* typedefs *******************************************************************/

typedef struct lock_record_t    lock_record_t;
typedef struct lock_hashtable_t lock_hashtable_t;


/* lock_record_t ***************************************************************

   Lock record struct representing an inflated ("fat") lock.

*******************************************************************************/

struct lock_record_t {
	java_object_t       *object;             /* object for which this lock is */
	struct threadobject *owner;              /* current owner of this monitor */
	s4                   count;              /* recursive lock count          */
	Mutex*               mutex;              /* mutex for synchronizing       */
#ifdef __cplusplus
	List<threadobject*>* waiters;            /* list of threads waiting       */
#else
	List* waiters;
#endif
	lock_record_t       *hashlink;           /* next record in hash chain     */
};


/* lock_hashtable_t ************************************************************
 
   The global hashtable mapping objects to lock records.

*******************************************************************************/

struct lock_hashtable_t {
    Mutex*               mutex;       /* mutex for synch. access to the table */
	u4                   size;        /* number of slots                      */
	u4                   entries;     /* current number of entries            */
	lock_record_t      **ptr;         /* the table of slots, uses ext. chain. */
};


/* functions ******************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

void lock_init(void);

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


/* defines ********************************************************************/

/* only define the following stuff with thread enabled ************************/

#if defined(ENABLE_THREADS)

#define LOCK_MONITOR_ENTER(o)    lock_monitor_enter((java_handle_t *) LLNI_QUICKWRAP(o))
#define LOCK_MONITOR_EXIT(o)     lock_monitor_exit((java_handle_t *) LLNI_QUICKWRAP(o))

#endif

#endif // _LOCK_HPP


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