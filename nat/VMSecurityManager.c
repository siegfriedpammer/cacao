/* nat/SecurityManager.c - java/lang/SecurityManager

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

   $Id: VMSecurityManager.c 873 2004-01-11 20:59:29Z twisti $

*/


#include "jni.h"
#include "builtin.h"
#include "native.h"
#include "tables.h"
#include "toolbox/loging.h"
#include "java_lang_ClassLoader.h"


/*
 * Class:     java/lang/SecurityManager
 * Method:    currentClassLoader
 * Signature: ()Ljava/lang/ClassLoader;
 */
JNIEXPORT java_lang_ClassLoader* JNICALL Java_java_lang_VMSecurityManager_currentClassLoader(JNIEnv *env, jclass clazz)
{
	init_systemclassloader();

	return SystemClassLoader;
}


/*
 * Class:     java/lang/SecurityManager
 * Method:    getClassContext
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMSecurityManager_getClassContext(JNIEnv *env, jclass clazz)
{
  /*log_text("Java_java_lang_VMSecurityManager_getClassContext  called");*/
#warning return something more usefull here

  /* XXX should use vftbl directly */
  return (java_objectarray *) builtin_newarray(0, class_array_of(class_java_lang_Class)->vftbl);
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
