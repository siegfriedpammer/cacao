/* builtin.c - functions for unsupported operations

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
            Andreas Krall
            Mark Probst

   Contains C functions for JavaVM Instructions that cannot be
   translated to machine language directly. Consequently, the
   generated machine code for these instructions contains function
   calls instead of machine instructions, using the C calling
   convention.

   $Id: builtin.c 890 2004-01-19 12:24:13Z edwin $

*/


#include <assert.h>
#include <string.h>
#include <math.h>
#include "main.h"
#include "global.h"
#include "builtin.h"
#include "native.h"
#include "loader.h"
#include "tables.h"
#include "asmpart.h"
#include "mm/boehm.h"
#include "threads/thread.h"
#include "threads/locks.h"
#include "toolbox/loging.h"
#include "toolbox/memory.h"
#include "nat/java_lang_Cloneable.h"
#include "nat/java_lang_VMObject.h"


#undef DEBUG /*define DEBUG 1*/


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

	/*classinfo *tmp;*/
	if (super->flags & ACC_INTERFACE)
		return (sub->vftbl->interfacetablelength > super->index) &&
			(sub->vftbl->interfacetable[-super->index] != NULL);

	/*
	  while (sub != 0)
	  if (sub == super)
	  return 1;
	  else
	  sub = sub->super;

	  return 0;
	*/

/*
	for (tmp=sub;tmp!=0;tmp=tmp->super) {
		printf("->");
		utf_display(tmp->name);
	}
		printf("\n\n");
	
	for (tmp=super;tmp!=0;tmp=tmp->super) {
		printf("->");
		utf_display(tmp->name);
	}
		printf("\n");
	

	printf("sub->vftbl->baseval %d, super->vftbl->baseval %d\n diff %d, super->vftbl->diffval %d\n",
			sub->vftbl->baseval, super->vftbl->baseval, (unsigned)(sub->vftbl->baseval - super->vftbl->baseval),
			super->vftbl->diffval); */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	cast_lock();
#endif

	res = (unsigned) (sub->vftbl->baseval - super->vftbl->baseval) <=
		(unsigned) (super->vftbl->diffval);

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	cast_unlock();
#endif

	return res;
}

s4 builtin_isanysubclass_vftbl(vftbl *sub,vftbl *super)
{
	int base;
	s4 res;
	
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	cast_lock();
#endif

	if ((base = super->baseval) <= 0)
		/* super is an interface */
		res = (sub->interfacetablelength > -base) &&
			(sub->interfacetable[base] != NULL);
	else
	    res = (unsigned) (sub->baseval - base)
			<= (unsigned) (super->diffval);

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	cast_unlock();
#endif

	return res;
}


/****************** function: builtin_instanceof *****************************

	Checks if an object is an instance of some given class (or subclass of
	that class). If class is an interface, checks if the interface is
	implemented.
	Return value:  1 ... obj is an instance of class or implements the interface
				   0 ... otherwise or if obj == NULL
			 
*****************************************************************************/

/* XXX should use vftbl */
s4 builtin_instanceof(java_objectheader *obj, classinfo *class)
{
#ifdef DEBUG
	log_text ("builtin_instanceof called");
#endif	
	if (!obj) return 0;
	return builtin_isanysubclass (obj->vftbl->class, class);
}



/**************** function: builtin_checkcast *******************************

	The same as builtin_instanceof except that 1 is returned when
	obj == NULL
			  
****************************************************************************/

/* XXX should use vftbl */
s4 builtin_checkcast(java_objectheader *obj, classinfo *class)
{
#ifdef DEBUG
	log_text("builtin_checkcast called");
#endif

	if (obj == NULL)
		return 1;
	if (builtin_isanysubclass(obj->vftbl->class, class))
		return 1;

#if DEBUG
	printf("#### checkcast failed ");
	utf_display(obj->vftbl->class->name);
	printf(" -> ");
	utf_display(class->name);
	printf("\n");
#endif

	return 0;
}


/*********** internal function: builtin_descriptorscompatible ******************

	Checks if two array type descriptors are assignment compatible
	Return value:  1 ... target = desc is possible
				   0 ... otherwise
			
******************************************************************************/

static s4 builtin_descriptorscompatible(arraydescriptor *desc,arraydescriptor *target)
{
	if (desc==target) return 1;
	if (desc->arraytype != target->arraytype) return 0;
	if (desc->arraytype != ARRAYTYPE_OBJECT) return 1;
	
	/* {both arrays are arrays of references} */
	if (desc->dimension == target->dimension)
		return builtin_isanysubclass_vftbl(desc->elementvftbl,target->elementvftbl);
	if (desc->dimension < target->dimension) return 0;

	/* {desc has higher dimension than target} */
	return builtin_isanysubclass_vftbl(pseudo_class_Arraystub_vftbl,target->elementvftbl);
}


/******************** function: builtin_checkarraycast ***********************

	Checks if an object is really a subtype of the requested array type.
	The object has to be an array to begin with. For simple arrays (int, short,
	double, etc.) the types have to match exactly.
	For arrays of objects, the type of elements in the array has to be a
	subtype (or the same type) of the requested element type. For arrays of
	arrays (which in turn can again be arrays of arrays), the types at the
	lowest level have to satisfy the corresponding sub class relation.
	
	Return value:  1 ... cast is possible
				   0 ... otherwise
	
	ATTENTION: a cast with a NULL pointer is always possible.
			
*****************************************************************************/

s4 builtin_checkarraycast(java_objectheader *o, vftbl *target)
{
	arraydescriptor *desc;
	
	if (!o) return 1;
	if ((desc = o->vftbl->arraydesc) == NULL) return 0;

	return builtin_descriptorscompatible(desc, target->arraydesc);
}


s4 builtin_arrayinstanceof(java_objectheader *obj, vftbl *target)
{
	if (!obj) return 1;
	return builtin_checkarraycast(obj, target);
}


/************************** exception functions *******************************

******************************************************************************/

java_objectheader *builtin_throw_exception(java_objectheader *local_exceptionptr)
{
	if (verbose) {
		char logtext[MAXLOGTEXT];
		sprintf(logtext, "Builtin exception thrown: ");
		if (local_exceptionptr)
			utf_sprint(logtext + strlen(logtext), local_exceptionptr->vftbl->class->name);
		else {
			sprintf(logtext+strlen(logtext),"%s","Error: <Nullpointer instead of exception>");
			if (!proto_java_lang_ClassCastException) sprintf(logtext+strlen(logtext),"%s","proto_java_lang_ClassCastException==0");
			if (!proto_java_lang_NullPointerException) sprintf(logtext+strlen(logtext),"%s","proto_java_lang_NullPointerException==0");
			if (!proto_java_lang_NullPointerException) sprintf(logtext+strlen(logtext),"%s","proto_java_lang_NullPointerException==0");
			if (!proto_java_lang_ArrayIndexOutOfBoundsException) sprintf(logtext+strlen(logtext),"%s","proto_java_lang_ArrayIndexOutOfBoundsException==0");
			if (!proto_java_lang_NegativeArraySizeException) sprintf(logtext+strlen(logtext),"%s","proto_java_lang_NegativeArraySizeException==0");
			if (!proto_java_lang_OutOfMemoryError) sprintf(logtext+strlen(logtext),"%s","proto_java_lang_OutOfMemoryError==0");
			if (!proto_java_lang_ArithmeticException) sprintf(logtext+strlen(logtext),"%s","proto_java_lang_ArithmeticException==0");
			if (!proto_java_lang_ArrayStoreException) sprintf(logtext+strlen(logtext),"%s","proto_java_lang_ArrayStoreException==0");
			if (!proto_java_lang_ThreadDeath) sprintf(logtext+strlen(logtext),"%s","proto_java_lang_ThreadDeath==0");
			if (!proto_java_lang_ThreadDeath) sprintf(logtext+strlen(logtext),"%s","proto_java_lang_ThreadDeath==0");
			}
		log_text(logtext);
	}
	*exceptionptr = local_exceptionptr;
	return local_exceptionptr;
}

void builtin_reset_exceptionptr()
{
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
#ifdef HAVE___THREAD
	_exceptionptr = NULL;
#else
	((nativethread*) pthread_getspecific(tkey_exceptionptr))->_exceptionptr = 0;
#endif
#else
	panic("builtin_reset_exceptionptr should not be used in this configuration");
#endif
}


/******************* function: builtin_canstore *******************************

	Checks, if an object can be stored in an array.
	Return value:  1 ... possible
				   0 ... otherwise

******************************************************************************/

s4 builtin_canstore (java_objectarray *a, java_objectheader *o)
{
	arraydescriptor *desc;
	arraydescriptor *valuedesc;
	vftbl *componentvftbl;
	vftbl *valuevftbl;
    int dim_m1;
	int base;
	
	if (!o) return 1;

	/* The following is guaranteed (by verifier checks):
	 *
	 *     *) a->...vftbl->arraydesc != NULL
	 *     *) a->...vftbl->arraydesc->componentvftbl != NULL
	 *     *) o->vftbl is not an interface vftbl
	 */
	
	desc = a->header.objheader.vftbl->arraydesc;
    componentvftbl = desc->componentvftbl;
	valuevftbl = o->vftbl;

    if ((dim_m1 = desc->dimension - 1) == 0) {
		s4 res;

		/* {a is a one-dimensional array} */
		/* {a is an array of references} */
		
		if (valuevftbl == componentvftbl)
			return 1;

		if ((base = componentvftbl->baseval) <= 0)
			/* an array of interface references */
			return (valuevftbl->interfacetablelength > -base &&
					valuevftbl->interfacetable[base] != NULL);
		
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
		cast_lock();
#endif

		res = (unsigned)(valuevftbl->baseval - base)
			<= (unsigned)(componentvftbl->diffval);

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
		cast_unlock();
#endif

		return res;
    }
    /* {a has dimension > 1} */
	/* {componentvftbl->arraydesc != NULL} */

	/* check if o is an array */
	if ((valuedesc = valuevftbl->arraydesc) == NULL)
		return 0;
	/* {o is an array} */

	return builtin_descriptorscompatible(valuedesc,componentvftbl->arraydesc);
}


/* This is an optimized version where a is guaranteed to be one-dimensional */
s4 builtin_canstore_onedim (java_objectarray *a, java_objectheader *o)
{
	arraydescriptor *desc;
	vftbl *elementvftbl;
	vftbl *valuevftbl;
	int base;
	s4 res;
	
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

	if ((base = elementvftbl->baseval) <= 0)
		/* an array of interface references */
		return (valuevftbl->interfacetablelength > -base &&
				valuevftbl->interfacetable[base] != NULL);
	
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	cast_lock();
#endif

	res = (unsigned)(valuevftbl->baseval - base)
		<= (unsigned)(elementvftbl->diffval);

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	cast_unlock();
#endif

	return res;
}


/* This is an optimized version where a is guaranteed to be a
 * one-dimensional array of a class type */
s4 builtin_canstore_onedim_class(java_objectarray *a, java_objectheader *o)
{
	vftbl *elementvftbl;
	vftbl *valuevftbl;
	s4 res;
	
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

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	cast_lock();
#endif

	res = (unsigned)(valuevftbl->baseval - elementvftbl->baseval)
		<= (unsigned)(elementvftbl->diffval);

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	cast_unlock();
#endif

	return res;
}


/******************** Function: builtin_new **********************************

	Creates a new instance of class c on the heap.
	Return value:  pointer to the object or NULL if no memory is
				   available
			
*****************************************************************************/

#define ALIGNMENT 3
#define align_size(size)	((size + ((1 << ALIGNMENT) - 1)) & ~((1 << ALIGNMENT) - 1))

java_objectheader *builtin_new(classinfo *c)
{
	java_objectheader *o;

	if (initverbose) {
		char logtext[MAXLOGTEXT];
		sprintf(logtext, "Initialize class ");
		utf_sprint(logtext + strlen(logtext), c->name);
		sprintf(logtext + strlen(logtext), " (from builtin_new)");
		log_text(logtext);
	}
	class_init(c);

#ifdef SIZE_FROM_CLASSINFO
	c->alignedsize = align_size(c->instancesize);
	o = heap_allocate(c->alignedsize, true, c->finalizer);
#else
	o = heap_allocate(c->instancesize, true, c->finalizer);
#endif
	if (!o) return NULL;
	
	memset(o, 0, c->instancesize);

	o->vftbl = c->vftbl;

	return o;
}

/********************** Function: builtin_newarray **************************

	Creates an array with the given vftbl on the heap.

	Return value:  pointer to the array or NULL if no memory is available

    CAUTION: The given vftbl must be the vftbl of the *array* class,
    not of the element class.

*****************************************************************************/

java_arrayheader *builtin_newarray(s4 size, vftbl *arrayvftbl)
{
	java_arrayheader *a;
	arraydescriptor *desc = arrayvftbl->arraydesc;
	s4 dataoffset = desc->dataoffset;
	s4 componentsize = desc->componentsize;
	s4 actualsize;

	if (size<0) {
		*exceptionptr=native_new_and_init(loader_load(utf_new_char("java/lang/NegativeArraySizeException")));
		return NULL;
	}
#ifdef SIZE_FROM_CLASSINFO
	actualsize = align_size(dataoffset + size * componentsize);
#else
	actualsize = dataoffset + size * componentsize;
#endif

	if (((u4)actualsize)<((u4)size)) { /* overflow */
		*exceptionptr = native_new_and_init(loader_load(utf_new_char("java/lang/OutOfMemoryError")));
		return NULL;
	}
	a = heap_allocate(actualsize,
					  (desc->arraytype == ARRAYTYPE_OBJECT),
					  NULL);

	if (!a) return NULL;
	memset(a, 0, actualsize);


	/*printf("builtin_newarray: Created an array of size : %d\n",size);*/

	a->objheader.vftbl = arrayvftbl;
	a->size = size;
#ifdef SIZE_FROM_CLASSINFO
	a->alignedsize = actualsize;
#endif

	return a;
}


/********************** Function: builtin_anewarray *************************

	Creates an array of references to the given class type on the heap.

	Return value:  pointer to the array or NULL if no memory is available

    XXX This function does not do The Right Thing, because it uses a
    classinfo pointer at runtime. builtin_newarray should be used
    instead.

*****************************************************************************/

java_objectarray *builtin_anewarray(s4 size, classinfo *component)
{
	return (java_objectarray*) builtin_newarray(size, class_array_of(component)->vftbl);
}


/******************** Function: builtin_newarray_int ***********************

	Creates an array of 32 bit Integers on the heap.

	Return value:  pointer to the array or NULL if no memory is available

*****************************************************************************/

java_intarray *builtin_newarray_int(s4 size)
{
	return (java_intarray*) builtin_newarray(size, primitivetype_table[ARRAYTYPE_INT].arrayvftbl);
}


/******************** Function: builtin_newarray_long ***********************

	Creates an array of 64 bit Integers on the heap.

	Return value:  pointer to the array or NULL if no memory is available

*****************************************************************************/

java_longarray *builtin_newarray_long(s4 size)
{
	return (java_longarray*) builtin_newarray(size, primitivetype_table[ARRAYTYPE_LONG].arrayvftbl);
}


/******************** function: builtin_newarray_float ***********************

	Creates an array of 32 bit IEEE floats on the heap.

	Return value:  pointer to the array or NULL if no memory is available

*****************************************************************************/

java_floatarray *builtin_newarray_float(s4 size)
{
	return (java_floatarray*) builtin_newarray(size, primitivetype_table[ARRAYTYPE_FLOAT].arrayvftbl);
}


/******************** function: builtin_newarray_double ***********************

	Creates an array of 64 bit IEEE floats on the heap.

	Return value:  pointer to the array or NULL if no memory is available

*****************************************************************************/

java_doublearray *builtin_newarray_double(s4 size)
{
	return (java_doublearray*) builtin_newarray(size, primitivetype_table[ARRAYTYPE_DOUBLE].arrayvftbl);
}


/******************** function: builtin_newarray_byte ***********************

	Creates an array of 8 bit Integers on the heap.

	Return value:  pointer to the array or NULL if no memory is available

*****************************************************************************/

java_bytearray *builtin_newarray_byte(s4 size)
{
	return (java_bytearray*) builtin_newarray(size, primitivetype_table[ARRAYTYPE_BYTE].arrayvftbl);
}


/******************** function: builtin_newarray_char ************************

	Creates an array of characters on the heap.

	Return value:  pointer to the array or NULL if no memory is available

*****************************************************************************/

java_chararray *builtin_newarray_char(s4 size)
{
	return (java_chararray*) builtin_newarray(size, primitivetype_table[ARRAYTYPE_CHAR].arrayvftbl);
}


/******************** function: builtin_newarray_short ***********************

	Creates an array of 16 bit Integers on the heap.

	Return value:  pointer to the array or NULL if no memory is available

*****************************************************************************/

java_shortarray *builtin_newarray_short(s4 size)
{
	return (java_shortarray*) builtin_newarray(size, primitivetype_table[ARRAYTYPE_SHORT].arrayvftbl);
}


/******************** function: builtin_newarray_boolean ************************

	Creates an array of bytes on the heap. The array is designated as an array
	of booleans (important for casts)
	
	Return value:  pointer to the array or NULL if no memory is available

*****************************************************************************/

java_booleanarray *builtin_newarray_boolean(s4 size)
{
	return (java_booleanarray*) builtin_newarray(size, primitivetype_table[ARRAYTYPE_BOOLEAN].arrayvftbl);
}


/**************** function: builtin_nmultianewarray ***************************

	Creates a multi-dimensional array on the heap. The dimensions are passed in
	an array of longs.

    Arguments:
        n............number of dimensions to create
        arrayvftbl...vftbl of the array class
        dims.........array containing the size of each dimension to create

	Return value:  pointer to the array or NULL if no memory is available

******************************************************************************/

	/* Helper functions */

java_arrayheader *builtin_nmultianewarray (int n, vftbl *arrayvftbl, long *dims)
{
	int size, i;
	java_arrayheader *a;
	vftbl *componentvftbl;

	/* create this dimension */
	size = (int) dims[0];
	a = builtin_newarray(size,arrayvftbl);
	if (!a) return NULL;

	/* if this is the last dimension return */
	if (!--n) return a;

	/* get the vftbl of the components to create */
	componentvftbl = arrayvftbl->arraydesc->componentvftbl;

	/* The verifier guarantees this. */
	/* if (!componentvftbl) */
	/*	panic ("multianewarray with too many dimensions"); */

	/* create the component arrays */
	for (i = 0; i < size; i++) {
		java_arrayheader *ea = 
			builtin_nmultianewarray(n,componentvftbl,dims+1);
		if (!ea) return NULL;
		((java_objectarray*)a)->data[i] = (java_objectheader *) ea;
	}

	return a;
}


/*****************************************************************************
					  METHOD LOGGING

	Various functions for printing a message at method entry or exit (for
	debugging)
	
*****************************************************************************/


u4 methodindent = 0;

java_objectheader *builtin_trace_exception(java_objectheader *_exceptionptr,
										   methodinfo *method, int *pos, 
										   int noindent)
{
	if (!noindent) {
		if (methodindent)
			methodindent--;
		else
			log_text("WARNING: unmatched methodindent--");
	}
	if (verbose || runverbose) {
		printf("Exception ");
		if (_exceptionptr) {
			utf_display (_exceptionptr->vftbl->class->name);
		}
		else {
			printf("Error: <Nullpointer instead of exception>");
			if (!proto_java_lang_ClassCastException) printf("%s","proto_java_lang_ClassCastException==0");
			if (!proto_java_lang_NullPointerException) printf("%s","proto_java_lang_NullPointerException==0");
			if (!proto_java_lang_NullPointerException) printf("%s","proto_java_lang_NullPointerException==0");
			if (!proto_java_lang_ArrayIndexOutOfBoundsException) printf("%s","proto_java_lang_ArrayIndexOutOfBoundsException==0");
			if (!proto_java_lang_NegativeArraySizeException) printf("%s","proto_java_lang_NegativeArraySizeException==0");
			if (!proto_java_lang_OutOfMemoryError) printf("%s","proto_java_lang_OutOfMemoryError==0");
			if (!proto_java_lang_ArithmeticException) printf("%s","proto_java_lang_ArithmeticException==0");
			if (!proto_java_lang_ArrayStoreException) printf("%s","proto_java_lang_ArrayStoreException==0");
			if (!proto_java_lang_ThreadDeath) printf("%s","proto_java_lang_ThreadDeath==0");
			if (!proto_java_lang_ThreadDeath) printf("%s","proto_java_lang_ThreadDeath==0");

		}
		printf(" thrown in ");
		if (method) {
			utf_display (method->class->name);
			printf(".");
			utf_display (method->name);
			if (method->flags & ACC_SYNCHRONIZED)
				printf("(SYNC)");
			else
				printf("(NOSYNC)");
			printf("(%p) at position %p\n", method->entrypoint, pos);
		}
		else
			printf("call_java_method\n");
		fflush (stdout);
	}
	return _exceptionptr;
}


#ifdef TRACE_ARGS_NUM
void builtin_trace_args(s8 a0, s8 a1, s8 a2, s8 a3, s8 a4, s8 a5,
#if TRACE_ARGS_NUM > 6
						s8 a6, s8 a7,
#endif
						methodinfo *method)
{
	int i;
	char logtext[MAXLOGTEXT];
	for (i = 0; i < methodindent; i++)
		logtext[i] = '\t';
	sprintf(logtext + methodindent, "called: ");
	utf_sprint(logtext + strlen(logtext), method->class->name);
	sprintf(logtext + strlen(logtext), ".");
	utf_sprint(logtext + strlen(logtext), method->name);
	utf_sprint(logtext + strlen(logtext), method->descriptor);
	sprintf(logtext + strlen(logtext), "(");

	switch (method->paramcount) {
	case 0:
		break;

#if defined(__I386__)
	case 1:
		sprintf(logtext+strlen(logtext), "%llx", a0);
		break;

	case 2:
		sprintf(logtext+strlen(logtext), "%llx, %llx", a0, a1);
		break;

	case 3:
		sprintf(logtext+strlen(logtext), "%llx, %llx, %llx", a0, a1, a2);
		break;

	case 4:
		sprintf(logtext+strlen(logtext), "%llx, %llx, %llx, %llx",
				a0,   a1,   a2,   a3);
		break;

	case 5:
		sprintf(logtext+strlen(logtext), "%llx, %llx, %llx, %llx, %llx",
				a0,   a1,   a2,   a3,   a4);
		break;

	case 6:
		sprintf(logtext+strlen(logtext), "%llx, %llx, %llx, %llx, %llx, %llx",
				a0,   a1,   a2,   a3,   a4,   a5);
		break;

#if TRACE_ARGS_NUM > 6
	case 7:
		sprintf(logtext+strlen(logtext), "%llx, %llx, %llx, %llx, %llx, %llx, %llx",
				a0,   a1,   a2,   a3,   a4,   a5,   a6);
		break;

	case 8:
		sprintf(logtext+strlen(logtext), "%llx, %llx, %llx, %llx, %llx, %llx, %llx, %llx",
				a0,   a1,   a2,   a3,   a4,   a5,   a6,   a7);
		break;

	default:
		sprintf(logtext+strlen(logtext), "%llx, %llx, %llx, %llx, %llx, %llx, %llx, %llx, ...(%d)",
				a0,   a1,   a2,   a3,   a4,   a5,   a6,   a7,   method->paramcount - 8);
		break;
#else
	default:
		sprintf(logtext+strlen(logtext), "%llx, %llx, %llx, %llx, %llx, %llx, ...(%d)",
				a0,   a1,   a2,   a3,   a4,   a5,   method->paramcount - 6);
		break;
#endif
#else
	case 1:
		sprintf(logtext+strlen(logtext), "%lx", a0);
		break;

	case 2:
		sprintf(logtext+strlen(logtext), "%lx, %lx", a0, a1);
		break;

	case 3:
		sprintf(logtext+strlen(logtext), "%lx, %lx, %lx", a0, a1, a2);
		break;

	case 4:
		sprintf(logtext+strlen(logtext), "%lx, %lx, %lx, %lx",
				a0,  a1,  a2,  a3);
		break;

	case 5:
		sprintf(logtext+strlen(logtext), "%lx, %lx, %lx, %lx, %lx",
				a0,  a1,  a2,  a3,  a4);
		break;

	case 6:
		sprintf(logtext+strlen(logtext), "%lx, %lx, %lx, %lx, %lx, %lx",
				a0,  a1,  a2,  a3,  a4,  a5);
		break;

#if TRACE_ARGS_NUM > 6
	case 7:
		sprintf(logtext+strlen(logtext), "%lx, %lx, %lx, %lx, %lx, %lx, %lx",
				a0,  a1,  a2,  a3,  a4,  a5,  a6);
		break;

	case 8:
		sprintf(logtext+strlen(logtext), "%lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx",
				a0,  a1,  a2,  a3,  a4,  a5,  a6,  a7);
		break;

	default:
		sprintf(logtext+strlen(logtext), "%lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx, ...(%d)",
				a0,  a1,  a2,  a3,  a4,  a5,  a6,  a7,  method->paramcount - 8);
		break;
#else
	default:
		sprintf(logtext+strlen(logtext), "%lx, %lx, %lx, %lx, %lx, %lx, ...(%d)",
				a0,  a1,  a2,  a3,  a4,  a5,   method->paramcount - 6);
		break;
#endif
#endif
	}
	sprintf (logtext+strlen(logtext), ")");
	log_text(logtext);

	methodindent++;
}
#endif


void builtin_displaymethodstart(methodinfo *method)
{
	char logtext[MAXLOGTEXT];
	sprintf(logtext, "												");
	sprintf(logtext + methodindent, "called: ");
	utf_sprint(logtext + strlen(logtext), method->class->name);
	sprintf(logtext + strlen(logtext), ".");
	utf_sprint(logtext + strlen(logtext), method->name);
	utf_sprint(logtext + strlen(logtext), method->descriptor);
	log_text(logtext);
	methodindent++;
}


void builtin_displaymethodstop(methodinfo *method, s8 l, double d, float f)
{
	int i;
	char logtext[MAXLOGTEXT];
	for (i = 0; i < methodindent; i++)
		logtext[i] = '\t';
	if (methodindent)
		methodindent--;
	else
		log_text("WARNING: unmatched methodindent--");
	sprintf(logtext + methodindent, "finished: ");
	utf_sprint(logtext + strlen(logtext), method->class->name);
	sprintf(logtext + strlen(logtext), ".");
	utf_sprint(logtext + strlen(logtext), method->name);
	utf_sprint(logtext + strlen(logtext), method->descriptor);

	switch (method->returntype) {
	case TYPE_INT:
		sprintf(logtext + strlen(logtext), "->%d", (s4) l);
		break;
	case TYPE_LONG:
#if defined(__I386__)
		sprintf(logtext + strlen(logtext), "->%lld", (s8) l);
#else
		sprintf(logtext + strlen(logtext), "->%ld", (s8) l);
#endif
		break;
	case TYPE_ADDRESS:
#if defined(__I386__)
		sprintf(logtext + strlen(logtext), "->%p", (u1*) ((s4) l));
#else
		sprintf(logtext + strlen(logtext), "->%p", (u1*) l);
#endif
		break;
	case TYPE_FLOAT:
		sprintf(logtext + strlen(logtext), "->%g", f);
		break;
	case TYPE_DOUBLE:
		sprintf(logtext + strlen(logtext), "->%g", d);
		break;
	}
	log_text(logtext);
}


void builtin_displaymethodexception(methodinfo *method)
{
	int i;
	char logtext[MAXLOGTEXT];
	for (i = 0; i < methodindent; i++)
		logtext[i] = '\t';
	sprintf(logtext + methodindent, "exception abort: ");
	utf_sprint(logtext + strlen(logtext), method->class->name);
	sprintf(logtext + strlen(logtext), ".");
	utf_sprint(logtext + strlen(logtext), method->name);
	utf_sprint(logtext + strlen(logtext), method->descriptor);
	log_text(logtext);
}


/****************************************************************************
			 SYNCHRONIZATION FUNCTIONS
*****************************************************************************/

/*
 * Lock the mutex of an object.
 */
void internal_lock_mutex_for_object(java_objectheader *object)
{
#ifdef USE_THREADS
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
#endif
}


/*
 * Unlocks the mutex of an object.
 */
void internal_unlock_mutex_for_object (java_objectheader *object)
{
#ifdef USE_THREADS
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
#endif
}


void builtin_monitorenter(java_objectheader *o)
{
#ifdef USE_THREADS
	int hashValue;

	assert(blockInts == 0);

	++blockInts;

	hashValue = MUTEX_HASH_VALUE(o);
	if (mutexHashTable[hashValue].object == o 
		&& mutexHashTable[hashValue].mutex.holder == currentThread)
		++mutexHashTable[hashValue].mutex.count;
	else
		internal_lock_mutex_for_object(o);

	--blockInts;

	assert(blockInts == 0);
#endif
}


void builtin_monitorexit (java_objectheader *o)
{
#ifdef USE_THREADS
	int hashValue;

	assert(blockInts == 0);

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

	assert(blockInts == 0);
#endif
}


/*****************************************************************************
			  MISCELLANEOUS HELPER FUNCTIONS
*****************************************************************************/



/*********** Functions for integer divisions *****************************
 
	On some systems (eg. DEC ALPHA), integer division is not supported by the
	CPU. These helper functions implement the missing functionality.

******************************************************************************/

s4 builtin_idiv(s4 a, s4 b) { return a / b; }
s4 builtin_irem(s4 a, s4 b) { return a % b; }


/************** Functions for long arithmetics *******************************

	On systems where 64 bit Integers are not supported by the CPU, these
	functions are needed.

******************************************************************************/


s8 builtin_ladd(s8 a, s8 b)
{ 
#if U8_AVAILABLE
	return a + b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lsub(s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	return a - b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lmul(s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	return a * b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_ldiv(s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	return a / b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lrem(s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	return a % b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lshl(s8 a, s4 b) 
{ 
#if U8_AVAILABLE
	return a << (b & 63);
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lshr(s8 a, s4 b) 
{ 
#if U8_AVAILABLE
	return a >> (b & 63);
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lushr(s8 a, s4 b) 
{ 
#if U8_AVAILABLE
	return ((u8) a) >> (b & 63);
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_land(s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	return a & b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lor(s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	return a | b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lxor(s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	return a ^ b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lneg(s8 a) 
{ 
#if U8_AVAILABLE
	return -a;
#else
	return builtin_i2l(0);
#endif
}

s4 builtin_lcmp(s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	if (a < b) return -1;
	if (a > b) return 1;
	return 0;
#else
	return 0;
#endif
}





/*********** Functions for floating point operations *************************/

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


float builtin_fdiv(float a, float b)
{
	if (finitef(a) && finitef(b)) {
		if (b != 0)
			return a / b;
		else {
			if (a > 0)
				return intBitsToFloat(FLT_POSINF);
			else if (a < 0)
				return intBitsToFloat(FLT_NEGINF);
		}
	}
	return intBitsToFloat(FLT_NAN);
}


float builtin_frem(float a, float b)
{
	return fmodf(a, b);
}


float builtin_fneg(float a)
{
	if (isnanf(a)) return a;
	else {
		if (finitef(a)) return -a;
		else return copysignf(a, -copysignf(1.0, a));
	}
}


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



/************************* Functions for doubles ****************************/

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


double builtin_ddiv(double a, double b)
{
	if (finite(a)) {
		if (finite(b)) {
			return a / b;

		} else {
			if (isnan(b))
				return longBitsToDouble(DBL_NAN);
			else
				return copysign(0.0, b);
		}

	} else {
		if (finite(b)) {
			if (a > 0)
				return longBitsToDouble(DBL_POSINF);
			else if (a < 0)
				return longBitsToDouble(DBL_NEGINF);

		} else
			return longBitsToDouble(DBL_NAN);
	}

/*  	if (finite(a) && finite(b)) { */
/*  		if (b != 0) */
/*  			return a / b; */
/*  		else { */
/*  			if (a > 0) */
/*  				return longBitsToDouble(DBL_POSINF); */
/*  			else if (a < 0) */
/*  				return longBitsToDouble(DBL_NEGINF); */
/*  		} */
/*  	} */

	/* keep compiler happy */
	return 0;
}


double builtin_drem(double a, double b)
{
	return fmod(a, b);
}


double builtin_dneg(double a)
{
	if (isnan(a)) return a;
	else {
		if (finite(a)) return -a;
		else return copysign(a, -copysign(1.0, a));
	}
}


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


/*********************** Conversion operations ****************************/

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


float builtin_i2f(s4 a)
{
	float f = (float) a;
	return f;
}


double builtin_i2d(s4 a)
{
	double d = (double) a;
	return d;
}


s4 builtin_l2i(s8 l)
{
#if U8_AVAILABLE
	return (s4) l;
#else
	return l.low;
#endif
}


float builtin_l2f(s8 a)
{
#if U8_AVAILABLE
	float f = (float) a;
	return f;
#else
	return 0.0;
#endif
}


double builtin_l2d(s8 a)
{
#if U8_AVAILABLE
	double d = (double) a;
	return d;
#else
	return 0.0;
#endif
}


s4 builtin_f2i(float a) 
{

	return builtin_d2i((double) a);

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


s8 builtin_f2l(float a)
{

	return builtin_d2l((double) a);

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


/* used to convert FLT_xxx defines into float values */

inline float intBitsToFloat(s4 i)
{
	imm_union imb;

	imb.i = i;
	return imb.f;
}


/* used to convert DBL_xxx defines into double values */

inline float longBitsToDouble(s8 l)
{
	imm_union imb;

	imb.l = l;
	return imb.d;
}


java_arrayheader *builtin_clone_array(void *env, java_arrayheader *o)
{
	return (java_arrayheader *) Java_java_lang_VMObject_clone(0, 0, (java_lang_Cloneable *) o);
}

s4 builtin_dummy()
{
	panic("Internal error: builtin_dummy called (native function is missing)");
	return 0; /* for the compiler */
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
