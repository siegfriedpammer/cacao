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

   $Id: tables.h 687 2003-12-04 22:29:54Z edwin $

*/


#ifndef _TABLES_H
#define _TABLES_H

#include <stdio.h>
#include "global.h" /* for unicode. -- phil */

#define CLASS(name)     (unicode_getclasslink(unicode_new_char(name)))

/* to determine the end of utf strings */
#define utf_end(utf) ((char *) utf->text+utf->blength)

/* switches for debug messages */
extern bool collectverbose;
extern list unloadedclasses;

/* function for disposing javastrings */
typedef void (*stringdeleter) (java_objectheader *string);
    
/* creates hashtables for symboltables */
void tables_init();

/* free memory for hashtables */ 
void tables_close(stringdeleter del);

/* write utf symbol to file/buffer */
void utf_sprint(char *buffer, utf *u);
void utf_fprint(FILE *file, utf *u);
void utf_display(utf *u);

/* create new utf-symbol */
utf *utf_new(char *text, u2 length);
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

/* return an array class with the given component class */
classinfo *class_array_of(classinfo *component);

/* return an array class with the given dimension and element class */
classinfo *class_multiarray_of(int dim,classinfo *element);

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


void heap_init (u4 size, u4 startsize, void **stackbottom);
void heap_close();
void *heap_allocate(u4 bytelength, bool references, methodinfo *finalizer);
void heap_addreference(void **reflocation);

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
