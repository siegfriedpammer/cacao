/* src/vmcore/class.h - class related functions header

   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/


#ifndef _CLASS_H
#define _CLASS_H

/* forward typedefs ***********************************************************/

typedef struct classinfo      classinfo; 
typedef struct innerclassinfo innerclassinfo;
typedef struct extra_classref extra_classref;
typedef struct castinfo       castinfo;


#include "config.h"

#include <stdint.h>

#include "vm/types.h"

#include "toolbox/list.h"

#include "vm/global.h"

#if defined(ENABLE_JAVASE)
# include "vmcore/annotation.h"
#endif

#include "vmcore/field.h"
#include "vmcore/linker.h"
#include "vmcore/loader.h"
#include "vmcore/method.h"
#include "vmcore/references.h"
#include "vmcore/utf8.h"


/* class state defines ********************************************************/

#define CLASS_LOADING         0x0001
#define CLASS_LOADED          0x0002
#define CLASS_LINKING         0x0004
#define CLASS_LINKED          0x0008
#define CLASS_INITIALIZING    0x0010
#define CLASS_INITIALIZED     0x0020
#define CLASS_ERROR           0x0040


/* some macros ****************************************************************/

#define CLASS_IS_OR_ALMOST_INITIALIZED(c) \
    (((c)->state & CLASS_INITIALIZING) || ((c)->state & CLASS_INITIALIZED))


/* classinfo ******************************************************************/

/* We define this dummy structure of java_lang_Class so we can
   bootstrap cacaoh without needing a java_lang_Class.h file.  Whether
   the size of the dummy structure is big enough is checked during
   runtime in vm_create. */

typedef struct {
	java_object_t      header;
#if defined(WITH_CLASSPATH_GNU)
	intptr_t           padding[4];
#elif defined(WITH_CLASSPATH_SUN)
	intptr_t           padding[19];
#elif defined(WITH_CLASSPATH_CLDC1_1)
	intptr_t           padding[3];
#else
# error unknown classpath configuration
#endif
} dummy_java_lang_Class;

struct classinfo {                /* class structure                          */
	dummy_java_lang_Class object;

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

	classinfo  *super;            /* super class                              */
	classinfo  *sub;              /* sub class pointer                        */
	classinfo  *nextsub;          /* pointer to next class in sub class list  */

	int32_t     interfacescount;  /* number of interfaces                     */
	classinfo **interfaces;       /* super interfaces                         */

	int32_t     fieldscount;      /* number of fields                         */
	fieldinfo  *fields;           /* field table                              */

	int32_t     methodscount;     /* number of methods                        */
	methodinfo *methods;          /* method table                             */

	s4          state;            /* current class state                      */
	s4          index;            /* hierarchy depth (classes) or index       */
	                              /* (interfaces)                             */
	s4          instancesize;     /* size of an instance of this class        */

	vftbl_t    *vftbl;            /* pointer to virtual function table        */

	methodinfo *finalizer;        /* finalizer method                         */

	u2          innerclasscount;  /* number of inner classes                  */
	innerclassinfo *innerclass;

	classref_or_classinfo  declaringclass;
	classref_or_classinfo  enclosingclass;  /* enclosing class                */
	constant_nameandtype  *enclosingmethod; /* enclosing method               */

	utf        *packagename;      /* full name of the package                 */
	utf        *sourcefile;       /* SourceFile attribute                     */
#if defined(ENABLE_JAVASE)
	utf        *signature;        /* Signature attribute                      */
#if defined(ENABLE_ANNOTATIONS)
	/* All the annotation attributes are NULL (and not a zero length array)   */
	/* if there is nothing.                                                   */
	java_object_t *annotations;   /* annotations of this class                */
	
	java_object_t *method_annotations; /* array of annotations of the methods */
	java_object_t *method_parameterannotations; /* array of parameter         */
	                              /* annotations of the methods               */
	java_object_t *method_annotationdefaults; /* array of annotation default  */
	                              /* values of the methods                    */

	java_object_t *field_annotations; /* array of annotations of the fields   */

#endif
#endif
	classloader *classloader;       /* NULL for bootstrap classloader         */

#if defined(ENABLE_JAVASE)
# if defined(WITH_CLASSPATH_SUN)
	java_object_t      *protectiondomain;
	java_objectarray_t *signers;
# endif
#endif
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


/* castinfo *******************************************************************/

struct castinfo {
	s4 super_baseval;
	s4 super_diffval;
	s4 sub_baseval;
};


/* global variables ***********************************************************/

/* frequently used classes ****************************************************/

/* important system classes */

extern classinfo *class_java_lang_Object;
extern classinfo *class_java_lang_Class;
extern classinfo *class_java_lang_ClassLoader;
extern classinfo *class_java_lang_Cloneable;
extern classinfo *class_java_lang_SecurityManager;
extern classinfo *class_java_lang_String;
extern classinfo *class_java_lang_System;
extern classinfo *class_java_lang_Thread;
extern classinfo *class_java_lang_ThreadGroup;
extern classinfo *class_java_lang_Throwable;
extern classinfo *class_java_io_Serializable;

#if defined(WITH_CLASSPATH_GNU)
extern classinfo *class_java_lang_VMSystem;
extern classinfo *class_java_lang_VMThread;
extern classinfo *class_java_lang_VMThrowable;
#endif

#if defined(WITH_CLASSPATH_SUN)
extern classinfo *class_sun_reflect_MagicAccessorImpl;
#endif

#if defined(ENABLE_JAVASE)
extern classinfo *class_java_lang_Void;
#endif

extern classinfo *class_java_lang_Boolean;
extern classinfo *class_java_lang_Byte;
extern classinfo *class_java_lang_Character;
extern classinfo *class_java_lang_Short;
extern classinfo *class_java_lang_Integer;
extern classinfo *class_java_lang_Long;
extern classinfo *class_java_lang_Float;
extern classinfo *class_java_lang_Double;

/* some classes which may be used more often */

#if defined(ENABLE_JAVASE)
extern classinfo *class_java_lang_StackTraceElement;
extern classinfo *class_java_lang_reflect_Constructor;
extern classinfo *class_java_lang_reflect_Field;
extern classinfo *class_java_lang_reflect_Method;
extern classinfo *class_java_security_PrivilegedAction;
extern classinfo *class_java_util_Vector;

extern classinfo *arrayclass_java_lang_Object;

# if defined(ENABLE_ANNOTATIONS)
extern classinfo *class_sun_reflect_ConstantPool;
#  if defined(WITH_CLASSPATH_GNU)
extern classinfo *class_sun_reflect_annotation_AnnotationParser;
#  endif
# endif
#endif


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


/* inline functions ***********************************************************/

/* class_is_primitive **********************************************************

   Checks if the given class is a primitive class.

*******************************************************************************/

static inline bool class_is_primitive(classinfo *c)
{
	if (c->flags & ACC_CLASS_PRIMITIVE)
		return true;

	return false;
}


/* class_is_anonymousclass *****************************************************

   Checks if the given class is an anonymous class.

*******************************************************************************/

static inline bool class_is_anonymousclass(classinfo *c)
{
	if (c->flags & ACC_CLASS_ANONYMOUS)
		return true;

	return false;
}


/* class_is_array **************************************************************

   Checks if the given class is an array class.

*******************************************************************************/

static inline bool class_is_array(classinfo *c)
{
	if (!(c->state & CLASS_LINKED))
		if (!link_class(c))
			return false;

	return (c->vftbl->arraydesc != NULL);
}


/* class_is_interface **********************************************************

   Checks if the given class is an interface.

*******************************************************************************/

static inline bool class_is_interface(classinfo *c)
{
	if (c->flags & ACC_INTERFACE)
		return true;

	return false;
}


/* class_is_localclass *********************************************************

   Checks if the given class is a local class.

*******************************************************************************/

static inline bool class_is_localclass(classinfo *c)
{
	if ((c->enclosingmethod != NULL) && !class_is_anonymousclass(c))
		return true;

	return false;
}


/* class_is_memberclass ********************************************************

   Checks if the given class is a member class.

*******************************************************************************/

static inline bool class_is_memberclass(classinfo *c)
{
	if (c->flags & ACC_CLASS_MEMBER)
		return true;

	return false;
}


/* class_get_classloader *******************************************************

   Return the classloader of the given class.

*******************************************************************************/

static inline classloader *class_get_classloader(classinfo *c)
{
	classloader *cl;

	cl = c->classloader;

	/* The classloader may be NULL. */

	return cl;
}


/* class_get_superclass ********************************************************

   Return the super class of the given class.

*******************************************************************************/

static inline classinfo *class_get_superclass(classinfo *c)
{
	/* For interfaces we return NULL. */

	if (c->flags & ACC_INTERFACE)
		return NULL;

	/* For java/lang/Object, primitive-type and Void classes c->super
	   is NULL and we return NULL. */

	return c->super;
}


/* function prototypes ********************************************************/

classinfo *class_create_classinfo(utf *u);
void       class_postset_header_vftbl(void);
classinfo *class_define(utf *name, classloader *cl, int32_t length, const uint8_t *data, java_handle_t *pd);
void       class_set_packagename(classinfo *c);

bool       class_load_attributes(classbuffer *cb);

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

/* return a classref to the class itself */
/* (does a linear search!)                    */
constant_classref *class_get_self_classref(classinfo *cls);

/* return a classref for an array with the given dimension of with the */
/* given component type */
constant_classref *class_get_classref_multiarray_of(s4 dim,constant_classref *ref);

/* return a classref for the component type of the given array type */
constant_classref *class_get_classref_component_of(constant_classref *ref);

/* get a class' field by name and descriptor */
fieldinfo *class_findfield(classinfo *c, utf *name, utf *desc);

/* search 'classinfo'-structure for a field with the specified name */
fieldinfo *class_findfield_by_name(classinfo *c, utf *name);
s4 class_findfield_index_by_name(classinfo *c, utf *name);

/* search class for a field */
fieldinfo *class_resolvefield(classinfo *c, utf *name, utf *desc, classinfo *referer, bool throwexception);

/* search for a method with a specified name and descriptor */
methodinfo *class_findmethod(classinfo *c, utf *name, utf *desc);
methodinfo *class_resolvemethod(classinfo *c, utf *name, utf *dest);
methodinfo *class_resolveclassmethod(classinfo *c, utf *name, utf *dest, classinfo *referer, bool throwexception);
methodinfo *class_resolveinterfacemethod(classinfo *c, utf *name, utf *dest, classinfo *referer, bool throwexception);

bool class_issubclass(classinfo *sub, classinfo *super);
bool class_isanysubclass(classinfo *sub, classinfo *super);

bool                       class_is_primitive(classinfo *c);
bool                       class_is_anonymousclass(classinfo *c);
bool                       class_is_array(classinfo *c);
bool                       class_is_interface(classinfo *c);
bool                       class_is_localclass(classinfo *c);
bool                       class_is_memberclass(classinfo *c);

classloader               *class_get_classloader(classinfo *c);
classinfo                 *class_get_superclass(classinfo *c);
classinfo                 *class_get_componenttype(classinfo *c);
java_handle_objectarray_t *class_get_declaredclasses(classinfo *c, bool publicOnly);
classinfo                 *class_get_declaringclass(classinfo *c);
classinfo                 *class_get_enclosingclass(classinfo *c);
java_handle_objectarray_t *class_get_interfaces(classinfo *c);
java_handle_bytearray_t   *class_get_annotations(classinfo *c);
int32_t                    class_get_modifiers(classinfo *c, bool ignoreInnerClassesAttrib);

#if defined(ENABLE_JAVASE)
utf                       *class_get_signature(classinfo *c);
#endif

/* some debugging functions */

#if !defined(NDEBUG)
void class_printflags(classinfo *c);
void class_print(classinfo *c);
void class_println(classinfo *c);
void class_classref_print(constant_classref *cr);
void class_classref_println(constant_classref *cr);
void class_classref_or_classinfo_print(classref_or_classinfo c);
void class_classref_or_classinfo_println(classref_or_classinfo c);
#endif

/* debug purposes */
void class_showmethods(classinfo *c);
void class_showconstantpool(classinfo *c);

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
