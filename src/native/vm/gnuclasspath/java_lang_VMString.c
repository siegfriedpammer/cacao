/* src/native/vm/gnuclasspath/java_lang_VMString.c - java/lang/VMString

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

#include <stdlib.h>

#include "native/jni.h"
#include "native/native.h"

#include "native/include/java_lang_String.h"
#include "native/include/java_lang_VMString.h"

#include "vm/string.hpp"


/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ "intern", "(Ljava/lang/String;)Ljava/lang/String;", (void *) (ptrint) &Java_java_lang_VMString_intern },
};


/* _Jv_java_lang_VMString_init *************************************************

   Register native functions.

*******************************************************************************/

void _Jv_java_lang_VMString_init(void)
{
	utf *u;

	u = utf_new_char("java/lang/VMString");

	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}


/*
 * Class:     java/lang/VMString
 * Method:    intern
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT java_lang_String* JNICALL Java_java_lang_VMString_intern(JNIEnv *env, jclass clazz, java_lang_String *str)
{
	if (str == NULL)
		return NULL;

	return (java_lang_String *) javastring_intern((java_handle_t *) str);
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
