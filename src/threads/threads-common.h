/* src/threads/threads-common.h - machine independent thread functions

   Copyright (C) 2007 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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

   $Id: threads-common.h 7825 2007-04-25 20:31:57Z twisti $

*/


#ifndef _THREADS_COMMON_H
#define _THREADS_COMMON_H

#include "config.h"
#include "vm/types.h"

#include "vm/global.h"

#include "native/jni.h"

#if defined(ENABLE_THREADS)
# include "threads/native/threads.h"
#else
# include "threads/none/threads.h"
#endif

#include "vmcore/utf8.h"


/* only define the following stuff with thread enabled ************************/

#if defined(ENABLE_THREADS)

/* thread states **************************************************************/

#define THREAD_STATE_NEW              0
#define THREAD_STATE_RUNNABLE         1
#define THREAD_STATE_BLOCKED          2
#define THREAD_STATE_WAITING          3
#define THREAD_STATE_TIMED_WAITING    4
#define THREAD_STATE_TERMINATED       5


/* thread priorities **********************************************************/

#define MIN_PRIORITY     1
#define NORM_PRIORITY    5
#define MAX_PRIORITY     10


/* function prototypes ********************************************************/

threadobject *threads_create_thread(void);
threadobject *threads_thread_create_internal(utf *name);
void          threads_start_javathread(void *vobject);
ptrint        threads_get_current_tid(void);
utf          *threads_thread_get_state(threadobject *thread);
bool          threads_thread_is_alive(threadobject *thread);
void          threads_dump(void);
void          threads_thread_print_stacktrace(threadobject *thread);
void          threads_print_stacktrace(void);

#endif /* ENABLE_THREADS */

#endif /* _THREADS_COMMON_H */


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
