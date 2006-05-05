/* src/vm/builtin.c - functions for unsupported operations

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
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

   Contact: cacao@cacaojvm.org

   Authors: Reinhard Grafl
            Andreas Krall
            Mark Probst

   Changes: Christian Thalinger
            Edwin Steiner

   Contains C functions for JavaVM Instructions that cannot be
   translated to machine language directly. Consequently, the
   generated machine code for these instructions contains function
   calls instead of machine instructions, using the C calling
   convention.

   $Id: builtin.c 4872 2006-05-05 13:48:25Z edwin $

*/


#include "config.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "vm/types.h"

#include "arch.h"
#include "md-abi.h"

#include "fdlibm/fdlibm.h"

#include "mm/boehm.h"
#include "mm/memory.h"
#include "native/native.h"
#include "native/include/java_lang_Cloneable.h"
#include "native/include/java_lang_Object.h"          /* required by VMObject */
#include "native/include/java_lang_VMObject.h"

#if defined(USE_THREADS)
# if defined(NATIVE_THREADS)
#  include "threads/native/threads.h"
# else
#  include "threads/green/threads.h"
#  include "threads/green/locks.h"
# endif
#endif

#include "toolbox/logging.h"
#include "toolbox/util.h"
#include "vm/builtin.h"
#include "vm/class.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/initialize.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/patcher.h"
#include "vm/rt-timing.h"
#include "vm/cycles-stats.h"


/* include builtin tables *****************************************************/

#include "vm/builtintable.inc"


CYCLES_STATS_DECLARE(builtin_monitorenter,100,2)
CYCLES_STATS_DECLARE(builtin_monitorexit ,100,2)
CYCLES_STATS_DECLARE(builtin_new         ,100,5)
CYCLES_STATS_DECLARE(builtin_overhead    , 80,1)

/* builtintable_init ***********************************************************

   Parse the descriptors of builtin functions and create the parsed
   descriptors.

*******************************************************************************/

static bool builtintable_init(void)
{
	descriptor_pool *descpool;
	s4               dumpsize;
	utf             *descriptor;
	s4               entries_internal;
	s4               entries_automatic;
	s4               i;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* create a new descriptor pool */

	descpool = descriptor_pool_new(class_java_lang_Object);

	/* add some entries we need */

	if (!descriptor_pool_add_class(descpool, utf_java_lang_Object))
		return false;

	if (!descriptor_pool_add_class(descpool, utf_java_lang_Class))
		return false;

	/* calculate table entries statically */

	entries_internal =
		sizeof(builtintable_internal) / sizeof(builtintable_entry);

	entries_automatic =
		sizeof(builtintable_automatic) / sizeof(builtintable_entry)
		- 1; /* last filler entry (comment see builtintable.inc) */

	/* first add all descriptors to the pool */

	for (i = 0; i < entries_internal; i++) {
		/* create a utf8 string from descriptor */

		descriptor = utf_new_char(builtintable_internal[i].descriptor);

		if (!descriptor_pool_add(descpool, descriptor, NULL)) {
			/* release dump area */

			dump_release(dumpsize);

			return false;
		}
	}

	for (i = 0; i < entries_automatic; i++) {
		/* create a utf8 string from descriptor */

		descriptor = utf_new_char(builtintable_automatic[i].descriptor);

		if (!descriptor_pool_add(descpool, descriptor, NULL)) {
			/* release dump area */

			dump_release(dumpsize);

			return false;
		}
	}

	/* create the class reference table */

	(void) descriptor_pool_create_classrefs(descpool, NULL);

	/* allocate space for the parsed descriptors */

	descriptor_pool_alloc_parsed_descriptors(descpool);

	/* now parse all descriptors */

	for (i = 0; i < entries_internal; i++) {
		/* create a utf8 string from descriptor */

		descriptor = utf_new_char(builtintable_internal[i].descriptor);

		/* parse the descriptor, builtin is always static (no `this' pointer) */

		builtintable_internal[i].md =
			descriptor_pool_parse_method_descriptor(descpool, descriptor,
													ACC_STATIC, NULL);
	}

	for (i = 0; i < entries_automatic; i++) {
		/* create a utf8 string from descriptor */

		descriptor = utf_new_char(builtintable_automatic[i].descriptor);

		/* parse the descriptor, builtin is always static (no `this' pointer) */

		builtintable_automatic[i].md =
			descriptor_pool_parse_method_descriptor(descpool, descriptor,
													ACC_STATIC, NULL);
	}

	/* release dump area */

	dump_release(dumpsize);

	return true;
}


/* builtintable_comparator *****************************************************

   qsort comparator for the automatic builtin table.

*******************************************************************************/

static int builtintable_comparator(const void *a, const void *b)
{
	builtintable_entry *bte1;
	builtintable_entry *bte2;

	bte1 = (builtintable_entry *) a;
	bte2 = (builtintable_entry *) b;

	return (bte1->opcode < bte2->opcode) ? -1 : (bte1->opcode > bte2->opcode);
}


/* builtintable_sort_automatic *************************************************

   Sorts the automatic builtin table.

*******************************************************************************/

static void builtintable_sort_automatic(void)
{
	s4 entries;

	/* calculate table size statically (`- 1' comment see builtintable.inc) */

	entries = sizeof(builtintable_automatic) / sizeof(builtintable_entry) - 1;

	qsort(builtintable_automatic, entries, sizeof(builtintable_entry),
		  builtintable_comparator);
}


/* builtin_init ****************************************************************

   XXX

*******************************************************************************/

bool builtin_init(void)
{
	/* initialize the builtin tables */

	if (!builtintable_init())
		return false;

	/* sort builtin tables */

	builtintable_sort_automatic();

	return true;
}


/* builtintable_get_internal ***************************************************

   Finds an entry in the builtintable for internal functions and
   returns the a pointer to the structure.

*******************************************************************************/

builtintable_entry *builtintable_get_internal(functionptr fp)
{
	s4 i;

	for (i = 0; builtintable_internal[i].fp != NULL; i++) {
		if (builtintable_internal[i].fp == fp)
			return &builtintable_internal[i];
	}

	return NULL;
}


/* builtintable_get_automatic **************************************************

   Finds an entry in the builtintable for functions which are replaced
   automatically and returns the a pointer to the structure.

*******************************************************************************/

builtintable_entry *builtintable_get_automatic(s4 opcode)
{
	builtintable_entry *first;
	builtintable_entry *last;
	builtintable_entry *middle;
	s4                  half;
	s4                  entries;

	/* calculate table size statically (`- 1' comment see builtintable.inc) */

	entries = sizeof(builtintable_automatic) / sizeof(builtintable_entry) - 1;

	first = builtintable_automatic;
	last = builtintable_automatic + entries;

	while (entries > 0) {
		half = entries / 2;
		middle = first + half;

		if (middle->opcode < opcode) {
			first = middle + 1;
			entries -= half + 1;
		} else
			entries = half;
	}

	return (first != last ? first : NULL);
}


/*****************************************************************************
								TYPE CHECKS
*****************************************************************************/



/*************** internal function: builtin_isanysubclass *********************

	Checks a subclass relation between two classes. Implemented interfaces
	are interpreted as super classes.
	Return value:  1 ... sub is subclass of super
				   0 ... otherwise
					
*****************************************************************************/					
s4 builtin_isanysubclass(classinfo *sub, classinfo *super)
{
	s4 res;
	castinfo classvalues;

	if (sub == super)
		return 1;

	if (super->flags & ACC_INTERFACE) {
		res = (sub->vftbl->interfacetablelength > super->index) &&
			(sub->vftbl->interfacetable[-super->index] != NULL);

	} else {
		ASM_GETCLASSVALUES_ATOMIC(super->vftbl, sub->vftbl, &classvalues);

		res = (u4) (classvalues.sub_baseval - classvalues.super_baseval) <=
			(u4) classvalues.super_diffval;
	}

	return res;
}


s4 builtin_isanysubclass_vftbl(vftbl_t *sub, vftbl_t *super)
{
	s4 res;
	s4 base;
	castinfo classvalues;

	if (sub == super)
		return 1;

	ASM_GETCLASSVALUES_ATOMIC(super, sub, &classvalues);

	if ((base = classvalues.super_baseval) <= 0) {
		/* super is an interface */
		res = (sub->interfacetablelength > -base) &&
			(sub->interfacetable[base] != NULL);
	} else {
	    res = (u4) (classvalues.sub_baseval - classvalues.super_baseval)
			<= (u4) classvalues.super_diffval;
	}

	return res;
}


/****************** function: builtin_instanceof *****************************

	Checks if an object is an instance of some given class (or subclass of
	that class). If class is an interface, checks if the interface is
	implemented.
	Return value:  1 ... obj is an instance of class or implements the interface
				   0 ... otherwise or if obj == NULL
			 
*****************************************************************************/

s4 builtin_instanceof(java_objectheader *obj, classinfo *class)
{
	if (!obj)
		return 0;

	return builtin_isanysubclass(obj->vftbl->class, class);
}



/**************** function: builtin_checkcast *******************************

	The same as builtin_instanceof except that 1 is returned when
	obj == NULL
			  
****************************************************************************/

s4 builtin_checkcast(java_objectheader *obj, classinfo *class)
{
	if (obj == NULL)
		return 1;

	if (builtin_isanysubclass(obj->vftbl->class, class))
		return 1;

	return 0;
}


/* builtin_descriptorscompatible ***********************************************

   Checks if two array type descriptors are assignment compatible

   Return value: 1 ... target = desc is possible
                 0 ... otherwise
			
*******************************************************************************/

static s4 builtin_descriptorscompatible(arraydescriptor *desc,
										arraydescriptor *target)
{
	if (desc == target)
		return 1;

	if (desc->arraytype != target->arraytype)
		return 0;

	if (desc->arraytype != ARRAYTYPE_OBJECT)
		return 1;
	
	/* {both arrays are arrays of references} */

	if (desc->dimension == target->dimension) {
		/* an array which contains elements of interface types is
           allowed to be casted to Object (JOWENN)*/

		if ((desc->elementvftbl->baseval < 0) &&
			(target->elementvftbl->baseval == 1))
			return 1;

		return builtin_isanysubclass_vftbl(desc->elementvftbl,
										   target->elementvftbl);
	}

	if (desc->dimension < target->dimension)
		return 0;

	/* {desc has higher dimension than target} */

	return builtin_isanysubclass_vftbl(pseudo_class_Arraystub->vftbl,
									   target->elementvftbl);
}


/* builtin_arraycheckcast ******************************************************

   Checks if an object is really a subtype of the requested array
   type.  The object has to be an array to begin with. For simple
   arrays (int, short, double, etc.) the types have to match exactly.
   For arrays of objects, the type of elements in the array has to be
   a subtype (or the same type) of the requested element type. For
   arrays of arrays (which in turn can again be arrays of arrays), the
   types at the lowest level have to satisfy the corresponding sub
   class relation.
	
*******************************************************************************/

s4 builtin_arraycheckcast(java_objectheader *o, classinfo *targetclass)
{
	arraydescriptor *desc;

	if (!o)
		return 1;

	if ((desc = o->vftbl->arraydesc) == NULL)
		return 0;
 
	return builtin_descriptorscompatible(desc, targetclass->vftbl->arraydesc);
}


s4 builtin_arrayinstanceof(java_objectheader *o, classinfo *targetclass)
{
	if (!o)
		return 0;

	return builtin_arraycheckcast(o, targetclass);
}


/* builtin_throw_exception *****************************************************

   Sets the exceptionptr with the thrown exception and prints some
   debugging information.  Called from asm_vm_call_method.

*******************************************************************************/

void *builtin_throw_exception(java_objectheader *xptr)
{
#if !defined(NDEBUG)
    java_lang_Throwable *t;
	char                *logtext;
	s4                   logtextlen;
	s4                   dumpsize;

	if (opt_verbose) {
		t = (java_lang_Throwable *) xptr;

		/* calculate message length */

		logtextlen = strlen("Builtin exception thrown: ") + strlen("0");

		if (t) {
			logtextlen +=
				utf_get_number_of_u2s(xptr->vftbl->class->name) +
				strlen(": ") +
				javastring_strlen(t->detailMessage);

		} else
			logtextlen += strlen("(nil)");

		/* allocate memory */

		dumpsize = dump_size();

		logtext = DMNEW(char, logtextlen);

		strcpy(logtext, "Builtin exception thrown: ");

		if (t) {
			utf_sprint_classname(logtext + strlen(logtext),
								 xptr->vftbl->class->name);

			if (t->detailMessage) {
				char *buf;

				buf = javastring_tochar((java_objectheader *) t->detailMessage);
				strcat(logtext, ": ");
				strcat(logtext, buf);
				MFREE(buf, char, strlen(buf));
			}

		} else {
			strcat(logtext, "(nil)");
		}

		log_text(logtext);

		/* release memory */

		dump_release(dumpsize);
	}
#endif /* !defined(NDEBUG) */

	/* actually set the exception */

	*exceptionptr = xptr;

	/* Return a NULL pointer.  This is required for vm_call_method to
	   check for an exception.  This is for convenience. */

	return NULL;
}


/* builtin_canstore ************************************************************

   Checks, if an object can be stored in an array.

   Return value: 1 ... possible
                 0 ... otherwise

*******************************************************************************/

s4 builtin_canstore(java_objectarray *oa, java_objectheader *o)
{
	arraydescriptor *desc;
	arraydescriptor *valuedesc;
	vftbl_t         *componentvftbl;
	vftbl_t         *valuevftbl;
	s4               base;
	castinfo         classvalues;

	if (!o)
		return 1;

	/* The following is guaranteed (by verifier checks):
	 *
	 *     *) oa->...vftbl->arraydesc != NULL
	 *     *) oa->...vftbl->arraydesc->componentvftbl != NULL
	 *     *) o->vftbl is not an interface vftbl
	 */
	
	desc           = oa->header.objheader.vftbl->arraydesc;
	componentvftbl = desc->componentvftbl;
	valuevftbl     = o->vftbl;

	if ((desc->dimension - 1) == 0) {
		s4 res;

		/* {oa is a one-dimensional array} */
		/* {oa is an array of references} */
		
		if (valuevftbl == componentvftbl)
			return 1;

		ASM_GETCLASSVALUES_ATOMIC(componentvftbl, valuevftbl, &classvalues);

		if ((base = classvalues.super_baseval) <= 0)
			/* an array of interface references */
			return (valuevftbl->interfacetablelength > -base &&
					valuevftbl->interfacetable[base] != NULL);
		
		res = ((unsigned) (classvalues.sub_baseval - classvalues.super_baseval)
			   <= (unsigned) classvalues.super_diffval);

		return res;
	}

	/* {oa has dimension > 1} */
	/* {componentvftbl->arraydesc != NULL} */

	/* check if o is an array */

	if ((valuedesc = valuevftbl->arraydesc) == NULL)
		return 0;

	/* {o is an array} */

	return builtin_descriptorscompatible(valuedesc, componentvftbl->arraydesc);
}


/* This is an optimized version where a is guaranteed to be one-dimensional */
s4 builtin_canstore_onedim (java_objectarray *a, java_objectheader *o)
{
	arraydescriptor *desc;
	vftbl_t *elementvftbl;
	vftbl_t *valuevftbl;
	s4 res;
	int base;
	castinfo classvalues;
	
	if (!o) return 1;

	/* The following is guaranteed (by verifier checks):
	 *
	 *     *) a->...vftbl->arraydesc != NULL
	 *     *) a->...vftbl->arraydesc->elementvftbl != NULL
	 *     *) a->...vftbl->arraydesc->dimension == 1
	 *     *) o->vftbl is not an interface vftbl
	 */

	desc = a->header.objheader.vftbl->arraydesc;
    elementvftbl = desc->elementvftbl;
	valuevftbl = o->vftbl;

	/* {a is a one-dimensional array} */
	
	if (valuevftbl == elementvftbl)
		return 1;

	ASM_GETCLASSVALUES_ATOMIC(elementvftbl, valuevftbl, &classvalues);

	if ((base = classvalues.super_baseval) <= 0)
		/* an array of interface references */
		return (valuevftbl->interfacetablelength > -base &&
				valuevftbl->interfacetable[base] != NULL);

	res = (unsigned) (classvalues.sub_baseval - classvalues.super_baseval)
		<= (unsigned) classvalues.super_diffval;

	return res;
}


/* This is an optimized version where a is guaranteed to be a
 * one-dimensional array of a class type */
s4 builtin_canstore_onedim_class(java_objectarray *a, java_objectheader *o)
{
	vftbl_t *elementvftbl;
	vftbl_t *valuevftbl;
	s4 res;
	castinfo classvalues;
	
	if (!o) return 1;

	/* The following is guaranteed (by verifier checks):
	 *
	 *     *) a->...vftbl->arraydesc != NULL
	 *     *) a->...vftbl->arraydesc->elementvftbl != NULL
	 *     *) a->...vftbl->arraydesc->elementvftbl is not an interface vftbl
	 *     *) a->...vftbl->arraydesc->dimension == 1
	 *     *) o->vftbl is not an interface vftbl
	 */

    elementvftbl = a->header.objheader.vftbl->arraydesc->elementvftbl;
	valuevftbl = o->vftbl;

	/* {a is a one-dimensional array} */
	
	if (valuevftbl == elementvftbl)
		return 1;

	ASM_GETCLASSVALUES_ATOMIC(elementvftbl, valuevftbl, &classvalues);

	res = (unsigned) (classvalues.sub_baseval - classvalues.super_baseval)
		<= (unsigned) classvalues.super_diffval;

	return res;
}


/* builtin_new *****************************************************************

   Creates a new instance of class c on the heap.

   Return value: pointer to the object or NULL if no memory is
   available
			
*******************************************************************************/

java_objectheader *builtin_new(classinfo *c)
{
	java_objectheader *o;
#if defined(ENABLE_RT_TIMING)
	struct timespec time_start, time_end;
#endif
#if defined(ENABLE_CYCLES_STATS)
	u8 cycles_start, cycles_end;
#endif

	RT_TIMING_GET_TIME(time_start);
	CYCLES_STATS_GET(cycles_start);

	/* is the class loaded */

	assert(c->state & CLASS_LOADED);

	/* check if we can instantiate this class */

	if (c->flags & ACC_ABSTRACT) {
		*exceptionptr =
			new_exception_utfmessage(string_java_lang_InstantiationError,
									 c->name);
		return NULL;
	}

	/* is the class linked */

	if (!(c->state & CLASS_LINKED))
		if (!link_class(c))
			return NULL;

	if (!(c->state & CLASS_INITIALIZED)) {
#if !defined(NDEBUG)
		if (initverbose)
			log_message_class("Initialize class (from builtin_new): ", c);
#endif

		if (!initialize_class(c))
			return NULL;
	}

	o = heap_allocate(c->instancesize, c->flags & ACC_CLASS_HAS_POINTERS,
					  c->finalizer);

	if (!o)
		return NULL;

	o->vftbl = c->vftbl;

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	initObjectLock(o);
#endif

	CYCLES_STATS_GET(cycles_end);
	RT_TIMING_GET_TIME(time_end);

	CYCLES_STATS_COUNT(builtin_new,cycles_end - cycles_start);
	RT_TIMING_TIME_DIFF(time_start, time_end, RT_TIMING_NEW_OBJECT);

	return o;
}


/* builtin_newarray ************************************************************

   Creates an array with the given vftbl on the heap. This function
   takes as class argument an array class.

   Return value: pointer to the array or NULL if no memory is available

*******************************************************************************/

java_arrayheader *builtin_newarray(s4 size, classinfo *arrayclass)
{
	arraydescriptor  *desc;
	s4                dataoffset;
	s4                componentsize;
	s4                actualsize;
	java_arrayheader *a;
#if defined(ENABLE_RT_TIMING)
	struct timespec time_start, time_end;
#endif

	RT_TIMING_GET_TIME(time_start);

	desc          = arrayclass->vftbl->arraydesc;
	dataoffset    = desc->dataoffset;
	componentsize = desc->componentsize;

	if (size < 0) {
		exceptions_throw_negativearraysizeexception();
		return NULL;
	}

	actualsize = dataoffset + size * componentsize;

	if (((u4) actualsize) < ((u4) size)) { /* overflow */
		*exceptionptr = new_exception(string_java_lang_OutOfMemoryError);
		return NULL;
	}

	a = heap_allocate(actualsize, (desc->arraytype == ARRAYTYPE_OBJECT), NULL);

	if (!a)
		return NULL;

	a->objheader.vftbl = arrayclass->vftbl;

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	initObjectLock(&a->objheader);
#endif

	a->size = size;

	RT_TIMING_GET_TIME(time_end);
	RT_TIMING_TIME_DIFF(time_start, time_end, RT_TIMING_NEW_ARRAY);

	return a;
}


/* builtin_anewarray ***********************************************************

   Creates an array of references to the given class type on the heap.

   Return value: pointer to the array or NULL if no memory is
   available

*******************************************************************************/

java_objectarray *builtin_anewarray(s4 size, classinfo *componentclass)
{
	classinfo *arrayclass;
	
	/* is class loaded */

	assert(componentclass->state & CLASS_LOADED);

	/* is class linked */

	if (!(componentclass->state & CLASS_LINKED))
		if (!link_class(componentclass))
			return NULL;

	arrayclass = class_array_of(componentclass, true);

	if (!arrayclass)
		return NULL;

	return (java_objectarray *) builtin_newarray(size, arrayclass);
}


/* builtin_newarray_boolean ****************************************************

   Creates an array of bytes on the heap. The array is designated as
   an array of booleans (important for casts)
	
   Return value: pointer to the array or NULL if no memory is
   available

*******************************************************************************/

java_booleanarray *builtin_newarray_boolean(s4 size)
{
	return (java_booleanarray *)
		builtin_newarray(size,
						 primitivetype_table[ARRAYTYPE_BOOLEAN].arrayclass);
}


/* builtin_newarray_byte *******************************************************

   Creates an array of 8 bit Integers on the heap.

   Return value: pointer to the array or NULL if no memory is
   available

*******************************************************************************/

java_bytearray *builtin_newarray_byte(s4 size)
{
	return (java_bytearray *)
		builtin_newarray(size, primitivetype_table[ARRAYTYPE_BYTE].arrayclass);
}


/* builtin_newarray_char *******************************************************

   Creates an array of characters on the heap.

   Return value: pointer to the array or NULL if no memory is
   available

*******************************************************************************/

java_chararray *builtin_newarray_char(s4 size)
{
	return (java_chararray *)
		builtin_newarray(size, primitivetype_table[ARRAYTYPE_CHAR].arrayclass);
}


/* builtin_newarray_short ******************************************************

   Creates an array of 16 bit Integers on the heap.

   Return value: pointer to the array or NULL if no memory is
   available

*******************************************************************************/

java_shortarray *builtin_newarray_short(s4 size)
{
	return (java_shortarray *)
		builtin_newarray(size, primitivetype_table[ARRAYTYPE_SHORT].arrayclass);
}


/* builtin_newarray_int ********************************************************

   Creates an array of 32 bit Integers on the heap.

   Return value: pointer to the array or NULL if no memory is
   available

*******************************************************************************/

java_intarray *builtin_newarray_int(s4 size)
{
	return (java_intarray *)
		builtin_newarray(size, primitivetype_table[ARRAYTYPE_INT].arrayclass);
}


/* builtin_newarray_long *******************************************************

   Creates an array of 64 bit Integers on the heap.

   Return value: pointer to the array or NULL if no memory is
   available

*******************************************************************************/

java_longarray *builtin_newarray_long(s4 size)
{
	return (java_longarray *)
		builtin_newarray(size, primitivetype_table[ARRAYTYPE_LONG].arrayclass);
}


/* builtin_newarray_float ******************************************************

   Creates an array of 32 bit IEEE floats on the heap.

   Return value: pointer to the array or NULL if no memory is
   available

*******************************************************************************/

java_floatarray *builtin_newarray_float(s4 size)
{
	return (java_floatarray *)
		builtin_newarray(size, primitivetype_table[ARRAYTYPE_FLOAT].arrayclass);
}


/* builtin_newarray_double *****************************************************

   Creates an array of 64 bit IEEE floats on the heap.

   Return value: pointer to the array or NULL if no memory is
   available

*******************************************************************************/

java_doublearray *builtin_newarray_double(s4 size)
{
	return (java_doublearray *)
		builtin_newarray(size,
						 primitivetype_table[ARRAYTYPE_DOUBLE].arrayclass);
}


/* builtin_multianewarray_intern ***********************************************

   Creates a multi-dimensional array on the heap. The dimensions are
   passed in an array of longs.

   Arguments:
       n............number of dimensions to create
       arrayvftbl...vftbl of the array class
       dims.........array containing the size of each dimension to create

   Return value: pointer to the array or NULL if no memory is
   available

******************************************************************************/

static java_arrayheader *builtin_multianewarray_intern(int n,
													   classinfo *arrayclass,
													   long *dims)
{
	s4                size;
	java_arrayheader *a;
	classinfo        *componentclass;
	s4                i;

	/* create this dimension */

	size = (s4) dims[0];
  	a = builtin_newarray(size, arrayclass);

	if (!a)
		return NULL;

	/* if this is the last dimension return */

	if (!--n)
		return a;

	/* get the class of the components to create */

	componentclass = arrayclass->vftbl->arraydesc->componentvftbl->class;

	/* The verifier guarantees that the dimension count is in the range. */

	/* create the component arrays */

	for (i = 0; i < size; i++) {
		java_arrayheader *ea =
#if defined(__MIPS__) && (SIZEOF_VOID_P == 4)
			/* we save an s4 to a s8 slot, 8-byte aligned */

			builtin_multianewarray_intern(n, componentclass, dims + 2);
#else
			builtin_multianewarray_intern(n, componentclass, dims + 1);
#endif

		if (!ea)
			return NULL;
		
		((java_objectarray *) a)->data[i] = (java_objectheader *) ea;
	}

	return a;
}


/* builtin_multianewarray ******************************************************

   Wrapper for builtin_multianewarray_intern which checks all
   dimensions before we start allocating.

******************************************************************************/

java_arrayheader *builtin_multianewarray(int n, classinfo *arrayclass,
										 long *dims)
{
	s4 i;
	s4 size;

	/* check all dimensions before doing anything */

	for (i = 0; i < n; i++) {
#if defined(__MIPS__) && (SIZEOF_VOID_P == 4)
		/* we save an s4 to a s8 slot, 8-byte aligned */
		size = (s4) dims[i * 2];
#else
		size = (s4) dims[i];
#endif

		if (size < 0) {
			exceptions_throw_negativearraysizeexception();
			return NULL;
		}
	}

	/* now call the real function */

	return builtin_multianewarray_intern(n, arrayclass, dims);
}


/*****************************************************************************
					  METHOD LOGGING

	Various functions for printing a message at method entry or exit (for
	debugging)
	
*****************************************************************************/

#if !defined(NDEBUG)
static s4 methodindent = 0;
static u4 callcount = 0;

java_objectheader *builtin_trace_exception(java_objectheader *xptr,
										   methodinfo *m,
										   void *pos,
										   s4 indent)
{
	char *logtext;
	s4    logtextlen;
	s4    dumpsize;
	codeinfo *code;

	if (opt_verbosecall && indent)
		methodindent--;

	/* calculate message length */

	if (xptr) {
		logtextlen =
			strlen("Exception ") +
			utf_get_number_of_u2s(xptr->vftbl->class->name);

	} else {
		logtextlen = strlen("Some Throwable");
	}

	logtextlen += strlen(" thrown in ");

	if (m) {
		logtextlen +=
			utf_get_number_of_u2s(m->class->name) +
			strlen(".") +
			utf_get_number_of_u2s(m->name) +
			utf_get_number_of_u2s(m->descriptor) +
			strlen("(NOSYNC,NATIVE");

#if SIZEOF_VOID_P == 8
		logtextlen +=
			strlen(")(0x123456789abcdef0) at position 0x123456789abcdef0 (");
#else
		logtextlen += strlen(")(0x12345678) at position 0x12345678 (");
#endif

		if (m->class->sourcefile == NULL)
			logtextlen += strlen("<NO CLASSFILE INFORMATION>");
		else
			logtextlen += utf_get_number_of_u2s(m->class->sourcefile);

		logtextlen += strlen(":65536)");

	} else
		logtextlen += strlen("call_java_method");

	logtextlen += strlen("0");

	/* allocate memory */

	dumpsize = dump_size();

	logtext = DMNEW(char, logtextlen);

	if (xptr) {
		strcpy(logtext, "Exception ");
		utf_strcat_classname(logtext, xptr->vftbl->class->name);

	} else {
		strcpy(logtext, "Some Throwable");
	}

	strcat(logtext, " thrown in ");

	if (m) {
		utf_strcat_classname(logtext, m->class->name);
		strcat(logtext, ".");
		utf_strcat(logtext, m->name);
		utf_strcat(logtext, m->descriptor);

		if (m->flags & ACC_SYNCHRONIZED)
			strcat(logtext, "(SYNC");
		else
			strcat(logtext, "(NOSYNC");

		if (m->flags & ACC_NATIVE) {
			strcat(logtext, ",NATIVE");

			code = m->code;

#if SIZEOF_VOID_P == 8
			sprintf(logtext + strlen(logtext),
					")(0x%016lx) at position 0x%016lx",
					(ptrint) code->entrypoint, (ptrint) pos);
#else
			sprintf(logtext + strlen(logtext),
					")(0x%08x) at position 0x%08x",
					(ptrint) code->entrypoint, (ptrint) pos);
#endif

		} else {

			/* XXX preliminary: This should get the actual codeinfo */
			/* in which the exception happened.                     */
			code = m->code;
			
#if SIZEOF_VOID_P == 8
			sprintf(logtext + strlen(logtext),
					")(0x%016lx) at position 0x%016lx (",
					(ptrint) code->entrypoint, (ptrint) pos);
#else
			sprintf(logtext + strlen(logtext),
					")(0x%08x) at position 0x%08x (",
					(ptrint) code->entrypoint, (ptrint) pos);
#endif

			if (m->class->sourcefile == NULL)
				strcat(logtext, "<NO CLASSFILE INFORMATION>");
			else
				utf_strcat(logtext, m->class->sourcefile);

			sprintf(logtext + strlen(logtext), ":%d)", 0);
		}

	} else
		strcat(logtext, "call_java_method");

	log_text(logtext);

	/* release memory */

	dump_release(dumpsize);

	return xptr;
}
#endif /* !defined(NDEBUG) */


/* builtin_trace_args **********************************************************

   XXX

*******************************************************************************/

#if !defined(NDEBUG)

#ifdef TRACE_ARGS_NUM
void builtin_trace_args(s8 a0, s8 a1,
#if TRACE_ARGS_NUM >= 4
						s8 a2, s8 a3,
#endif /* TRACE_ARGS_NUM >= 4 */
#if TRACE_ARGS_NUM >= 6
						s8 a4, s8 a5,
#endif /* TRACE_ARGS_NUM >= 6 */
#if TRACE_ARGS_NUM == 8
						s8 a6, s8 a7,
#endif /* TRACE_ARGS_NUM == 8 */
						methodinfo *m)
{
	methoddesc *md;
	char       *logtext;
	s4          logtextlen;
	s4          dumpsize;
	s4          i;
	s4          pos;

	md = m->parseddesc;

	/* calculate message length */

	logtextlen =
		strlen("4294967295 ") +
		strlen("-2147483647-") +        /* INT_MAX should be sufficient       */
		methodindent +
		strlen("called: ") +
		utf_get_number_of_u2s(m->class->name) +
		strlen(".") +
		utf_get_number_of_u2s(m->name) +
		utf_get_number_of_u2s(m->descriptor) +
		strlen(" SYNCHRONIZED") + strlen("(") + strlen(")");

	/* add maximal argument length */

	logtextlen += strlen("0x123456789abcdef0, 0x123456789abcdef0, 0x123456789abcdef0, 0x123456789abcdef0, 0x123456789abcdef0, 0x123456789abcdef0, 0x123456789abcdef0, 0x123456789abcdef0, ...(255)");

	/* allocate memory */

	dumpsize = dump_size();

	logtext = DMNEW(char, logtextlen);

	callcount++;

	sprintf(logtext, "%10d ", callcount);
	sprintf(logtext + strlen(logtext), "-%d-", methodindent);

	pos = strlen(logtext);

	for (i = 0; i < methodindent; i++)
		logtext[pos++] = '\t';

	strcpy(logtext + pos, "called: ");

	utf_strcat_classname(logtext, m->class->name);
	strcat(logtext, ".");
	utf_strcat(logtext, m->name);
	utf_strcat(logtext, m->descriptor);

	if (m->flags & ACC_PUBLIC)       strcat(logtext, " PUBLIC");
	if (m->flags & ACC_PRIVATE)      strcat(logtext, " PRIVATE");
	if (m->flags & ACC_PROTECTED)    strcat(logtext, " PROTECTED");
   	if (m->flags & ACC_STATIC)       strcat(logtext, " STATIC");
   	if (m->flags & ACC_FINAL)        strcat(logtext, " FINAL");
   	if (m->flags & ACC_SYNCHRONIZED) strcat(logtext, " SYNCHRONIZED");
   	if (m->flags & ACC_VOLATILE)     strcat(logtext, " VOLATILE");
   	if (m->flags & ACC_TRANSIENT)    strcat(logtext, " TRANSIENT");
   	if (m->flags & ACC_NATIVE)       strcat(logtext, " NATIVE");
   	if (m->flags & ACC_INTERFACE)    strcat(logtext, " INTERFACE");
   	if (m->flags & ACC_ABSTRACT)     strcat(logtext, " ABSTRACT");

	strcat(logtext, "(");

	/* xxxprintf ?Bug? an PowerPc Linux (rlwinm.inso)                */
	/* Only Arguments in integer Registers are passed correctly here */
	/* long longs spilled on Stack have an wrong offset of +4        */
	/* So preliminary Bugfix: Only pass 3 params at once to sprintf  */
	/* for SIZEOG_VOID_P == 4 && TRACE_ARGS_NUM == 8                 */
	switch (md->paramcount) {
	case 0:
		break;

#if SIZEOF_VOID_P == 4
	case 1:
		sprintf(logtext + strlen(logtext),
				"0x%llx",
				a0);
		break;

	case 2:
		sprintf(logtext + strlen(logtext),
				"0x%llx, 0x%llx",
				a0, a1);
		break;

#if TRACE_ARGS_NUM >= 4
	case 3:
		sprintf(logtext + strlen(logtext),
				"0x%llx, 0x%llx, 0x%llx",
				a0, a1, a2);
		break;

	case 4:
		sprintf(logtext + strlen(logtext), "0x%llx, 0x%llx, 0x%llx"
				, a0, a1, a2);
		sprintf(logtext + strlen(logtext), ", 0x%llx", a3);

		break;
#endif /* TRACE_ARGS_NUM >= 4 */

#if TRACE_ARGS_NUM >= 6
	case 5:
		sprintf(logtext + strlen(logtext), "0x%llx, 0x%llx, 0x%llx"
				, a0, a1, a2);
		sprintf(logtext + strlen(logtext), ", 0x%llx, 0x%llx", a3, a4);
		break;


	case 6:
		sprintf(logtext + strlen(logtext), "0x%llx, 0x%llx, 0x%llx"
				, a0, a1, a2);
		sprintf(logtext + strlen(logtext), ", 0x%llx, 0x%llx, 0x%llx"
				, a3, a4, a5);
		break;
#endif /* TRACE_ARGS_NUM >= 6 */

#if TRACE_ARGS_NUM == 8
	case 7:
		sprintf(logtext + strlen(logtext), "0x%llx, 0x%llx, 0x%llx"
				, a0, a1, a2);
		sprintf(logtext + strlen(logtext), ", 0x%llx, 0x%llx, 0x%llx"
				, a3, a4, a5);
		sprintf(logtext + strlen(logtext), ", 0x%llx", a6);
		break;

	case 8:
		sprintf(logtext + strlen(logtext), "0x%llx, 0x%llx, 0x%llx"
				, a0, a1, a2);
		sprintf(logtext + strlen(logtext), ", 0x%llx, 0x%llx, 0x%llx"
				, a3, a4, a5);
		sprintf(logtext + strlen(logtext), ", 0x%llx, 0x%llx", a6, a7);
		break;
#endif /* TRACE_ARGS_NUM == 8 */

	default:
#if TRACE_ARGS_NUM == 2
		sprintf(logtext + strlen(logtext), "0x%llx, 0x%llx, ...(%d)", a0, a1, md->paramcount - 2);

#elif TRACE_ARGS_NUM == 4
		sprintf(logtext + strlen(logtext),
				"0x%llx, 0x%llx, 0x%llx, 0x%llx, ...(%d)",
				a0, a1, a2, a3, md->paramcount - 4);

#elif TRACE_ARGS_NUM == 6
		sprintf(logtext + strlen(logtext),
				"0x%llx, 0x%llx, 0x%llx, 0x%llx, 0x%llx, 0x%llx, ...(%d)",
				a0, a1, a2, a3, a4, a5, md->paramcount - 6);

#elif TRACE_ARGS_NUM == 8
		sprintf(logtext + strlen(logtext),"0x%llx, 0x%llx, 0x%llx,"
				, a0, a1, a2);
		sprintf(logtext + strlen(logtext)," 0x%llx, 0x%llx, 0x%llx,"
				, a3, a4, a5);
		sprintf(logtext + strlen(logtext)," 0x%llx, 0x%llx, ...(%d)"
				, a6, a7, md->paramcount - 8);
#endif
		break;

#else /* SIZEOF_VOID_P == 4 */

	case 1:
		sprintf(logtext + strlen(logtext),
				"0x%lx",
				a0);
		break;

	case 2:
		sprintf(logtext + strlen(logtext),
				"0x%lx, 0x%lx",
				a0, a1);
		break;

	case 3:
		sprintf(logtext + strlen(logtext),
				"0x%lx, 0x%lx, 0x%lx", a0, a1, a2);
		break;

	case 4:
		sprintf(logtext + strlen(logtext),
				"0x%lx, 0x%lx, 0x%lx, 0x%lx",
				a0, a1, a2, a3);
		break;

#if TRACE_ARGS_NUM >= 6
	case 5:
		sprintf(logtext + strlen(logtext),
				"0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx",
				a0, a1, a2, a3, a4);
		break;

	case 6:
		sprintf(logtext + strlen(logtext),
				"0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx",
				a0, a1, a2, a3, a4, a5);
		break;
#endif /* TRACE_ARGS_NUM >= 6 */

#if TRACE_ARGS_NUM == 8
	case 7:
		sprintf(logtext + strlen(logtext),
				"0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx",
				a0, a1, a2, a3, a4, a5, a6);
		break;

	case 8:
		sprintf(logtext + strlen(logtext),
				"0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx",
				a0, a1, a2, a3, a4, a5, a6, a7);
		break;
#endif /* TRACE_ARGS_NUM == 8 */

	default:
#if TRACE_ARGS_NUM == 4
		sprintf(logtext + strlen(logtext),
				"0x%lx, 0x%lx, 0x%lx, 0x%lx, ...(%d)",
				a0, a1, a2, a3, md->paramcount - 4);

#elif TRACE_ARGS_NUM == 6
		sprintf(logtext + strlen(logtext),
				"0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, ...(%d)",
				a0, a1, a2, a3, a4, a5, md->paramcount - 6);

#elif TRACE_ARGS_NUM == 8
		sprintf(logtext + strlen(logtext),
				"0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, ...(%d)",
				a0, a1, a2, a3, a4, a5, a6, a7, md->paramcount - 8);
#endif
		break;
#endif /* SIZEOF_VOID_P == 4 */
	}

	strcat(logtext, ")");

	log_text(logtext);

	/* release memory */

	dump_release(dumpsize);

	methodindent++;
}
#endif
#endif /* !defined(NDEBUG) */


/* builtin_displaymethodstop ***************************************************

   XXX

*******************************************************************************/

#if !defined(NDEBUG)
void builtin_displaymethodstop(methodinfo *m, s8 l, double d, float f)
{
	methoddesc  	  *md;
	char        	  *logtext;
	s4          	   logtextlen;
	s4          	   dumpsize;
	s4          	   i;
	s4                 pos;
	java_objectheader *o;
	java_lang_String  *s;
	classinfo         *c;
	s4                 len;
	utf               *u;
	imm_union          imu;

	md = m->parseddesc;

	/* calculate message length */

	logtextlen =
		strlen("4294967295 ") +
		strlen("-2147483647-") +        /* INT_MAX should be sufficient       */
		methodindent +
		strlen("finished: ") +
		utf_get_number_of_u2s(m->class->name) +
		strlen(".") +
		utf_get_number_of_u2s(m->name) +
		utf_get_number_of_u2s(m->descriptor) +
		strlen(" SYNCHRONIZED") + strlen("(") + strlen(")");

	/* add maximal argument length */

	logtextlen += strlen("->0.4872328470301428 (0x0123456789abcdef)");

	/* allocate memory */

	dumpsize = dump_size();

	logtext = DMNEW(char, logtextlen);

	/* outdent the log message */

	if (methodindent)
		methodindent--;
	else
		log_text("WARNING: unmatched methodindent--");

	/* generate the message */

	sprintf(logtext, "           ");
	sprintf(logtext + strlen(logtext), "-%d-", methodindent);

	pos = strlen(logtext);

	for (i = 0; i < methodindent; i++)
		logtext[pos++] = '\t';

	strcpy(logtext + pos, "finished: ");
	utf_strcat_classname(logtext, m->class->name);
	strcat(logtext, ".");
	utf_strcat(logtext, m->name);
	utf_strcat(logtext, m->descriptor);

	switch (md->returntype.type) {
	case TYPE_INT:
		sprintf(logtext + strlen(logtext), "->%d (0x%08x)", (s4) l, (s4) l);
		break;

	case TYPE_LNG:
#if SIZEOF_VOID_P == 4
		sprintf(logtext + strlen(logtext), "->%lld (0x%016llx)", (s8) l, l);
#else
		sprintf(logtext + strlen(logtext), "->%ld (0x%016lx)", (s8) l, l);
#endif
		break;

	case TYPE_ADR:
		sprintf(logtext + strlen(logtext), "->%p", (void *) (ptrint) l);

		/* check return argument for java.lang.Class or java.lang.String */

		o = (java_objectheader *) (ptrint) l;

		if (o != NULL) {
			if (o->vftbl->class == class_java_lang_String) {
				/* get java.lang.String object and the length of the
                   string */

				s= (java_lang_String *) (ptrint) l;

				len = strlen(", String = \"") + javastring_strlen(s) +
					strlen("\"");

				/* realloc memory for string length */

				DMREALLOC(logtext, char, logtextlen, logtextlen + len);

				/* convert to utf8 string and strcat it to the logtext */

				u = javastring_toutf(s, false);

				strcat(logtext, ", String = \"");
				utf_strcat(logtext, u);
				strcat(logtext, "\"");

			} else {
				if (o->vftbl->class == class_java_lang_Class) {
					/* if the object returned is a java.lang.Class
					   cast it to classinfo structure and get the name
					   of the class */

					c = (classinfo *) (ptrint) l;

					u = c->name;

				} else {
					/* if the object returned is not a java.lang.String or
					   a java.lang.Class just print the name of the class */

					u = o->vftbl->class->name;
				}

				len = strlen(", Class = \"") + utf_get_number_of_u2s(u) + strlen("\"");

				/* realloc memory for string length */

				DMREALLOC(logtext, char, logtextlen, logtextlen + len);

				/* strcat to the logtext */

				strcat(logtext, ", Class = \"");
				utf_strcat(logtext, u);
				strcat(logtext, "\"");
			}
		}
		break;

	case TYPE_FLT:
		imu.f = f;
		sprintf(logtext + strlen(logtext), "->%.8f (0x%08x)", f, imu.i);
		break;

	case TYPE_DBL:
		imu.d = d;
#if SIZEOF_VOID_P == 4
		sprintf(logtext + strlen(logtext), "->%.16g (0x%016llx)", d, imu.l);
#else
		sprintf(logtext + strlen(logtext), "->%.16g (0x%016lx)", d, imu.l);
#endif
		break;
	}

	log_text(logtext);

	/* release memory */

	dump_release(dumpsize);
}
#endif /* !defined(NDEBUG) */


/****************************************************************************
			 SYNCHRONIZATION FUNCTIONS
*****************************************************************************/

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
/*
 * Lock the mutex of an object.
 */
void internal_lock_mutex_for_object(java_objectheader *object)
{
	mutexHashEntry *entry;
	int hashValue;

	assert(object != 0);

	hashValue = MUTEX_HASH_VALUE(object);
	entry = &mutexHashTable[hashValue];

	if (entry->object != 0) {
		if (entry->mutex.count == 0 && entry->conditionCount == 0) {
			entry->object = 0;
			entry->mutex.holder = 0;
			entry->mutex.count = 0;
			entry->mutex.muxWaiters = 0;

		} else {
			while (entry->next != 0 && entry->object != object)
				entry = entry->next;

			if (entry->object != object) {
				entry->next = firstFreeOverflowEntry;
				firstFreeOverflowEntry = firstFreeOverflowEntry->next;

				entry = entry->next;
				entry->object = 0;
				entry->next = 0;
				assert(entry->conditionCount == 0);
			}
		}

	} else {
		entry->mutex.holder = 0;
		entry->mutex.count = 0;
		entry->mutex.muxWaiters = 0;
	}

	if (entry->object == 0)
		entry->object = object;
	
	internal_lock_mutex(&entry->mutex);
}
#endif


#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
/*
 * Unlocks the mutex of an object.
 */
void internal_unlock_mutex_for_object (java_objectheader *object)
{
	int hashValue;
	mutexHashEntry *entry;

	hashValue = MUTEX_HASH_VALUE(object);
	entry = &mutexHashTable[hashValue];

	if (entry->object == object) {
		internal_unlock_mutex(&entry->mutex);

	} else {
		while (entry->next != 0 && entry->next->object != object)
			entry = entry->next;

		assert(entry->next != 0);

		internal_unlock_mutex(&entry->next->mutex);

		if (entry->next->mutex.count == 0 && entry->conditionCount == 0) {
			mutexHashEntry *unlinked = entry->next;

			entry->next = unlinked->next;
			unlinked->next = firstFreeOverflowEntry;
			firstFreeOverflowEntry = unlinked;
		}
	}
}
#endif

#if defined(ENABLE_CYCLES_STATS)
void builtin_print_cycles_stats(FILE *file)
{
	fprintf(file,"builtin cylce count statistics:\n");

	CYCLES_STATS_PRINT_OVERHEAD(builtin_overhead,file);
	CYCLES_STATS_PRINT(builtin_monitorenter,file);
	CYCLES_STATS_PRINT(builtin_monitorexit ,file);
	CYCLES_STATS_PRINT(builtin_new         ,file);

	fprintf(file,"\n");
}
#endif /* defined(ENABLE_CYCLES_STATS) */

#if defined(USE_THREADS)
void builtin_monitorenter(java_objectheader *o)
{
#if defined(ENABLE_CYCLES_STATS)
	u8 cycles_start, cycles_overhead, cycles_end;
#endif

#if !defined(NATIVE_THREADS)
	int hashValue;

	++blockInts;

	hashValue = MUTEX_HASH_VALUE(o);
	if (mutexHashTable[hashValue].object == o 
		&& mutexHashTable[hashValue].mutex.holder == currentThread)
		++mutexHashTable[hashValue].mutex.count;
	else
		internal_lock_mutex_for_object(o);

	--blockInts;
#else /* defined(NATIVE_THREADS) */
	CYCLES_STATS_GET(cycles_start);
	CYCLES_STATS_GET(cycles_overhead);

	monitorEnter((threadobject *) THREADOBJECT, o);

	CYCLES_STATS_GET(cycles_end);
	CYCLES_STATS_COUNT(builtin_monitorenter, cycles_end - cycles_overhead);
	CYCLES_STATS_COUNT(builtin_overhead    , cycles_overhead - cycles_start);
#endif /* defined(NATIVE_THREADS) */
}
#endif


#if defined(USE_THREADS)
/*
 * Locks the class object - needed for static synchronized methods.
 */
void builtin_staticmonitorenter(classinfo *c)
{
	builtin_monitorenter(&c->object.header);
}
#endif


#if defined(USE_THREADS)
void builtin_monitorexit(java_objectheader *o)
{
#if defined(ENABLE_CYCLES_STATS)
	u8 cycles_start, cycles_end;
#endif

#if !defined(NATIVE_THREADS)
	int hashValue;

	++blockInts;

	hashValue = MUTEX_HASH_VALUE(o);
	if (mutexHashTable[hashValue].object == o) {
		if (mutexHashTable[hashValue].mutex.count == 1
			&& mutexHashTable[hashValue].mutex.muxWaiters != 0)
			internal_unlock_mutex_for_object(o);
		else
			--mutexHashTable[hashValue].mutex.count;

	} else
		internal_unlock_mutex_for_object(o);

	--blockInts;
#else /* defined(NATIVE_THREADS) */
	CYCLES_STATS_GET(cycles_start);

	monitorExit((threadobject *) THREADOBJECT, o);

	CYCLES_STATS_GET(cycles_end);
	CYCLES_STATS_COUNT(builtin_monitorexit, cycles_end - cycles_start);
#endif /* defined(NATIVE_THREADS) */
}
#endif


/*****************************************************************************
			  MISCELLANEOUS HELPER FUNCTIONS
*****************************************************************************/



/*********** Functions for integer divisions *****************************
 
	On some systems (eg. DEC ALPHA), integer division is not supported by the
	CPU. These helper functions implement the missing functionality.

******************************************************************************/

#if !SUPPORT_DIVISION
s4 builtin_idiv(s4 a, s4 b)
{
	s4 c;

	c = a / b;

	return c;
}

s4 builtin_irem(s4 a, s4 b)
{
	s4 c;

	c = a % b;

	return c;
}
#endif /* !SUPPORT_DIVISION */


/* functions for long arithmetics **********************************************

   On systems where 64 bit Integers are not supported by the CPU,
   these functions are needed.

******************************************************************************/

#if !(SUPPORT_LONG && SUPPORT_LONG_ADD)
s8 builtin_ladd(s8 a, s8 b)
{
	s8 c;

#if U8_AVAILABLE
	c = a + b; 
#else
	c = builtin_i2l(0);
#endif

	return c;
}

s8 builtin_lsub(s8 a, s8 b)
{
	s8 c;

#if U8_AVAILABLE
	c = a - b; 
#else
	c = builtin_i2l(0);
#endif

	return c;
}

s8 builtin_lneg(s8 a)
{
	s8 c;

#if U8_AVAILABLE
	c = -a;
#else
	c = builtin_i2l(0);
#endif

	return c;
}
#endif /* !(SUPPORT_LONG && SUPPORT_LONG_ADD) */


#if !(SUPPORT_LONG && SUPPORT_LONG_MUL)
s8 builtin_lmul(s8 a, s8 b)
{
	s8 c;

#if U8_AVAILABLE
	c = a * b; 
#else
	c = builtin_i2l(0);
#endif

	return c;
}
#endif /* !(SUPPORT_LONG && SUPPORT_LONG_MUL) */


#if !(SUPPORT_DIVISION && SUPPORT_LONG && SUPPORT_LONG_DIV)
s8 builtin_ldiv(s8 a, s8 b)
{
	s8 c;

#if U8_AVAILABLE
	c = a / b; 
#else
	c = builtin_i2l(0);
#endif

	return c;
}

s8 builtin_lrem(s8 a, s8 b)
{
	s8 c;

#if U8_AVAILABLE
	c = a % b; 
#else
	c = builtin_i2l(0);
#endif

	return c;
}
#endif /* !(SUPPORT_DIVISION && SUPPORT_LONG && SUPPORT_LONG_DIV) */


#if !(SUPPORT_LONG && SUPPORT_LONG_SHIFT)
s8 builtin_lshl(s8 a, s4 b)
{
	s8 c;

#if U8_AVAILABLE
	c = a << (b & 63);
#else
	c = builtin_i2l(0);
#endif

	return c;
}

s8 builtin_lshr(s8 a, s4 b)
{
	s8 c;

#if U8_AVAILABLE
	c = a >> (b & 63);
#else
	c = builtin_i2l(0);
#endif

	return c;
}

s8 builtin_lushr(s8 a, s4 b)
{
	s8 c;

#if U8_AVAILABLE
	c = ((u8) a) >> (b & 63);
#else
	c = builtin_i2l(0);
#endif

	return c;
}
#endif /* !(SUPPORT_LONG && SUPPORT_LONG_SHIFT) */


#if !(SUPPORT_LONG && SUPPORT_LONG_LOGICAL)
s8 builtin_land(s8 a, s8 b)
{
	s8 c;

#if U8_AVAILABLE
	c = a & b; 
#else
	c = builtin_i2l(0);
#endif

	return c;
}

s8 builtin_lor(s8 a, s8 b)
{
	s8 c;

#if U8_AVAILABLE
	c = a | b; 
#else
	c = builtin_i2l(0);
#endif

	return c;
}

s8 builtin_lxor(s8 a, s8 b) 
{
	s8 c;

#if U8_AVAILABLE
	c = a ^ b; 
#else
	c = builtin_i2l(0);
#endif

	return c;
}
#endif /* !(SUPPORT_LONG && SUPPORT_LONG_LOGICAL) */


#if !(SUPPORT_LONG && SUPPORT_LONG_CMP)
s4 builtin_lcmp(s8 a, s8 b)
{ 
#if U8_AVAILABLE
	if (a < b)
		return -1;

	if (a > b)
		return 1;

	return 0;
#else
	return 0;
#endif
}
#endif /* !(SUPPORT_LONG && SUPPORT_LONG_CMP) */


/* functions for unsupported floating instructions ****************************/

/* used to convert FLT_xxx defines into float values */

static inline float intBitsToFloat(s4 i)
{
	imm_union imb;

	imb.i = i;
	return imb.f;
}


/* used to convert DBL_xxx defines into double values */

static inline float longBitsToDouble(s8 l)
{
	imm_union imb;

	imb.l = l;
	return imb.d;
}


#if !SUPPORT_FLOAT
float builtin_fadd(float a, float b)
{
	if (isnanf(a)) return intBitsToFloat(FLT_NAN);
	if (isnanf(b)) return intBitsToFloat(FLT_NAN);
	if (finitef(a)) {
		if (finitef(b))
			return a + b;
		else
			return b;
	}
	else {
		if (finitef(b))
			return a;
		else {
			if (copysignf(1.0, a) == copysignf(1.0, b))
				return a;
			else
				return intBitsToFloat(FLT_NAN);
		}
	}
}


float builtin_fsub(float a, float b)
{
	return builtin_fadd(a, builtin_fneg(b));
}


float builtin_fmul(float a, float b)
{
	if (isnanf(a)) return intBitsToFloat(FLT_NAN);
	if (isnanf(b)) return intBitsToFloat(FLT_NAN);
	if (finitef(a)) {
		if (finitef(b)) return a * b;
		else {
			if (a == 0) return intBitsToFloat(FLT_NAN);
			else return copysignf(b, copysignf(1.0, b)*a);
		}
	}
	else {
		if (finitef(b)) {
			if (b == 0) return intBitsToFloat(FLT_NAN);
			else return copysignf(a, copysignf(1.0, a)*b);
		}
		else {
			return copysignf(a, copysignf(1.0, a)*copysignf(1.0, b));
		}
	}
}


/* builtin_ddiv ****************************************************************

   Implementation as described in VM Spec.

*******************************************************************************/

float builtin_fdiv(float a, float b)
{
	if (finitef(a)) {
		if (finitef(b)) {
			/* If neither value1' nor value2' is NaN, the sign of the result */
			/* is positive if both values have the same sign, negative if the */
			/* values have different signs. */

			return a / b;

		} else {
			if (isnanf(b)) {
				/* If either value1' or value2' is NaN, the result is NaN. */

				return intBitsToFloat(FLT_NAN);

			} else {
				/* Division of a finite value by an infinity results in a */
				/* signed zero, with the sign-producing rule just given. */

				/* is sign equal? */

				if (copysignf(1.0, a) == copysignf(1.0, b))
					return 0.0;
				else
					return -0.0;
			}
		}

	} else {
		if (isnanf(a)) {
			/* If either value1' or value2' is NaN, the result is NaN. */

			return intBitsToFloat(FLT_NAN);

		} else if (finitef(b)) {
			/* Division of an infinity by a finite value results in a signed */
			/* infinity, with the sign-producing rule just given. */

			/* is sign equal? */

			if (copysignf(1.0, a) == copysignf(1.0, b))
				return intBitsToFloat(FLT_POSINF);
			else
				return intBitsToFloat(FLT_NEGINF);

		} else {
			/* Division of an infinity by an infinity results in NaN. */

			return intBitsToFloat(FLT_NAN);
		}
	}
}


float builtin_fneg(float a)
{
	if (isnanf(a)) return a;
	else {
		if (finitef(a)) return -a;
		else return copysignf(a, -copysignf(1.0, a));
	}
}
#endif /* !SUPPORT_FLOAT */


#if !SUPPORT_FLOAT || defined(ENABLE_INTRP)
s4 builtin_fcmpl(float a, float b)
{
	if (isnanf(a)) return -1;
	if (isnanf(b)) return -1;
	if (!finitef(a) || !finitef(b)) {
		a = finitef(a) ? 0 : copysignf(1.0,	a);
		b = finitef(b) ? 0 : copysignf(1.0, b);
	}
	if (a > b) return 1;
	if (a == b) return 0;
	return -1;
}


s4 builtin_fcmpg(float a, float b)
{
	if (isnanf(a)) return 1;
	if (isnanf(b)) return 1;
	if (!finitef(a) || !finitef(b)) {
		a = finitef(a) ? 0 : copysignf(1.0, a);
		b = finitef(b) ? 0 : copysignf(1.0, b);
	}
	if (a > b) return 1;
	if (a == b) return 0;
	return -1;
}
#endif /* !SUPPORT_FLOAT || defined(ENABLE_INTRP) */


float builtin_frem(float a, float b)
{
	return fmodf(a, b);
}


/* functions for unsupported double instructions ******************************/

#if !SUPPORT_DOUBLE
double builtin_dadd(double a, double b)
{
	if (isnan(a)) return longBitsToDouble(DBL_NAN);
	if (isnan(b)) return longBitsToDouble(DBL_NAN);
	if (finite(a)) {
		if (finite(b)) return a + b;
		else return b;
	}
	else {
		if (finite(b)) return a;
		else {
			if (copysign(1.0, a)==copysign(1.0, b)) return a;
			else return longBitsToDouble(DBL_NAN);
		}
	}
}


double builtin_dsub(double a, double b)
{
	return builtin_dadd(a, builtin_dneg(b));
}


double builtin_dmul(double a, double b)
{
	if (isnan(a)) return longBitsToDouble(DBL_NAN);
	if (isnan(b)) return longBitsToDouble(DBL_NAN);
	if (finite(a)) {
		if (finite(b)) return a * b;
		else {
			if (a == 0) return longBitsToDouble(DBL_NAN);
			else return copysign(b, copysign(1.0, b) * a);
		}
	}
	else {
		if (finite(b)) {
			if (b == 0) return longBitsToDouble(DBL_NAN);
			else return copysign(a, copysign(1.0, a) * b);
		}
		else {
			return copysign(a, copysign(1.0, a) * copysign(1.0, b));
		}
	}
}


/* builtin_ddiv ****************************************************************

   Implementation as described in VM Spec.

*******************************************************************************/

double builtin_ddiv(double a, double b)
{
	if (finite(a)) {
		if (finite(b)) {
			/* If neither value1' nor value2' is NaN, the sign of the result */
			/* is positive if both values have the same sign, negative if the */
			/* values have different signs. */

			return a / b;

		} else {
			if (isnan(b)) {
				/* If either value1' or value2' is NaN, the result is NaN. */

				return longBitsToDouble(DBL_NAN);

			} else {
				/* Division of a finite value by an infinity results in a */
				/* signed zero, with the sign-producing rule just given. */

				/* is sign equal? */

				if (copysign(1.0, a) == copysign(1.0, b))
					return 0.0;
				else
					return -0.0;
			}
		}

	} else {
		if (isnan(a)) {
			/* If either value1' or value2' is NaN, the result is NaN. */

			return longBitsToDouble(DBL_NAN);

		} else if (finite(b)) {
			/* Division of an infinity by a finite value results in a signed */
			/* infinity, with the sign-producing rule just given. */

			/* is sign equal? */

			if (copysign(1.0, a) == copysign(1.0, b))
				return longBitsToDouble(DBL_POSINF);
			else
				return longBitsToDouble(DBL_NEGINF);

		} else {
			/* Division of an infinity by an infinity results in NaN. */

			return longBitsToDouble(DBL_NAN);
		}
	}
}


/* builtin_dneg ****************************************************************

   Implemented as described in VM Spec.

*******************************************************************************/

double builtin_dneg(double a)
{
	if (isnan(a)) {
		/* If the operand is NaN, the result is NaN (recall that NaN has no */
		/* sign). */

		return a;

	} else {
		if (finite(a)) {
			/* If the operand is a zero, the result is the zero of opposite */
			/* sign. */

			return -a;

		} else {
			/* If the operand is an infinity, the result is the infinity of */
			/* opposite sign. */

			return copysign(a, -copysign(1.0, a));
		}
	}
}
#endif /* !SUPPORT_DOUBLE */


#if !SUPPORT_DOUBLE || defined(ENABLE_INTRP)
s4 builtin_dcmpl(double a, double b)
{
	if (isnan(a)) return -1;
	if (isnan(b)) return -1;
	if (!finite(a) || !finite(b)) {
		a = finite(a) ? 0 : copysign(1.0, a);
		b = finite(b) ? 0 : copysign(1.0, b);
	}
	if (a > b) return 1;
	if (a == b) return 0;
	return -1;
}


s4 builtin_dcmpg(double a, double b)
{
	if (isnan(a)) return 1;
	if (isnan(b)) return 1;
	if (!finite(a) || !finite(b)) {
		a = finite(a) ? 0 : copysign(1.0, a);
		b = finite(b) ? 0 : copysign(1.0, b);
	}
	if (a > b) return 1;
	if (a == b) return 0;
	return -1;
}
#endif /* !SUPPORT_DOUBLE || defined(ENABLE_INTRP) */


double builtin_drem(double a, double b)
{
	return fmod(a, b);
}


/* conversion operations ******************************************************/

#if 0
s8 builtin_i2l(s4 i)
{
#if U8_AVAILABLE
	return i;
#else
	s8 v;
	v.high = 0;
	v.low = i;
	return v;
#endif
}

s4 builtin_l2i(s8 l)
{
#if U8_AVAILABLE
	return (s4) l;
#else
	return l.low;
#endif
}
#endif


#if !(SUPPORT_FLOAT && SUPPORT_I2F)
float builtin_i2f(s4 a)
{
	float f = (float) a;
	return f;
}
#endif /* !(SUPPORT_FLOAT && SUPPORT_I2F) */


#if !(SUPPORT_DOUBLE && SUPPORT_I2D)
double builtin_i2d(s4 a)
{
	double d = (double) a;
	return d;
}
#endif /* !(SUPPORT_DOUBLE && SUPPORT_I2D) */


#if !(SUPPORT_LONG && SUPPORT_FLOAT && SUPPORT_L2F)
float builtin_l2f(s8 a)
{
#if U8_AVAILABLE
	float f = (float) a;
	return f;
#else
	return 0.0;
#endif
}
#endif /* !(SUPPORT_LONG && SUPPORT_FLOAT && SUPPORT_L2F) */


#if !(SUPPORT_LONG && SUPPORT_DOUBLE && SUPPORT_L2D)
double builtin_l2d(s8 a)
{
#if U8_AVAILABLE
	double d = (double) a;
	return d;
#else
	return 0.0;
#endif
}
#endif /* !(SUPPORT_LONG && SUPPORT_DOUBLE && SUPPORT_L2D) */


#if !(SUPPORT_FLOAT && SUPPORT_F2I) || defined(ENABLE_INTRP)
s4 builtin_f2i(float a) 
{
	s4 i;

	i = builtin_d2i((double) a);

	return i;

	/*	float f;
	
		if (isnanf(a))
		return 0;
		if (finitef(a)) {
		if (a > 2147483647)
		return 2147483647;
		if (a < (-2147483648))
		return (-2147483648);
		return (s4) a;
		}
		f = copysignf((float) 1.0, a);
		if (f > 0)
		return 2147483647;
		return (-2147483648); */
}
#endif /* !(SUPPORT_FLOAT && SUPPORT_F2I) || defined(ENABLE_INTRP) */


#if !(SUPPORT_FLOAT && SUPPORT_LONG && SUPPORT_F2L)
s8 builtin_f2l(float a)
{
	s8 l;

	l = builtin_d2l((double) a);

	return l;

	/*	float f;
	
		if (finitef(a)) {
		if (a > 9223372036854775807L)
		return 9223372036854775807L;
		if (a < (-9223372036854775808L))
		return (-9223372036854775808L);
		return (s8) a;
		}
		if (isnanf(a))
		return 0;
		f = copysignf((float) 1.0, a);
		if (f > 0)
		return 9223372036854775807L;
		return (-9223372036854775808L); */
}
#endif /* !(SUPPORT_FLOAT && SUPPORT_LONG && SUPPORT_F2L) */


#if !(SUPPORT_DOUBLE && SUPPORT_D2I) || defined(ENABLE_INTRP)
s4 builtin_d2i(double a) 
{ 
	double d;
	
	if (finite(a)) {
		if (a >= 2147483647)
			return 2147483647;
		if (a <= (-2147483647-1))
			return (-2147483647-1);
		return (s4) a;
	}
	if (isnan(a))
		return 0;
	d = copysign(1.0, a);
	if (d > 0)
		return 2147483647;
	return (-2147483647-1);
}
#endif /* !(SUPPORT_DOUBLE && SUPPORT_D2I) || defined(ENABLE_INTRP) */


#if !(SUPPORT_DOUBLE && SUPPORT_LONG && SUPPORT_D2L)
s8 builtin_d2l(double a)
{
	double d;
	
	if (finite(a)) {
		if (a >= 9223372036854775807LL)
			return 9223372036854775807LL;
		if (a <= (-9223372036854775807LL-1))
			return (-9223372036854775807LL-1);
		return (s8) a;
	}
	if (isnan(a))
		return 0;
	d = copysign(1.0, a);
	if (d > 0)
		return 9223372036854775807LL;
	return (-9223372036854775807LL-1);
}
#endif /* !(SUPPORT_DOUBLE && SUPPORT_LONG && SUPPORT_D2L) */


#if !(SUPPORT_FLOAT && SUPPORT_DOUBLE)
double builtin_f2d(float a)
{
	if (finitef(a)) return (double) a;
	else {
		if (isnanf(a))
			return longBitsToDouble(DBL_NAN);
		else
			return copysign(longBitsToDouble(DBL_POSINF), (double) copysignf(1.0, a) );
	}
}

float builtin_d2f(double a)
{
	if (finite(a))
		return (float) a;
	else {
		if (isnan(a))
			return intBitsToFloat(FLT_NAN);
		else
			return copysignf(intBitsToFloat(FLT_POSINF), (float) copysign(1.0, a));
	}
}
#endif /* !(SUPPORT_FLOAT && SUPPORT_DOUBLE) */


/* builtin_clone_array *********************************************************

   Wrapper function for cloning arrays.

*******************************************************************************/

java_arrayheader *builtin_clone_array(void *env, java_arrayheader *o)
{
	java_arrayheader    *ah;
	java_lang_Cloneable *c;

	c = (java_lang_Cloneable *) o;

	ah = (java_arrayheader *) Java_java_lang_VMObject_clone(0, 0, c);

	return ah;
}


/* builtin_asm_get_exceptionptrptr *********************************************

   this is a wrapper for calls from asmpart

*******************************************************************************/

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
java_objectheader **builtin_asm_get_exceptionptrptr(void)
{
	return builtin_get_exceptionptrptr();
}
#endif


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
 * vim:noexpandtab:sw=4:ts=4:
 */
