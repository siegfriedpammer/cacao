/* src/vm/class.h - class related functions header

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

   $Id: class.h 2091 2005-03-27 14:49:48Z edwin $

*/


#ifndef _CLASS_H
#define _CLASS_H

#include "vm/tables.h"
#include "vm/utf8.h"


/* global variables ***********************************************************/

extern hashtable class_hash;            /* hashtable for classes              */

extern list unlinkedclasses;   /* this is only used for eager class loading   */


/* frequently used classes ****************************************************/

/* important system classes */

extern classinfo *class_java_lang_Object;
extern classinfo *class_java_lang_Class;
extern classinfo *class_java_lang_ClassLoader;
extern classinfo *class_java_lang_Cloneable;
extern classinfo *class_java_lang_SecurityManager;
extern classinfo *class_java_lang_String;
extern classinfo *class_java_lang_System;
extern classinfo *class_java_io_Serializable;


/* system exception classes required in cacao */

extern classinfo *class_java_lang_Throwable;
extern classinfo *class_java_lang_VMThrowable;
extern classinfo *class_java_lang_Exception;
extern classinfo *class_java_lang_Error;
extern classinfo *class_java_lang_OutOfMemoryError;


extern classinfo *class_java_lang_Void;
extern classinfo *class_java_lang_Boolean;
extern classinfo *class_java_lang_Byte;
extern classinfo *class_java_lang_Character;
extern classinfo *class_java_lang_Short;
extern classinfo *class_java_lang_Integer;
extern classinfo *class_java_lang_Long;
extern classinfo *class_java_lang_Float;
extern classinfo *class_java_lang_Double;


/* some classes which may be used more often */

extern classinfo *class_java_util_Vector;


/* function prototypes ********************************************************/

void class_init_foo(void);

/* search for class and create it if not found */
classinfo *class_new(utf *u);

/* without locking (caller already holding lock*/
classinfo *class_new_intern(utf *u);

/* search for class in classtable */
classinfo *class_get(utf *u);

/* remove class from classtable */
bool class_remove(classinfo *c);

/* return an array class with the given component class */
classinfo *class_array_of(classinfo *component);

/* return an array class with the given dimension and element class */
classinfo *class_multiarray_of(s4 dim, classinfo *element);

/* return a classref for the given class name */
/* (does a linear search!)                    */
constant_classref *class_lookup_classref(classinfo *cls,utf *name);

/* return a classref for the given class name */
/* (does a linear search!)                    */
constant_classref *class_get_classref(classinfo *cls,utf *name);

/* return a classref for an array with the given dimension of with the */
/* given component type */
constant_classref *class_get_classref_multiarray_of(s4 dim,constant_classref *ref);

/* return a classref for the component type of the given array type */
constant_classref *class_get_classref_component_of(constant_classref *ref);

#endif /* _CLASS_H */


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
