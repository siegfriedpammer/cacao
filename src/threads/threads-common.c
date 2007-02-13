/* src/threads/threads-common.c - machine independent thread functions

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

   $Id: signal.c 7246 2007-01-29 18:49:05Z twisti $

*/


#include "config.h"
#include "vm/types.h"

#include "native/jni.h"
#include "native/include/java_lang_Thread.h"

#if defined(WITH_CLASSPATH_GNU)
# include "native/include/java_lang_VMThread.h"
#endif

#include "threads/threads-common.h"

#if defined(ENABLE_THREADS)
# include "threads/native/threads.h"
#endif

#include "vm/builtin.h"
#include "vm/stringlocal.h"

#include "vmcore/class.h"
#include "vmcore/utf8.h"


/* threads_create_thread *******************************************************

   Creates a thread object with the given name.

*******************************************************************************/
#if defined(ENABLE_THREADS)
threadobject *threads_create_thread(utf *name)
{
	threadobject *thread;
#if defined(WITH_CLASSPATH_GNU)
	java_lang_VMThread *vmt;
#endif

	/* create the finalizer object */

	thread = (threadobject *) builtin_new(class_java_lang_Thread);

	if (thread == NULL)
		return NULL;

#if defined(WITH_CLASSPATH_GNU)
	vmt = (java_lang_VMThread *) builtin_new(class_java_lang_VMThread);

	if (vmt == NULL)
		return NULL;

	vmt->thread = (java_lang_Thread *) thread;

	thread->o.vmThread = vmt;
#endif

	thread->flags      = THREAD_FLAG_DAEMON;

	/* set java.lang.Thread fields */

	thread->o.name     = javastring_new(name);
#if defined(ENABLE_JAVASE)
	thread->o.daemon   = true;
#endif
	thread->o.priority = NORM_PRIORITY;

	/* return the thread object */

	return thread;
}
#endif

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
