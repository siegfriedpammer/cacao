/* tables.h - 

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

   $Id: tables.h 1034 2004-04-26 16:20:33Z twisti $

*/


#ifndef _TABLES_H
#define _TABLES_H

#include <stdio.h>
#include "global.h" /* for unicode. -- phil */

#define CLASS(name)     (unicode_getclasslink(unicode_new_char(name)))

/* to determine the end of utf strings */
#define utf_end(utf) ((char *) utf->text+utf->blength)

/* function for disposing javastrings */
typedef void (*stringdeleter) (java_objectheader *string);
    
extern hashtable utf_hash;     /* hashtable for utf8-symbols */
extern hashtable string_hash;  /* hashtable for javastrings  */
extern hashtable class_hash;   /* hashtable for classes      */

/* creates hashtables for symboltables */
void tables_init();

/* free memory for hashtables */ 
void tables_close(stringdeleter del);

/* check if a UTF-8 string is valid */
bool is_valid_utf(char *utf_ptr, char *end_pos);

/* check if a UTF-8 string may be used as a class/field/method name */
bool is_valid_name(char *utf_ptr, char *end_pos);
bool is_valid_name_utf(utf *u);

/* write utf symbol to file/buffer */
void utf_sprint(char *buffer, utf *u);
void utf_sprint_classname(char *buffer, utf *u);
void utf_fprint(FILE *file, utf *u);
void utf_display(utf *u);
void utf_display_classname(utf *u);

/* write utf symbol to logfile/stdout */
void log_utf(utf *u);
void log_plain_utf(utf *u);

/* create new utf-symbol */
utf *utf_new(char *text, u2 length);
/* without locking (caller already holding lock*/
utf *utf_new_int(char *text, u2 length);
utf *utf_new_char(char *text);
utf *utf_new_char_classname(char *text);

/* show utf-table */
void utf_show();

/* get next unicode character of a utf-string */
u2 utf_nextu2(char **utf);

/* get number of unicode characters of a utf string */
u4 utf_strlen(utf *u);

/* search for class and create it if not found */
classinfo *class_new(utf *u);
/* without locking (caller already holding lock*/
classinfo *class_new_int(utf *u);

/* return an array class with the given component class */
classinfo *class_array_of(classinfo *component);

/* return an array class with the given dimension and element class */
classinfo *class_multiarray_of(int dim, classinfo *element);

/* get javatype according to a typedescriptor */
u2 desc_to_type(utf *descriptor);

/* get length of a datatype */
u2 desc_typesize(utf *descriptor);

/* determine hashkey of a unicode-symbol */
u4 unicode_hashkey(u2 *text, u2 length);

/* create hashtable */
void init_hashtable(hashtable *hash, u4 size);

/* search for class in classtable */
classinfo *class_get(utf *u);

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void tables_lock();
void tables_unlock();
#endif

#endif /* _TABLES_H */


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
