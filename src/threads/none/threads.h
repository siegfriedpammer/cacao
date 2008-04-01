/* src/threads/none/threads.h - fake threads header

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


#ifndef _THREADS_H
#define _THREADS_H

#include "config.h"

#include "vm/types.h"

#include "vm/global.h"

#include "vm/jit/stacktrace.h"


/* define some stuff we need to no-ops ****************************************/

#define THREADSPECIFIC
#define THREADOBJECT      NULL
#define THREADINFO        NULL

#define threadobject      void


/* native-world flags *********************************************************/

#define THREAD_NATIVEWORLD_ENTER /*nop*/
#define THREAD_NATIVEWORLD_EXIT  /*nop*/


#if defined(ENABLE_DEBUG_FILTER)
extern u2 _no_threads_filterverbosecallctr[2];
#define FILTERVERBOSECALLCTR (_no_threads_filterverbosecallctr)
#endif

/* state for trace java call **************************************************/

#if !defined(NDEBUG)
extern s4 _no_threads_tracejavacallindent;
#define TRACEJAVACALLINDENT (_no_threads_tracejavacallindent)

extern u4 _no_threads_tracejavacallcount;
#define TRACEJAVACALLCOUNT (_no_threads_tracejavacallcount)
#endif


/* inline functions ***********************************************************/

inline static java_handle_t *thread_get_current_object(void)
{
	java_handle_t *o;

	/* We return a fake java.lang.Thread object, otherwise we get
	   NullPointerException's in GNU Classpath. */

	o = builtin_new(class_java_lang_Thread);

	return o;
}

inline static stackframeinfo_t *threads_get_current_stackframeinfo(void)
{
	return _no_threads_stackframeinfo;
}

inline static void threads_set_current_stackframeinfo(stackframeinfo_t *sfi)
{
	_no_threads_stackframeinfo = sfi;
}

#endif /* _THREADS_H */


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
 */
