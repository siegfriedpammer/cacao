/* src/threads/thread-classpath.hpp - thread functions specific to the GNU classpath library

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


#ifndef _THREAD_CLASSPATH_HPP
#define _THREAD_CLASSPATH_HPP

#ifdef __cplusplus

#include "config.h"

#include "vm/types.h"

// Include early to get threadobject.
#if defined(ENABLE_THREADS)
# include "threads/posix/thread-posix.hpp"
#else
# include "threads/none/thread-none.h"
#endif

class java_lang_Thread;

/* only define the following stuff with thread enabled ************************/

#if defined(ENABLE_THREADS) && defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)

struct ThreadRuntimeClasspath {
	static classinfo *get_thread_class_from_object(java_handle_t *object);
	static java_handle_t *get_vmthread_handle(const java_lang_Thread &jlt);
	static java_handle_t *get_thread_exception_handler(const java_lang_Thread &jlt);
	static methodinfo *get_threadgroup_remove_method(classinfo *c);
	static methodinfo *get_thread_init_method();
	static void setup_thread_vmdata(const java_lang_Thread& jlt, threadobject *t);
	static void print_thread_name(const java_lang_Thread& jlt, FILE *stream);
	static void set_javathread_state(threadobject *t, int state);
	static threadobject *get_threadobject_from_thread(java_handle_t *h);
	static void thread_create_initial_threadgroups(java_handle_t **threadgroup_system, java_handle_t **threadgroup_main);
	static bool invoke_thread_initializer(java_lang_Thread& jlt, threadobject *t, methodinfo *thread_method_init, java_handle_t *name, java_handle_t *group);
};

typedef ThreadRuntimeClasspath ThreadRuntime;

#endif /* ENABLE_THREADS */

#endif	// __cplusplus

#endif // _THREAD_CLASSPATH_HPP


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
