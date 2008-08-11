/* src/vm/javaobjects.cpp - functions to create and access Java objects

   Copyright (C) 2008 Theobroma Systems Ltd.

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


#include "config.h"

#include <stdint.h>

#include "native/vm/reflection.hpp"

#include "vm/access.h"
#include "vm/builtin.h"
#include "vm/global.h"
#include "vm/globals.hpp"
#include "vm/initialize.h"
#include "vm/javaobjects.hpp"


#if defined(ENABLE_JAVASE)

/**
 * Constructs a Java object with the given
 * java.lang.reflect.Constructor.
 *
 * @param args     Constructor arguments.
 *
 * @return Handle to Java object.
 */
java_handle_t* java_lang_reflect_Constructor::new_instance(java_handle_objectarray_t* args)
{
	methodinfo* m = get_method();

	// Should we bypass security the checks (AccessibleObject)?
	if (get_override() == false) {
		/* This method is always called like this:
		       [0] java.lang.reflect.Constructor.constructNative (Native Method)
		       [1] java.lang.reflect.Constructor.newInstance
		       [2] <caller>
		*/

		if (!access_check_method(m, 2))
			return NULL;
	}

	// Create a Java object.
	java_handle_t* h = builtin_new(m->clazz);

	if (h == NULL)
		return NULL;
        
	// Call initializer.
	(void) Reflection::invoke(m, h, args);

	return h;
}


/**
 * Invokes the given method.
 *
 * @param args Method arguments.
 *
 * @return return value of the method
 */
java_handle_t* java_lang_reflect_Method::invoke(java_handle_t* o, java_handle_objectarray_t* args)
{
	methodinfo* m = get_method();

	// Should we bypass security the checks (AccessibleObject)?
	if (get_override() == false) {
#if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)
		/* This method is always called like this:
		       [0] java.lang.reflect.Method.invokeNative (Native Method)
		       [1] java.lang.reflect.Method.invoke (Method.java:329)
		       [2] <caller>
		*/

		if (!access_check_method(m, 2))
			return NULL;
#elif defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)
		/* We only pass 1 here as stacktrace_get_caller_class, which
		   is called from access_check_method, skips
		   java.lang.reflect.Method.invoke(). */

		if (!access_check_method(m, 1))
			return NULL;
#else
# error unknown classpath configuration
#endif
	}

	// Check if method class is initialized.
	if (!(m->clazz->state & CLASS_INITIALIZED))
		if (!initialize_class(m->clazz))
			return NULL;

	// Call the Java method.
	java_handle_t* result = Reflection::invoke(m, o, args);

	return result;
}


// Legacy C interface.

extern "C" {
	java_handle_t* java_lang_reflect_Constructor_create(methodinfo* m) { return java_lang_reflect_Constructor(m).get_handle(); }
	java_handle_t* java_lang_reflect_Field_create(fieldinfo* f) { return java_lang_reflect_Field(f).get_handle(); }
	java_handle_t* java_lang_reflect_Method_create(methodinfo* m) { return java_lang_reflect_Method(m).get_handle(); }
}

#endif // ENABLE_JAVASE


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
