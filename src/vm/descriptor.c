/* vm/descriptor.c - checking and parsing of field / method descriptors

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

   $Id: descriptor.c 2182 2005-04-01 20:56:33Z edwin $

*/

#include <assert.h>

#include "vm/descriptor.h"
#include "vm/exceptions.h"
#include "vm/resolve.h"


/* constants (private to descriptor.c) ****************************************/

/* initial number of entries for the classrefhash of a descriptor_pool */
/* (currently the hash is never grown!) */
#define CLASSREFHASH_INIT_SIZE  64

/* initial number of entries for the descriptorhash of a descriptor_pool */
/* (currently the hash is never grown!) */
#define DESCRIPTORHASH_INIT_SIZE  128

/* data structures (private to descriptor.c) **********************************/ 

typedef struct classref_hash_entry classref_hash_entry;
typedef struct descriptor_hash_entry descriptor_hash_entry;

/* entry struct for the classrefhash of descriptor_pool */
struct classref_hash_entry {
	classref_hash_entry *hashlink;  /* for hash chaining            */
	utf                 *name;      /* name of the class refered to */
	u2                   index;     /* index into classref table    */
};

/* entry struct for the descriptorhash of descriptor_pool */
struct descriptor_hash_entry {
	descriptor_hash_entry *hashlink;
	utf                   *desc;
	parseddesc             parseddesc;
	s2                     paramslots; /* number of params, LONG/DOUBLE counted as 2 */
};

/****************************************************************************/
/* DEBUG HELPERS                                                            */
/****************************************************************************/

#ifndef NDEBUG
#define DESCRIPTOR_DEBUG
#endif

#ifdef DESCRIPTOR_DEBUG
#define DESCRIPTOR_ASSERT(cond)  assert(cond)
#else
#define DESCRIPTOR_ASSERT(cond)
#endif

/* name_from_descriptor ********************************************************

   Return the class name indicated by the given descriptor
   (Internally used helper function)

   IN:
       c................class containing the descriptor
       utf_ptr..........first character of descriptor
       end_ptr..........first character after the end of the string
       mode.............a combination (binary or) of the following flags:

               (Flags marked with * are the default settings.)

               How to handle "V" descriptors:

			     * DESCRIPTOR_VOID.....handle it like other primitive types
                   DESCRIPTOR_NOVOID...treat it as an error

               How to deal with extra characters after the end of the
               descriptor:

			     * DESCRIPTOR_NOCHECKEND...ignore (useful for parameter lists)
                   DESCRIPTOR_CHECKEND.....treat them as an error

   OUT:
       *next............if non-NULL, *next is set to the first character after
                        the descriptor. (Undefined if an error occurs.)
       *name............set to the utf name of the class

   RETURN VALUE:
       true.............descriptor parsed successfully
	   false............an exception has been thrown

*******************************************************************************/

#define DESCRIPTOR_VOID          0      /* default */
#define DESCRIPTOR_NOVOID        0x0040
#define DESCRIPTOR_NOCHECKEND    0      /* default */
#define DESCRIPTOR_CHECKEND      0x1000

static bool 
name_from_descriptor(classinfo *c,
					 char *utf_ptr, char *end_ptr,
					 char **next, int mode, utf **name)
{
	char *start = utf_ptr;
	bool error = false;

	DESCRIPTOR_ASSERT(c);
	DESCRIPTOR_ASSERT(utf_ptr);
	DESCRIPTOR_ASSERT(end_ptr);
	DESCRIPTOR_ASSERT(name);
	
	*name = NULL;		
	SKIP_FIELDDESCRIPTOR_SAFE(utf_ptr, end_ptr, error);

	if (mode & DESCRIPTOR_CHECKEND)
		error |= (utf_ptr != end_ptr);
	
	if (!error) {
		if (next) *next = utf_ptr;
		
		switch (*start) {
		  case 'V':
			  if (mode & DESCRIPTOR_NOVOID)
				  break;
			  /* FALLTHROUGH! */
		  case 'I':
		  case 'J':
		  case 'F':
		  case 'D':
		  case 'B':
		  case 'C':
		  case 'S':
		  case 'Z':
			  return true;
			  
		  case 'L':
			  start++;
			  utf_ptr--;
			  /* FALLTHROUGH! */
		  case '[':
			  *name = utf_new(start, utf_ptr - start);
			  return true;
		}
	}

	*exceptionptr = new_classformaterror(c,"invalid descriptor");
	return false;
}

/* descriptor_to_typedesc ******************************************************
 
   Parse the given type descriptor and fill a typedesc struct
   (Internally used helper function)

   IN:
       pool.............the descriptor pool
	   utf_ptr..........points to first character of type descriptor
	   end_pos..........points after last character of the whole descriptor

   OUT:
       *next............set to next character after type descriptor
	   *d...............filled with parsed information

   RETURN VALUE:
       true.............parsing succeeded  
	   false............an exception has been thrown

*******************************************************************************/

static bool
descriptor_to_typedesc(descriptor_pool *pool,char *utf_ptr,char *end_pos,
					   char **next,typedesc *d)
{
	utf *name;
	
	if (!name_from_descriptor(pool->referer,utf_ptr,end_pos,next,0,&name))
		return false;

	if (name) {
		/* a reference type */
		d->type = TYPE_ADDRESS;
		d->decltype = TYPE_ADDRESS;
		d->arraydim = 0;
		for (utf_ptr=name->text; *utf_ptr == '['; ++utf_ptr)
			d->arraydim++;
		d->classref = descriptor_pool_lookup_classref(pool,name);
	}
	else {
		/* a primitive type */
		switch (*utf_ptr) {
			case 'B': 
				d->decltype = PRIMITIVETYPE_BYTE;
				goto int_type;
			case 'C':
				d->decltype = PRIMITIVETYPE_CHAR;
				goto int_type;
			case 'S':  
				d->decltype = PRIMITIVETYPE_SHORT;
				goto int_type;
			case 'Z':
				d->decltype = PRIMITIVETYPE_BOOLEAN;
				goto int_type;
			case 'I':
				d->decltype = PRIMITIVETYPE_INT;
				/* FALLTHROUGH */
int_type:
				d->type = TYPE_INT;
				break;
			case 'D':
				d->decltype = PRIMITIVETYPE_DOUBLE;
				d->type = TYPE_DOUBLE;
				break;
			case 'F':
				d->decltype = PRIMITIVETYPE_FLOAT;
				d->type = TYPE_FLOAT;
				break;
			case 'J':
				d->decltype = PRIMITIVETYPE_LONG;
				d->type = TYPE_LONG;
				break;
			case 'V':
				d->decltype = PRIMITIVETYPE_VOID;
				d->type = TYPE_VOID;
				break;
			default:
				DESCRIPTOR_ASSERT(false);
		}
		d->arraydim = 0;
		d->classref = NULL;
	}

	return true;
}

/* descriptor_pool_new *********************************************************
 
   Allocate a new descriptor_pool

   IN:
       referer..........class for which to create the pool

   RETURN VALUE:
       a pointer to the new descriptor_pool

*******************************************************************************/

descriptor_pool * 
descriptor_pool_new(classinfo *referer)
{
	descriptor_pool *pool;
	u4 hashsize;
	u4 slot;

	pool = DNEW(descriptor_pool);
	DESCRIPTOR_ASSERT(pool);

	pool->referer = referer;
	pool->fieldcount = 0;
	pool->methodcount = 0;
	pool->paramcount = 0;
	pool->descriptorsize = 0;
	pool->descriptors = NULL;
	pool->descriptors_next = NULL;
	pool->classrefs = NULL;
	pool->descriptor_kind = NULL;
	pool->descriptor_kind_next = NULL;

	hashsize = CLASSREFHASH_INIT_SIZE;
	pool->classrefhash.size = hashsize;
	pool->classrefhash.entries = 0;
	pool->classrefhash.ptr = DMNEW(voidptr,hashsize);
	for (slot=0; slot<hashsize; ++slot)
		pool->classrefhash.ptr[slot] = NULL;

	hashsize = DESCRIPTORHASH_INIT_SIZE;
	pool->descriptorhash.size = hashsize;
	pool->descriptorhash.entries = 0;
	pool->descriptorhash.ptr = DMNEW(voidptr,hashsize);
	for (slot=0; slot<hashsize; ++slot)
		pool->descriptorhash.ptr[slot] = NULL;

	return pool;
}

/* descriptor_pool_add_class ***************************************************
 
   Add the given class reference to the pool

   IN:
       pool.............the descriptor_pool
	   name.............the class reference to add

   RETURN VALUE:
       true.............reference has been added
	   false............an exception has been thrown

*******************************************************************************/

bool 
descriptor_pool_add_class(descriptor_pool *pool,utf *name)
{
	u4 key,slot;
	classref_hash_entry *c;
	
	DESCRIPTOR_ASSERT(pool);
	DESCRIPTOR_ASSERT(name);

	/* find a place in the hashtable */
	key = utf_hashkey(name->text, name->blength);
	slot = key & (pool->classrefhash.size - 1);
	c = (classref_hash_entry *) pool->classrefhash.ptr[slot];
	while (c) {
		if (c->name == name)
			return true; /* already stored */
		c = c->hashlink;
	}

	/* check if the name is a valid classname */
	if (!is_valid_name(name->text,utf_end(name))) {
		*exceptionptr = new_classformaterror(pool->referer,"Invalid class name");
		return false; /* exception */
	}

	/* XXX check maximum array dimension */
	
	c = DNEW(classref_hash_entry);
	c->name = name;
	c->index = pool->classrefhash.entries++;
	c->hashlink = (classref_hash_entry *) pool->classrefhash.ptr[slot];
	pool->classrefhash.ptr[slot] = c;

	return true;
}

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

bool 
descriptor_pool_add(descriptor_pool *pool,utf *desc,int *paramslots)
{
	u4 key,slot;
	descriptor_hash_entry *c;
	char *utf_ptr;
	char *end_pos;
	utf *name;
	s4 argcount = 0;
	
	DESCRIPTOR_ASSERT(pool);
	DESCRIPTOR_ASSERT(desc);

	/* find a place in the hashtable */
	key = utf_hashkey(desc->text, desc->blength);
	slot = key & (pool->descriptorhash.size - 1);
	c = (descriptor_hash_entry *) pool->descriptorhash.ptr[slot];
	while (c) {
		if (c->desc == desc) {
			if (paramslots)
				*paramslots = c->paramslots;
			return true; /* already stored */
		}
		c = c->hashlink;
	}
	/* add the descriptor to the pool */
	c = DNEW(descriptor_hash_entry);
	c->desc = desc;
	c->parseddesc.any = NULL;
	c->hashlink = (descriptor_hash_entry *) pool->descriptorhash.ptr[slot];
	pool->descriptorhash.ptr[slot] = c;

	/* now check the descriptor */
	utf_ptr = desc->text;
	end_pos = utf_end(desc);
	
	if (*utf_ptr == '(') {
		/* a method descriptor */
		pool->methodcount++;
		utf_ptr++;

		/* check arguments */
		while (utf_ptr != end_pos && *utf_ptr != ')') {
			pool->paramcount++;
			/* We cannot count the this argument here because
			 * we don't know if the method is static. */
			if (*utf_ptr == 'J' || *utf_ptr == 'D')
				argcount+=2;
			else
				argcount++;
			if (!name_from_descriptor(pool->referer,utf_ptr,end_pos,&utf_ptr,
								      DESCRIPTOR_NOVOID,&name))
				return false;

			if (name)
				descriptor_pool_add_class(pool,name);
		}

		if (utf_ptr == end_pos) {
			*exceptionptr = new_classformaterror(pool->referer,"Missing ')' in method descriptor");
			return false;
		}

		utf_ptr++; /* skip ')' */

		if (!name_from_descriptor(pool->referer,utf_ptr,end_pos,NULL,
							  	  DESCRIPTOR_CHECKEND,&name))
			return false;

		if (name)
			descriptor_pool_add_class(pool,name);

		if (argcount > 255) {
			*exceptionptr =
				new_classformaterror(pool->referer,"Too many arguments in signature");
			return false;
		}
	}
	else {
		/* a field descriptor */
		pool->fieldcount++;
		
	    if (!name_from_descriptor(pool->referer,utf_ptr,end_pos,NULL,
						    	  DESCRIPTOR_NOVOID
						  		| DESCRIPTOR_CHECKEND,&name))
			return false;

		if (name)
			descriptor_pool_add_class(pool,name);
	}

	c->paramslots = argcount;
	if (paramslots)
		*paramslots = argcount;

	return true;
}

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

constant_classref * 
descriptor_pool_create_classrefs(descriptor_pool *pool,s4 *count)
{
	u4 nclasses;
	u4 slot;
	classref_hash_entry *c;
	constant_classref *ref;
	
	DESCRIPTOR_ASSERT(pool);

	nclasses = pool->classrefhash.entries;
	pool->classrefs = MNEW(constant_classref,nclasses);

	/* fill the constant_classref structs */
	for (slot=0; slot<pool->classrefhash.size; ++slot) {
		c = (classref_hash_entry *) pool->classrefhash.ptr[slot];
		while (c) {
			ref = pool->classrefs + c->index;
			CLASSREF_INIT(*ref,pool->referer,c->name);
			c = c->hashlink;
		}
	}

	if (count)
		*count = nclasses;
	return pool->classrefs;
}

/* descriptor_pool_lookup_classref *********************************************
 
   Return the constant_classref for the given class name

   IN:
       pool.............the descriptor_pool
	   classname........name of the class to look up

   RETURN VALUE:
       a pointer to the constant_classref, or
	   NULL if an exception has been thrown

*******************************************************************************/

constant_classref * 
descriptor_pool_lookup_classref(descriptor_pool *pool,utf *classname)
{
	u4 key,slot;
	classref_hash_entry *c;

	DESCRIPTOR_ASSERT(pool);
	DESCRIPTOR_ASSERT(pool->classrefs);
	DESCRIPTOR_ASSERT(classname);

	key = utf_hashkey(classname->text, classname->blength);
	slot = key & (pool->classrefhash.size - 1);
	c = (classref_hash_entry *) pool->classrefhash.ptr[slot];
	while (c) {
		if (c->name == classname)
			return pool->classrefs + c->index;
		c = c->hashlink;
	}

	*exceptionptr = new_exception_message(string_java_lang_InternalError,
			 							  "Class reference not found in descriptor pool");
	return NULL;
}

/* descriptor_pool_alloc_parsed_descriptors ************************************
 
   Allocate space for the parsed descriptors

   IN:
       pool.............the descriptor_pool

   NOTE:
       This function must be called after all descriptors have been added
	   with descriptor_pool_add.

*******************************************************************************/

void 
descriptor_pool_alloc_parsed_descriptors(descriptor_pool *pool)
{
	u4 size;
	
	DESCRIPTOR_ASSERT(pool);

	size = pool->fieldcount * sizeof(typedesc)
		 + pool->methodcount * (sizeof(methoddesc) - sizeof(typedesc))
		 + pool->paramcount * sizeof(typedesc);

	pool->descriptorsize = size;
	if (size) {
		pool->descriptors = MNEW(u1,size);
		pool->descriptors_next = pool->descriptors;
	}

	size = pool->fieldcount + pool->methodcount;
	if (size) {
		pool->descriptor_kind = DMNEW(u1,size);
		pool->descriptor_kind_next = pool->descriptor_kind;
	}
}

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

typedesc * 
descriptor_pool_parse_field_descriptor(descriptor_pool *pool,utf *desc)
{
	u4 key,slot;
	descriptor_hash_entry *c;
	typedesc *d;

	DESCRIPTOR_ASSERT(pool);
	DESCRIPTOR_ASSERT(pool->descriptors);
	DESCRIPTOR_ASSERT(pool->descriptors_next);

	/* lookup the descriptor in the hashtable */
	key = utf_hashkey(desc->text, desc->blength);
	slot = key & (pool->descriptorhash.size - 1);
	c = (descriptor_hash_entry *) pool->descriptorhash.ptr[slot];
	while (c) {
		if (c->desc == desc) {
			/* found */
			if (c->parseddesc.fd)
				return c->parseddesc.fd;
			break;
		}
		c = c->hashlink;
	}

	DESCRIPTOR_ASSERT(c);
	
	if (desc->text[0] == '(') {
		*exceptionptr = new_classformaterror(pool->referer,
				"Method descriptor used in field reference");
		return NULL;
	}

	d = (typedesc *) pool->descriptors_next;
	pool->descriptors_next += sizeof(typedesc);
	
	if (!descriptor_to_typedesc(pool,desc->text,utf_end(desc),NULL,d))
		return NULL;

	*(pool->descriptor_kind_next++) = 'f';

	c->parseddesc.fd = d;
	return d;
}

/* descriptor_pool_parse_method_descriptor *************************************
 
   Parse the given method descriptor

   IN:
       pool.............the descriptor_pool
	   desc.............the method descriptor

   RETURN VALUE:
       a pointer to the parsed method descriptor, or
	   NULL if an exception has been thrown

   NOTE:
       descriptor_pool_alloc_parsed_descriptors must be called (once) before this
	   function is used.

*******************************************************************************/

methoddesc * 
descriptor_pool_parse_method_descriptor(descriptor_pool *pool,utf *desc)
{
	u4 key,slot;
	descriptor_hash_entry *c;
	typedesc *d;
	methoddesc *md;
	char *utf_ptr;
	char *end_pos;
	s2 paramcount = 0;
	s2 paramslots = 0;

	DESCRIPTOR_ASSERT(pool);
	DESCRIPTOR_ASSERT(pool->descriptors);
	DESCRIPTOR_ASSERT(pool->descriptors_next);

	/* lookup the descriptor in the hashtable */
	key = utf_hashkey(desc->text, desc->blength);
	slot = key & (pool->descriptorhash.size - 1);
	c = (descriptor_hash_entry *) pool->descriptorhash.ptr[slot];
	while (c) {
		if (c->desc == desc) {
			/* found */
			if (c->parseddesc.md)
				return c->parseddesc.md;
			break;
		}
		c = c->hashlink;
	}

	DESCRIPTOR_ASSERT(c);
	
	md = (methoddesc *) pool->descriptors_next;
	pool->descriptors_next += sizeof(methoddesc) - sizeof(typedesc);

	utf_ptr = desc->text;
	end_pos = utf_end(desc);

	if (*utf_ptr++ != '(') {
		*exceptionptr = new_classformaterror(pool->referer,
				"Field descriptor used in method reference");
		return NULL;
	}

	d = md->paramtypes;
	while (*utf_ptr != ')') {
		/* parse a parameter type */
		if (!descriptor_to_typedesc(pool,utf_ptr,end_pos,&utf_ptr,d))
			return NULL;

		if (d->type == TYPE_LONG || d->type == TYPE_DOUBLE)
			paramslots++;
		
		d++;
		pool->descriptors_next += sizeof(typedesc);
		paramcount++;
		paramslots++;
	}
	utf_ptr++; /* skip ')' */
	
	/* parse return type */
	if (!descriptor_to_typedesc(pool,utf_ptr,end_pos,NULL,&(md->returntype)))
			return NULL;

	md->paramcount = paramcount;
	md->paramslots = paramslots;
	*(pool->descriptor_kind_next++) = 'm';
	c->parseddesc.md = md;
	return md;
}

/* descriptor_pool_get_parsed_descriptors **************************************
 
   Return a pointer to the block of parsed descriptors

   IN:
       pool.............the descriptor_pool

   OUT:
   	   *size............if size is non-NULL, this is set to the size of the
	                    parsed descriptor block (in u1)

   RETURN VALUE:
       a pointer to the block of parsed descriptors

   NOTE:
       descriptor_pool_alloc_parsed_descriptors must be called (once) before this
	   function is used.

*******************************************************************************/

void * 
descriptor_pool_get_parsed_descriptors(descriptor_pool *pool,s4 *size)
{
	DESCRIPTOR_ASSERT(pool);
	DESCRIPTOR_ASSERT((!pool->fieldcount && !pool->methodcount) || pool->descriptors);
	
	if (size)
		*size = pool->descriptorsize;
	return pool->descriptors;
}

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

void 
descriptor_pool_get_sizes(descriptor_pool *pool,
					      u4 *classrefsize,u4 *descsize)
{
	DESCRIPTOR_ASSERT(pool);
	DESCRIPTOR_ASSERT((!pool->fieldcount && !pool->methodcount) || pool->descriptors);
	DESCRIPTOR_ASSERT(pool->classrefs);
	DESCRIPTOR_ASSERT(classrefsize);
	DESCRIPTOR_ASSERT(descsize);

	*classrefsize = pool->classrefhash.entries * sizeof(constant_classref);
	*descsize = pool->descriptorsize;
}

/* descriptor_debug_print_typedesc *********************************************
 
   Print the given typedesc to the given stream

   IN:
	   file.............stream to print to
	   d................the parsed descriptor

*******************************************************************************/

void 
descriptor_debug_print_typedesc(FILE *file,typedesc *d)
{
	int ch;

	if (!d) {
		fprintf(file,"(typedesc *)NULL");
		return;
	}
	
	if (d->type == TYPE_ADDRESS) {
		utf_fprint(file,d->classref->name);
	}
	else {
		switch (d->decltype) {
			case PRIMITIVETYPE_INT    : ch='I'; break;
			case PRIMITIVETYPE_CHAR   : ch='C'; break;
			case PRIMITIVETYPE_BYTE   : ch='B'; break;
			case PRIMITIVETYPE_SHORT  : ch='S'; break;
			case PRIMITIVETYPE_BOOLEAN: ch='Z'; break;
			case PRIMITIVETYPE_LONG   : ch='J'; break;
			case PRIMITIVETYPE_FLOAT  : ch='F'; break;
			case PRIMITIVETYPE_DOUBLE : ch='D'; break;
			case PRIMITIVETYPE_VOID   : ch='V'; break;
			default                   : ch='!';
		}
		fputc(ch,file);
	}
	if (d->arraydim)
		fprintf(file,"[%d]",d->arraydim);
}

/* descriptor_debug_print_methoddesc *******************************************
 
   Print the given methoddesc to the given stream

   IN:
	   file.............stream to print to
	   d................the parsed descriptor

*******************************************************************************/

void 
descriptor_debug_print_methoddesc(FILE *file,methoddesc *d)
{
	int i;
	
	if (!d) {
		fprintf(file,"(methoddesc *)NULL");
		return;
	}
	
	fputc('(',file);
	for (i=0; i<d->paramcount; ++i) {
		if (i)
			fputc(',',file);
		descriptor_debug_print_typedesc(file,d->paramtypes + i);
	}
	fputc(')',file);
	descriptor_debug_print_typedesc(file,&(d->returntype));
}

/* descriptor_pool_debug_dump **************************************************
 
   Print the state of the descriptor_pool to the given stream

   IN:
       pool.............the descriptor_pool
	   file.............stream to print to

*******************************************************************************/

void 
descriptor_pool_debug_dump(descriptor_pool *pool,FILE *file)
{
	u4 slot;
	u1 *pos;
	u1 *kind;
	u4 size;
	
	fprintf(file,"======[descriptor_pool for ");
	utf_fprint(file,pool->referer->name);
	fprintf(file,"]======\n");

	fprintf(file,"fieldcount:     %d\n",pool->fieldcount);
	fprintf(file,"methodcount:    %d\n",pool->methodcount);
	fprintf(file,"paramcount:     %d\n",pool->paramcount);
	fprintf(file,"classrefcount:  %d\n",pool->classrefhash.entries);
	fprintf(file,"descriptorsize: %d bytes\n",pool->descriptorsize);
	fprintf(file,"classrefsize:   %d bytes\n",
			(int)pool->classrefhash.entries * sizeof(constant_classref));

	fprintf(file,"class references:\n");
	for (slot=0; slot<pool->classrefhash.size; ++slot) {
		classref_hash_entry *c = (classref_hash_entry *) pool->classrefhash.ptr[slot];
		while (c) {
			fprintf(file,"    %4d: ",c->index);
			utf_fprint(file,c->name);
			fprintf(file,"\n");
			c = c->hashlink;
		}
	}

	fprintf(file,"hashed descriptors:\n");
	for (slot=0; slot<pool->descriptorhash.size; ++slot) {
		descriptor_hash_entry *c = (descriptor_hash_entry *) pool->descriptorhash.ptr[slot];
		while (c) {
			fprintf(file,"    %p: ",c->parseddesc.any);
			utf_fprint(file,c->desc);
			fprintf(file,"\n");
			c = c->hashlink;
		}
	}

	fprintf(file,"descriptors:\n");
	if (pool->descriptors) {
		pos = pool->descriptors;
		size = pool->descriptors_next - pool->descriptors;
		fprintf(file,"    size: %d bytes\n",size);
		
		if (pool->descriptor_kind) {
			kind = pool->descriptor_kind;

			while (pos < (pool->descriptors + size)) {
				fprintf(file,"    %p: ",pos);
				switch (*kind++) {
					case 'f':
						descriptor_debug_print_typedesc(file,(typedesc*)pos);
						pos += sizeof(typedesc);
						break;
					case 'm':
						descriptor_debug_print_methoddesc(file,(methoddesc*)pos);
						pos += ((methoddesc*)pos)->paramcount * sizeof(typedesc);
						pos += sizeof(methoddesc) - sizeof(typedesc);
						break;
					default:
						fprintf(file,"INVALID KIND");
				}
				fputc('\n',file);
			}
		}
		else {
			while (size >= sizeof(voidptr)) {
				fprintf(file,"    %p\n",*((voidptr*)pos));
				pos += sizeof(voidptr);
				size -= sizeof(voidptr);
			}
		}
	}

	fprintf(file,"==========================================================\n");
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
 * vim:noexpandtab:sw=4:ts=4:
 */

