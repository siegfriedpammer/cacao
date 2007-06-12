/* src/native/vm/gnu/java_lang_reflect_Constructor.c

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

   $Id: java_lang_reflect_Constructor.c 8067 2007-06-12 12:32:18Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <stdlib.h>

#include "vm/types.h"

#include "native/jni.h"
#include "native/native.h"

#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_String.h"
#include "native/include/java_lang_reflect_Constructor.h"

#include "native/vm/java_lang_reflect_Constructor.h"

#include "vmcore/utf8.h"


/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ "getModifiersInternal", "()I",                                                       (void *) (ptrint) &_Jv_java_lang_reflect_Constructor_getModifiers      },
	{ "getParameterTypes",    "()[Ljava/lang/Class;",                                      (void *) (ptrint) &_Jv_java_lang_reflect_Constructor_getParameterTypes },
	{ "getExceptionTypes",    "()[Ljava/lang/Class;",                                      (void *) (ptrint) &_Jv_java_lang_reflect_Constructor_getExceptionTypes },
	{ "constructNative",      "([Ljava/lang/Object;Ljava/lang/Class;I)Ljava/lang/Object;", (void *) (ptrint) &Java_java_lang_reflect_Constructor_constructNative  },
	{ "getSignature",         "()Ljava/lang/String;",                                      (void *) (ptrint) &_Jv_java_lang_reflect_Constructor_getSignature      },
};


/* _Jv_java_lang_reflect_Constructor_init **************************************

   Register native functions.

*******************************************************************************/

void _Jv_java_lang_reflect_Constructor_init(void)
{
	utf *u;

	u = utf_new_char("java/lang/reflect/Constructor");

	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}


/*
 * Class:     java/lang/reflect/Constructor
 * Method:    constructNative
 * Signature: ([Ljava/lang/Object;Ljava/lang/Class;I)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_java_lang_reflect_Constructor_constructNative(JNIEnv *env, java_lang_reflect_Constructor *this, java_objectarray *args, java_lang_Class *declaringClass, s4 slot)
{
	/* just to be sure */

	assert(this->clazz == declaringClass);
	assert(this->slot  == slot);

	return _Jv_java_lang_reflect_Constructor_newInstance(env, this, args);
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
