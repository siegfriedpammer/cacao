/* native.h - table of native functions

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

   Authors: Reinhard Grafl

   $Id: native.h 930 2004-03-02 21:18:23Z jowenn $

*/


#ifndef _NATIVE_H
#define _NATIVE_H


#include "jni.h"
#include "nat/java_lang_String.h"
#include "nat/java_lang_ClassLoader.h"


/* searchpath for classfiles */
extern char *classpath;

extern classinfo *class_java_lang_Class;
extern classinfo *class_java_lang_VMClass;
extern methodinfo *method_vmclass_init;
/* extern classinfo *class_java_lang_Cloneable=0; */ /* now in global.h */
extern classinfo *class_java_lang_CloneNotSupportedException;
extern classinfo *class_java_lang_System;
extern classinfo *class_java_lang_ClassLoader;
extern classinfo *class_java_lang_NoClassDefFoundError;
extern classinfo *class_java_lang_ClassNotFoundException;
extern classinfo *class_java_lang_LinkageError;
extern classinfo *class_java_lang_InstantiationException;
extern classinfo *class_java_lang_NoSuchMethodError;   
extern classinfo *class_java_lang_NoSuchFieldError;
extern classinfo *class_java_lang_ClassFormatError;
extern classinfo *class_java_lang_IllegalArgumentException;
extern classinfo *class_java_lang_ArrayIndexOutOfBoundsException;
extern classinfo *class_java_lang_NoSuchFieldException;
extern classinfo *class_java_io_SyncFailedException;
extern classinfo *class_java_io_IOException;
extern classinfo *class_java_io_FileNotFoundException;
extern classinfo *class_java_io_UnixFileSystem;
extern classinfo *class_java_security_PrivilegedActionException;
extern classinfo *class_java_net_UnknownHostException;
extern classinfo *class_java_net_SocketException;
extern classinfo *class_java_lang_NoSuchMethodException;
extern classinfo *class_java_lang_Double;
extern classinfo *class_java_lang_Float;
extern classinfo *class_java_lang_Long;
extern classinfo *class_java_lang_Byte;
extern classinfo *class_java_lang_Short;
extern classinfo *class_java_lang_Boolean;
extern classinfo *class_java_lang_Void;
extern classinfo *class_java_lang_Character;
extern classinfo *class_java_lang_Integer;

/* the system classloader object */
extern struct java_lang_ClassLoader *SystemClassLoader;

/* for raising exceptions from native methods */
/* extern java_objectheader* exceptionptr; */

/* javastring-hashtable */
extern hashtable string_hash; 


/* throw classnotfoundexcetion with detail message */
void throw_noclassdeffounderror_message(utf* classname);

/* throw linkageerror with detail message */
void throw_linkageerror_message(utf* classname);

void use_class_as_object(classinfo *c);

/* load classes required for native methods */
void native_loadclasses();

/* set searchpath for classfiles */
void native_setclasspath(char *path);

/* find native function */
functionptr native_findfunction(utf *cname, utf *mname, 
								utf *desc, bool isstatic);

/* creates a new object of type java/lang/String from a utf-text */
/*  java_objectheader *javastring_new(utf *text); */
java_lang_String *javastring_new(utf *text);

/* creates a new object of type java/lang/String from a c-string */
/*  java_objectheader *javastring_new_char(char *text); */
java_lang_String *javastring_new_char(char *text);

/* make c-string from a javastring (debugging) */
char *javastring_tochar(java_objectheader *s);

/* create new object on the heap and call the initializer */
java_objectheader *native_new_and_init(classinfo *c);

/* 
   create new object on the heap and call the initializer 
   mainly used for exceptions with a message
*/
java_objectheader *native_new_and_init_string(classinfo *c, java_lang_String *s);

/* add property to system-property vector */
void attach_property(char *name, char *value);

/* correct vftbl-entries of javastring-hash */
void stringtable_update();


/* make utf symbol from javastring */
utf *javastring_toutf(struct java_lang_String *string, bool isclassname);

/* make utf symbol from u2 array */
utf *utf_new_u2(u2 *unicodedata, u4 unicodelength, bool isclassname);

/* determine utf length in bytes of a u2 array */
u4 u2_utflength(u2 *text, u4 u2_length);

/* create systemclassloader object and initialize its instance fields  */
void init_systemclassloader();

/* search 'classinfo'-structure for a field with the specified name */
fieldinfo *class_findfield_approx(classinfo *c, utf *name);
s4 class_findfield_index_approx(classinfo *c, utf *name);

/* creates a new javastring with the text of the utf-symbol */
java_objectheader *literalstring_new(utf *u);

/* creates a new javastring with the text of the u2-array */
java_objectheader *literalstring_u2(java_chararray *a, u4 length, u4 offset,
									bool copymode);

/* dispose a javastring */
void literalstring_free(java_objectheader*);

void systemclassloader_addlibname(java_objectheader *o);
void systemclassloader_addlibrary(java_objectheader *o);

void copy_vftbl(vftbl **dest, vftbl *src);

utf *create_methodsig(java_objectarray* types, char *retType);
classinfo *get_type(char **utf_ptr,char *desc_end, bool skip);
java_objectarray* get_parametertypes(methodinfo *m);
java_objectarray* get_exceptiontypes(methodinfo *m);
classinfo *get_returntype(methodinfo *m);




java_objectarray *builtin_asm_createclasscontextarray(classinfo **end,classinfo **start);
java_lang_ClassLoader *builtin_asm_getclassloader(classinfo **end,classinfo **start);

#endif /* _NATIVE_H */


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
