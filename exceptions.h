/* exceptions.h - exception related functions prototypes

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

   Authors: Christian Thalinger

   $Id: exceptions.h 1441 2004-11-05 13:44:03Z twisti $

*/


#ifndef _EXCEPTIONS_H
#define _EXCEPTIONS_H


#include "global.h"
#include "nat/java_lang_String.h"
#include "nat/java_lang_Throwable.h"


/* system exception classes required in cacao */

extern classinfo *class_java_lang_Throwable;
extern classinfo *class_java_lang_Exception;
extern classinfo *class_java_lang_Error;
extern classinfo *class_java_lang_OutOfMemoryError;


/* exception/error super class */

extern char *string_java_lang_Throwable;


/* specify some exception strings for code generation */

extern char *string_java_lang_ArithmeticException;
extern char *string_java_lang_ArithmeticException_message;
extern char *string_java_lang_ArrayIndexOutOfBoundsException;
extern char *string_java_lang_ArrayStoreException;
extern char *string_java_lang_ClassCastException;
extern char *string_java_lang_ClassNotFoundException;
extern char *string_java_lang_CloneNotSupportedException;
extern char *string_java_lang_Exception;
extern char *string_java_lang_IllegalArgumentException;
extern char *string_java_lang_IllegalMonitorStateException;
extern char *string_java_lang_IndexOutOfBoundsException;
extern char *string_java_lang_InterruptedException;
extern char *string_java_lang_NegativeArraySizeException;
extern char *string_java_lang_NoSuchFieldException;
extern char *string_java_lang_NoSuchMethodException;
extern char *string_java_lang_NullPointerException;


/* specify some error strings for code generation */

extern char *string_java_lang_AbstractMethodError;
extern char *string_java_lang_ClassCircularityError;
extern char *string_java_lang_ClassFormatError;
extern char *string_java_lang_Error;
extern char *string_java_lang_ExceptionInInitializerError;
extern char *string_java_lang_IncompatibleClassChangeError;
extern char *string_java_lang_InternalError;
extern char *string_java_lang_LinkageError;
extern char *string_java_lang_NoClassDefFoundError;
extern char *string_java_lang_NoSuchFieldError;
extern char *string_java_lang_NoSuchMethodError;
extern char *string_java_lang_OutOfMemoryError;
extern char *string_java_lang_UnsupportedClassVersionError;
extern char *string_java_lang_VerifyError;
extern char *string_java_lang_VirtualMachineError;


/* function prototypes */

/* load, link and compile exceptions used in the system */

void init_system_exceptions();


/* exception throwing functions */

void throw_exception();
void throw_exception_exit();

void throw_main_exception();
void throw_main_exception_exit();

void throw_cacao_exception_exit(char *exception, char *message, ...);


/* initialize new exceptions */

java_objectheader *new_exception(char *classname);
java_objectheader *new_exception_message(char *classname, char *message);
java_objectheader *new_exception_throwable(char *classname, java_lang_Throwable *cause);
java_objectheader *new_exception_utfmessage(char *classname, utf *message);
java_objectheader *new_exception_javastring(char *classname, java_lang_String *message);
java_objectheader *new_exception_int(char *classname, s4 i);


/* functions to generate compiler exceptions */

java_objectheader *new_classformaterror(classinfo *c, char *message, ...);
java_objectheader *new_verifyerror(methodinfo *m, char *message);
java_objectheader *new_unsupportedclassversionerror(classinfo *c, char *message, ...);

java_objectheader *new_arithmeticexception();
java_objectheader *new_arrayindexoutofboundsexception(s4 index);
java_objectheader *new_arraystoreexception();
java_objectheader *new_classcastexception();
java_objectheader *new_negativearraysizeexception();
java_objectheader *new_nullpointerexception();

#endif /* _EXCEPTIONS_H */


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
