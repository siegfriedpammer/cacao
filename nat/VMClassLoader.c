/* nat/VMClassLoader.c - java/lang/ClassLoader

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

   $Id: VMClassLoader.c 1003 2004-03-30 22:56:04Z twisti $

*/


#include "jni.h"
#include "loader.h"
#include "native.h"
#include "builtin.h"
#include "toolbox/loging.h"
#include "java_lang_Class.h"
#include "java_lang_String.h"
#include "java_lang_ClassLoader.h"

/*
 * Class:     java/lang/ClassLoader
 * Method:    defineClass0
 * Signature: (Ljava/lang/String;[BII)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClassLoader_defineClass(JNIEnv *env, jclass clazz, java_lang_ClassLoader *this, java_lang_String *name, java_bytearray *buf, s4 off, s4 len)
{
	classinfo *c;

	log_text("Java_java_lang_VMClassLoader_defineClass called");

	/* call JNI-function to load the class */
	c = (*env)->DefineClass(env, javastring_tochar((java_objectheader*) name), (jobject) this, (const jbyte *) &buf[off], len);
	use_class_as_object (c);

	return (java_lang_Class *) c;
}


/*
 * Class:     java/lang/Class
 * Method:    getPrimitiveClass
 * Signature: (Ljava/lang/String;)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClassLoader_getPrimitiveClass(JNIEnv *env, jclass clazz, java_lang_String *name)
{
	classinfo *c;
	utf *u = javastring_toutf(name, false);

	if (u) {
		/* get primitive class */
		c = loader_load_sysclass(NULL, u);
		use_class_as_object(c);

		return (java_lang_Class *) c;
	}

	/* illegal primitive classname specified */
	*exceptionptr = new_exception(string_java_lang_ClassNotFoundException);

	return NULL;
}


/*
 * Class:     java/lang/ClassLoader
 * Method:    resolveClass0
 * Signature: (Ljava/lang/Class;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMClassLoader_resolveClass(JNIEnv *env, jclass clazz, java_lang_Class *par1)
{
	/* class already linked, so return */
	return;
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
