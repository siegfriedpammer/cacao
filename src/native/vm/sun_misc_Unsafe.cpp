/* src/native/vm/sun_misc_Unsafe.cpp - sun/misc/Unsafe

   Copyright (C) 2006, 2007, 2008
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


#include "config.h"

#include <stdint.h>
#include <unistd.h>

#include "threads/atomic.hpp"

#include "mm/memory.h"

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"

#include "native/include/java_lang_Object.h"                  /* before c.l.C */
#include "native/include/java_lang_String.h"            /* required by j.l.CL */

#if defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)
# include "native/include/java_nio_ByteBuffer.h"        /* required by j.l.CL */
#endif

#include "native/include/java_lang_ClassLoader.h"        /* required by j.l.C */
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_reflect_Field.h"
#include "native/include/java_lang_Thread.h"             /* required by s.m.U */
#include "native/include/java_lang_Throwable.h"

#if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)
# include "native/include/java_lang_reflect_VMField.h"
#endif

#include "native/include/java_security_ProtectionDomain.h" /* required by smU */

// FIXME
extern "C" {
#include "native/include/sun_misc_Unsafe.h"
}

#include "vm/builtin.h"
#include "vm/exceptions.hpp"
#include "vm/initialize.h"
#include "vm/stringlocal.h"

#include "vmcore/system.h"
#include "vmcore/utf8.h"


// Native functions are exported as C functions.
extern "C" {

/*
 * Class:     sun/misc/Unsafe
 * Method:    registerNatives
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_registerNatives(JNIEnv *env, jclass clazz)
{
	/* The native methods of this function are already registered in
	   _Jv_sun_misc_Unsafe_init() which is called during VM
	   startup. */
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getInt
 * Signature: (Ljava/lang/Object;J)I
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_getInt__Ljava_lang_Object_2J(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset)
{
	int32_t *p;
	int32_t  value;

	p = (int32_t *) (((uint8_t *) o) + offset);

	value = *p;

	return value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putInt
 * Signature: (Ljava/lang/Object;JI)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putInt__Ljava_lang_Object_2JI(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset, int32_t x)
{
	int32_t *p;

	p = (int32_t *) (((uint8_t *) o) + offset);

	*p = x;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getObject
 * Signature: (Ljava/lang/Object;J)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_sun_misc_Unsafe_getObject(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset)
{
	void **p;
	void  *value;

	p = (void **) (((uint8_t *) o) + offset);

	value = *p;

	return (java_lang_Object*) value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putObject
 * Signature: (Ljava/lang/Object;JLjava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putObject(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset, java_lang_Object *x)
{
	void **p;

	p = (void **) (((uint8_t *) o) + offset);

	*p = (void *) x;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getBoolean
 * Signature: (Ljava/lang/Object;J)Z
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_getBoolean(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset)
{
	int32_t *p;
	int32_t  value;

	p = (int32_t *) (((uint8_t *) o) + offset);

	value = *p;

	return value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putBoolean
 * Signature: (Ljava/lang/Object;JZ)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putBoolean(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset, int32_t x)
{
	int32_t *p;

	p = (int32_t *) (((uint8_t *) o) + offset);

	*p = x;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getByte
 * Signature: (Ljava/lang/Object;J)B
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_getByte__Ljava_lang_Object_2J(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset)
{
	int32_t *p;
	int32_t  value;

	p = (int32_t *) (((uint8_t *) o) + offset);

	value = *p;

	return value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putByte
 * Signature: (Ljava/lang/Object;JB)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putByte__Ljava_lang_Object_2JB(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset, int32_t x)
{
	int32_t *p;

	p = (int32_t *) (((uint8_t *) o) + offset);

	*p = x;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getShort
 * Signature: (Ljava/lang/Object;J)S
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_getShort__Ljava_lang_Object_2J(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset)
{
	int32_t *p;
	int32_t  value;

	p = (int32_t *) (((uint8_t *) o) + offset);

	value = *p;

	return value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putShort
 * Signature: (Ljava/lang/Object;JS)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putShort__Ljava_lang_Object_2JS(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset, int32_t x)
{
	int32_t *p;

	p = (int32_t *) (((uint8_t *) o) + offset);

	*p = x;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getChar
 * Signature: (Ljava/lang/Object;J)C
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_getChar__Ljava_lang_Object_2J(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset)
{
	int32_t *p;
	int32_t  value;

	p = (int32_t *) (((uint8_t *) o) + offset);

	value = *p;

	return value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putChar
 * Signature: (Ljava/lang/Object;JC)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putChar__Ljava_lang_Object_2JC(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset, int32_t x)
{
	int32_t *p;

	p = (int32_t *) (((uint8_t *) o) + offset);

	*p = x;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getLong
 * Signature: (Ljava/lang/Object;J)J
 */
JNIEXPORT int64_t JNICALL Java_sun_misc_Unsafe_getLong__Ljava_lang_Object_2J(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset)
{
	int64_t *p;
	int64_t  value;

	p = (int64_t *) (((uint8_t *) o) + offset);

	value = *p;

	return value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putLong
 * Signature: (Ljava/lang/Object;JJ)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putLong__Ljava_lang_Object_2JJ(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset, int64_t x)
{
	int64_t *p;

	p = (int64_t *) (((uint8_t *) o) + offset);

	*p = x;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getFloat
 * Signature: (Ljava/lang/Object;J)F
 */
JNIEXPORT float JNICALL Java_sun_misc_Unsafe_getFloat__Ljava_lang_Object_2J(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset)
{
	float *p;
	float  value;

	p = (float *) (((uint8_t *) o) + offset);

	value = *p;

	return value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putFloat
 * Signature: (Ljava/lang/Object;JF)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putFloat__Ljava_lang_Object_2JF(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset, float x)
{
	float *p;

	p = (float *) (((uint8_t *) o) + offset);

	*p = x;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getDouble
 * Signature: (Ljava/lang/Object;J)D
 */
JNIEXPORT double JNICALL Java_sun_misc_Unsafe_getDouble__Ljava_lang_Object_2J(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset)
{
	double *p;
	double  value;

	p = (double *) (((uint8_t *) o) + offset);

	value = *p;

	return value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putDouble
 * Signature: (Ljava/lang/Object;JD)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putDouble__Ljava_lang_Object_2JD(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset, double x)
{
	double *p;

	p = (double *) (((uint8_t *) o) + offset);

	*p = x;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getByte
 * Signature: (J)B
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_getByte__J(JNIEnv *env, sun_misc_Unsafe *_this, int64_t address)
{
	int8_t *p;
	int8_t  value;

	p = (int8_t *) (intptr_t) address;

	value = *p;

	return (int32_t) value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putByte
 * Signature: (JB)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putByte__JB(JNIEnv *env, sun_misc_Unsafe *_this, int64_t address, int32_t value)
{
	int8_t *p;

	p = (int8_t *) (intptr_t) address;

	*p = (int8_t) value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getShort
 * Signature: (J)S
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_getShort__J(JNIEnv *env, sun_misc_Unsafe *_this, int64_t address)
{
	int16_t *p;
	int16_t  value;

	p = (int16_t *) (intptr_t) address;

	value = *p;

	return (int32_t) value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putShort
 * Signature: (JS)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putShort__JS(JNIEnv *env, sun_misc_Unsafe *_this, int64_t address, int32_t value)
{
	int16_t *p;

	p = (int16_t *) (intptr_t) address;

	*p = (int16_t) value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getChar
 * Signature: (J)C
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_getChar__J(JNIEnv *env, sun_misc_Unsafe *_this, int64_t address)
{
	uint16_t *p;
	uint16_t  value;

	p = (uint16_t *) (intptr_t) address;

	value = *p;

	return (int32_t) value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putChar
 * Signature: (JC)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putChar__JC(JNIEnv *env, sun_misc_Unsafe *_this, int64_t address, int32_t value)
{
	uint16_t *p;

	p = (uint16_t *) (intptr_t) address;

	*p = (uint16_t) value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getInt
 * Signature: (J)I
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_getInt__J(JNIEnv *env, sun_misc_Unsafe *_this, int64_t address)
{
	int32_t *p;
	int32_t  value;

	p = (int32_t *) (intptr_t) address;

	value = *p;

	return value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putInt
 * Signature: (JI)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putInt__JI(JNIEnv *env, struct sun_misc_Unsafe* _this, int64_t address, int32_t value)
{
	int32_t *p;

	p = (int32_t *) (intptr_t) address;

	*p = value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getLong
 * Signature: (J)J
 */
JNIEXPORT int64_t JNICALL Java_sun_misc_Unsafe_getLong__J(JNIEnv *env, sun_misc_Unsafe *_this, int64_t address)
{
	int64_t *p;
	int64_t  value;

	p = (int64_t *) (intptr_t) address;

	value = *p;

	return value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putLong
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putLong__JJ(JNIEnv *env, sun_misc_Unsafe *_this, int64_t address, int64_t value)
{
	int64_t *p;

	p = (int64_t *) (intptr_t) address;

	*p = value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getFloat
 * Signature: (J)F
 */
JNIEXPORT float JNICALL Java_sun_misc_Unsafe_getFloat__J(JNIEnv *env, sun_misc_Unsafe *_this, int64_t address)
{
	float *p;
	float  value;

	p = (float *) (intptr_t) address;

	value = *p;

	return value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putFloat
 * Signature: (JF)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putFloat__JF(JNIEnv *env, struct sun_misc_Unsafe* __this, int64_t address, float value)
{
	float* p;

	p = (float*) (intptr_t) address;

	*p = value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    objectFieldOffset
 * Signature: (Ljava/lang/reflect/Field;)J
 */
JNIEXPORT int64_t JNICALL Java_sun_misc_Unsafe_objectFieldOffset(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_reflect_Field *field)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;

#if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)
	java_lang_reflect_VMField *rvmf;
#endif

#if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)

	LLNI_field_get_ref(field, f,     rvmf);
	LLNI_field_get_cls(rvmf,  clazz, c);
	LLNI_field_get_val(rvmf,  slot , slot);

#elif defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)

	LLNI_field_get_cls(field, clazz, c);
	LLNI_field_get_val(field, slot , slot);

#else
# error unknown configuration
#endif

	f = &(c->fields[slot]);

	return (int64_t) f->offset;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    allocateMemory
 * Signature: (J)J
 */
JNIEXPORT int64_t JNICALL Java_sun_misc_Unsafe_allocateMemory(JNIEnv *env, sun_misc_Unsafe *_this, int64_t bytes)
{
	size_t  length;
	void   *p;

	length = (size_t) bytes;

	if ((length != (uint64_t) bytes) || (bytes < 0)) {
		exceptions_throw_illegalargumentexception();
		return 0;
	}

	p = MNEW(uint8_t, length);

	return (int64_t) (intptr_t) p;
}


#if 0
/* OpenJDK 7 */

/*
 * Class:     sun/misc/Unsafe
 * Method:    setMemory
 * Signature: (Ljava/lang/Object;JJB)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_setMemory(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset, int64_t bytes, int32_t value)
{
	size_t  length;
	void   *p;

	length = (size_t) bytes;

	if ((length != (uint64_t) bytes) || (bytes < 0)) {
		exceptions_throw_illegalargumentexception();
		return;
	}

	/* XXX Missing LLNI: we need to unwrap _this object. */

	p = (void *) (((uint8_t *) o) + offset);

	/* XXX Not sure this is correct. */

	system_memset(p, value, length);
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    copyMemory
 * Signature: (Ljava/lang/Object;JLjava/lang/Object;JJ)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_copyMemory(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *srcBase, int64_t srcOffset, java_lang_Object *destBase, int64_t destOffset, int64_t bytes)
{
	size_t  length;
	void   *src;
	void   *dest;

	if (bytes == 0)
		return;

	length = (size_t) bytes;

	if ((length != (uint64_t) bytes) || (bytes < 0)) {
		exceptions_throw_illegalargumentexception();
		return;
	}

	/* XXX Missing LLNI: We need to unwrap these objects. */

	src  = (void *) (((uint8_t *) srcBase) + srcOffset);
	dest = (void *) (((uint8_t *) destBase) + destOffset);

	system_memcpy(dest, src, length);
}
#else
/*
 * Class:     sun/misc/Unsafe
 * Method:    setMemory
 * Signature: (JJB)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_setMemory(JNIEnv *env, sun_misc_Unsafe *_this, int64_t address, int64_t bytes, int32_t value)
{
	size_t  length;
	void   *p;

	length = (size_t) bytes;

	if ((length != (uint64_t) bytes) || (bytes < 0)) {
		exceptions_throw_illegalargumentexception();
		return;
	}

	p = (void *) (intptr_t) address;

	/* XXX Not sure this is correct. */

	system_memset(p, value, length);
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    copyMemory
 * Signature: (JJJ)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_copyMemory(JNIEnv *env, sun_misc_Unsafe *_this, int64_t srcAddress, int64_t destAddress, int64_t bytes)
{
	size_t  length;
	void   *src;
	void   *dest;

	if (bytes == 0)
		return;

	length = (size_t) bytes;

	if ((length != (uint64_t) bytes) || (bytes < 0)) {
		exceptions_throw_illegalargumentexception();
		return;
	}

	src  = (void *) (intptr_t) srcAddress;
	dest = (void *) (intptr_t) destAddress;

	system_memcpy(dest, src, length);
}
#endif


/*
 * Class:     sun/misc/Unsafe
 * Method:    freeMemory
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_freeMemory(JNIEnv *env, sun_misc_Unsafe *_this, int64_t address)
{
	void *p;

	p = (void *) (intptr_t) address;

	if (p == NULL)
		return;

	/* we pass length 1 to trick the free function */

	MFREE(p, uint8_t, 1);
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    staticFieldOffset
 * Signature: (Ljava/lang/reflect/Field;)J
 */
JNIEXPORT int64_t JNICALL Java_sun_misc_Unsafe_staticFieldOffset(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_reflect_Field *f)
{
	/* The offset of static fields is 0. */

	return 0;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    staticFieldBase
 * Signature: (Ljava/lang/reflect/Field;)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_sun_misc_Unsafe_staticFieldBase(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_reflect_Field *rf)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;

#if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)
	java_lang_reflect_VMField *rvmf;
#endif

#if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)

	LLNI_field_get_ref(rf,   f,     rvmf);
	LLNI_field_get_cls(rvmf, clazz, c);
	LLNI_field_get_val(rvmf, slot , slot);

#elif defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)

	LLNI_field_get_cls(rf, clazz, c);
	LLNI_field_get_val(rf, slot , slot);

#else
# error unknown configuration
#endif

	f = &(c->fields[slot]);

	return (java_lang_Object *) (f->value);
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    ensureClassInitialized
 * Signature: (Ljava/lang/Class;)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_ensureClassInitialized(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Class *clazz)
{
	classinfo *c;

	c = LLNI_classinfo_unwrap(clazz);

	if (!(c->state & CLASS_INITIALIZED))
		initialize_class(c);
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    arrayBaseOffset
 * Signature: (Ljava/lang/Class;)I
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_arrayBaseOffset(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Class *arrayClass)
{
	classinfo       *c;
	arraydescriptor *ad;

	c  = LLNI_classinfo_unwrap(arrayClass);
	ad = c->vftbl->arraydesc;

	if (ad == NULL) {
		/* XXX does that exception exist? */
		exceptions_throw_internalerror("java/lang/InvalidClassException");
		return 0;
	}

	return ad->dataoffset;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    arrayIndexScale
 * Signature: (Ljava/lang/Class;)I
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_arrayIndexScale(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Class *arrayClass)
{
	classinfo       *c;
	arraydescriptor *ad;

	c  = LLNI_classinfo_unwrap(arrayClass);
	ad = c->vftbl->arraydesc;

	if (ad == NULL) {
		/* XXX does that exception exist? */
		exceptions_throw_internalerror("java/lang/InvalidClassException");
		return 0;
	}

	return ad->componentsize;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    addressSize
 * Signature: ()I
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_addressSize(JNIEnv *env, sun_misc_Unsafe *_this)
{
	return SIZEOF_VOID_P;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    pageSize
 * Signature: ()I
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_pageSize(JNIEnv *env, sun_misc_Unsafe *_this)
{
	int sz;

	sz = getpagesize();

	return sz;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    defineClass
 * Signature: (Ljava/lang/String;[BIILjava/lang/ClassLoader;Ljava/security/ProtectionDomain;)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_sun_misc_Unsafe_defineClass__Ljava_lang_String_2_3BIILjava_lang_ClassLoader_2Ljava_security_ProtectionDomain_2(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_String *name, java_handle_bytearray_t *b, int32_t off, int32_t len, java_lang_ClassLoader *loader, java_security_ProtectionDomain *protectionDomain)
{
	classloader_t   *cl;
	utf             *utfname;
	classinfo       *c;
	java_lang_Class *o;

	cl = loader_hashtable_classloader_add((java_handle_t *) loader);

	/* check if data was passed */

	if (b == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	/* check the indexes passed */

	if ((off < 0) || (len < 0) || ((off + len) > LLNI_array_size(b))) {
		exceptions_throw_arrayindexoutofboundsexception();
		return NULL;
	}

	if (name != NULL) {
		/* convert '.' to '/' in java string */

		utfname = javastring_toutf((java_handle_t *) name, true);
	} 
	else {
		utfname = NULL;
	}

	/* define the class */

	c = class_define(utfname, cl, len, (uint8_t *) &(LLNI_array_direct(b, off)),
					 (java_handle_t *) protectionDomain);

	if (c == NULL)
		return NULL;

	/* for convenience */

	o = LLNI_classinfo_wrap(c);

#if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)
	/* set ProtectionDomain */

	LLNI_field_set_ref(o, pd, protectionDomain);
#endif

	return o;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    allocateInstance
 * Signature: (Ljava/lang/Class;)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_sun_misc_Unsafe_allocateInstance(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Class *cls)
{
	classinfo     *c;
	java_handle_t *o;

	c = LLNI_classinfo_unwrap(cls);

	o = builtin_new(c);

	return (java_lang_Object *) o;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    throwException
 * Signature: (Ljava/lang/Throwable;)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_throwException(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Throwable *ee)
{
	java_handle_t *o;

	o = (java_handle_t *) ee;

	exceptions_set_exception(o);
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    compareAndSwapObject
 * Signature: (Ljava/lang/Object;JLjava/lang/Object;Ljava/lang/Object;)Z
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_compareAndSwapObject(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset, java_lang_Object *expected, java_lang_Object *x)
{
	volatile void **p;
	void           *result;

	/* XXX Use LLNI */

	p = (volatile void **) (((uint8_t *) o) + offset);

	result = Atomic::compare_and_swap(p, expected, x);

	if (result == expected)
		return true;

	return false;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    compareAndSwapInt
 * Signature: (Ljava/lang/Object;JII)Z
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_compareAndSwapInt(JNIEnv *env, sun_misc_Unsafe* _this, java_lang_Object* o, int64_t offset, int32_t expected, int32_t x)
{
	uint32_t *p;
	uint32_t  result;

	/* XXX Use LLNI */

	p = (uint32_t *) (((uint8_t *) o) + offset);

	result = Atomic::compare_and_swap(p, expected, x);

	if (result == (uint32_t) expected)
		return true;

	return false;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    compareAndSwapLong
 * Signature: (Ljava/lang/Object;JJJ)Z
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_compareAndSwapLong(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset, int64_t expected, int64_t x)
{
	uint64_t *p;
	uint64_t  result;

	/* XXX Use LLNI */

	p = (uint64_t *) (((uint8_t *) o) + offset);

	result = Atomic::compare_and_swap(p, expected, x);

	if (result == (uint64_t) expected)
		return true;

	return false;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getObjectVolatile
 * Signature: (Ljava/lang/Object;J)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_sun_misc_Unsafe_getObjectVolatile(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset)
{
	volatile void **p;
	volatile void  *value;

	p = (volatile void **) (((uint8_t *) o) + offset);

	value = *p;

	return (java_lang_Object *) value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putObjectVolatile
 * Signature: (Ljava/lang/Object;JLjava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putObjectVolatile(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset, java_lang_Object *x)
{
	volatile void **p;

	p = (volatile void **) (((uint8_t *) o) + offset);

	*p = x;
}


#define UNSAFE_GET_VOLATILE(type)							\
	java_handle_t *_h;										\
	java_object_t *_o;										\
	volatile type *_p;										\
	volatile type  _x;										\
															\
	_h = (java_handle_t *) o;								\
															\
	LLNI_CRITICAL_START;									\
															\
	_o = LLNI_UNWRAP(_h);									\
	_p = (volatile type *) (((uint8_t *) _o) + offset);		\
															\
	_x = *_p;												\
															\
	LLNI_CRITICAL_END;										\
															\
	return _x;


#define UNSAFE_PUT_VOLATILE(type)							\
	java_handle_t *_h;										\
	java_object_t *_o;										\
	volatile type *_p;										\
															\
	_h = (java_handle_t *) o;								\
															\
	LLNI_CRITICAL_START;									\
															\
	_o = LLNI_UNWRAP(_h);									\
	_p = (volatile type *) (((uint8_t *) _o) + offset);		\
															\
	*_p = x;												\
															\
	Atomic::memory_barrier();								\
															\
	LLNI_CRITICAL_END;


/*
 * Class:     sun/misc/Unsafe
 * Method:    getIntVolatile
 * Signature: (Ljava/lang/Object;J)I
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_getIntVolatile(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset)
{
	UNSAFE_GET_VOLATILE(int32_t);
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putIntVolatile
 * Signature: (Ljava/lang/Object;JI)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putIntVolatile(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset, int32_t x)
{
	UNSAFE_PUT_VOLATILE(int32_t);
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getLongVolatile
 * Signature: (Ljava/lang/Object;J)J
 */
JNIEXPORT int64_t JNICALL Java_sun_misc_Unsafe_getLongVolatile(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset)
{
	UNSAFE_GET_VOLATILE(int64_t);
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putLongVolatile
 * Signature: (Ljava/lang/Object;JJ)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putLongVolatile(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset, int64_t x)
{
	UNSAFE_PUT_VOLATILE(int64_t);
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getDoubleVolatile
 * Signature: (Ljava/lang/Object;J)D
 */
JNIEXPORT double JNICALL Java_sun_misc_Unsafe_getDoubleVolatile(JNIEnv *env, sun_misc_Unsafe* __this, java_lang_Object* o, int64_t offset)
{
	UNSAFE_GET_VOLATILE(double);
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putOrderedObject
 * Signature: (Ljava/lang/Object;JLjava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putOrderedObject(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset, java_lang_Object *x)
{
	java_handle_t  *_h;
	java_handle_t  *_hx;
	java_object_t  *_o;
	java_object_t  *_x;
	volatile void **_p;

	_h  = (java_handle_t *) o;
	_hx = (java_handle_t *) x;

	LLNI_CRITICAL_START;

	_o = LLNI_UNWRAP(_h);
	_x = LLNI_UNWRAP(_hx);
	_p = (volatile void **) (((uint8_t *) _o) + offset);

	*_p = _x;

	Atomic::memory_barrier();

	LLNI_CRITICAL_END;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putOrderedInt
 * Signature: (Ljava/lang/Object;JI)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putOrderedInt(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset, int32_t x)
{
	UNSAFE_PUT_VOLATILE(int32_t);
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putOrderedLong
 * Signature: (Ljava/lang/Object;JJ)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putOrderedLong(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *o, int64_t offset, int64_t x)
{
	UNSAFE_PUT_VOLATILE(int64_t);
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    unpark
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_unpark(JNIEnv *env, sun_misc_Unsafe *_this, java_lang_Object *thread)
{
	/* XXX IMPLEMENT ME */
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    park
 * Signature: (ZJ)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_park(JNIEnv *env, sun_misc_Unsafe *_this, int32_t isAbsolute, int64_t time)
{
	/* XXX IMPLEMENT ME */
}

} // extern "C"


/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ (char*) "registerNatives",        (char*) "()V",                                                        (void*) (uintptr_t) &Java_sun_misc_Unsafe_registerNatives                  },
	{ (char*) "getInt",                 (char*) "(Ljava/lang/Object;J)I",                                     (void*) (uintptr_t) &Java_sun_misc_Unsafe_getInt__Ljava_lang_Object_2J     },
	{ (char*) "putInt",                 (char*) "(Ljava/lang/Object;JI)V",                                    (void*) (uintptr_t) &Java_sun_misc_Unsafe_putInt__Ljava_lang_Object_2JI    },
	{ (char*) "getObject",              (char*) "(Ljava/lang/Object;J)Ljava/lang/Object;",                    (void*) (uintptr_t) &Java_sun_misc_Unsafe_getObject                        },
	{ (char*) "putObject",              (char*) "(Ljava/lang/Object;JLjava/lang/Object;)V",                   (void*) (uintptr_t) &Java_sun_misc_Unsafe_putObject                        },
	{ (char*) "getBoolean",             (char*) "(Ljava/lang/Object;J)Z",                                     (void*) (uintptr_t) &Java_sun_misc_Unsafe_getBoolean                       },
	{ (char*) "putBoolean",             (char*) "(Ljava/lang/Object;JZ)V",                                    (void*) (uintptr_t) &Java_sun_misc_Unsafe_putBoolean                       },
	{ (char*) "getByte",                (char*) "(Ljava/lang/Object;J)B",                                     (void*) (uintptr_t) &Java_sun_misc_Unsafe_getByte__Ljava_lang_Object_2J    },
	{ (char*) "putByte",                (char*) "(Ljava/lang/Object;JB)V",                                    (void*) (uintptr_t) &Java_sun_misc_Unsafe_putByte__Ljava_lang_Object_2JB   },
	{ (char*) "getShort",               (char*) "(Ljava/lang/Object;J)S",                                     (void*) (uintptr_t) &Java_sun_misc_Unsafe_getShort__Ljava_lang_Object_2J   },
	{ (char*) "putShort",               (char*) "(Ljava/lang/Object;JS)V",                                    (void*) (uintptr_t) &Java_sun_misc_Unsafe_putShort__Ljava_lang_Object_2JS  },
	{ (char*) "getChar",                (char*) "(Ljava/lang/Object;J)C",                                     (void*) (uintptr_t) &Java_sun_misc_Unsafe_getChar__Ljava_lang_Object_2J    },
	{ (char*) "putChar",                (char*) "(Ljava/lang/Object;JC)V",                                    (void*) (uintptr_t) &Java_sun_misc_Unsafe_putChar__Ljava_lang_Object_2JC   },
	{ (char*) "getLong",                (char*) "(Ljava/lang/Object;J)J",                                     (void*) (uintptr_t) &Java_sun_misc_Unsafe_getLong__Ljava_lang_Object_2J    },
	{ (char*) "putLong",                (char*) "(Ljava/lang/Object;JJ)V",                                    (void*) (uintptr_t) &Java_sun_misc_Unsafe_putLong__Ljava_lang_Object_2JJ   },
	{ (char*) "getFloat",               (char*) "(Ljava/lang/Object;J)F",                                     (void*) (uintptr_t) &Java_sun_misc_Unsafe_getFloat__Ljava_lang_Object_2J   },
	{ (char*) "putFloat",               (char*) "(Ljava/lang/Object;JF)V",                                    (void*) (uintptr_t) &Java_sun_misc_Unsafe_putFloat__Ljava_lang_Object_2JF  },
	{ (char*) "getDouble",              (char*) "(Ljava/lang/Object;J)D",                                     (void*) (uintptr_t) &Java_sun_misc_Unsafe_getDouble__Ljava_lang_Object_2J  },
	{ (char*) "putDouble",              (char*) "(Ljava/lang/Object;JD)V",                                    (void*) (uintptr_t) &Java_sun_misc_Unsafe_putDouble__Ljava_lang_Object_2JD },
	{ (char*) "getByte",                (char*) "(J)B",                                                       (void*) (uintptr_t) &Java_sun_misc_Unsafe_getByte__J                       },
	{ (char*) "putByte",                (char*) "(JB)V",                                                      (void*) (uintptr_t) &Java_sun_misc_Unsafe_putByte__JB                      },
	{ (char*) "getShort",               (char*) "(J)S",                                                       (void*) (uintptr_t) &Java_sun_misc_Unsafe_getShort__J                      },
	{ (char*) "putShort",               (char*) "(JS)V",                                                      (void*) (uintptr_t) &Java_sun_misc_Unsafe_putShort__JS                     },
	{ (char*) "getChar",                (char*) "(J)C",                                                       (void*) (uintptr_t) &Java_sun_misc_Unsafe_getChar__J                       },
	{ (char*) "putChar",                (char*) "(JC)V",                                                      (void*) (uintptr_t) &Java_sun_misc_Unsafe_putChar__JC                      },
	{ (char*) "getInt",                 (char*) "(J)I",                                                       (void*) (uintptr_t) &Java_sun_misc_Unsafe_getInt__J                        },
	{ (char*) "putInt",                 (char*) "(JI)V",                                                      (void*) (uintptr_t) &Java_sun_misc_Unsafe_putInt__JI                       },
	{ (char*) "getLong",                (char*) "(J)J",                                                       (void*) (uintptr_t) &Java_sun_misc_Unsafe_getLong__J                       },
	{ (char*) "putLong",                (char*) "(JJ)V",                                                      (void*) (uintptr_t) &Java_sun_misc_Unsafe_putLong__JJ                      },
	{ (char*) "getFloat",               (char*) "(J)F",                                                       (void*) (uintptr_t) &Java_sun_misc_Unsafe_getFloat__J                      },
	{ (char*) "putFloat",               (char*) "(JF)V",                                                      (void*) (uintptr_t) &Java_sun_misc_Unsafe_putFloat__JF                     },
	{ (char*) "objectFieldOffset",      (char*) "(Ljava/lang/reflect/Field;)J",                               (void*) (uintptr_t) &Java_sun_misc_Unsafe_objectFieldOffset                },
	{ (char*) "allocateMemory",         (char*) "(J)J",                                                       (void*) (uintptr_t) &Java_sun_misc_Unsafe_allocateMemory                   },
#if 0
	// OpenJDK 7
	{ (char*) "setMemory",              (char*) "(Ljava/lang/Object;JJB)V",                                   (void*) (uintptr_t) &Java_sun_misc_Unsafe_setMemory                        },
	{ (char*) "copyMemory",             (char*) "(Ljava/lang/Object;JLjava/lang/Object;JJ)V",                 (void*) (uintptr_t) &Java_sun_misc_Unsafe_copyMemory                       },
#else
	{ (char*) "setMemory",              (char*) "(JJB)V",                                                     (void*) (uintptr_t) &Java_sun_misc_Unsafe_setMemory                        },
	{ (char*) "copyMemory",             (char*) "(JJJ)V",                                                     (void*) (uintptr_t) &Java_sun_misc_Unsafe_copyMemory                       },
#endif
	{ (char*) "freeMemory",             (char*) "(J)V",                                                       (void*) (uintptr_t) &Java_sun_misc_Unsafe_freeMemory                       },
	{ (char*) "staticFieldOffset",      (char*) "(Ljava/lang/reflect/Field;)J",                               (void*) (uintptr_t) &Java_sun_misc_Unsafe_staticFieldOffset                },
	{ (char*) "staticFieldBase",        (char*) "(Ljava/lang/reflect/Field;)Ljava/lang/Object;",              (void*) (uintptr_t) &Java_sun_misc_Unsafe_staticFieldBase                  },
	{ (char*) "ensureClassInitialized", (char*) "(Ljava/lang/Class;)V",                                       (void*) (uintptr_t) &Java_sun_misc_Unsafe_ensureClassInitialized           },
	{ (char*) "arrayBaseOffset",        (char*) "(Ljava/lang/Class;)I",                                       (void*) (uintptr_t) &Java_sun_misc_Unsafe_arrayBaseOffset                  },
	{ (char*) "arrayIndexScale",        (char*) "(Ljava/lang/Class;)I",                                       (void*) (uintptr_t) &Java_sun_misc_Unsafe_arrayIndexScale                  },
	{ (char*) "addressSize",            (char*) "()I",                                                        (void*) (uintptr_t) &Java_sun_misc_Unsafe_addressSize                      },
	{ (char*) "pageSize",               (char*) "()I",                                                        (void*) (uintptr_t) &Java_sun_misc_Unsafe_pageSize                         },
	{ (char*) "defineClass",            (char*) "(Ljava/lang/String;[BIILjava/lang/ClassLoader;Ljava/security/ProtectionDomain;)Ljava/lang/Class;", (void*) (uintptr_t) &Java_sun_misc_Unsafe_defineClass__Ljava_lang_String_2_3BIILjava_lang_ClassLoader_2Ljava_security_ProtectionDomain_2 },
	{ (char*) "allocateInstance",       (char*) "(Ljava/lang/Class;)Ljava/lang/Object;",                      (void*) (uintptr_t) &Java_sun_misc_Unsafe_allocateInstance                 },
	{ (char*) "throwException",         (char*) "(Ljava/lang/Throwable;)V",                                   (void*) (uintptr_t) &Java_sun_misc_Unsafe_throwException                   },
	{ (char*) "compareAndSwapObject",   (char*) "(Ljava/lang/Object;JLjava/lang/Object;Ljava/lang/Object;)Z", (void*) (uintptr_t) &Java_sun_misc_Unsafe_compareAndSwapObject             },
	{ (char*) "compareAndSwapInt",      (char*) "(Ljava/lang/Object;JII)Z",                                   (void*) (uintptr_t) &Java_sun_misc_Unsafe_compareAndSwapInt                },
	{ (char*) "compareAndSwapLong",     (char*) "(Ljava/lang/Object;JJJ)Z",                                   (void*) (uintptr_t) &Java_sun_misc_Unsafe_compareAndSwapLong               },
	{ (char*) "getObjectVolatile",      (char*) "(Ljava/lang/Object;J)Ljava/lang/Object;",                    (void*) (uintptr_t) &Java_sun_misc_Unsafe_getObjectVolatile                },
	{ (char*) "putObjectVolatile",      (char*) "(Ljava/lang/Object;JLjava/lang/Object;)V",                   (void*) (uintptr_t) &Java_sun_misc_Unsafe_putObjectVolatile                },
	{ (char*) "getIntVolatile",         (char*) "(Ljava/lang/Object;J)I",                                     (void*) (uintptr_t) &Java_sun_misc_Unsafe_getIntVolatile                   },
	{ (char*) "putIntVolatile",         (char*) "(Ljava/lang/Object;JI)V",                                    (void*) (uintptr_t) &Java_sun_misc_Unsafe_putIntVolatile                   },
	{ (char*) "getLongVolatile",        (char*) "(Ljava/lang/Object;J)J",                                     (void*) (uintptr_t) &Java_sun_misc_Unsafe_getLongVolatile                  },
	{ (char*) "putLongVolatile",        (char*) "(Ljava/lang/Object;JJ)V",                                    (void*) (uintptr_t) &Java_sun_misc_Unsafe_putLongVolatile                  },
	{ (char*) "getDoubleVolatile",      (char*) "(Ljava/lang/Object;J)D",                                     (void*) (uintptr_t) &Java_sun_misc_Unsafe_getDoubleVolatile                },
	{ (char*) "putOrderedObject",       (char*) "(Ljava/lang/Object;JLjava/lang/Object;)V",                   (void*) (uintptr_t) &Java_sun_misc_Unsafe_putOrderedObject                 },
	{ (char*) "putOrderedInt",          (char*) "(Ljava/lang/Object;JI)V",                                    (void*) (uintptr_t) &Java_sun_misc_Unsafe_putOrderedInt                    },
	{ (char*) "putOrderedLong",         (char*) "(Ljava/lang/Object;JJ)V",                                    (void*) (uintptr_t) &Java_sun_misc_Unsafe_putOrderedLong                   },
	{ (char*) "unpark",                 (char*) "(Ljava/lang/Object;)V",                                      (void*) (uintptr_t) &Java_sun_misc_Unsafe_unpark                           },
	{ (char*) "park",                   (char*) "(ZJ)V",                                                      (void*) (uintptr_t) &Java_sun_misc_Unsafe_park                             },
};


/* _Jv_sun_misc_Unsafe_init ****************************************************

   Register native functions.

*******************************************************************************/

// FIXME
extern "C" {
void _Jv_sun_misc_Unsafe_init(void)
{
	utf *u;

	u = utf_new_char("sun/misc/Unsafe");

	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}
}

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
