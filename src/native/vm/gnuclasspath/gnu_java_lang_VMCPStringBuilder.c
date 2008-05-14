/* src/native/vm/gnu/java_lang_reflect_VMConstructor.c

   Copyright (C) 1996-2005, 2006, 2007, 2008
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

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"

#include "native/include/java_lang_String.h"

#include "native/include/gnu_java_lang_VMCPStringBuilder.h"

#include "vm/builtin.h"
#include "vm/exceptions.h"


/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ "toString", "([CII)Ljava/lang/String;", (void *) (intptr_t) &Java_gnu_java_lang_VMCPStringBuilder_toString },
};


/* _Jv_gnu_java_lang_VMCPStringBuilder *****************************************

   Register native functions.

*******************************************************************************/

void _Jv_gnu_java_lang_VMCPStringBuilder_init(void)
{
	utf *u;

	u = utf_new_char("gnu/java/lang/VMCPStringBuilder");

	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}


/*
 * Class:     gnu/java/lang/VMCPStringBuilder
 * Method:    toString
 * Signature: ([CII)Ljava/lang/String;
 */
JNIEXPORT java_lang_String* JNICALL Java_gnu_java_lang_VMCPStringBuilder_toString(JNIEnv *env, jclass clazz, java_handle_chararray_t *value, int32_t startIndex, int32_t count)
{
	int32_t          length;
	java_handle_t    *o;
	java_lang_String *s;

	/* This is a native version of
	   java.lang.String.<init>([CIIZ)Ljava/lang/String; */

    if (startIndex < 0) {
/* 		exceptions_throw_stringindexoutofboundsexception("offset: " + offset); */
		exceptions_throw_stringindexoutofboundsexception();
		return NULL;
	}

    if (count < 0) {
/* 		exceptions_throw_stringindexoutofboundsexception("count: " + count); */
		exceptions_throw_stringindexoutofboundsexception();
		return NULL;
	}

    /* equivalent to: offset + count < 0 || offset + count > data.length */

	LLNI_CRITICAL_START;
	length = LLNI_array_size(value);
	LLNI_CRITICAL_END;

    if (length - startIndex < count) {
/* 		exceptions_throw_stringindexoutofboundsexception("offset + count: " + (offset + count)); */
		exceptions_throw_stringindexoutofboundsexception();
		return NULL;
	}

	o = builtin_new(class_java_lang_String);

	if (o == NULL)
		return NULL;

	s = (java_lang_String *) o;

	LLNI_field_set_ref(s, value,  value);
	LLNI_field_set_val(s, count,  count);
	LLNI_field_set_val(s, offset, startIndex);

	return s;
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
