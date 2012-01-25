/* src/threads/thread-openjdk.cpp - thread functions specific to the OpenJDK library

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


#include "thread-openjdk.hpp"

#include "vm/global.h"
#include "vm/globals.hpp"

#include "mm/gc.hpp"
#include "vm/globals.hpp"
#include "vm/javaobjects.hpp"
#include "vm/exceptions.hpp"

#include "threadlist.hpp"

#if defined(ENABLE_THREADS) && defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)

classinfo *ThreadRuntimeOpenjdk::get_thread_class_from_object(java_handle_t *object) {
	classinfo *c;
	LLNI_class_get(object, c);
	return c;
}

java_handle_t *ThreadRuntimeOpenjdk::get_vmthread_handle(const java_lang_Thread &jlt) {
	return jlt.get_handle();
}

java_handle_t *ThreadRuntimeOpenjdk::get_thread_exception_handler(const java_lang_Thread &jlt)
{
	return jlt.get_uncaughtExceptionHandler();
}

methodinfo *ThreadRuntimeOpenjdk::get_threadgroup_remove_method(classinfo *c)
{
	return class_resolveclassmethod(c,
									utf_remove,
									utf_java_lang_Thread__V,
									class_java_lang_ThreadGroup,
									true);
}

methodinfo *ThreadRuntimeOpenjdk::get_thread_init_method()
{
	return class_resolveclassmethod(class_java_lang_Thread,
									utf_init,
									utf_new_char("(Ljava/lang/ThreadGroup;Ljava/lang/String;)V"),
									class_java_lang_Thread,
									true);
}

void ThreadRuntimeOpenjdk::setup_thread_vmdata(const java_lang_Thread& jlt, threadobject *t)
{
	// Nothing to do.
}

void ThreadRuntimeOpenjdk::print_thread_name(const java_lang_Thread& jlt, FILE *stream)
{
	/* FIXME: In OpenJDK and CLDC the name is a char[]. */
	//java_chararray_t *name;

	/* FIXME This prints to stdout. */
	utf_display_printable_ascii(utf_null);
}

void ThreadRuntimeOpenjdk::set_javathread_state(threadobject *t, int state)
{
	// Set the state of the java.lang.Thread object.
	java_lang_Thread thread(LLNI_WRAP(t->object));
	assert(thread.is_non_null());
	thread.set_threadStatus(state);
}

threadobject *ThreadRuntimeOpenjdk::get_threadobject_from_thread(java_handle_t *h)
{
	/* XXX This is just a quick hack. */
	return ThreadList::get_thread_from_java_object(h);
}

void ThreadRuntimeOpenjdk::thread_create_initial_threadgroups(java_handle_t **threadgroup_system, java_handle_t **threadgroup_main)
{
	java_handle_t *name;
	methodinfo    *m;

	/* Allocate and initialize the system thread group. */

	*threadgroup_system = native_new_and_init(class_java_lang_ThreadGroup);

	if (*threadgroup_system == NULL)
		vm_abort("thread_create_initial_threadgroups: failed to allocate system threadgroup");

	/* Allocate and initialize the main thread group. */

	*threadgroup_main = builtin_new(class_java_lang_ThreadGroup);

	if (*threadgroup_main == NULL)
		vm_abort("thread_create_initial_threadgroups: failed to allocate main threadgroup");

	name = javastring_new(utf_main);

	m = class_resolveclassmethod(class_java_lang_ThreadGroup,
								 utf_init,
								 utf_Ljava_lang_ThreadGroup_Ljava_lang_String__V,
								 class_java_lang_ThreadGroup,
								 true);

	if (m == NULL)
		vm_abort("thread_create_initial_threadgroups: failed to resolve threadgroup init method");

	(void) vm_call_method(m, *threadgroup_main, *threadgroup_system, name);

	if (exceptions_get_exception())
		vm_abort("thread_create_initial_threadgroups: exception while initializing main threadgroup");

}

bool ThreadRuntimeOpenjdk::invoke_thread_initializer(java_lang_Thread& jlt, threadobject *t, methodinfo *thread_method_init, java_handle_t *name, java_handle_t *group)
{
	/* Set the priority.  java.lang.Thread.<init> requires it because
	   it sets the priority of the current thread to the parent's one
	   (which is the current thread in this case). */
	jlt.set_priority(NORM_PRIORITY);

	// Call: java.lang.Thread.<init>(Ljava/lang/ThreadGroup;Ljava/lang/String;)V

	(void) vm_call_method(thread_method_init, jlt.get_handle(), group, name);

	if (exceptions_get_exception())
		return false;

	return true;
}

void ThreadRuntimeOpenjdk::clear_heap_reference(java_lang_Thread& jlt)
{
#ifndef WITH_JAVA_RUNTIME_LIBRARY_OPENJDK_7
	jlt.set_me(0);
#endif
}

#endif /* ENABLE_THREADS && WITH_JAVA_RUNTIME_LIBRARY_OPENJDK */


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
