/* native/vm/Proxy.c - java/lang/reflect/Proxy

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Roman Obermaiser

   Changes: Joseph Wenninger

   $Id: Proxy.c 1735 2004-12-07 14:33:27Z twisti $

*/


#include "native/jni.h"
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_ClassLoader.h"
#include "toolbox/logging.h"


/*
 * Class:     java_lang_reflect_Proxy
 * Method:    getProxyClass0
 * Signature: (Ljava/lang/ClassLoader;[Ljava/lang/Class;)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_reflect_Proxy_getProxyClass0(JNIEnv *env, jclass clazz, java_lang_ClassLoader *par1, java_objectarray *par2)
{
	log_text("Java_java_lang_reflect_Proxy_getProxyClass0");

	return 0;
}


/*
 * Class:     java_lang_reflect_Proxy
 * Method:    getProxyData0
 * Signature: (Ljava/lang/ClassLoader;[Ljava/lang/Class;)Ljava/lang/reflect/Proxy$ProxyData;
 */
JNIEXPORT struct java_lang_reflect_Proxy_ProxyData* JNICALL Java_java_lang_reflect_Proxy_getProxyData0(JNIEnv *env, jclass clazz, java_lang_ClassLoader *par1, java_objectarray *par2)
{
	log_text("Java_java_lang_reflect_Proxy_getProxyData0");

	return 0;
}


/*
 * Class:     java_lang_reflect_Proxy
 * Method:    generateProxyClass0
 * Signature: (Ljava/lang/ClassLoader;Ljava/lang/reflect/Proxy$ProxyData;)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_reflect_Proxy_generateProxyClass0(JNIEnv *env, jclass clazz, java_lang_ClassLoader *par1, struct java_lang_reflect_Proxy_ProxyData *par2)
{
	log_text("Java_java_lang_reflect_Proxy_generateProxyClass0");

	return 0;
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
