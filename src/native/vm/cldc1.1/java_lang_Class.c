/* src/native/vm/cldc1.1/java_lang_Class.c

   Copyright (C) 2006, 2007 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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

   $Id: java_lang_VMRuntime.c 5900 2006-11-04 17:30:44Z michi $

*/


#include "config.h"
#include "vm/types.h"

#include "native/jni.h"
#include "native/native.h"

#include "native/include/java_lang_String.h"             /* required by j.l.C */
#include "native/include/java_lang_Object.h"

#include "native/include/java_lang_Class.h"

#include "native/vm/java_lang_Class.h"


/* native methods implemented by this file ************************************/
 
static JNINativeMethod methods[] = {
	{ "forName",          "(Ljava/lang/String;)Ljava/lang/Class;",(void *) (ptrint) &Java_java_lang_Class_forName          },
	{ "newInstance",      "()Ljava/lang/Object;",                 (void *) (ptrint) &Java_java_lang_Class_newInstance      },
	{ "isInstance",       "(Ljava/lang/Object;)Z",                (void *) (ptrint) &Java_java_lang_Class_isInstance       },
	{ "isAssignableFrom", "(Ljava/lang/Class;)Z",                 (void *) (ptrint) &Java_java_lang_Class_isAssignableFrom },
	{ "isInterface",      "()Z",                                  (void *) (ptrint) &_Jv_java_lang_Class_isInterface       },
	{ "isArray",          "()Z",                                  (void *) (ptrint) &_Jv_java_lang_Class_isArray           },
	{ "getName",          "()Ljava/lang/String;",                 (void *) (ptrint) &Java_java_lang_Class_getName          },
};

/* _Jv_java_lang_Class_init ****************************************************
 
   Register native functions.
 
*******************************************************************************/
 
void _Jv_java_lang_Class_init(void)
{
	utf *u;
 
	u = utf_new_char("java/lang/Class");
 
	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}


/*
 * Class:     java/lang/Class
 * Method:    forName
 * Signature: (Ljava/lang/String;)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_Class_forName(JNIEnv *env, jclass clazz, java_lang_String *name)
{
	return _Jv_java_lang_Class_forName(name);
}


/*
 * Class:     java/lang/Class
 * Method:    newInstance
 * Signature: ()Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_java_lang_Class_newInstance(JNIEnv *env, java_lang_Class* this)
{
	classinfo     *c;
	java_handle_t *o;

	c = LLNI_classinfo_unwrap(this);

	o = native_new_and_init(c);

	return (java_lang_Object *) o;
}


/*
 * Class:     java/lang/Class
 * Method:    isInstance
 * Signature: (Ljava/lang/Object;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Class_isInstance(JNIEnv *env, java_lang_Class *this, java_lang_Object *obj)
{
	return _Jv_java_lang_Class_isInstance(this, obj);
}


/*
 * Class:     java/lang/Class
 * Method:    isAssignableFrom
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Class_isAssignableFrom(JNIEnv *env, java_lang_Class *this, java_lang_Class *cls)
{
	return _Jv_java_lang_Class_isAssignableFrom(this, cls);
}


/*
 * Class:     java/lang/Class
 * Method:    getName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT java_lang_String* JNICALL Java_java_lang_Class_getName(JNIEnv *env, java_lang_Class *this)
{
	return _Jv_java_lang_Class_getName(this);
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
