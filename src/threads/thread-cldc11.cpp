/* src/threads/thread-cldc11.cpp - thread functions specific to the CLDC 1.1 library

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


#include "thread-cldc11.hpp"


#include "mm/gc.hpp"
#include "vm/exceptions.hpp"
#include "vm/globals.hpp"
#include "vm/javaobjects.hpp"
#include "threadlist.hpp"

#if defined(ENABLE_THREADS) && defined(WITH_JAVA_RUNTIME_LIBRARY_CLDC1_1)

classinfo *ThreadRuntimeCldc11::get_thread_class_from_object(java_handle_t *object) {
	classinfo *c;
	LLNI_class_get(object, c);
	return c;
}

java_handle_t *ThreadRuntimeCldc11::get_vmthread_handle(const java_lang_Thread &jlt) {
	return jlt.get_handle();
}

java_handle_t *ThreadRuntimeCldc11::get_thread_exception_handler(const java_lang_Thread &jlt)
{
	#error unknown
}

methodinfo *ThreadRuntimeCldc11::get_threadgroup_remove_method(classinfo *c)
{
	#error unknown
}

methodinfo *ThreadRuntimeCldc11::get_thread_init_method()
{
	#error unknown
}

void ThreadRuntimeCldc11::setup_thread_vmdata(const java_lang_Thread& jlt, threadobject *t)
{
	// Nothing to do.
}

void ThreadRuntimeCldc11::print_thread_name(const java_lang_Thread& jlt, FILE *stream)
{
	#error unknown
}

void ThreadRuntimeCldc11::set_javathread_state(threadobject *t, int state)
{
	// Nothing to do.
}

threadobject *ThreadRuntimeCldc11::get_threadobject_from_thread(java_handle_t *h)
{
	#error unknown
}

void ThreadRuntimeCldc11::thread_create_initial_threadgroups(java_handle_t **threadgroup_system, java_handle_t **threadgroup_main)
{
	#error unknown
}

bool ThreadRuntimeCldc11::invoke_thread_initializer(java_lang_Thread& jlt, threadobject *t, methodinfo *thread_method_init, java_handle_t *name, java_handle_t *group)
{
	// Set the thread data-structure in the Java thread object.
	jlt.set_vm_thread(t);

	// Call: public Thread(Ljava/lang/String;)V
	(void) vm_call_method(thread_method_init, jlt.get_handle(), name);

	if (exceptions_get_exception())
		return false;
}

void ThreadRuntimeOpenjdk::clear_heap_reference(java_lang_Thread& jlt)
{
	// Nothing to do.
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
