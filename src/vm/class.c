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

   $Id: class.c 2122 2005-03-29 22:12:06Z twisti $

*/

#include <assert.h>
#include <string.h>

#include "config.h"
#include "types.h"

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

hashtable class_hash;                   /* hashtable for classes              */

list unlinkedclasses;                   /* this is only used for eager class  */
                                        /* loading                            */


/* frequently used classes ****************************************************/

/* important system classes */

classinfo *class_java_lang_Object;
classinfo *class_java_lang_Class;
classinfo *class_java_lang_ClassLoader;
classinfo *class_java_lang_Cloneable;
classinfo *class_java_lang_SecurityManager;
classinfo *class_java_lang_String;
classinfo *class_java_lang_System;
classinfo *class_java_io_Serializable;


/* system exception classes required in cacao */

classinfo *class_java_lang_Throwable;
classinfo *class_java_lang_VMThrowable;
classinfo *class_java_lang_Exception;
classinfo *class_java_lang_Error;
classinfo *class_java_lang_OutOfMemoryError;


classinfo *class_java_lang_Void;
classinfo *class_java_lang_Boolean;
classinfo *class_java_lang_Byte;
classinfo *class_java_lang_Character;
classinfo *class_java_lang_Short;
classinfo *class_java_lang_Integer;
classinfo *class_java_lang_Long;
classinfo *class_java_lang_Float;
classinfo *class_java_lang_Double;

/* some classes which may be used more often */

classinfo *class_java_util_Vector;


/* pseudo classes for the typechecker */

classinfo *pseudo_class_Arraystub;
classinfo *pseudo_class_Null;
classinfo *pseudo_class_New;


/* class_init ******************************************************************

   Initialize the class subsystem.

*******************************************************************************/

void class_init_foo(void)
{
	class_java_lang_Object          = class_new_intern(utf_java_lang_Object);

	class_java_lang_Class           = class_new(utf_java_lang_Class);
	class_java_lang_ClassLoader     = class_new(utf_java_lang_ClassLoader);
	class_java_lang_Cloneable       = class_new(utf_java_lang_Cloneable);
	class_java_lang_SecurityManager = class_new(utf_java_lang_SecurityManager);
	class_java_lang_String          = class_new(utf_java_lang_String);
	class_java_lang_System          = class_new(utf_java_lang_System);
	class_java_io_Serializable      = class_new(utf_java_io_Serializable);

	class_java_lang_Throwable       = class_new(utf_java_lang_Throwable);
	class_java_lang_VMThrowable     = class_new(utf_java_lang_VMThrowable);
	class_java_lang_Exception       = class_new(utf_java_lang_Exception);
	class_java_lang_Error           = class_new(utf_java_lang_Error);
	class_java_lang_OutOfMemoryError =
		class_new(utf_java_lang_OutOfMemoryError);

	class_java_lang_Void            = class_new(utf_java_lang_Void);
	class_java_lang_Boolean         = class_new(utf_java_lang_Boolean);
	class_java_lang_Byte            = class_new(utf_java_lang_Byte);
	class_java_lang_Character       = class_new(utf_java_lang_Character);
	class_java_lang_Short           = class_new(utf_java_lang_Short);
	class_java_lang_Integer         = class_new(utf_java_lang_Integer);
	class_java_lang_Long            = class_new(utf_java_lang_Long);
	class_java_lang_Float           = class_new(utf_java_lang_Float);
	class_java_lang_Double          = class_new(utf_java_lang_Double);

	class_java_util_Vector          = class_new(utf_java_util_Vector);

    pseudo_class_Arraystub = class_new_intern(utf_new_char("$ARRAYSTUB$"));
	pseudo_class_Null      = class_new_intern(utf_new_char("$NULL$"));
	pseudo_class_New       = class_new_intern(utf_new_char("$NEW$"));
}


/* class_new *******************************************************************

   Searches for the class with the specified name in the classes
   hashtable, if there is no such class a new classinfo structure is
   created and inserted into the list of classes to be loaded.

*******************************************************************************/

classinfo *class_new(utf *classname)
{
    classinfo *c;

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
    tables_lock();
#endif

    c = class_new_intern(classname);

	/* we support eager class loading and linking on demand */

	if (opt_eager) {
		classinfo *tc;
		classinfo *tmp;

		list_init(&unlinkedclasses, OFFSET(classinfo, listnode));

		if (!c->loaded) {
			if (!class_load(c)) {
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
				tables_unlock();
#endif
				return c;
			}
		}

		/* link all referenced classes */

		tc = list_first(&unlinkedclasses);

		while (tc) {
			/* skip the current loaded/linked class */
			if (tc != c) {
				if (!class_link(tc)) {
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
					tables_unlock();
#endif
					return c;
				}
			}

			/* we need a tmp variable here, because list_remove sets prev and
			   next to NULL */
			tmp = list_next(&unlinkedclasses, tc);
   			list_remove(&unlinkedclasses, tc);
			tc = tmp;
		}

		if (!c->linked) {
			if (!class_link(c)) {
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
				tables_unlock();
#endif
				return c;
			}
		}
	}

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
    tables_unlock();
#endif

    return c;
}


classinfo *class_new_intern(utf *classname)
{
	classinfo *c;     /* hashtable element */
	u4 key;           /* hashkey computed from classname */
	u4 slot;          /* slot in hashtable */
	u2 i;

	key  = utf_hashkey(classname->text, classname->blength);
	slot = key & (class_hash.size - 1);
	c    = class_hash.ptr[slot];

	/* search external hash chain for the class */
	while (c) {
		if (c->name->blength == classname->blength) {
			for (i = 0; i < classname->blength; i++)
				if (classname->text[i] != c->name->text[i]) goto nomatch;
						
			/* class found in hashtable */
			return c;
		}
			
	nomatch:
		c = c->hashlink; /* next element in external chain */
	}

	/* location in hashtable found, create new classinfo structure */

#if defined(STATISTICS)
	if (opt_stat)
		count_class_infos += sizeof(classinfo);
#endif

	if (initverbose) {
		char logtext[MAXLOGTEXT];
		sprintf(logtext, "Creating class: ");
		utf_sprint_classname(logtext + strlen(logtext), classname);
		log_text(logtext);
	}

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
	c->super = NULL;
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
	
	/* insert class into the hashtable */
	c->hashlink = class_hash.ptr[slot];
	class_hash.ptr[slot] = c;

	/* update number of hashtable-entries */
	class_hash.entries++;

	if (class_hash.entries > (class_hash.size * 2)) {

		/* reorganization of hashtable, average length of 
		   the external chains is approx. 2                */  

		u4 i;
		classinfo *c;
		hashtable newhash;  /* the new hashtable */

		/* create new hashtable, double the size */
		init_hashtable(&newhash, class_hash.size * 2);
		newhash.entries = class_hash.entries;

		/* transfer elements to new hashtable */
		for (i = 0; i < class_hash.size; i++) {
			c = (classinfo *) class_hash.ptr[i];
			while (c) {
				classinfo *nextc = c->hashlink;
				u4 slot = (utf_hashkey(c->name->text, c->name->blength)) & (newhash.size - 1);
						
				c->hashlink = newhash.ptr[slot];
				newhash.ptr[slot] = c;

				c = nextc;
			}
		}
	
		/* dispose old table */	
		MFREE(class_hash.ptr, void*, class_hash.size);
		class_hash = newhash;
	}

	/* Array classes need further initialization. */
	if (c->name->text[0] == '[') {
		/* Array classes are not loaded from classfiles. */
		c->loaded = true;
		class_new_array(c);
		c->packagename = array_packagename;

	} else {
		/* Find the package name */
		/* Classes in the unnamed package keep packagename == NULL. */
		char *p = utf_end(c->name) - 1;
		char *start = c->name->text;
		for (;p > start; --p) {
			if (*p == '/') {
				c->packagename = utf_new (start, p - start);
				break;
			}
		}
	}

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	initObjectLock(&c->header);
#endif

	return c;
}


/* class_get *******************************************************************

   Searches for the class with the specified name in the classes
   hashtable if there is no such class NULL is returned.

*******************************************************************************/

classinfo *class_get(utf *classname)
{
	classinfo *c;  /* hashtable element */ 
	u4 key;        /* hashkey computed from classname */   
	u4 slot;       /* slot in hashtable */
	u2 i;  

	key  = utf_hashkey(classname->text, classname->blength);
	slot = key & (class_hash.size-1);
	c    = class_hash.ptr[slot];

	/* search external hash-chain */
	while (c) {
		if (c->name->blength == classname->blength) {
			/* compare classnames */
			for (i = 0; i < classname->blength; i++) 
				if (classname->text[i] != c->name->text[i])
					goto nomatch;

			/* class found in hashtable */				
			return c;
		}
			
	nomatch:
		c = c->hashlink;
	}

	/* class not found */
	return NULL;
}


/* class_remove ****************************************************************

   Removes the class entry wth the specified name in the classes
   hashtable, furthermore the class' resources are freed if there is
   no such class false is returned.

*******************************************************************************/

bool class_remove(classinfo *c)
{
	classinfo *tc;                      /* hashtable element                  */
	classinfo *pc;
	u4 key;                             /* hashkey computed from classname    */
	u4 slot;                            /* slot in hashtable                  */
	u2 i;  

	key  = utf_hashkey(c->name->text, c->name->blength);
	slot = key & (class_hash.size - 1);
	tc   = class_hash.ptr[slot];
	pc   = NULL;

	/* search external hash-chain */
	while (tc) {
		if (tc->name->blength == c->name->blength) {
			
			/* compare classnames */
			for (i = 0; i < c->name->blength; i++)
				if (tc->name->text[i] != c->name->text[i])
					goto nomatch;

			/* class found in hashtable */
			if (!pc)
				class_hash.ptr[slot] = tc->hashlink;
			else
				pc->hashlink = tc->hashlink;

			class_free(tc);

			return true;
		}
			
	nomatch:
		pc = tc;
		tc = tc->hashlink;
	}

	/* class not found */
	return false;
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


/* class_array_of **************************************************************

   Returns an array class with the given component class. The array
   class is dynamically created if neccessary.

*******************************************************************************/

classinfo *class_array_of(classinfo *component)
{
    s4 namelen;
    char *namebuf;
	classinfo *c;

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

	c = class_new(utf_new(namebuf, namelen));

	/* load this class ;-) and link it */

	if (!c->loaded)
		c->loaded = true;

	if (!c->linked)
		if (!class_link(c))
			return NULL;

    return c;
}


/* class_multiarray_of *********************************************************

   Returns an array class with the given dimension and element class.
   The array class is dynamically created if neccessary.

*******************************************************************************/

classinfo *class_multiarray_of(s4 dim, classinfo *element)
{
    s4 namelen;
    char *namebuf;

	if (dim < 1)
		panic("Invalid array dimension requested");

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

    return class_new(utf_new(namebuf, namelen));
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

constant_classref *class_lookup_classref(classinfo *cls,utf *name)
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
	for (xref=cls->extclassrefs; xref; xref=xref->next) {
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

constant_classref *class_get_classref(classinfo *cls,utf *name)
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

constant_classref *class_get_classref_multiarray_of(s4 dim,constant_classref *ref)
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

    return class_get_classref(ref->referer,utf_new(name, namelen));
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
