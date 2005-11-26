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

   $Id: class.c 3807 2005-11-26 21:51:11Z edwin $

*/

#include <assert.h>
#include <string.h>

#include "config.h"
#include "vm/types.h"

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
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/resolve.h"
#include "vm/statistics.h"
#include "vm/stringlocal.h"
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
classinfo *class_java_lang_Thread = NULL;
classinfo *class_java_lang_ThreadGroup = NULL;
classinfo *class_java_lang_VMThread = NULL;
classinfo *class_java_io_Serializable = NULL;


/* system exception classes required in cacao */

classinfo *class_java_lang_Throwable = NULL;
classinfo *class_java_lang_VMThrowable = NULL;
classinfo *class_java_lang_Error = NULL;
classinfo *class_java_lang_NoClassDefFoundError = NULL;
classinfo *class_java_lang_LinkageError = NULL;
classinfo *class_java_lang_NoSuchMethodError = NULL;
classinfo *class_java_lang_OutOfMemoryError = NULL;

classinfo *class_java_lang_Exception = NULL;
classinfo *class_java_lang_ClassNotFoundException = NULL;
classinfo *class_java_lang_IllegalArgumentException = NULL;
classinfo *class_java_lang_IllegalMonitorStateException = NULL;

classinfo *class_java_lang_Void = NULL;
classinfo *class_java_lang_Boolean = NULL;
classinfo *class_java_lang_Byte = NULL;
classinfo *class_java_lang_Character = NULL;
classinfo *class_java_lang_Short = NULL;
classinfo *class_java_lang_Integer = NULL;
classinfo *class_java_lang_Long = NULL;
classinfo *class_java_lang_Float = NULL;
classinfo *class_java_lang_Double = NULL;


/* some runtime exception */

classinfo *class_java_lang_NullPointerException = NULL;


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
	char *p = UTF_END(c->name) - 1;
	char *start = c->name->text;

	/* set the package name */
	/* classes in the unnamed package keep packagename == NULL */

	if (c->name->text[0] == '[') {
		/* set packagename of arrays to the element's package */

		for (; *start == '['; start++);

		/* skip the 'L' in arrays of references */
		if (*start == 'L')
			start++;

		for (; (p > start) && (*p != '/'); --p);

		c->packagename = utf_new(start, p - start);

	} else {
		for (; (p > start) && (*p != '/'); --p);

		c->packagename = utf_new(start, p - start);
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

	/* GCNEW_UNCOLLECTABLE clears the allocated memory */

	c = GCNEW_UNCOLLECTABLE(classinfo, 1);
	/*c=NEW(classinfo);*/
	c->name = classname;

	/* set the header.vftbl of all loaded classes to the one of
       java.lang.Class, so Java code can use a class as object */

	if (class_java_lang_Class)
		if (class_java_lang_Class->vftbl)
			c->header.vftbl = class_java_lang_Class->vftbl;
	
	if (classname != utf_not_named_yet)
		class_set_packagename(c);

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	initObjectLock(&c->header);
#endif

	return c;
}


/* class_postset_header_vftbl **************************************************

   Set the header.vftbl of all classes created before java.lang.Class
   was linked.  This is necessary that Java code can use a class as
   object.

*******************************************************************************/

void class_postset_header_vftbl(void)
{
	classinfo *c;
	u4 slot;
	classcache_name_entry *nmen;
	classcache_class_entry *clsen;

	assert(class_java_lang_Class);

	for (slot = 0; slot < classcache_hash.size; slot++) {
		nmen = (classcache_name_entry *) classcache_hash.ptr[slot];

		for (; nmen; nmen = nmen->hashlink) {
			/* iterate over all class entries */

			for (clsen = nmen->classes; clsen; clsen = clsen->next) {
				c = clsen->classobj;

				/* now set the the vftbl */

				if (c->header.vftbl == NULL)
					c->header.vftbl = class_java_lang_Class->vftbl;
			}
		}
	}
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


/* class_findmethod ************************************************************
	
   Searches a 'classinfo' structure for a method having the given name
   and descriptor. If descriptor is NULL, it is ignored.

*******************************************************************************/

methodinfo *class_findmethod(classinfo *c, utf *name, utf *desc)
{
	methodinfo *m;
	s4          i;

	for (i = 0; i < c->methodscount; i++) {
		m = &(c->methods[i]);

		if ((m->name == name) && ((desc == NULL) || (m->descriptor == desc)))
			return m;
	}

	return NULL;
}


/************************* Function: class_findmethod_approx ******************
	
	like class_findmethod but ignores the return value when comparing the
	descriptor.

*******************************************************************************/

methodinfo *class_findmethod_approx(classinfo *c, utf *name, utf *desc)
{
	s4 i;

	for (i = 0; i < c->methodscount; i++) {
		if (c->methods[i].name == name) {
			utf *meth_descr = c->methods[i].descriptor;
			
			if (desc == NULL) 
				/* ignore type */
				return &(c->methods[i]);

			if (desc->blength <= meth_descr->blength) {
				/* current position in utf text   */
				char *desc_utf_ptr = desc->text;      
				char *meth_utf_ptr = meth_descr->text;					  
				/* points behind utf strings */
				char *desc_end = UTF_END(desc);         
				char *meth_end = UTF_END(meth_descr);   
				char ch;

				/* compare argument types */
				while (desc_utf_ptr < desc_end && meth_utf_ptr < meth_end) {

					if ((ch = *desc_utf_ptr++) != (*meth_utf_ptr++))
						break; /* no match */

					if (ch == ')')
						return &(c->methods[i]); /* all parameter types equal */
				}
			}
		}
	}

	return NULL;
}


/* class_resolvemethod *********************************************************
	
   Searches a class and it's super classes for a method.

   Superinterfaces are *not* searched.

*******************************************************************************/

methodinfo *class_resolvemethod(classinfo *c, utf *name, utf *desc)
{
	methodinfo *m;

	while (c) {
		m = class_findmethod(c, name, desc);

		if (m)
			return m;

		/* JVM Specification bug: 

		   It is important NOT to resolve special <init> and <clinit>
		   methods to super classes or interfaces; yet, this is not
		   explicited in the specification.  Section 5.4.3.3 should be
		   updated appropriately.  */

		if (name == utf_init || name == utf_clinit)
			return NULL;

		c = c->super.cls;
	}

	return NULL;
}


/* class_resolveinterfacemethod_intern *****************************************

   Internally used helper function. Do not use this directly.

*******************************************************************************/

static methodinfo *class_resolveinterfacemethod_intern(classinfo *c,
													   utf *name, utf *desc)
{
	methodinfo *m;
	s4          i;
	
	m = class_findmethod(c, name, desc);

	if (m)
		return m;

	/* try the superinterfaces */

	for (i = 0; i < c->interfacescount; i++) {
		m = class_resolveinterfacemethod_intern(c->interfaces[i].cls,
												name, desc);

		if (m)
			return m;
	}
	
	return NULL;
}


/* class_resolveclassmethod ****************************************************
	
   Resolves a reference from REFERER to a method with NAME and DESC in
   class C.

   If the method cannot be resolved the return value is NULL. If
   EXCEPT is true *exceptionptr is set, too.

*******************************************************************************/

methodinfo *class_resolveclassmethod(classinfo *c, utf *name, utf *desc,
									 classinfo *referer, bool except)
{
	classinfo  *cls;
	methodinfo *m;
	s4          i;

	/* XXX resolve class c */
	/* XXX check access from REFERER to C */
	
/*  	if (c->flags & ACC_INTERFACE) { */
/*  		if (except) */
/*  			*exceptionptr = */
/*  				new_exception(string_java_lang_IncompatibleClassChangeError); */
/*  		return NULL; */
/*  	} */

	/* try class c and its superclasses */

	cls = c;

	m = class_resolvemethod(cls, name, desc);

	if (m)
		goto found;

	/* try the superinterfaces */

	for (i = 0; i < c->interfacescount; i++) {
		m = class_resolveinterfacemethod_intern(c->interfaces[i].cls,
												 name, desc);

		if (m)
			goto found;
	}
	
	if (except)
		*exceptionptr = exceptions_new_nosuchmethoderror(c, name, desc);

	return NULL;

 found:
	if ((m->flags & ACC_ABSTRACT) && !(c->flags & ACC_ABSTRACT)) {
		if (except)
			*exceptionptr = new_exception(string_java_lang_AbstractMethodError);

		return NULL;
	}

	/* XXX check access rights */

	return m;
}


/* class_resolveinterfacemethod ************************************************

   Resolves a reference from REFERER to a method with NAME and DESC in
   interface C.

   If the method cannot be resolved the return value is NULL. If
   EXCEPT is true *exceptionptr is set, too.

*******************************************************************************/

methodinfo *class_resolveinterfacemethod(classinfo *c, utf *name, utf *desc,
										 classinfo *referer, bool except)
{
	methodinfo *mi;

	/* XXX resolve class c */
	/* XXX check access from REFERER to C */
	
	if (!(c->flags & ACC_INTERFACE)) {
		if (except)
			*exceptionptr =
				new_exception(string_java_lang_IncompatibleClassChangeError);

		return NULL;
	}

	mi = class_resolveinterfacemethod_intern(c, name, desc);

	if (mi)
		return mi;

	/* try class java.lang.Object */

	mi = class_findmethod(class_java_lang_Object, name, desc);

	if (mi)
		return mi;

	if (except)
		*exceptionptr =
			exceptions_new_nosuchmethoderror(c, name, desc);

	return NULL;
}


/* class_findfield *************************************************************
	
   Searches for field with specified name and type in a classinfo
   structure. If no such field is found NULL is returned.

*******************************************************************************/

fieldinfo *class_findfield(classinfo *c, utf *name, utf *desc)
{
	s4 i;

	for (i = 0; i < c->fieldscount; i++)
		if ((c->fields[i].name == name) && (c->fields[i].descriptor == desc))
			return &(c->fields[i]);

	if (c->super.cls)
		return class_findfield(c->super.cls, name, desc);

	return NULL;
}


/* class_findfield_approx ******************************************************
	
   Searches in 'classinfo'-structure for a field with the specified
   name.

*******************************************************************************/
 
fieldinfo *class_findfield_by_name(classinfo *c, utf *name)
{
	s4 i;

	/* get field index */

	i = class_findfield_index_by_name(c, name);

	/* field was not found, return */

	if (i == -1)
		return NULL;

	/* return field address */

	return &(c->fields[i]);
}


s4 class_findfield_index_by_name(classinfo *c, utf *name)
{
	s4 i;

	for (i = 0; i < c->fieldscount; i++) {
		/* compare field names */

		if ((c->fields[i].name == name))
			return i;
	}

	/* field was not found, raise exception */	

	*exceptionptr = new_exception(string_java_lang_NoSuchFieldException);

	return -1;
}


/****************** Function: class_resolvefield_int ***************************

    This is an internally used helper function. Do not use this directly.

	Tries to resolve a field having the given name and type.
    If the field cannot be resolved, NULL is returned.

*******************************************************************************/

static fieldinfo *class_resolvefield_int(classinfo *c, utf *name, utf *desc)
{
	fieldinfo *fi;
	s4         i;

	/* search for field in class c */

	for (i = 0; i < c->fieldscount; i++) { 
		if ((c->fields[i].name == name) && (c->fields[i].descriptor == desc)) {
			return &(c->fields[i]);
		}
    }

	/* try superinterfaces recursively */

	for (i = 0; i < c->interfacescount; i++) {
		fi = class_resolvefield_int(c->interfaces[i].cls, name, desc);
		if (fi)
			return fi;
	}

	/* try superclass */

	if (c->super.cls)
		return class_resolvefield_int(c->super.cls, name, desc);

	/* not found */

	return NULL;
}


/********************* Function: class_resolvefield ***************************
	
	Resolves a reference from REFERER to a field with NAME and DESC in class C.

    If the field cannot be resolved the return value is NULL. If EXCEPT is
    true *exceptionptr is set, too.

*******************************************************************************/

fieldinfo *class_resolvefield(classinfo *c, utf *name, utf *desc,
							  classinfo *referer, bool except)
{
	fieldinfo *fi;

	/* XXX resolve class c */
	/* XXX check access from REFERER to C */
	
	fi = class_resolvefield_int(c, name, desc);

	if (!fi) {
		if (except)
			*exceptionptr =
				new_exception_utfmessage(string_java_lang_NoSuchFieldError,
										 name);

		return NULL;
	}

	/* XXX check access rights */

	return fi;
}


/* class_issubclass ************************************************************

   Checks if sub is a descendant of super.
	
*******************************************************************************/

bool class_issubclass(classinfo *sub, classinfo *super)
{
	for (;;) {
		if (!sub)
			return false;

		if (sub == super)
			return true;

		sub = sub->super.cls;
	}
}


void class_showconstanti(classinfo *c, int ii) 
{
	u4 i = ii;
	voidptr e;
		
	e = c->cpinfos [i];
	printf ("#%d:  ", (int) i);
	if (e) {
		switch (c->cptags [i]) {
		case CONSTANT_Class:
			printf("Classreference -> ");
			utf_display(((constant_classref*)e)->name);
			break;
				
		case CONSTANT_Fieldref:
			printf("Fieldref -> "); goto displayFMIi;
		case CONSTANT_Methodref:
			printf("Methodref -> "); goto displayFMIi;
		case CONSTANT_InterfaceMethodref:
			printf("InterfaceMethod -> "); goto displayFMIi;
		displayFMIi:
			{
				constant_FMIref *fmi = e;
				utf_display(fmi->classref->name);
				printf(".");
				utf_display(fmi->name);
				printf(" ");
				utf_display(fmi->descriptor);
			}
			break;

		case CONSTANT_String:
			printf("String -> ");
			utf_display(e);
			break;
		case CONSTANT_Integer:
			printf("Integer -> %d", (int) (((constant_integer*)e)->value));
			break;
		case CONSTANT_Float:
			printf("Float -> %f", ((constant_float*)e)->value);
			break;
		case CONSTANT_Double:
			printf("Double -> %f", ((constant_double*)e)->value);
			break;
		case CONSTANT_Long:
			{
				u8 v = ((constant_long*)e)->value;
#if U8_AVAILABLE
				printf("Long -> %ld", (long int) v);
#else
				printf("Long -> HI: %ld, LO: %ld\n", 
					    (long int) v.high, (long int) v.low);
#endif 
			}
			break;
		case CONSTANT_NameAndType:
			{ 
				constant_nameandtype *cnt = e;
				printf("NameAndType: ");
				utf_display(cnt->name);
				printf(" ");
				utf_display(cnt->descriptor);
			}
			break;
		case CONSTANT_Utf8:
			printf("Utf8 -> ");
			utf_display(e);
			break;
		default: 
			log_text("Invalid type of ConstantPool-Entry");
			assert(0);
		}
	}
	printf("\n");
}


void class_showconstantpool (classinfo *c) 
{
	u4 i;
	voidptr e;

	printf ("---- dump of constant pool ----\n");

	for (i=0; i<c->cpcount; i++) {
		printf ("#%d:  ", (int) i);
		
		e = c -> cpinfos [i];
		if (e) {
			
			switch (c -> cptags [i]) {
			case CONSTANT_Class:
				printf ("Classreference -> ");
				utf_display ( ((constant_classref*)e) -> name );
				break;
				
			case CONSTANT_Fieldref:
				printf ("Fieldref -> "); goto displayFMI;
			case CONSTANT_Methodref:
				printf ("Methodref -> "); goto displayFMI;
			case CONSTANT_InterfaceMethodref:
				printf ("InterfaceMethod -> "); goto displayFMI;
			displayFMI:
				{
					constant_FMIref *fmi = e;
					utf_display ( fmi->classref->name );
					printf (".");
					utf_display ( fmi->name);
					printf (" ");
					utf_display ( fmi->descriptor );
				}
				break;

			case CONSTANT_String:
				printf ("String -> ");
				utf_display (e);
				break;
			case CONSTANT_Integer:
				printf ("Integer -> %d", (int) ( ((constant_integer*)e) -> value) );
				break;
			case CONSTANT_Float:
				printf ("Float -> %f", ((constant_float*)e) -> value);
				break;
			case CONSTANT_Double:
				printf ("Double -> %f", ((constant_double*)e) -> value);
				break;
			case CONSTANT_Long:
				{
					u8 v = ((constant_long*)e) -> value;
#if U8_AVAILABLE
					printf ("Long -> %ld", (long int) v);
#else
					printf ("Long -> HI: %ld, LO: %ld\n", 
							(long int) v.high, (long int) v.low);
#endif 
				}
				break;
			case CONSTANT_NameAndType:
				{
					constant_nameandtype *cnt = e;
					printf ("NameAndType: ");
					utf_display (cnt->name);
					printf (" ");
					utf_display (cnt->descriptor);
				}
				break;
			case CONSTANT_Utf8:
				printf ("Utf8 -> ");
				utf_display (e);
				break;
			default: 
				log_text("Invalid type of ConstantPool-Entry");
				assert(0);
			}
		}

		printf ("\n");
	}
}



/********** Function: class_showmethods   (debugging only) *************/

void class_showmethods (classinfo *c)
{
	s4 i;
	
	printf ("--------- Fields and Methods ----------------\n");
	printf ("Flags: ");	printflags (c->flags);	printf ("\n");

	printf ("This: "); utf_display (c->name); printf ("\n");
	if (c->super.cls) {
		printf ("Super: "); utf_display (c->super.cls->name); printf ("\n");
		}
	printf ("Index: %d\n", c->index);
	
	printf ("interfaces:\n");	
	for (i=0; i < c-> interfacescount; i++) {
		printf ("   ");
		utf_display (c -> interfaces[i].cls -> name);
		printf (" (%d)\n", c->interfaces[i].cls -> index);
		}

	printf ("fields:\n");		
	for (i=0; i < c -> fieldscount; i++) {
		field_display (&(c -> fields[i]));
		}

	printf ("methods:\n");
	for (i=0; i < c -> methodscount; i++) {
		methodinfo *m = &(c->methods[i]);
		if ( !(m->flags & ACC_STATIC)) 
			printf ("vftblindex: %d   ", m->vftblindex);

		method_display ( m );

		}

	printf ("Virtual function table:\n");
	for (i=0; i<c->vftbl->vftbllength; i++) {
		printf ("entry: %d,  %ld\n", i, (long int) (c->vftbl->table[i]) );
		}

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
