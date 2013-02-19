/* src/native/vm/gnuclasspath/java_lang_VMObject.cpp - java/lang/VMObject

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

#include "native/jni.hpp"
#include "native/llni.h"
#include "native/native.hpp"

#if defined(ENABLE_JNI_HEADERS)
# include "native/vm/include/java_lang_VMObject.h"
#endif

#include "threads/lock.hpp"

#include "vm/jit/builtin.hpp"
#include "vm/exceptions.hpp"
#include "vm/javaobjects.hpp"
#include "vm/utf8.hpp"


// Native functions are exported as C functions.
extern "C" {

/*
 * Class:     java/lang/VMObject
 * Method:    getClass
 * Signature: (Ljava/lang/Object;)Ljava/lang/Class;
 */
JNIEXPORT jclass JNICALL Java_java_lang_VMObject_getClass(JNIEnv* env, jclass clazz, jobject obj)
{
	if (obj == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	java_lang_Object o(obj);

	return (jclass) LLNI_classinfo_wrap(o.get_Class());
}


/*
 * Class:     java/lang/VMObject
 * Method:    clone
 * Signature: (Ljava/lang/Cloneable;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_java_lang_VMObject_clone(JNIEnv* env, jclass clazz, jobject _this)
{
	return builtin_clone(NULL, _this);
}


/*
 * Class:     java/lang/VMObject
 * Method:    notify
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMObject_notify(JNIEnv* env, jclass clazz, jobject _this)
{
#if defined(ENABLE_THREADS)
	lock_notify_object(_this);
#endif
}


/*
 * Class:     java/lang/VMObject
 * Method:    notifyAll
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMObject_notifyAll(JNIEnv* env, jclass clazz, jobject _this)
{
#if defined(ENABLE_THREADS)
	lock_notify_all_object(_this);
#endif
}


/*
 * Class:     java/lang/VMObject
 * Method:    wait
 * Signature: (Ljava/lang/Object;JI)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMObject_wait(JNIEnv* env, jclass clazz, jobject o, jlong ms, jint ns)
{
#if defined(ENABLE_THREADS)
	lock_wait_for_object(o, ms, ns);
#endif
}

} // extern "C"


/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ (char*) "getClass",  (char*) "(Ljava/lang/Object;)Ljava/lang/Class;",     (void*) (uintptr_t) &Java_java_lang_VMObject_getClass  },
	{ (char*) "clone",     (char*) "(Ljava/lang/Cloneable;)Ljava/lang/Object;", (void*) (uintptr_t) &Java_java_lang_VMObject_clone     },
	{ (char*) "notify",    (char*) "(Ljava/lang/Object;)V",                     (void*) (uintptr_t) &Java_java_lang_VMObject_notify    },
	{ (char*) "notifyAll", (char*) "(Ljava/lang/Object;)V",                     (void*) (uintptr_t) &Java_java_lang_VMObject_notifyAll },
	{ (char*) "wait",      (char*) "(Ljava/lang/Object;JI)V",                   (void*) (uintptr_t) &Java_java_lang_VMObject_wait      },
};


/* _Jv_java_lang_VMObject_init *************************************************

   Register native functions.

*******************************************************************************/

void _Jv_java_lang_VMObject_init(void)
{
	utf* u = utf_new_char("java/lang/VMObject");

	NativeMethods& nm = VM::get_current()->get_nativemethods();
	nm.register_methods(u, methods, NATIVE_METHODS_COUNT);
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
