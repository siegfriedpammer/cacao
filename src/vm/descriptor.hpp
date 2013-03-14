/* src/vm/descriptor.hpp - checking and parsing of field / method descriptors

   Copyright (C) 1996-2013
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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


#ifndef DESCRIPTOR_HPP_
#define DESCRIPTOR_HPP_ 1

/* forward typedefs ***********************************************************/

typedef struct descriptor_pool descriptor_pool;
typedef struct typedesc        typedesc;
typedef struct paramdesc       paramdesc;
typedef struct methoddesc      methoddesc;


#include "config.h"

#include <stdint.h>

#include "vm/types.hpp"

#include "toolbox/hashtable.hpp"

#include "vm/class.hpp"
#include "vm/global.hpp"
#include "vm/method.hpp"
#include "vm/references.h"
#include "vm/utf8.hpp"

#include "arch.h"		/* needed for HAS_ADDRESS_REGISTER_FILE */

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
	constant_classref *classref;      /* class reference for TYPE_ADR types   */
	u1                 type;          /* TYPE_??? constant [1]                */
	u1                 primitivetype; /* (PRIMITIVE)TYPE_??? constant [2]     */
	u1                 arraydim;      /* array dimension (0 if no array)      */
};

/* [1]...the type field contains the basic type used within the VM. So ints,  */
/*       shorts, chars, bytes, booleans all have TYPE_INT.                    */
/* [2]...the primitivetype field contains the declared type.                  */
/*       So short is PRIMITIVETYPE_SHORT, char is PRIMITIVETYPE_CHAR.         */
/*       For non-primitive types primitivetype is TYPE_ADR.                   */

struct paramdesc {
#if defined(__MIPS__)
	u1   type;                  /* TYPE_??? of the register allocated         */
#endif
	bool     inmemory;          /* argument in register or on stack           */
	uint32_t index;             /* index into argument register array         */
	uint32_t regoff;            /* register index or stack offset             */
};

struct methoddesc {
	s2         paramcount;      /* number of parameters                       */
	s2         paramslots;      /* like above but LONG,DOUBLE count twice     */
	s4         argintreguse;    /* number of used integer argument registers  */
	s4         argfltreguse;    /* number of used float argument registers    */
#if defined(HAS_ADDRESS_REGISTER_FILE)
	s4         argadrreguse;    /* number of used address registers */
#endif
	s4         memuse;          /* number of stack slots used                 */
	paramdesc *params;          /* allocated parameter descriptions [3]       */
	Mutex     *pool_lock;       /* synchronizes access to params              */
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

#ifdef __cplusplus

/* function prototypes ********************************************************/

extern "C" {

descriptor_pool * descriptor_pool_new(classinfo *referer);

bool descriptor_pool_add_class(descriptor_pool *pool, Utf8String name);
bool descriptor_pool_add(descriptor_pool *pool,Utf8String desc,int *paramslots);

int  descriptor_to_basic_type(Utf8String desc);
int  descriptor_typesize(typedesc *td);

constant_classref * descriptor_pool_create_classrefs(descriptor_pool *pool,
													 s4 *count);
constant_classref * descriptor_pool_lookup_classref(descriptor_pool *pool,Utf8String classname);

void descriptor_pool_alloc_parsed_descriptors(descriptor_pool *pool);

typedesc *descriptor_pool_parse_field_descriptor(descriptor_pool *pool, Utf8String desc);
methoddesc *descriptor_pool_parse_method_descriptor(descriptor_pool *pool, Utf8String desc, s4 mflags,
													constant_classref *thisclass);

void descriptor_params_from_paramtypes(methoddesc *md, s4 mflags);

void descriptor_pool_get_sizes(descriptor_pool *pool, u4 *classrefsize,
							   u4 *descsize);

#ifndef NDEBUG
void descriptor_debug_print_typedesc(FILE *file,typedesc *d);
void descriptor_debug_print_methoddesc(FILE *file,methoddesc *d);
void descriptor_debug_print_paramdesc(FILE *file,paramdesc *d);
void descriptor_pool_debug_dump(descriptor_pool *pool, FILE *file);
#endif /* !defined(NDEBUG) */

}
#endif /* __cplusplus */

#endif // DESCRIPTOR_HPP_


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
