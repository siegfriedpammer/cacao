/* nat/Method.c - java/lang/reflect/Method

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser,
   M. Probst, S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck,
   P. Tomsich, J. Wenninger

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Roman Obermaiser

   Changes: Joseph Wenninger

   $Id: Method.c 873 2004-01-11 20:59:29Z twisti $

*/


#include "jni.h"
#include "loader.h"
#include "global.h"
#include "tables.h"
#include "types.h"
#include "builtin.h"
#include "native.h"
#include "toolbox/loging.h"
#include "java_lang_Object.h"
#include "java_lang_Class.h"
#include "java_lang_reflect_Method.h"

/*
 * Class:     java_lang_reflect_Method
 * Method:    getModifiers
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Method_getModifiers(JNIEnv *env, java_lang_reflect_Method *this)
{
	classinfo *c = (classinfo *) this->declaringClass;

	if (this->slot < 0 || this->slot >= c->methodscount)
		panic("error illegal slot for method in class (getReturnType)");
	
	return (c->methods[this->slot]).flags &
		(ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_ABSTRACT | ACC_STATIC | ACC_FINAL |
		 ACC_SYNCHRONIZED | ACC_NATIVE | ACC_STRICT);
}


/*
 * Class:     java_lang_reflect_Method
 * Method:    getReturnType
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_reflect_Method_getReturnType(JNIEnv *env, java_lang_reflect_Method *this)
{
	classinfo *c = (classinfo *) this->declaringClass;

	if (this->slot < 0 || this->slot >= c->methodscount)
		panic("error illegal slot for method in class (getReturnType)");

	return (java_lang_Class *) get_returntype(&(c->methods[this->slot]));
}


/*
 * Class:     java_lang_reflect_Method
 * Method:    getParameterTypes
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_reflect_Method_getParameterTypes(JNIEnv *env, java_lang_reflect_Method *this)
{
	classinfo *c = (classinfo *) this->declaringClass;

	if (this->slot < 0 || this->slot >= c->methodscount)
		panic("error illegal slot for method in class(getParameterTypes)");

	return get_parametertypes(&(c->methods[this->slot]));
}


/*
 * Class:     java_lang_reflect_Method
 * Method:    getExceptionTypes
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_reflect_Method_getExceptionTypes(JNIEnv *env, java_lang_reflect_Method *this)
{
#if 0
	classinfo *c=(classinfo*)(this->declaringClass);
	if ((this->slot<0) || (this->slot>=c->methodscount))
		panic("error illegal slot for method in class(getParameterTypes)");
	return get_exceptiontypes(&(c->methods[this->slot]));

#else
	java_objectarray *exceptiontypes;

	/* array of exceptions declared to be thrown, information not available !! */
	exceptiontypes = builtin_anewarray(0, class_java_lang_Class);

	return exceptiontypes;
#endif

}


/*
 * Class:     java_lang_reflect_Method
 * Method:    invokeNative
 * Signature: (Ljava/lang/Object;[Ljava/lang/Object;Ljava/lang/Class;I)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_java_lang_reflect_Method_invokeNative(JNIEnv *env, java_lang_reflect_Method *this, java_lang_Object *obj, java_objectarray *params, java_lang_Class *declaringClass, s4 slot)
{
    struct methodinfo *mi;

    classinfo *c = (classinfo *) declaringClass;
    if (slot < 0 || slot >= c->methodscount) {
		panic("error illegal slot for method in class(getParameterTypes)");
    }

    mi = &(c->methods[slot]);

    return (java_lang_Object *) jni_method_invokeNativeHelper(env, mi, (jobject) obj, params);
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
