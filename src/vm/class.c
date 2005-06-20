/* src/vm/class.c - class related functions

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

   Changes: Mark Probst
            Andreas Krall
            Christian Thalinger

   $Id: class.c 2743 2005-06-20 11:54:06Z edwin $

*/

#include <assert.h>
#include <string.h>

#include "vm/global.h"
#include "mm/memory.h"

#if defined(USE_THREADS)
# if defined(NATIVE_THREADS)
#  include "threads/native/threads.h"
# else
#  include "threads/green/threads.h"
#  include "threads/green/locks.h"
# endif
#endif

#include "toolbox/logging.h"
#include "vm/class.h"
#include "vm/classcache.h"
#include "vm/exceptions.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/resolve.h"
#include "vm/statistics.h"
#include "vm/tables.h"
#include "vm/utf8.h"


/******************************************************************************/
/* DEBUG HELPERS                                                              */
/******************************************************************************/

#ifndef NDEBUG
#define CLASS_DEBUG
#endif

#ifdef CLASS_DEBUG
#define CLASS_ASSERT(cond)  assert(cond)
#else
#define CLASS_ASSERT(cond)
#endif


/* global variables ***********************************************************/

list unlinkedclasses;                   /* this is only used for eager class  */
                                        /* loading                            */


/* frequently used classes ****************************************************/

/* important system classes */

classinfo *class_java_lang_Object = NULL;
classinfo *class_java_lang_Class = NULL;
classinfo *class_java_lang_ClassLoader = NULL;
classinfo *class_java_lang_Cloneable = NULL;
classinfo *class_java_lang_SecurityManager = NULL;
classinfo *class_java_lang_String = NULL;
classinfo *class_java_lang_System = NULL;
classinfo *class_java_lang_ThreadGroup = NULL;
classinfo *class_java_io_Serializable = NULL;


/* system exception classes required in cacao */

classinfo *class_java_lang_Throwable = NULL;
classinfo *class_java_lang_VMThrowable = NULL;
classinfo *class_java_lang_Error = NULL;
classinfo *class_java_lang_NoClassDefFoundError = NULL;
classinfo *class_java_lang_OutOfMemoryError = NULL;

classinfo *class_java_lang_Exception = NULL;
classinfo *class_java_lang_ClassNotFoundException = NULL;

classinfo *class_java_lang_Void = NULL;
classinfo *class_java_lang_Boolean = NULL;
classinfo *class_java_lang_Byte = NULL;
classinfo *class_java_lang_Character = NULL;
classinfo *class_java_lang_Short = NULL;
classinfo *class_java_lang_Integer = NULL;
classinfo *class_java_lang_Long = NULL;
classinfo *class_java_lang_Float = NULL;
classinfo *class_java_lang_Double = NULL;


/* some classes which may be used more often */

classinfo *class_java_lang_StackTraceElement = NULL;
classinfo *class_java_lang_reflect_Constructor = NULL;
classinfo *class_java_lang_reflect_Field = NULL;
classinfo *class_java_lang_reflect_Method = NULL;
classinfo *class_java_security_PrivilegedAction = NULL;
classinfo *class_java_util_Vector = NULL;

classinfo *arrayclass_java_lang_Object = NULL;


/* pseudo classes for the typechecker */

classinfo *pseudo_class_Arraystub = NULL;
classinfo *pseudo_class_Null = NULL;
classinfo *pseudo_class_New = NULL;

/* class_set_packagename *******************************************************

   Derive the package name from the class name and store it in the struct.

*******************************************************************************/

void class_set_packagename(classinfo *c)
{
	if (c->name->text[0] == '[') {
		/* Array classes are not loaded from classfiles. */
		c->packagename = array_packagename;
	} 
	else {
		/* Find the package name */
		/* Classes in the unnamed package keep packagename == NULL. */
		char *p = UTF_END(c->name) - 1;
		char *start = c->name->text;
		for (;p > start; --p) {
			if (*p == '/') {
				c->packagename = utf_new(start, p - start);
				break;
			}
		}
	}
}

/* class_create_classinfo ******************************************************

   Create a new classinfo struct. The class name is set to the given utf *,
   most other fields are initialized to zero.

   Note: classname may be NULL. In this case a not-yet-named classinfo is
         created. The name must be filled in later and class_set_packagename
		 must be called after that.

*******************************************************************************/

classinfo *class_create_classinfo(utf *classname)
{
	classinfo *c;

#if defined(STATISTICS)
	if (opt_stat)
		count_class_infos += sizeof(classinfo);
#endif

	/* we use a safe name for temporarily unnamed classes */
	if (!classname)
		classname = utf_not_named_yet;

	if (initverbose)
		log_message_utf("Creating class: ", classname);

	c = GCNEW(classinfo, 1); /*JOWENN: NEW*/
	/*c=NEW(classinfo);*/
	c->vmClass = 0;
	c->flags = 0;
	c->name = classname;
	c->packagename = NULL;
	c->cpcount = 0;
	c->cptags = NULL;
	c->cpinfos = NULL;
	c->classrefs = NULL;
	c->extclassrefs = NULL;
	c->classrefcount = 0;
	c->parseddescs = NULL;
	c->parseddescsize = 0;
	c->super.any = NULL;
	c->sub = NULL;
	c->nextsub = NULL;
	c->interfacescount = 0;
	c->interfaces = NULL;
	c->fieldscount = 0;
	c->fields = NULL;
	c->methodscount = 0;
	c->methods = NULL;
	c->linked = false;
	c->loaded = false;
	c->index = 0;
	c->instancesize = 0;
	c->header.vftbl = NULL;
	c->innerclasscount = 0;
	c->innerclass = NULL;
	c->vftbl = NULL;
	c->initialized = false;
	c->initializing = false;
	c->classvftbl = false;
    c->classUsed = 0;
    c->impldBy = NULL;
	c->classloader = NULL;
	c->sourcefile = NULL;
	
	if (classname != utf_not_named_yet) {
		class_set_packagename(c);
	}

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	initObjectLock(&c->header);
#endif

	return c;
}

/* class_freepool **************************************************************

	Frees all resources used by this classes Constant Pool.

*******************************************************************************/

static void class_freecpool(classinfo *c)
{
	u4 idx;
	u4 tag;
	voidptr info;
	
	if (c->cptags && c->cpinfos) {
		for (idx = 0; idx < c->cpcount; idx++) {
			tag = c->cptags[idx];
			info = c->cpinfos[idx];
		
			if (info != NULL) {
				switch (tag) {
				case CONSTANT_Fieldref:
				case CONSTANT_Methodref:
				case CONSTANT_InterfaceMethodref:
					FREE(info, constant_FMIref);
					break;
				case CONSTANT_Integer:
					FREE(info, constant_integer);
					break;
				case CONSTANT_Float:
					FREE(info, constant_float);
					break;
				case CONSTANT_Long:
					FREE(info, constant_long);
					break;
				case CONSTANT_Double:
					FREE(info, constant_double);
					break;
				case CONSTANT_NameAndType:
					FREE(info, constant_nameandtype);
					break;
				}
			}
		}
	}

	if (c->cptags)
		MFREE(c->cptags, u1, c->cpcount);

	if (c->cpinfos)
		MFREE(c->cpinfos, voidptr, c->cpcount);
}


/* class_getconstant ***********************************************************

   Retrieves the value at position 'pos' of the constantpool of a
   class. If the type of the value is other than 'ctype', an error is
   thrown.

*******************************************************************************/

voidptr class_getconstant(classinfo *c, u4 pos, u4 ctype)
{
	/* check index and type of constantpool entry */
	/* (pos == 0 is caught by type comparison) */

	if (pos >= c->cpcount || c->cptags[pos] != ctype) {
		*exceptionptr = new_classformaterror(c, "Illegal constant pool index");
		return NULL;
	}

	return c->cpinfos[pos];
}


/* innerclass_getconstant ******************************************************

   Like class_getconstant, but if cptags is ZERO, null is returned.
	
*******************************************************************************/

voidptr innerclass_getconstant(classinfo *c, u4 pos, u4 ctype)
{
	/* invalid position in constantpool */
	if (pos >= c->cpcount) {
		*exceptionptr = new_classformaterror(c, "Illegal constant pool index");
		return NULL;
	}

	/* constantpool entry of type 0 */	
	if (!c->cptags[pos])
		return NULL;

	/* check type of constantpool entry */
	if (c->cptags[pos] != ctype) {
		*exceptionptr = new_classformaterror(c, "Illegal constant pool index");
		return NULL;
	}
		
	return c->cpinfos[pos];
}


/* class_free ******************************************************************

   Frees all resources used by the class.

*******************************************************************************/

void class_free(classinfo *c)
{
	s4 i;
	vftbl_t *v;
		
	class_freecpool(c);

	if (c->interfaces)
		MFREE(c->interfaces, classinfo*, c->interfacescount);

	if (c->fields) {
		for (i = 0; i < c->fieldscount; i++)
			field_free(&(c->fields[i]));
/*  	MFREE(c->fields, fieldinfo, c->fieldscount); */
	}
	
	if (c->methods) {
		for (i = 0; i < c->methodscount; i++)
			method_free(&(c->methods[i]));
		MFREE(c->methods, methodinfo, c->methodscount);
	}

	if ((v = c->vftbl) != NULL) {
		if (v->arraydesc)
			mem_free(v->arraydesc,sizeof(arraydescriptor));
		
		for (i = 0; i < v->interfacetablelength; i++) {
			MFREE(v->interfacetable[-i], methodptr, v->interfacevftbllength[i]);
		}
		MFREE(v->interfacevftbllength, s4, v->interfacetablelength);

		i = sizeof(vftbl_t) + sizeof(methodptr) * (v->vftbllength - 1) +
		    sizeof(methodptr*) * (v->interfacetablelength -
		                         (v->interfacetablelength > 0));
		v = (vftbl_t*) (((methodptr*) v) -
						(v->interfacetablelength - 1) * (v->interfacetablelength > 1));
		mem_free(v, i);
	}

	if (c->innerclass)
		MFREE(c->innerclass, innerclassinfo, c->innerclasscount);

	/*	if (c->classvftbl)
		mem_free(c->header.vftbl, sizeof(vftbl) + sizeof(methodptr)*(c->vftbl->vftbllength-1)); */
	
/*  	GCFREE(c); */
}


/* get_array_class *************************************************************

   Returns the array class with the given name for the given
   classloader, or NULL if an exception occurred.

   Note: This function does eager loading. 

*******************************************************************************/

static classinfo *get_array_class(utf *name,java_objectheader *initloader,
											java_objectheader *defloader,bool link)
{
	classinfo *c;
	
	/* lookup this class in the classcache */
	c = classcache_lookup(initloader,name);
	if (!c)
		c = classcache_lookup_defined(defloader,name);

	if (!c) {
		/* we have to create it */
		c = class_create_classinfo(name);
		c = load_newly_created_array(c,initloader);
		if (c == NULL)
			return NULL;
	}

	CLASS_ASSERT(c);
	CLASS_ASSERT(c->loaded);
	CLASS_ASSERT(c->classloader == defloader);

	if (link && !c->linked)
		if (!link_class(c))
			return NULL;

	CLASS_ASSERT(!link || c->linked);

	return c;
}


/* class_array_of **************************************************************

   Returns an array class with the given component class. The array
   class is dynamically created if neccessary.

*******************************************************************************/

classinfo *class_array_of(classinfo *component, bool link)
{
    s4 namelen;
    char *namebuf;

    /* Assemble the array class name */
    namelen = component->name->blength;
    
    if (component->name->text[0] == '[') {
        /* the component is itself an array */
        namebuf = DMNEW(char, namelen + 1);
        namebuf[0] = '[';
        MCOPY(namebuf + 1, component->name->text, char, namelen);
        namelen++;

    } else {
        /* the component is a non-array class */
        namebuf = DMNEW(char, namelen + 3);
        namebuf[0] = '[';
        namebuf[1] = 'L';
        MCOPY(namebuf + 2, component->name->text, char, namelen);
        namebuf[2 + namelen] = ';';
        namelen += 3;
    }

	return get_array_class(utf_new(namebuf, namelen),
						   component->classloader,
						   component->classloader,
						   link);
}


/* class_multiarray_of *********************************************************

   Returns an array class with the given dimension and element class.
   The array class is dynamically created if neccessary.

*******************************************************************************/

classinfo *class_multiarray_of(s4 dim, classinfo *element, bool link)
{
    s4 namelen;
    char *namebuf;

	if (dim < 1) {
		log_text("Invalid array dimension requested");
		assert(0);
	}

    /* Assemble the array class name */
    namelen = element->name->blength;
    
    if (element->name->text[0] == '[') {
        /* the element is itself an array */
        namebuf = DMNEW(char, namelen + dim);
        memcpy(namebuf + dim, element->name->text, namelen);
        namelen += dim;
    }
    else {
        /* the element is a non-array class */
        namebuf = DMNEW(char, namelen + 2 + dim);
        namebuf[dim] = 'L';
        memcpy(namebuf + dim + 1, element->name->text, namelen);
        namelen += (2 + dim);
        namebuf[namelen - 1] = ';';
    }
	memset(namebuf, '[', dim);

	return get_array_class(utf_new(namebuf, namelen),
						   element->classloader,
						   element->classloader,
						   link);
}


/* class_lookup_classref *******************************************************

   Looks up the constant_classref for a given classname in the classref
   tables of a class.

   IN:
       cls..............the class containing the reference
	   name.............the name of the class refered to

    RETURN VALUE:
	   a pointer to a constant_classref, or 
	   NULL if the reference was not found
   
*******************************************************************************/

constant_classref *class_lookup_classref(classinfo *cls, utf *name)
{
	constant_classref *ref;
	extra_classref *xref;
	int count;

	CLASS_ASSERT(cls);
	CLASS_ASSERT(name);
	CLASS_ASSERT(!cls->classrefcount || cls->classrefs);
	
	/* first search the main classref table */
	count = cls->classrefcount;
	ref = cls->classrefs;
	for (; count; --count, ++ref)
		if (ref->name == name)
			return ref;

	/* next try the list of extra classrefs */
	for (xref = cls->extclassrefs; xref; xref = xref->next) {
		if (xref->classref.name == name)
			return &(xref->classref);
	}

	/* not found */
	return NULL;
}


/* class_get_classref **********************************************************

   Returns the constant_classref for a given classname.

   IN:
       cls..............the class containing the reference
	   name.............the name of the class refered to

   RETURN VALUE:
       a pointer to a constant_classref (never NULL)

   NOTE:
       The given name is not checked for validity!
   
*******************************************************************************/

constant_classref *class_get_classref(classinfo *cls, utf *name)
{
	constant_classref *ref;
	extra_classref *xref;

	CLASS_ASSERT(cls);
	CLASS_ASSERT(name);

	ref = class_lookup_classref(cls,name);
	if (ref)
		return ref;

	xref = NEW(extra_classref);
	CLASSREF_INIT(xref->classref,cls,name);

	xref->next = cls->extclassrefs;
	cls->extclassrefs = xref;

	return &(xref->classref);
}


/* class_get_self_classref *****************************************************

   Returns the constant_classref to the class itself.

   IN:
       cls..............the class containing the reference

   RETURN VALUE:
       a pointer to a constant_classref (never NULL)

*******************************************************************************/

constant_classref *class_get_self_classref(classinfo *cls)
{
	/* XXX this should be done in a faster way. Maybe always make */
	/* the classref of index 0 a self reference.                  */
	return class_get_classref(cls,cls->name);
}

/* class_get_classref_multiarray_of ********************************************

   Returns an array type reference with the given dimension and element class
   reference.

   IN:
       dim..............the requested dimension
	                    dim must be in [1;255]. This is NOT checked!
	   ref..............the component class reference

   RETURN VALUE:
       a pointer to the class reference for the array type

   NOTE:
       The referer of `ref` is used as the referer for the new classref.

*******************************************************************************/

constant_classref *class_get_classref_multiarray_of(s4 dim, constant_classref *ref)
{
    s4 namelen;
    char *namebuf;

	CLASS_ASSERT(ref);
	CLASS_ASSERT(dim >= 1 && dim <= 255);

    /* Assemble the array class name */
    namelen = ref->name->blength;
    
    if (ref->name->text[0] == '[') {
        /* the element is itself an array */
        namebuf = DMNEW(char, namelen + dim);
        memcpy(namebuf + dim, ref->name->text, namelen);
        namelen += dim;
    }
    else {
        /* the element is a non-array class */
        namebuf = DMNEW(char, namelen + 2 + dim);
        namebuf[dim] = 'L';
        memcpy(namebuf + dim + 1, ref->name->text, namelen);
        namelen += (2 + dim);
        namebuf[namelen - 1] = ';';
    }
	memset(namebuf, '[', dim);

    return class_get_classref(ref->referer,utf_new(namebuf, namelen));
}

/* class_get_classref_component_of *********************************************

   Returns the component classref of a given array type reference

   IN:
       ref..............the array type reference

   RETURN VALUE:
       a reference to the component class, or
	   NULL if `ref` is not an object array type reference

   NOTE:
       The referer of `ref` is used as the referer for the new classref.

*******************************************************************************/

constant_classref *class_get_classref_component_of(constant_classref *ref)
{
	s4 namelen;
	char *name;
	
	CLASS_ASSERT(ref);

	name = ref->name->text;
	if (*name++ != '[')
		return NULL;
	
	namelen = ref->name->blength - 1;
	if (*name == 'L') {
		name++;
		namelen -= 2;
	}
	else if (*name != '[') {
		return NULL;
	}

    return class_get_classref(ref->referer, utf_new(name, namelen));
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
