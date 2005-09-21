/* src/vm/tables.c - 

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

   Contains support functions for:
       - Reading of Java class files
       - Unicode symbols
       - the heap
       - additional support functions

   $Id: tables.c 3239 2005-09-21 14:09:22Z twisti $

*/


#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

#include "config.h"
#include "vm/types.h"

#include "mm/memory.h"
#include "native/native.h"
#include "toolbox/logging.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/statistics.h"
#include "vm/stringlocal.h"
#include "vm/tables.h"
#include "vm/classcache.h"


hashtable string_hash;  /* hashtable for javastrings  */


/******************************************************************************
 *********************** hashtable functions **********************************
 ******************************************************************************/

/* hashsize must be power of 2 */

#define UTF_HASHSTART   16384   /* initial size of utf-hash */    
#define HASHSTART        2048   /* initial size of javastring and class-hash */


/******************** function: init_hashtable ******************************

    Initializes a hashtable structure and allocates memory.
    The parameter size specifies the initial size of the hashtable.
	
*****************************************************************************/

void init_hashtable(hashtable *hash, u4 size)
{
	u4 i;

	hash->entries = 0;
	hash->size    = size;
	hash->ptr     = MNEW(void*, size);

	/* clear table */
	for (i = 0; i < size; i++) hash->ptr[i] = NULL;
}


/*********************** function: tables_init  *****************************

    creates hashtables for symboltables 
	(called once at startup)			 
	
*****************************************************************************/

void tables_init()
{
	init_hashtable(&utf_hash,    UTF_HASHSTART);  /* hashtable for utf8-symbols */
	init_hashtable(&string_hash, HASHSTART);      /* hashtable for javastrings */

	classcache_init();

/*  	if (opt_eager) */
/*  		list_init(&unlinkedclasses, OFFSET(classinfo, listnode)); */

#if defined(STATISTICS)
	if (opt_stat)
		count_utf_len += sizeof(utf*) * utf_hash.size;
#endif
}


/********************** function: tables_close ******************************

        free memory for hashtables		      
	
*****************************************************************************/

void tables_close()
{
	utf *u = NULL;
	literalstring *s;
	u4 i;

	classcache_free();
	
	/* dispose utf symbols */
	for (i = 0; i < utf_hash.size; i++) {
		u = utf_hash.ptr[i];
		while (u) {
			/* process elements in external hash chain */
			utf *nextu = u->hashlink;
			MFREE(u->text, u1, u->blength);
			FREE(u, utf);
			u = nextu;
		}	
	}

	/* dispose javastrings */
	for (i = 0; i < string_hash.size; i++) {
		s = string_hash.ptr[i];
		while (u) {
			/* process elements in external hash chain */
			literalstring *nexts = s->hashlink;
			literalstring_free(s->string);
			FREE(s, literalstring);
			s = nexts;
		}	
	}

	/* dispose hashtable structures */
	MFREE(utf_hash.ptr,    void*, utf_hash.size);
	MFREE(string_hash.ptr, void*, string_hash.size);
}


/******************************************************************************
*********************** Misc support functions ********************************
******************************************************************************/


/******************** Function: desc_to_type **********************************
   
	Determines the corresponding Java base data type for a given type
	descriptor.
	
******************************************************************************/

u2 desc_to_type(utf *descriptor)
{
	char *utf_ptr = descriptor->text;  /* current position in utf text */

	if (descriptor->blength < 1) {
		log_text("Type-Descriptor is empty string");
		assert(0);
	}
	
	switch (*utf_ptr++) {
	case 'B': 
	case 'C':
	case 'I':
	case 'S':  
	case 'Z':  return TYPE_INT;
	case 'D':  return TYPE_DOUBLE;
	case 'F':  return TYPE_FLOAT;
	case 'J':  return TYPE_LONG;
	case 'L':
	case '[':  return TYPE_ADDRESS;
	}
			
	assert(0);

	return 0;
}


/********************** Function: desc_typesize *******************************

	Calculates the lenght in bytes needed for a data element of the type given
	by its type descriptor.
	
******************************************************************************/

u2 desc_typesize(utf *descriptor)
{
	switch (desc_to_type(descriptor)) {
	case TYPE_INT:     return 4;
	case TYPE_LONG:    return 8;
	case TYPE_FLOAT:   return 4;
	case TYPE_DOUBLE:  return 8;
	case TYPE_ADDRESS: return sizeof(voidptr);
	default:           return 0;
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
