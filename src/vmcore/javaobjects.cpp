/* src/vmcore/javaobjects.cpp - functions to create and access Java objects

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

// REMOVEME
#include "native/vm/reflection.hpp"

#include "vm/access.h"
#include "vm/builtin.h"
#include "vm/global.h"

#include "vmcore/globals.hpp"
#include "vmcore/javaobjects.hpp"


#if defined(ENABLE_JAVASE)

/**
 * Allocates a new java.lang.reflect.Constructor object and
 * initializes the fields with the method passed.
 */
java_handle_t* java_lang_reflect_Constructor::create(methodinfo *m)
{
	java_handle_t* h;

	/* Allocate a java.lang.reflect.Constructor object. */

	h = builtin_new(class_java_lang_reflect_Constructor);

	if (h == NULL)
		return NULL;

	java_lang_reflect_Constructor rc(h);

#if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)

	/* Allocate a java.lang.reflect.VMConstructor object. */

	h = builtin_new(class_java_lang_reflect_VMConstructor);

	if (h == NULL)
		return NULL;

	java_lang_reflect_VMConstructor rvmc(h, m);

	// Link the two Java objects.

	rc.set_cons(rvmc.get_handle());
	rvmc.set_cons(rc.get_handle());

#elif defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)

	/* Calculate the slot. */

	int slot = m - m->clazz->methods;

	/* Set Java object instance fields. */

	LLNI_field_set_cls(rc, clazz               , m->clazz);
	LLNI_field_set_ref(rc, parameterTypes      , method_get_parametertypearray(m));
	LLNI_field_set_ref(rc, exceptionTypes      , method_get_exceptionarray(m));
	LLNI_field_set_val(rc, modifiers           , m->flags & ACC_CLASS_REFLECT_MASK);
	LLNI_field_set_val(rc, slot                , slot);
	LLNI_field_set_ref(rc, signature           , m->signature ? (java_lang_String *) javastring_new(m->signature) : NULL);
	LLNI_field_set_ref(rc, annotations         , method_get_annotations(m));
	LLNI_field_set_ref(rc, parameterAnnotations, method_get_parameterannotations(m));

#else
# error unknown classpath configuration
#endif

	return rc.get_handle();
}


/**
 * Constructs a Java object with the given
 * java.lang.reflect.Constructor.
 *
 * @param m        Method structure of the constructor.
 * @param args     Constructor arguments.
 * @param override Override security checks.
 *
 * @return Handle to Java object.
 */
java_handle_t* java_lang_reflect_Constructor::new_instance(methodinfo* m, java_handle_objectarray_t* args, bool override)
{
	java_handle_t* h;

	// Should we bypass security the checks (AccessibleObject)?
	if (override == false) {
		/* This method is always called like this:
		       [0] java.lang.reflect.Constructor.constructNative (Native Method)
		       [1] java.lang.reflect.Constructor.newInstance
		       [2] <caller>
		*/

		if (!access_check_method(m, 2))
			return NULL;
	}

	// Create a Java object.
	h = builtin_new(m->clazz);

	if (h == NULL)
		return NULL;
        
	// Call initializer.
	(void) Reflection::invoke(m, h, args);

	return h;
}


/**
 * Creates a java.lang.reflect.Field object on the GC heap and
 * intializes it with the given field.
 *
 * @param f Field structure.
 *
 * @return Handle to Java object.
 */
java_handle_t* java_lang_reflect_Field::create(fieldinfo* f)
{
	java_handle_t* h;

	/* Allocate a java.lang.reflect.Field object. */

	h = builtin_new(class_java_lang_reflect_Field);

	if (h == NULL)
		return NULL;

	java_lang_reflect_Field rf(h);

#if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)

	// Allocate a java.lang.reflect.VMField object.

	h = builtin_new(class_java_lang_reflect_VMField);

	if (h == NULL)
		return NULL;

	java_lang_reflect_VMField rvmf(h, f);

	// Link the two Java objects.

	rf.set_f(rvmf.get_handle());
	rvmf.set_f(rf.get_handle());

#elif defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)

	/* Calculate the slot. */

	int slot = f - f->clazz->fields;

	/* Set the Java object fields. */

	LLNI_field_set_cls(rf, clazz,       f->clazz);

	/* The name needs to be interned */
	/* XXX implement me better! */

	LLNI_field_set_ref(rf, name,        (java_lang_String *) javastring_intern(javastring_new(f->name)));
	LLNI_field_set_cls(rf, type,        (java_lang_Class *) field_get_type(f));
	LLNI_field_set_val(rf, modifiers,   f->flags);
	LLNI_field_set_val(rf, slot,        slot);
	LLNI_field_set_ref(rf, signature,   f->signature ? (java_lang_String *) javastring_new(f->signature) : NULL);
	LLNI_field_set_ref(rf, annotations, field_get_annotations(f));

#else
# error unknown classpath configuration
#endif

	return rf.get_handle();
}


/*
 * Allocates a new java.lang.reflect.Method object and initializes the
 * fields with the method passed.
 *
 * @param m Method structure.
 *
 * @return Handle to Java object.
 */
java_handle_t* java_lang_reflect_Method::create(methodinfo *m)
{
	java_handle_t* h;

	/* Allocate a java.lang.reflect.Method object. */

	h = builtin_new(class_java_lang_reflect_Method);

	if (h == NULL)
		return NULL;

	java_lang_reflect_Method rm(h);

#if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)

	/* Allocate a java.lang.reflect.VMMethod object. */

	h = builtin_new(class_java_lang_reflect_VMMethod);

	if (h == NULL)
		return NULL;

	java_lang_reflect_VMMethod rvmm(h, m);

	// Link the two Java objects.

	rm.set_m(rvmm.get_handle());
	rvmm.set_m(rm.get_handle());

#elif defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)

	/* Calculate the slot. */

	int slot = m - m->clazz->methods;

	LLNI_field_set_cls(rm, clazz,                m->clazz);

	/* The name needs to be interned */
	/* XXX implement me better! */

	LLNI_field_set_ref(rm, name,                 (java_lang_String *) javastring_intern(javastring_new(m->name)));
	LLNI_field_set_ref(rm, parameterTypes,       method_get_parametertypearray(m));
	LLNI_field_set_cls(rm, returnType,           (java_lang_Class *) method_returntype_get(m));
	LLNI_field_set_ref(rm, exceptionTypes,       method_get_exceptionarray(m));
	LLNI_field_set_val(rm, modifiers,            m->flags & ACC_CLASS_REFLECT_MASK);
	LLNI_field_set_val(rm, slot,                 slot);
	LLNI_field_set_ref(rm, signature,            m->signature ? (java_lang_String *) javastring_new(m->signature) : NULL);
	LLNI_field_set_ref(rm, annotations,          method_get_annotations(m));
	LLNI_field_set_ref(rm, parameterAnnotations, method_get_parameterannotations(m));
	LLNI_field_set_ref(rm, annotationDefault,    method_get_annotationdefault(m));

#else
# error unknown classpath configuration
#endif

	return rm.get_handle();
}


// Legacy C interface.

extern "C" {
java_handle_t* java_lang_reflect_Constructor_create(methodinfo* m) { return java_lang_reflect_Constructor::create(m); }
java_handle_t* java_lang_reflect_Field_create(fieldinfo* f) { return java_lang_reflect_Field::create(f); }
java_handle_t* java_lang_reflect_Method_create(methodinfo* m) { return java_lang_reflect_Method::create(m); }
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
