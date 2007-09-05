/* src/native/vm/gnu/sun_reflect_ConstantPool.c

   Copyright (C) 2007 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, M. S. Panzenböck Institut f. Computersprachen - TU Wien

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

/*******************************************************************************

   XXX: The Methods in this file are very redundant to thouse in
        src/native/vm/sun/jvm.c Unless someone has a good idea how to cover
	such redundancy I leave it how it is.

*******************************************************************************/

#include "config.h"

#include <assert.h>
#include <stdint.h>

#include "mm/memory.h"

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"

#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_Class.h"
#include "native/include/sun_reflect_ConstantPool.h"

#include "native/vm/reflect.h"

#include "toolbox/logging.h"

#include "vm/vm.h"
#include "vm/exceptions.h"
#include "vm/resolve.h"
#include "vm/stringlocal.h"

#include "vmcore/class.h"
#include "vmcore/utf8.h"


/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ "getSize0",             "(Ljava/lang/Object;I)I",                          (void *) (intptr_t) &Java_sun_reflect_ConstantPool_getSize0             },
	{ "getClassAt0",          "(Ljava/lang/Object;I)Ljava/lang/Class;",          (void *) (intptr_t) &Java_sun_reflect_ConstantPool_getClassAt0          },
	{ "getClassAtIfLoaded0",  "(Ljava/lang/Object;I)Ljava/lang/Class;",          (void *) (intptr_t) &Java_sun_reflect_ConstantPool_getClassAtIfLoaded0  },
	{ "getMethodAt0",         "(Ljava/lang/Object;I)Ljava/lang/reflect/Member;", (void *) (intptr_t) &Java_sun_reflect_ConstantPool_getMethodAt0         },
	{ "getMethodAtIfLoaded0", "(Ljava/lang/Object;I)Ljava/lang/reflect/Member;", (void *) (intptr_t) &Java_sun_reflect_ConstantPool_getMethodAtIfLoaded0 },
	{ "getFieldAt0",          "(Ljava/lang/Object;I)Ljava/lang/reflect/Field;",  (void *) (intptr_t) &Java_sun_reflect_ConstantPool_getFieldAt0          },
	{ "getFieldAtIfLoaded0",  "(Ljava/lang/Object;I)Ljava/lang/reflect/Field;",  (void *) (intptr_t) &Java_sun_reflect_ConstantPool_getFieldAtIfLoaded0  },
	{ "getMemberRefInfoAt0",  "(Ljava/lang/Object;I)[Ljava/lang/String;",        (void *) (intptr_t) &Java_sun_reflect_ConstantPool_getMemberRefInfoAt0  },
	{ "getIntAt0",            "(Ljava/lang/Object;I)I",                          (void *) (intptr_t) &Java_sun_reflect_ConstantPool_getIntAt0            },
	{ "getLongAt0",           "(Ljava/lang/Object;I)J",                          (void *) (intptr_t) &Java_sun_reflect_ConstantPool_getLongAt0           },
	{ "getFloatAt0",          "(Ljava/lang/Object;I)F",                          (void *) (intptr_t) &Java_sun_reflect_ConstantPool_getFloatAt0          },
	{ "getDoubleAt0",         "(Ljava/lang/Object;I)D",                          (void *) (intptr_t) &Java_sun_reflect_ConstantPool_getDoubleAt0         },
	{ "getStringAt0",         "(Ljava/lang/Object;I)Ljava/lang/String;",         (void *) (intptr_t) &Java_sun_reflect_ConstantPool_getStringAt0         },
	{ "getUTF8At0",           "(Ljava/lang/Object;I)Ljava/lang/String;",         (void *) (intptr_t) &Java_sun_reflect_ConstantPool_getUTF8At0           },	
};


/* _Jv_sun_reflect_ConstantPool_init ********************************************

   Register native functions.

*******************************************************************************/

void _Jv_sun_reflect_ConstantPool_init(void)
{
	native_method_register(utf_new_char("sun/reflect/ConstantPool"), methods, NATIVE_METHODS_COUNT);
}

/*
 * Class:     sun/reflect/ConstantPool
 * Method:    getSize0
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT int32_t JNICALL Java_sun_reflect_ConstantPool_getSize0(JNIEnv *env, struct sun_reflect_ConstantPool* this, struct java_lang_Object* jcpool)
{
	classinfo *cls = LLNI_classinfo_unwrap(jcpool);
	return cls->cpcount;
}


/*
 * Class:     sun/reflect/ConstantPool
 * Method:    getClassAt0
 * Signature: (Ljava/lang/Object;I)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_sun_reflect_ConstantPool_getClassAt0(JNIEnv *env, struct sun_reflect_ConstantPool* this, struct java_lang_Object* jcpool, int32_t index)
{
	constant_classref *ref;
	classinfo *cls = LLNI_classinfo_unwrap(jcpool);

	ref = (constant_classref*)class_getconstant(
		cls, index, CONSTANT_Class);

	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return NULL;
	}

	return LLNI_classinfo_wrap(resolve_classref_eager(ref));
}


/*
 * Class:     sun/reflect/ConstantPool
 * Method:    getClassAtIfLoaded0
 * Signature: (Ljava/lang/Object;I)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_sun_reflect_ConstantPool_getClassAtIfLoaded0(JNIEnv *env, struct sun_reflect_ConstantPool* this, struct java_lang_Object* jcpool, int32_t index)
{
	constant_classref *ref;
	classinfo *c = NULL;
	classinfo *cls = LLNI_classinfo_unwrap(jcpool);

	ref = (constant_classref*)class_getconstant(
		cls, index, CONSTANT_Class);

	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return NULL;
	}
	
	if (!resolve_classref(NULL,ref,resolveLazy,true,true,&c)) {
		return NULL;
	}

	if (c == NULL || !(c->state & CLASS_LOADED)) {
		return NULL;
	}
	
	return LLNI_classinfo_wrap(c);
}


/*
 * Class:     sun/reflect/ConstantPool
 * Method:    getMethodAt0
 * Signature: (Ljava/lang/Object;I)Ljava/lang/reflect/Member;
 */
JNIEXPORT struct java_lang_reflect_Member* JNICALL Java_sun_reflect_ConstantPool_getMethodAt0(JNIEnv *env, struct sun_reflect_ConstantPool* this, struct java_lang_Object* jcpool, int32_t index)
{
	constant_FMIref *ref;
	classinfo *cls = LLNI_classinfo_unwrap(jcpool);
	
	ref = (constant_FMIref*)class_getconstant(
		cls, index, CONSTANT_Methodref);
	
	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return NULL;
	}

	/* XXX: is that right? or do I have to use resolve_method_*? */
	return (jobject)reflect_method_new(ref->p.method);
}


/*
 * Class:     sun/reflect/ConstantPool
 * Method:    getMethodAtIfLoaded0
 * Signature: (Ljava/lang/Object;I)Ljava/lang/reflect/Member;
 */
JNIEXPORT struct java_lang_reflect_Member* JNICALL Java_sun_reflect_ConstantPool_getMethodAtIfLoaded0(JNIEnv *env, struct sun_reflect_ConstantPool* this, struct java_lang_Object* jcpool, int32_t index)
{
	constant_FMIref *ref;
	classinfo *c = NULL;
	classinfo *cls = LLNI_classinfo_unwrap(jcpool);

	ref = (constant_FMIref*)class_getconstant(
		cls, index, CONSTANT_Methodref);

	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return NULL;
	}

	if (!resolve_classref(NULL,ref->p.classref,resolveLazy,true,true,&c)) {
		return NULL;
	}

	if (c == NULL || !(c->state & CLASS_LOADED)) {
		return NULL;
	}

	return (jobject)reflect_method_new(ref->p.method);
}


/*
 * Class:     sun/reflect/ConstantPool
 * Method:    getFieldAt0
 * Signature: (Ljava/lang/Object;I)Ljava/lang/reflect/Field;
 */
JNIEXPORT struct java_lang_reflect_Field* JNICALL Java_sun_reflect_ConstantPool_getFieldAt0(JNIEnv *env, struct sun_reflect_ConstantPool* this, struct java_lang_Object* jcpool, int32_t index)
{
	constant_FMIref *ref;
	classinfo *cls = LLNI_classinfo_unwrap(jcpool);

	ref = (constant_FMIref*)class_getconstant(
		cls, index, CONSTANT_Fieldref);

	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return NULL;
	}

	return (jobject)reflect_field_new(ref->p.field);
}


/*
 * Class:     sun/reflect/ConstantPool
 * Method:    getFieldAtIfLoaded0
 * Signature: (Ljava/lang/Object;I)Ljava/lang/reflect/Field;
 */
JNIEXPORT struct java_lang_reflect_Field* JNICALL Java_sun_reflect_ConstantPool_getFieldAtIfLoaded0(JNIEnv *env, struct sun_reflect_ConstantPool* this, struct java_lang_Object* jcpool, int32_t index)
{
	constant_FMIref *ref;
	classinfo *c;
	classinfo *cls = LLNI_classinfo_unwrap(jcpool);

	ref = (constant_FMIref*)class_getconstant(
		cls, index, CONSTANT_Fieldref);

	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return NULL;
	}

	if (!resolve_classref(NULL,ref->p.classref,resolveLazy,true,true,&c)) {
		return NULL;
	}

	if (c == NULL || !(c->state & CLASS_LOADED)) {
		return NULL;
	}

	return (jobject)reflect_field_new(ref->p.field);
}


/*
 * Class:     sun/reflect/ConstantPool
 * Method:    getMemberRefInfoAt0
 * Signature: (Ljava/lang/Object;I)[Ljava/lang/String;
 */
JNIEXPORT java_handle_objectarray_t* JNICALL Java_sun_reflect_ConstantPool_getMemberRefInfoAt0(JNIEnv *env, struct sun_reflect_ConstantPool* this, struct java_lang_Object* jcpool, int32_t index)
{
	log_println("Java_sun_reflect_ConstantPool_getMemberRefInfoAt0(env=%p, jcpool=%p, index=%d): IMPLEMENT ME!", env, jcpool, index);
	return NULL;
}


/*
 * Class:     sun/reflect/ConstantPool
 * Method:    getIntAt0
 * Signature: (Ljava/lang/Object;I)I
 */
JNIEXPORT int32_t JNICALL Java_sun_reflect_ConstantPool_getIntAt0(JNIEnv *env, struct sun_reflect_ConstantPool* this, struct java_lang_Object* jcpool, int32_t index)
{
	constant_integer *ref;
	classinfo *cls = LLNI_classinfo_unwrap(jcpool);

	ref = (constant_integer*)class_getconstant(
		cls, index, CONSTANT_Integer);

	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return 0;
	}

	return ref->value;
}


/*
 * Class:     sun/reflect/ConstantPool
 * Method:    getLongAt0
 * Signature: (Ljava/lang/Object;I)J
 */
JNIEXPORT int64_t JNICALL Java_sun_reflect_ConstantPool_getLongAt0(JNIEnv *env, struct sun_reflect_ConstantPool* this, struct java_lang_Object* jcpool, int32_t index)
{
	constant_long *ref;
	classinfo *cls = LLNI_classinfo_unwrap(jcpool);

	ref = (constant_long*)class_getconstant(
		cls, index, CONSTANT_Long);

	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return 0;
	}

	return ref->value;
}


/*
 * Class:     sun/reflect/ConstantPool
 * Method:    getFloatAt0
 * Signature: (Ljava/lang/Object;I)F
 */
JNIEXPORT float JNICALL Java_sun_reflect_ConstantPool_getFloatAt0(JNIEnv *env, struct sun_reflect_ConstantPool* this, struct java_lang_Object* jcpool, int32_t index)
{
	constant_float *ref;
	classinfo *cls = LLNI_classinfo_unwrap(jcpool);

	ref = (constant_float*)class_getconstant(
		cls, index, CONSTANT_Float);

	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return 0;
	}

	return ref->value;
}


/*
 * Class:     sun/reflect/ConstantPool
 * Method:    getDoubleAt0
 * Signature: (Ljava/lang/Object;I)D
 */
JNIEXPORT double JNICALL Java_sun_reflect_ConstantPool_getDoubleAt0(JNIEnv *env, struct sun_reflect_ConstantPool* this, struct java_lang_Object* jcpool, int32_t index)
{
	constant_double *ref;
	classinfo *cls = LLNI_classinfo_unwrap(jcpool);

	ref = (constant_double*)class_getconstant(
		cls, index, CONSTANT_Double);

	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return 0;
	}

	return ref->value;
}


/*
 * Class:     sun/reflect/ConstantPool
 * Method:    getStringAt0
 * Signature: (Ljava/lang/Object;I)Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_sun_reflect_ConstantPool_getStringAt0(JNIEnv *env, struct sun_reflect_ConstantPool* this, struct java_lang_Object* jcpool, int32_t index)
{
	utf *ref;
	classinfo *cls = LLNI_classinfo_unwrap(jcpool);
	
	ref = (utf*)class_getconstant(cls, index, CONSTANT_String);

	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return NULL;
	}

	/* XXX: I hope literalstring_new is the right Function. */
	return (java_lang_String*)literalstring_new(ref);
}


/*
 * Class:     sun/reflect/ConstantPool
 * Method:    getUTF8At0
 * Signature: (Ljava/lang/Object;I)Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_sun_reflect_ConstantPool_getUTF8At0(JNIEnv *env, struct sun_reflect_ConstantPool* this, struct java_lang_Object* jcpool, int32_t index)
{
	utf *ref;
	classinfo *cls = LLNI_classinfo_unwrap(jcpool);

	ref = (utf*)class_getconstant(cls, index, CONSTANT_Utf8);

	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return NULL;
	}

	/* XXX: I hope literalstring_new is the right Function. */
	return (java_lang_String*)literalstring_new(ref);
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
