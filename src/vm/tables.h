/* vm/tables.h - 

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

   Authors: Reinhard Grafl

   $Id: tables.h 1930 2005-02-10 10:54:28Z twisti $

*/


#ifndef _TABLES_H
#define _TABLES_H

#include <stdio.h>

#include "vm/global.h"


/* data structures for hashtables ********************************************


   All utf-symbols, javastrings and classes are stored in global
   hashtables, so every symbol exists only once. Equal symbols have
   identical pointers.  The functions for adding hashtable elements
   search the table for the element with the specified name/text and
   return it on success. Otherwise a new hashtable element is created.

   The hashtables use external linking for handling collisions. The
   hashtable structure contains a pointer <ptr> to the array of
   hashtable slots. The number of hashtable slots and therefore the
   size of this array is specified by the element <size> of hashtable
   structure. <entries> contains the number of all hashtable elements
   stored in the table, including those in the external chains.  The
   hashtable element structures (utf, literalstring, classinfo)
   contain both a pointer to the next hashtable element as a link for
   the external hash chain and the key of the element. The key is
   computed from the text of the string or the classname by using up
   to 8 characters.
	
   If the number of entries in the hashtable exceeds twice the size of
   the hashtableslot-array it is supposed that the average length of
   the external chains has reached a value beyond 2. Therefore the
   functions for adding hashtable elements (utf_new, class_new,
   literalstring_new) double the hashtableslot-array. In this
   restructuring process all elements have to be inserted into the new
   hashtable and new external chains must be built.

   Example for the layout of a hashtable:

hashtable.ptr-->+-------------------+
                |                   |
                         ...
                |                   |
                +-------------------+   +-------------------+   +-------------------+
                | hashtable element |-->| hashtable element |-->| hashtable element |-->NULL
                +-------------------+   +-------------------+   +-------------------+
                | hashtable element |
                +-------------------+   +-------------------+   
                | hashtable element |-->| hashtable element |-->NULL
                +-------------------+   +-------------------+   
                | hashtable element |-->NULL
                +-------------------+
                |                   |
                         ...
                |                   |
                +-------------------+

*/


/* data structure for accessing hashtables ************************************/

typedef struct hashtable hashtable;

struct hashtable {            
	u4     size;
	u4     entries;                     /* number of entries in the table     */
	void **ptr;                         /* pointer to hashtable               */
};


#define CLASS(name)     (unicode_getclasslink(unicode_new_char(name)))

/* to determine the end of utf strings */
#define utf_end(utf) ((char *) utf->text+utf->blength)

extern hashtable utf_hash;     /* hashtable for utf8-symbols */
extern hashtable string_hash;  /* hashtable for javastrings  */


/* creates hashtables for symboltables */
void tables_init(void);

/* free memory for hashtables */ 
void tables_close(void);

/* get javatype according to a typedescriptor */
u2 desc_to_type(utf *descriptor);

/* get length of a datatype */
u2 desc_typesize(utf *descriptor);

/* create hashtable */
void init_hashtable(hashtable *hash, u4 size);

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void tables_lock(void);
void tables_unlock(void);
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
