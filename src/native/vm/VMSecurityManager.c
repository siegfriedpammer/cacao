/* native/vm/VMSecurityManager.c - java/lang/VMSecurityManager

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

   $Id: VMSecurityManager.c 1735 2004-12-07 14:33:27Z twisti $

*/


#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_ClassLoader.h"
#include "toolbox/logging.h"
#include "vm/builtin.h"
#include "vm/tables.h"
#include "vm/jit/stacktrace.h"


/*
 * Class:     java/lang/VMSecurityManager
 * Method:    currentClassLoader
 * Signature: ()Ljava/lang/ClassLoader;
 */
JNIEXPORT java_lang_ClassLoader* JNICALL Java_java_lang_VMSecurityManager_currentClassLoader(JNIEnv *env, jclass clazz)
{
	log_text("Java_java_lang_VMSecurityManager_currentClassLoader");

	if (cacao_initializing)
		return NULL;

#if defined(__I386__) || defined(__ALPHA__)
	return (java_lang_ClassLoader *) cacao_currentClassLoader();
#else
/*  	return 0; */
	/* XXX TWISTI: only a quick hack */
	init_systemclassloader();
	return SystemClassLoader;
#endif
}


/*
 * Class:     java/lang/VMSecurityManager
 * Method:    getClassContext
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMSecurityManager_getClassContext(JNIEnv *env, jclass clazz)
{
	log_text("Java_java_lang_VMSecurityManager_getClassContext  called");
	if (cacao_initializing) return 0;
#if defined(__I386__) || defined(__ALPHA__)
	return cacao_createClassContextArray();
#else
/*  	return 0; */

	/* XXX TWISTI: only a quick hack */
	return (java_objectarray *) builtin_newarray(0, class_array_of(class_java_lang_Class)->vftbl);
#endif
}

java_objectarray* temporaryGetClassContextHelper(methodinfo *m) {
	if (!m) {
		log_text("BUILTIN");
		return 0;
	}
	if (!(m->name)) log_text("method or block has no name");
	else
	utf_display(m->name);
	printf("--");  
	if (!(m->class)) log_text("method or block has no class");
	else
	utf_display(m->class->name);
	printf("\n");
	return NULL;
#if 0
  log_text("temporaryGetClassContextHelper called");
  if (adr==0) log_text("NO REAL METHOD");
  else {
	
	struct methodinfo *m=(struct methodinfo*)(*(adr-1));
	if (!(m->name)) log_text("method or block has no name");
	else
	utf_display(m->name);
	if (!(m->class)) log_text("method or block has no class");
	else
	utf_display(m->class->name);
	printf("\nFrame size:%ld\n",(long)(*(adr-2)));
	adr=adr-5;
	printf("saveint:%ld\n",(long)(*adr));
	printf("saveflt:%ld\n",(long)(*(adr-1)));
  }
  log_text("temporaryGetClassContextHelper leaving");
  return (java_objectarray *) builtin_newarray(0, class_array_of(class_java_lang_Class)->vftbl);
#endif
}


java_objectarray* temporaryGetClassContextHelper2() {
  log_text("temporaryGetClassContextHelper2 called");
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
