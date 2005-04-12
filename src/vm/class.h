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

   $Id: class.h 2275 2005-04-12 19:46:03Z twisti $

*/


#ifndef _CLASS_H
#define _CLASS_H

/* forward typedefs ***********************************************************/

typedef struct innerclassinfo innerclassinfo;
typedef struct extra_classref extra_classref;

#include "config.h"
#include "toolbox/list.h"
#include "vm/global.h"
#include "vm/utf8.h"
#include "vm/references.h"
#include "vm/field.h"
#include "vm/linker.h"
#include "vm/tables.h"
#include "vm/jit/inline/sets.h"

/* classinfo ******************************************************************/

struct classinfo {                /* class structure                          */
	java_objectheader header;     /* classes are also objects                 */
	java_objectarray* signers;
	struct java_security_ProtectionDomain* pd;
	struct java_lang_VMClass* vmClass;
	struct java_lang_reflect_Constructor* constructor;

	s4 initializing_thread;       /* gnu classpath                            */
	s4 erroneous_state;           /* gnu classpath                            */
	struct gnu_classpath_RawData* vmData; /* gnu classpath                    */

	s4          flags;            /* ACC flags                                */
	utf        *name;             /* class name                               */

	s4          cpcount;          /* number of entries in constant pool       */
	u1         *cptags;           /* constant pool tags                       */
	voidptr    *cpinfos;          /* pointer to constant pool info structures */

	s4          classrefcount;    /* number of symbolic class references      */
	constant_classref *classrefs; /* table of symbolic class references       */
	extra_classref *extclassrefs; /* additional classrefs                     */
	s4          parseddescsize;   /* size of the parsed descriptors block     */
	u1         *parseddescs;      /* parsed descriptors                       */

	classref_or_classinfo super;  /* super class                              */
	classinfo  *sub;              /* sub class pointer                        */
	classinfo  *nextsub;          /* pointer to next class in sub class list  */

	s4          interfacescount;  /* number of interfaces                     */
	classref_or_classinfo *interfaces; /* superinterfaces                     */

	s4          fieldscount;      /* number of fields                         */
	fieldinfo  *fields;           /* field table                              */

	s4          methodscount;     /* number of methods                        */
	methodinfo *methods;          /* method table                             */

	listnode    listnode;         /* linkage                                  */

	bool        initialized;      /* true, if class already initialized       */
	bool        initializing;     /* flag for the compiler                    */
	bool        loaded;           /* true, if class already loaded            */
	bool        linked;           /* true, if class already linked            */
	s4          index;            /* hierarchy depth (classes) or index       */
	                              /* (interfaces)                             */
	s4          instancesize;     /* size of an instance of this class        */

	vftbl_t    *vftbl;            /* pointer to virtual function table        */

	methodinfo *finalizer;        /* finalizer method                         */

	u2          innerclasscount;  /* number of inner classes                  */
	innerclassinfo *innerclass;

	bool        classvftbl;       /* has its own copy of the Class vtbl       */

	s4          classUsed;        /* 0= not used 1 = used   CO-RT             */

	classSetNode *impldBy;        /* interface class implemented by class set */
	                              /*   Object class 's impldBy is list of all */
	                              /*   interface classes used (RT & XTA only  */
	                              /*     normally no list of interfaces used) */
	utf        *packagename;      /* full name of the package                 */
	utf        *sourcefile;       /* classfile name containing this class     */
	java_objectheader *classloader; /* NULL for bootstrap classloader         */
};


/* innerclassinfo *************************************************************/

struct innerclassinfo {
	classref_or_classinfo inner_class; /* inner class pointer                 */
	classref_or_classinfo outer_class; /* outer class pointer                 */
	utf                  *name;        /* innerclass name                     */
	s4                    flags;       /* ACC flags                           */
};


/* extra_classref **************************************************************

   for classrefs not occurring within descriptors

*******************************************************************************/

struct extra_classref {
	extra_classref    *next;
	constant_classref  classref;
};


/* global variables ***********************************************************/

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
extern classinfo *class_java_lang_Error;
extern classinfo *class_java_lang_NoClassDefFoundError;
extern classinfo *class_java_lang_OutOfMemoryError;

extern classinfo *class_java_lang_Exception;
extern classinfo *class_java_lang_ClassNotFoundException;


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


/* pseudo classes for the type checker ****************************************/

/*
 * pseudo_class_Arraystub
 *     (extends Object implements Cloneable, java.io.Serializable)
 *
 *     If two arrays of incompatible component types are merged,
 *     the resulting reference has no accessible components.
 *     The result does, however, implement the interfaces Cloneable
 *     and java.io.Serializable. This pseudo class is used internally
 *     to represent such results. (They are *not* considered arrays!)
 *
 * pseudo_class_Null
 *
 *     This pseudo class is used internally to represent the
 *     null type.
 *
 * pseudo_class_New
 *
 *     This pseudo class is used internally to represent the
 *     the 'uninitialized object' type.
 */

extern classinfo *pseudo_class_Arraystub;
extern classinfo *pseudo_class_Null;
extern classinfo *pseudo_class_New;


/* function prototypes ********************************************************/

/* create a new classinfo struct */
classinfo *class_create_classinfo(utf *u);

/* retrieve constantpool element */
voidptr class_getconstant(classinfo *class, u4 pos, u4 ctype);
voidptr innerclass_getconstant(classinfo *c, u4 pos, u4 ctype);

/* frees all resources used by the class */
void class_free(classinfo *);

/* return an array class with the given component class */
classinfo *class_array_of(classinfo *component,bool link);

/* return an array class with the given dimension and element class */
classinfo *class_multiarray_of(s4 dim, classinfo *element,bool link);

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
