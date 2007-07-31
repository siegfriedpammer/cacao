/* src/native/vm/reflect.c - helper functions for java/lang/reflect

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

   $Id: reflect.c 8249 2007-07-31 12:59:03Z panzi $

*/


#include "config.h"

#include <stdint.h>

#if defined(ENABLE_ANNOTATIONS)
#include "mm/memory.h"
#endif

#include "native/jni.h"
#include "native/native.h"

/* keep this order of the native includes */

#include "native/include/java_lang_String.h"

#if defined(WITH_CLASSPATH_SUN)
# include "native/include/java_nio_ByteBuffer.h"        /* required by j.l.CL */
#endif
#include "native/include/java_lang_ClassLoader.h"

#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_reflect_Constructor.h"
#include "native/include/java_lang_reflect_Field.h"
#include "native/include/java_lang_reflect_Method.h"

#include "native/vm/java_lang_String.h"
#include "native/vm/reflect.h"

#include "vm/builtin.h"
#include "vm/global.h"
#include "vm/stringlocal.h"

#include "vmcore/method.h"


/* reflect_constructor_new *****************************************************

   Allocates a new java.lang.reflect.Constructor object and
   initializes the fields with the method passed.

*******************************************************************************/

java_lang_reflect_Constructor *reflect_constructor_new(methodinfo *m)
{
	classinfo                     *c;
	java_objectheader             *o;
	java_lang_reflect_Constructor *rc;
	int32_t                        slot;
	java_bytearray *annotations          = NULL;
	java_bytearray *parameterAnnotations = NULL;
	annotation_bytearray_t *ba = NULL;

#if defined(ENABLE_ANNOTATIONS)
	/* get annotations */
	ba = method_get_annotations(m);

	if (ba != NULL) {
		annotations = builtin_newarray_byte(ba->size);

		if (annotations == NULL)
			return NULL;
		
		MCOPY(annotations->data, ba->data, uint8_t, ba->size);
	}
	
	/* get parameter annotations */
	ba = method_get_parameterannotations(m);

	if (ba != NULL) {
		parameterAnnotations = builtin_newarray_byte(ba->size);

		if (parameterAnnotations == NULL)
			return NULL;
		
		MCOPY(parameterAnnotations->data, ba->data, uint8_t, ba->size);
	}
#endif

	/* get declaring class */

	c = (classinfo *) m->class;

	/* allocate a new object */

	o = builtin_new(class_java_lang_reflect_Constructor);

	if (o == NULL)
		return NULL;

	/* initialize instance fields */

	rc = (java_lang_reflect_Constructor *) o;

	/* calculate the slot */

	slot = m - c->methods;

#if defined(WITH_CLASSPATH_GNU)

	rc->clazz                = (java_lang_Class *) c;
	rc->slot                 = slot;

	/* TODO: add these private fields to java.lang.reflect.Constructor
	rc->annotations          = annotations;
	rc->parameterAnnotations = parameterAnnotations;
	*/

#elif defined(WITH_CLASSPATH_SUN)

	rc->clazz                = (java_lang_Class *) c;
	rc->parameterTypes       = method_get_parametertypearray(m);
	rc->exceptionTypes       = method_get_exceptionarray(m);
	rc->modifiers            = m->flags & ACC_CLASS_REFLECT_MASK;
	rc->slot                 = slot;
	rc->signature            = m->signature ? (java_lang_String *) javastring_new(m->signature) : NULL;
	rc->annotations          = annotations;
	rc->parameterAnnotations = parameterAnnotations;

#else
# error unknown classpath configuration
#endif

	return rc;
}


/* reflect_field_new ***********************************************************

   Allocates a new java.lang.reflect.Field object and initializes the
   fields with the field passed.

*******************************************************************************/

java_lang_reflect_Field *reflect_field_new(fieldinfo *f)
{
	classinfo               *c;
	java_objectheader       *o;
	java_lang_reflect_Field *rf;
	int32_t                  slot;
	java_bytearray *annotations = NULL;
	annotation_bytearray_t *ba = NULL;

#if defined(ENABLE_ANNOTATIONS)
	/* get annotations */
	ba = field_get_annotations(f);

	if (ba != NULL) {
		annotations = builtin_newarray_byte(ba->size);

		if (annotations == NULL)
			return NULL;
		
		MCOPY(annotations->data, ba->data, uint8_t, ba->size);
	}
#endif

	/* get declaring class */

	c = (classinfo *) f->class;

	/* allocate a new object */

	o = builtin_new(class_java_lang_reflect_Field);

	if (o == NULL)
		return NULL;

	/* initialize instance fields */

	rf = (java_lang_reflect_Field *) o;

	/* calculate the slot */

	slot = f - c->fields;

#if defined(WITH_CLASSPATH_GNU)

	rf->clazz = (java_lang_Class *) c;

	/* The name needs to be interned */
	/* XXX implement me better! */

	rf->name           = _Jv_java_lang_String_intern((java_lang_String *) javastring_new(f->name));
	rf->slot           = slot;
	rf->annotations    = annotations;

#elif defined(WITH_CLASSPATH_SUN)

	rf->clazz          = (java_lang_Class *) c;

	/* The name needs to be interned */
	/* XXX implement me better! */

	rf->name           = _Jv_java_lang_String_intern((java_lang_String *) javastring_new(f->name));
	rf->type           = (java_lang_Class *) field_get_type(f);
	rf->modifiers      = f->flags;
	rf->slot           = slot;
	rf->signature      = f->signature ? (java_lang_String *) javastring_new(f->signature) : NULL;
	rf->annotations    = annotations;

#else
# error unknown classpath configuration
#endif

	return rf;
}


/* reflect_method_new **********************************************************

   Allocates a new java.lang.reflect.Method object and initializes the
   fields with the method passed.

*******************************************************************************/

java_lang_reflect_Method *reflect_method_new(methodinfo *m)
{
	classinfo                *c;
	java_objectheader        *o;
	java_lang_reflect_Method *rm;
	int32_t                   slot;
	java_bytearray *annotations          = NULL;
	java_bytearray *parameterAnnotations = NULL;
	java_bytearray *annotationDefault    = NULL;
	annotation_bytearray_t *ba = NULL;

#if defined(ENABLE_ANNOTATIONS)
	/* get annotations */
	ba = method_get_annotations(m);

	if (ba != NULL) {
		annotations = builtin_newarray_byte(ba->size);

		if (annotations == NULL)
			return NULL;
		
		MCOPY(annotations->data, ba->data, uint8_t, ba->size);
	}
	
	/* get parameter annotations */
	ba = method_get_parameterannotations(m);

	if (ba != NULL) {
		parameterAnnotations = builtin_newarray_byte(ba->size);

		if (parameterAnnotations == NULL)
			return NULL;
		
		MCOPY(parameterAnnotations->data, ba->data, uint8_t, ba->size);
	}

	/* get annotation default value */
	ba = method_get_annotationdefault(m);

	if (ba != NULL) {
		annotationDefault = builtin_newarray_byte(ba->size);

		if (annotationDefault == NULL)
			return NULL;
		
		MCOPY(annotationDefault->data, ba->data, uint8_t, ba->size);
	}
#endif

	/* get declaring class */

	c = (classinfo *) m->class;

	/* allocate a new object */

	o = builtin_new(class_java_lang_reflect_Method);

	if (o == NULL)
		return NULL;

	/* initialize instance fields */

	rm = (java_lang_reflect_Method *) o;

	/* calculate the slot */

	slot = m - c->methods;

#if defined(WITH_CLASSPATH_GNU)

	rm->clazz                = (java_lang_Class *) m->class;

	/* The name needs to be interned */
	/* XXX implement me better! */

	rm->name                 = _Jv_java_lang_String_intern((java_lang_String *) javastring_new(m->name));
	rm->slot                 = slot;
	rm->annotations          = annotations;
	rm->parameterAnnotations = parameterAnnotations;
	rm->annotationDefault    = annotationDefault;

#elif defined(WITH_CLASSPATH_SUN)

	rm->clazz                = (java_lang_Class *) m->class;

	/* The name needs to be interned */
	/* XXX implement me better! */

	rm->name                 = _Jv_java_lang_String_intern((java_lang_String *) javastring_new(m->name));
	rm->parameterTypes       = method_get_parametertypearray(m);
	rm->returnType           = (java_lang_Class *) method_returntype_get(m);
	rm->exceptionTypes       = method_get_exceptionarray(m);
	rm->modifiers            = m->flags & ACC_CLASS_REFLECT_MASK;
	rm->slot                 = slot;
	rm->signature            = m->signature ? (java_lang_String *) javastring_new(m->signature) : NULL;
	rm->annotations          = annotations;
	rm->parameterAnnotations = parameterAnnotations;
	rm->annotationDefault    = annotationDefault;

#else
# error unknown classpath configuration
#endif

	return rm;
}


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
