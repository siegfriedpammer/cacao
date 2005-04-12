/* src/vm/utf.h - utf string header

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

   Authors: Christian Thalinger

   Changes:

   $Id: utf8.h 2277 2005-04-12 19:48:03Z twisti $

*/


#ifndef _UTF_H
#define _UTF_H

#include <stdio.h>


/* forward typedefs ***********************************************************/

typedef struct utf utf;


#include "vm/global.h"


/* data structure for utf8 symbols ********************************************/

struct utf {
	utf  *hashlink;                     /* link for external hash chain       */
	int   blength;                      /* text length in bytes               */
	char *text;                         /* pointer to text                    */
};


/* utf-symbols for pointer comparison of frequently used strings **************/

extern utf *utf_java_lang_Object;

extern utf *utf_java_lang_Class;
extern utf *utf_java_lang_ClassLoader;
extern utf *utf_java_lang_Cloneable;
extern utf *utf_java_lang_SecurityManager;
extern utf *utf_java_lang_String;
extern utf *utf_java_lang_System;
extern utf *utf_java_io_Serializable;

extern utf *utf_java_lang_Throwable;
extern utf *utf_java_lang_VMThrowable;
extern utf *utf_java_lang_Error;
extern utf *utf_java_lang_Exception;
extern utf *utf_java_lang_NoClassDefFoundError;
extern utf *utf_java_lang_OutOfMemoryError;
extern utf *utf_java_lang_ClassNotFoundException;

extern utf* utf_java_lang_Void;
extern utf* utf_java_lang_Boolean;
extern utf* utf_java_lang_Byte;
extern utf* utf_java_lang_Character;
extern utf* utf_java_lang_Short;
extern utf* utf_java_lang_Integer;
extern utf* utf_java_lang_Long;
extern utf* utf_java_lang_Float;
extern utf* utf_java_lang_Double;

extern utf *utf_java_util_Vector;

extern utf *utf_InnerClasses;
extern utf *utf_ConstantValue;
extern utf *utf_Code;
extern utf *utf_Exceptions;
extern utf *utf_LineNumberTable;
extern utf *utf_SourceFile;

extern utf *utf_init;
extern utf *utf_clinit;
extern utf *utf_finalize;

extern utf *utf_printStackTrace;
extern utf *utf_fillInStackTrace;
extern utf *utf_loadClass;

extern utf *utf_void__void;
extern utf *utf_boolean__void;
extern utf *utf_byte__void;
extern utf *utf_char__void;
extern utf *utf_short__void;
extern utf *utf_int__void;
extern utf *utf_long__void;
extern utf *utf_float__void;
extern utf *utf_double__void;
extern utf *utf_void__java_lang_Object;
extern utf *utf_void__java_lang_Throwable;
extern utf *utf_java_lang_String__void;
extern utf *utf_java_lang_String__java_lang_Class;
extern utf *utf_java_lang_Throwable__void;

extern utf *array_packagename;


/* function prototypes ********************************************************/

void utf8_init(void);

u4 utf_hashkey(const char *text, u4 length);

/* determine hashkey of a unicode-symbol */
u4 unicode_hashkey(u2 *text, u2 length);

/* create new utf-symbol */
utf *utf_new(const char *text, u2 length);

/* without locking (caller already holding lock*/
utf *utf_new_intern(const char *text, u2 length);

/* make utf symbol from u2 array */
utf *utf_new_u2(u2 *unicodedata, u4 unicodelength, bool isclassname);

utf *utf_new_char(const char *text);
utf *utf_new_char_classname(const char *text);

/* get next unicode character of a utf-string */
u2 utf_nextu2(char **utf);

/* get number of unicode characters of a utf string */
u4 utf_strlen(utf *u);

/* determine utf length in bytes of a u2 array */
u4 u2_utflength(u2 *text, u4 u2_length);

/* write utf symbol to file/buffer */
void utf_display(utf *u);
void utf_display_classname(utf *u);

void utf_sprint(char *buffer, utf *u);
void utf_sprint_classname(char *buffer, utf *u);

void utf_strcat(char *buffer, utf *u);
void utf_strcat_classname(char *buffer, utf *u);

void utf_fprint(FILE *file, utf *u);
void utf_fprint_classname(FILE *file, utf *u);

/* check if a UTF-8 string is valid */
bool is_valid_utf(char *utf_ptr, char *end_pos);

/* check if a UTF-8 string may be used as a class/field/method name */
bool is_valid_name(char *utf_ptr, char *end_pos);
bool is_valid_name_utf(utf *u);

/* show utf-table */
void utf_show(void);

#endif /* _UTF_H */


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
