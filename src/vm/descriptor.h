/* vm/descriptor.h - checking and parsing of field / method descriptors

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

   Authors: Edwin Steiner

   Changes:

   $Id: descriptor.h 2744 2005-06-20 11:59:14Z edwin $

*/


#ifndef _DESCRIPTOR_H
#define _DESCRIPTOR_H

/* forward typedefs ***********************************************************/

typedef struct descriptor_pool descriptor_pool;
typedef struct paramdesc paramdesc;

#include "vm/class.h"
#include "vm/global.h"
#include "vm/method.h"
#include "vm/references.h"
#include "vm/tables.h"


/* data structures ************************************************************/ 

/*----------------------------------------------------------------------------*/
/* Descriptor Pools                                                           */
/*                                                                            */
/* A descriptor_pool is a temporary data structure used during loading of     */
/* a class. The descriptor_pool is used to allocate the table of              */
/* constant_classrefs the class uses, and for parsing the field and method    */
/* descriptors which occurr within the class. The inner workings of           */
/* descriptor_pool are not important for outside code.                        */
/*                                                                            */
/* You use a descriptor_pool as follows:                                      */
/*                                                                            */
/* 1. create one with descriptor_pool_new                                     */
/* 2. add all explicit class references with descriptor_pool_add_class        */
/* 3. add all field/method descriptors with descriptor_pool_add               */
/* 4. call descriptor_pool_create_classrefs                                   */
/*    You can now lookup classrefs with descriptor_pool_lookup_classref       */
/* 5. call descriptor_pool_alloc_parsed_descriptors                           */
/* 6. for each field descriptor call descriptor_pool_parse_field_descriptor   */
/*    for each method descriptor call descriptor_pool_parse_method_descriptor */
/* 7. call descriptor_pool_get_parsed_descriptors                             */
/*                                                                            */
/* IMPORTANT: The descriptor_pool functions use DNEW and DMNEW for allocating */
/*            memory which can be thrown away when the steps above have been  */
/*            done.                                                           */
/*----------------------------------------------------------------------------*/

struct descriptor_pool {
	classinfo         *referer;
	u4                 fieldcount;
	u4                 methodcount;
	u4                 paramcount;
	u4                 descriptorsize;
	u1                *descriptors;
	u1                *descriptors_next;
	hashtable          descriptorhash;
	constant_classref *classrefs;
	hashtable          classrefhash;
	u1                *descriptor_kind;       /* useful for debugging */
	u1                *descriptor_kind_next;  /* useful for debugging */
};


/* data structures for parsed field/method descriptors ************************/

struct typedesc {
	constant_classref *classref;   /* class reference for TYPE_ADR types      */
	u1                 type;       /* TYPE_??? constant [1]                   */
	u1                 decltype;   /* (PRIMITIVE)TYPE_??? constant [2]        */
	u1                 arraydim;   /* array dimension (0 if no array)         */
};

/* [1]...the type field contains the basic type used within the VM. So ints,  */
/*       shorts, chars, bytes, booleans all have TYPE_INT.                    */
/* [2]...the decltype field contains the declared type.                       */
/*       So short is PRIMITIVETYPE_SHORT, char is PRIMITIVETYPE_CHAR.         */
/*       For non-primitive types decltype is TYPE_ADR.                        */

struct paramdesc {
	bool inmemory;              /* argument in register or on stack           */
	s4   regoff;                /* register index or stack offset             */
};

struct methoddesc {
	s2         paramcount;      /* number of parameters                       */
	s2         paramslots;      /* like above but LONG,DOUBLE count twice     */
	s4         argintreguse;    /* number of used integer argument registers  */
	s4         argfltreguse;    /* number of used float argument registers    */
	s4         memuse;          /* number of stack slots used                 */
	paramdesc *params;          /* allocated parameter descriptions [3]       */
	typedesc   returntype;      /* parsed descriptor of the return type       */
	typedesc   paramtypes[1];   /* parameter types, variable length!          */
};

/* [3]...If params is NULL, the parameter descriptions have not yet been      */
/*       allocated. In this case ___the possible 'this' pointer of the method */
/*       is NOT counted in paramcount/paramslots and it is NOT included in    */
/*       the paramtypes array___.                                             */
/*       If params != NULL, the parameter descriptions have been              */
/*       allocated, and the 'this' pointer of the method, if any, IS included.*/
/*       In case the method has no parameters at all, the special value       */
/*       METHODDESC_NO_PARAMS is used (see below).                            */

/* METHODDESC_NO_PARAMS is a special value for the methoddesc.params field    */
/* indicating that the method is a static method without any parameters.      */
/* This special value must be != NULL and it may only be set if               */
/* md->paramcount == 0.                                                       */

#define METHODDESC_NOPARAMS  ((paramdesc*)1)

/* function prototypes ********************************************************/

/* descriptor_debug_print_typedesc *********************************************
 
   Print the given typedesc to the given stream

   IN:
	   file.............stream to print to
	   d................the parsed descriptor

*******************************************************************************/

void descriptor_debug_print_typedesc(FILE *file,typedesc *d);


/* descriptor_debug_print_methoddesc *******************************************
 
   Print the given methoddesc to the given stream

   IN:
	   file.............stream to print to
	   d................the parsed descriptor

*******************************************************************************/

void descriptor_debug_print_methoddesc(FILE *file,methoddesc *d);


/* descriptor_debug_print_paramdesc ********************************************
 
   Print the given paramdesc to the given stream

   IN:
	   file.............stream to print to
	   d................the parameter descriptor

*******************************************************************************/

void descriptor_debug_print_paramdesc(FILE *file,paramdesc *d);

/* descriptor_pool_new *********************************************************
 
   Allocate a new descriptor_pool

   IN:
       referer..........class for which to create the pool

   RETURN VALUE:
       a pointer to the new descriptor_pool

*******************************************************************************/

descriptor_pool * descriptor_pool_new(classinfo *referer);


/* descriptor_pool_add_class ***************************************************
 
   Add the given class reference to the pool

   IN:
       pool.............the descriptor_pool
	   name.............the class reference to add

   RETURN VALUE:
       true.............reference has been added
	   false............an exception has been thrown

*******************************************************************************/

bool descriptor_pool_add_class(descriptor_pool *pool,utf *name);


/* descriptor_pool_add *********************************************************
 
   Check the given descriptor and add it to the pool

   IN:
       pool.............the descriptor_pool
	   desc.............the descriptor to add. Maybe a field or method desc.

   OUT:
       *paramslots......if non-NULL, set to the number of parameters.
	                    LONG and DOUBLE are counted twice

   RETURN VALUE:
       true.............descriptor has been added
	   false............an exception has been thrown

*******************************************************************************/

bool descriptor_pool_add(descriptor_pool *pool,utf *desc,int *paramslots);


/* descriptor_pool_create_classrefs ********************************************
 
   Create a table containing all the classrefs which were added to the pool

   IN:
       pool.............the descriptor_pool

   OUT:
       *count...........if count is non-NULL, this is set to the number
	                    of classrefs in the table

   RETURN VALUE:
       a pointer to the constant_classref table

*******************************************************************************/

constant_classref * descriptor_pool_create_classrefs(descriptor_pool *pool,
													 s4 *count);


/* descriptor_pool_lookup_classref *********************************************
 
   Return the constant_classref for the given class name

   IN:
       pool.............the descriptor_pool
	   classname........name of the class to look up

   RETURN VALUE:
       a pointer to the constant_classref, or
	   NULL if an exception has been thrown

*******************************************************************************/

constant_classref * descriptor_pool_lookup_classref(descriptor_pool *pool,utf *classname);


/* descriptor_pool_alloc_parsed_descriptors ************************************
 
   Allocate space for the parsed descriptors

   IN:
       pool.............the descriptor_pool

   NOTE:
       This function must be called after all descriptors have been added
	   with descriptor_pool_add.

*******************************************************************************/

void descriptor_pool_alloc_parsed_descriptors(descriptor_pool *pool);


/* descriptor_pool_parse_field_descriptor **************************************
 
   Parse the given field descriptor

   IN:
       pool.............the descriptor_pool
	   desc.............the field descriptor

   RETURN VALUE:
       a pointer to the parsed field descriptor, or
	   NULL if an exception has been thrown

   NOTE:
       descriptor_pool_alloc_parsed_descriptors must be called (once) before this
	   function is used.

*******************************************************************************/

typedesc *descriptor_pool_parse_field_descriptor(descriptor_pool *pool, utf *desc);


/* descriptor_pool_parse_method_descriptor *************************************
 
   Parse the given method descriptor

   IN:
       pool.............the descriptor_pool
       desc.............the method descriptor
       mflags...........the method flags
	   thisclass........classref to the class containing the method.
	   					This is ignored if mflags contains ACC_STATIC.
						The classref is stored for inserting the 'this' argument.

   RETURN VALUE:
       a pointer to the parsed method descriptor, or
	   NULL if an exception has been thrown

   NOTE:
       descriptor_pool_alloc_parsed_descriptors must be called (once) before this
	   function is used.

*******************************************************************************/

methoddesc *descriptor_pool_parse_method_descriptor(descriptor_pool *pool, utf *desc, s4 mflags,
													constant_classref *thisclass);

/* descriptor_params_from_paramtypes *******************************************
 
   Create the paramdescs for a method descriptor. This function is called
   when we know whether the method is static or not. This function may only
   be called once for each methoddesc, and only if md->params == NULL.

   IN:
       md...............the parsed method descriptor
	                    md->params MUST be NULL.
	   mflags...........the ACC_* access flags of the method. Only the
	                    ACC_STATIC bit is checked.
						The value ACC_UNDEF is NOT allowed.

   RETURN VALUE:
       true.............the paramdescs were created successfully
	   false............an exception has been thrown

   POSTCONDITION:
       md->parms != NULL

*******************************************************************************/

bool descriptor_params_from_paramtypes(methoddesc *md, s4 mflags);

/* descriptor_pool_get_parsed_descriptors **************************************
 
   Return a pointer to the block of parsed descriptors

   IN:
       pool.............the descriptor_pool

   OUT:
   	   *size............if size is non-NULL, this is set to the size of the
	                    parsed descriptor block (in u1)

   RETURN VALUE:
       a pointer to the block of parsed descriptors,
	   NULL if there are no descriptors in the pool

   NOTE:
       descriptor_pool_alloc_parsed_descriptors must be called (once) before this
	   function is used.

*******************************************************************************/

void *descriptor_pool_get_parsed_descriptors(descriptor_pool *pool, s4 *size);


/* descriptor_pool_get_sizes ***************************************************
 
   Get the sizes of the class reference table and the parsed descriptors

   IN:
       pool.............the descriptor_pool

   OUT:
       *classrefsize....set to size of the class reference table
	   *descsize........set to size of the parsed descriptors

   NOTE:
       This function may only be called after both
	       descriptor_pool_create_classrefs, and
		   descriptor_pool_alloc_parsed_descriptors
	   have been called.

*******************************************************************************/

void descriptor_pool_get_sizes(descriptor_pool *pool, u4 *classrefsize,
							   u4 *descsize);


/* descriptor_debug_print_typedesc *********************************************
 
   Print the given typedesc to the given stream

   IN:
	   file.............stream to print to
	   d................the parsed descriptor

*******************************************************************************/

void descriptor_debug_print_typedesc(FILE *file, typedesc *d);


/* descriptor_debug_print_methoddesc *******************************************
 
   Print the given methoddesc to the given stream

   IN:
	   file.............stream to print to
	   d................the parsed descriptor

*******************************************************************************/

void descriptor_debug_print_methoddesc(FILE *file, methoddesc *d);


/* descriptor_pool_debug_dump **************************************************
 
   Print the state of the descriptor_pool to the given stream

   IN:
       pool.............the descriptor_pool
	   file.............stream to print to

*******************************************************************************/

void descriptor_pool_debug_dump(descriptor_pool *pool, FILE *file);

#endif /* _DESCRIPTOR_H */


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
