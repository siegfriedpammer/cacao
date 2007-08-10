/* src/native/vm/gnu/java_lang_reflect_Method.c

   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
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

   $Id: java_lang_reflect_Method.c 8284 2007-08-10 08:58:39Z michi $

*/


#include "config.h"

#include <assert.h>

#include "vm/types.h"

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"

#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_String.h"

#include "native/include/java_lang_reflect_Method.h"

#include "native/vm/java_lang_reflect_Method.h"

#include "vm/access.h"
#include "vm/global.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/initialize.h"
#include "vm/resolve.h"
#include "vm/stringlocal.h"

#include "vmcore/method.h"


/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ "getModifiersInternal", "()I",                                                                         (void *) (ptrint) &Java_java_lang_reflect_Method_getModifiersInternal },
	{ "getReturnType",        "()Ljava/lang/Class;",                                                         (void *) (ptrint) &Java_java_lang_reflect_Method_getReturnType        },
	{ "getParameterTypes",    "()[Ljava/lang/Class;",                                                        (void *) (ptrint) &Java_java_lang_reflect_Method_getParameterTypes    },
	{ "getExceptionTypes",    "()[Ljava/lang/Class;",                                                        (void *) (ptrint) &Java_java_lang_reflect_Method_getExceptionTypes    },
	{ "invokeNative",         "(Ljava/lang/Object;[Ljava/lang/Object;Ljava/lang/Class;I)Ljava/lang/Object;", (void *) (ptrint) &Java_java_lang_reflect_Method_invokeNative         },
	{ "getSignature",         "()Ljava/lang/String;",                                                        (void *) (ptrint) &Java_java_lang_reflect_Method_getSignature         },
};


/* _Jv_java_lang_reflect_Method_init *******************************************

   Register native functions.

*******************************************************************************/

void _Jv_java_lang_reflect_Method_init(void)
{
	utf *u;

	u = utf_new_char("java/lang/reflect/Method");

	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}


/*
 * Class:     java/lang/reflect/Method
 * Method:    getModifiersInternal
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Method_getModifiersInternal(JNIEnv *env, java_lang_reflect_Method *this)
{
	classinfo  *c;
	methodinfo *m;
	int32_t     slot;

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	m = &(c->methods[slot]);

	return m->flags;
}


/*
 * Class:     java/lang/reflect/Method
 * Method:    getReturnType
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_reflect_Method_getReturnType(JNIEnv *env, java_lang_reflect_Method *this)
{
	classinfo  *c;
	methodinfo *m;
	classinfo  *result;
	int32_t     slot;

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	m = &(c->methods[slot]);

	result = method_returntype_get(m);

	return (java_lang_Class *) result;
}


/*
 * Class:     java/lang/reflect/Method
 * Method:    getParameterTypes
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_reflect_Method_getParameterTypes(JNIEnv *env, java_lang_reflect_Method *this)
{
	classinfo  *c;
	methodinfo *m;
	int32_t     slot;

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	m = &(c->methods[slot]);

	return method_get_parametertypearray(m);
}


/*
 * Class:     java/lang/reflect/Method
 * Method:    getExceptionTypes
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_reflect_Method_getExceptionTypes(JNIEnv *env, java_lang_reflect_Method *this)
{
	classinfo  *c;
	methodinfo *m;
	int32_t     slot;

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	m = &(c->methods[slot]);

	return method_get_exceptionarray(m);
}


/*
 * Class:     java/lang/reflect/Method
 * Method:    invokeNative
 * Signature: (Ljava/lang/Object;[Ljava/lang/Object;Ljava/lang/Class;I)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_java_lang_reflect_Method_invokeNative(JNIEnv *env, java_lang_reflect_Method *this, java_lang_Object *o, java_objectarray *args, java_lang_Class *clazz, s4 slot)
{
	/* just to be sure */

	assert(LLNI_field_direct(this, clazz) == clazz);
	assert(LLNI_field_direct(this, slot)  == slot);

	return _Jv_java_lang_reflect_Method_invoke(this, o, args);
}


/*
 * Class:     java/lang/reflect/Method
 * Method:    getSignature
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT java_lang_String* JNICALL Java_java_lang_reflect_Method_getSignature(JNIEnv *env, java_lang_reflect_Method* this)
{
	classinfo         *c;
	methodinfo        *m;
	java_objectheader *o;
	int32_t            slot;

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	m = &(c->methods[slot]);

	if (m->signature == NULL)
		return NULL;

	o = javastring_new(m->signature);

	/* in error case o is NULL */

	return (java_lang_String *) o;
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
 */
