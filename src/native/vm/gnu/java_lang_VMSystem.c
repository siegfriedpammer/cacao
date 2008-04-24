/* src/native/vm/gnu/java_lang_VMSystem.c - java/lang/VMSystem

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
#include <string.h>

#include "mm/gc-common.h"

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"

#include "native/include/java_lang_Object.h"
#include "native/include/java_io_InputStream.h"        /* required by j.l.VMS */
#include "native/include/java_io_PrintStream.h"        /* required by j.l.VMS */

#include "native/include/java_lang_VMSystem.h"

#include "vm/builtin.h"


/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ "arraycopy",        "(Ljava/lang/Object;ILjava/lang/Object;II)V", (void *) (uintptr_t) &Java_java_lang_VMSystem_arraycopy },
	{ "identityHashCode", "(Ljava/lang/Object;)I",                      (void *) (uintptr_t) &Java_java_lang_VMSystem_identityHashCode },
};


/* _Jv_java_lang_VMSystem_init *************************************************

   Register native functions.

*******************************************************************************/

void _Jv_java_lang_VMSystem_init(void)
{
	utf *u;

	u = utf_new_char("java/lang/VMSystem");

	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}


/*
 * Class:     java/lang/VMSystem
 * Method:    arraycopy
 * Signature: (Ljava/lang/Object;ILjava/lang/Object;II)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMSystem_arraycopy(JNIEnv *env, jclass clazz, java_lang_Object *src, int32_t srcStart, java_lang_Object *dest, int32_t destStart, int32_t len)
{
	builtin_arraycopy((java_handle_t *) src, srcStart,
					  (java_handle_t *) dest, destStart, len);
}


/*
 * Class:     java/lang/VMSystem
 * Method:    identityHashCode
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT int32_t JNICALL Java_java_lang_VMSystem_identityHashCode(JNIEnv *env, jclass clazz, java_lang_Object *o)
{
	int32_t hashcode;

	LLNI_CRITICAL_START;

	hashcode = heap_hashcode(LLNI_UNWRAP((java_handle_t *) o));

	LLNI_CRITICAL_END;

	return hashcode;
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
