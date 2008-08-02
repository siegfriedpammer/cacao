/* src/native/vm/cldc1.1/java_lang_Class.cpp

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

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"

#include "native/include/java_lang_String.h"             /* required by j.l.C */
#include "native/include/java_lang_Object.h"

// FIXME
extern "C" { 
#include "native/include/java_lang_Class.h"
}

#include "vm/exceptions.hpp"
#include "vm/initialize.h"


/* native methods implemented by this file ************************************/
 
static JNINativeMethod methods[] = {
	{ (char*) "forName",          (char*) "(Ljava/lang/String;)Ljava/lang/Class;",(void*) (uintptr_t) &Java_java_lang_Class_forName          },
	{ (char*) "newInstance",      (char*) "()Ljava/lang/Object;",                 (void*) (uintptr_t) &Java_java_lang_Class_newInstance      },
	{ (char*) "isInstance",       (char*) "(Ljava/lang/Object;)Z",                (void*) (uintptr_t) &Java_java_lang_Class_isInstance       },
	{ (char*) "isAssignableFrom", (char*) "(Ljava/lang/Class;)Z",                 (void*) (uintptr_t) &Java_java_lang_Class_isAssignableFrom },
	{ (char*) "isInterface",      (char*) "()Z",                                  (void*) (uintptr_t) &Java_java_lang_Class_isInterface      },
	{ (char*) "isArray",          (char*) "()Z",                                  (void*) (uintptr_t) &Java_java_lang_Class_isArray          },
	{ (char*) "getName",          (char*) "()Ljava/lang/String;",                 (void*) (uintptr_t) &Java_java_lang_Class_getName          },
};

/* _Jv_java_lang_Class_init ****************************************************
 
   Register native functions.
 
*******************************************************************************/

// FIXME
extern "C" { 
void _Jv_java_lang_Class_init(void)
{
	utf *u;
 
	u = utf_new_char("java/lang/Class");
 
	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}
}


// Native functions are exported as C functions.
extern "C" {

/*
 * Class:     java/lang/Class
 * Method:    forName
 * Signature: (Ljava/lang/String;)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_Class_forName(JNIEnv *env, jclass clazz, java_lang_String *name)
{
	utf       *ufile;
	utf       *uname;
	classinfo *c;
	u2        *pos;
	s4         i;

	/* illegal argument */

	if (name == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	/* create utf string in which '.' is replaced by '/' */

	ufile = javastring_toutf((java_handle_t *) name, true);
	uname = javastring_toutf((java_handle_t *) name, false);

	/* name must not contain '/' (mauve test) */

	for (i = 0, pos = LLNI_field_direct(name, value)->data + LLNI_field_direct(name, offset); i < LLNI_field_direct(name, count); i++, pos++) {
		if (*pos == '/') {
			exceptions_throw_classnotfoundexception(uname);
			return NULL;
		}
	}

	/* try to load, ... */

	c = load_class_bootstrap(ufile);

	if (c == NULL)
	    return NULL;

	/* link, ... */

	if (!link_class(c))
		return NULL;
	
	/* ...and initialize it. */

	if (!initialize_class(c))
		return NULL;

	return LLNI_classinfo_wrap(c);
}


/*
 * Class:     java/lang/Class
 * Method:    newInstance
 * Signature: ()Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_java_lang_Class_newInstance(JNIEnv *env, java_lang_Class* _this)
{
	classinfo     *c;
	java_handle_t *o;

	c = LLNI_classinfo_unwrap(_this);

	o = native_new_and_init(c);

	return (java_lang_Object *) o;
}


/*
 * Class:     java/lang/Class
 * Method:    isInstance
 * Signature: (Ljava/lang/Object;)Z
 */
JNIEXPORT int32_t JNICALL Java_java_lang_Class_isInstance(JNIEnv *env, java_lang_Class *_this, java_lang_Object *obj)
{
	classinfo     *c;
	java_handle_t *h;

	c = LLNI_classinfo_unwrap(_this);
	h = (java_handle_t *) obj;

	return class_is_instance(c, h);
}


/*
 * Class:     java/lang/Class
 * Method:    isAssignableFrom
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT int32_t JNICALL Java_java_lang_Class_isAssignableFrom(JNIEnv *env, java_lang_Class *_this, java_lang_Class *cls)
{
	classinfo *to;
	classinfo *from;

	to   = LLNI_classinfo_unwrap(_this);
	from = LLNI_classinfo_unwrap(cls);

	if (from == NULL) {
		exceptions_throw_nullpointerexception();
		return 0;
	}

	return class_is_assignable_from(to, from);
}


/*
 * Class:     java/lang/Class
 * Method:    isInterface
 * Signature: ()Z
 */
JNIEXPORT int32_t JNICALL Java_java_lang_Class_isInterface(JNIEnv *env, java_lang_Class *_this)
{
	classinfo *c;

	c = LLNI_classinfo_unwrap(_this);

	return class_is_interface(c);
}


/*
 * Class:     java/lang/Class
 * Method:    isArray
 * Signature: ()Z
 */
JNIEXPORT int32_t JNICALL Java_java_lang_Class_isArray(JNIEnv *env, java_lang_Class *_this)
{
	classinfo *c;

	c = LLNI_classinfo_unwrap(_this);

	return class_is_array(c);
}


/*
 * Class:     java/lang/Class
 * Method:    getName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT java_lang_String* JNICALL Java_java_lang_Class_getName(JNIEnv *env, java_lang_Class *_this)
{
	classinfo *c;

	c = LLNI_classinfo_unwrap(_this);

	return (java_lang_String*) class_get_classname(c);
}

} // extern "C"


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
 * vim:noexpandtab:sw=4:ts=4:
 */
