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

   $Id: VMSecurityManager.c 977 2004-03-25 18:37:52Z twisti $

*/


#include "jni.h"
#include "builtin.h"
#include "native.h"
#include "tables.h"
#include "toolbox/loging.h"
#include "java_lang_ClassLoader.h"


#ifndef __I386__
/*
 * Class:     java/lang/SecurityManager
 * Method:    currentClassLoader
 * Signature: ()Ljava/lang/ClassLoader;
 */
JNIEXPORT java_lang_ClassLoader* JNICALL Java_java_lang_VMSecurityManager_currentClassLoader(JNIEnv *env, jclass clazz)
{
	log_text("Java_java_lang_VMSecurityManager_currentClassLoader");
	init_systemclassloader();

	return SystemClassLoader;
}
#endif


#ifndef __I386__
/*THIS IS IN ASMPART NOW*/
/*
 * Class:     java/lang/SecurityManager
 * Method:    getClassContext
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMSecurityManager_getClassContext(JNIEnv *env, jclass clazz)
{
  log_text("Java_java_lang_VMSecurityManager_getClassContext  called");

  /* XXX should use vftbl directly */
  return (java_objectarray *) builtin_newarray(0, class_array_of(class_java_lang_Class)->vftbl);
}
#endif

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
