/* vm/classcache.c - loaded class cache and loading constraints

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

   $Id: classcache.c 2076 2005-03-25 12:34:09Z edwin $

*/

#include <assert.h>
#include "vm/classcache.h"
#include "vm/tables.h"
#include "vm/utf8.h"
#include "vm/exceptions.h"
#include "mm/memory.h"

/* initial number of slots in the classcache hash table */
#define CLASSCACHE_INIT_SIZE  2048

/*============================================================================*/
/* DEBUG HELPERS                                                              */
/*============================================================================*/

#ifndef NDEBUG
#define CLASSCACHE_DEBUG
#endif

#ifdef CLASSCACHE_DEBUG
#define CLASSCACHE_ASSERT(cond)  assert(cond)
#else
#define CLASSCACHE_ASSERT(cond)
#endif

/*============================================================================*/
/* GLOBAL VARIABLES                                                           */
/*============================================================================*/

static hashtable classcache_hash;

/*============================================================================*/
/*                                                                            */
/*============================================================================*/

/* classcache_init *************************************************************
 
   Initialize the loaded class cache
  
*******************************************************************************/

void 
classcache_init()
{
	init_hashtable(&classcache_hash,CLASSCACHE_INIT_SIZE);
}

/* classcache_new_loader_entry *************************************************
 
   Create a new classcache_loader_entry struct
   (internally used helper function)
  
   IN:
       loader...........the ClassLoader object
	   next.............the next classcache_loader_entry

   RETURN VALUE:
       the new classcache_loader_entry
  
*******************************************************************************/

static classcache_loader_entry *
classcache_new_loader_entry(classloader *loader,classcache_loader_entry *next)
{
	classcache_loader_entry *lden;

	lden = NEW(classcache_loader_entry);
	lden->loader = loader;
	lden->next = next;

	return lden;
}

/* classcache_merge_loaders ****************************************************
 
   Merge two lists of loaders into one
   (internally used helper function)
  
   IN:
       lista............first list (may be NULL)
	   listb............second list (may be NULL)

   RETURN VALUE:
       the merged list (may be NULL)

   NOTE:
       The lists given as arguments are destroyed!
  
*******************************************************************************/

static classcache_loader_entry *
classcache_merge_loaders(classcache_loader_entry *lista,
		                 classcache_loader_entry *listb)
{
	classcache_loader_entry *result;
	classcache_loader_entry *ldenA;
	classcache_loader_entry *ldenB;
	classcache_loader_entry **chain;
	
	/* XXX This is a quadratic algorithm. If this ever
	 * becomes a problem, the loader lists should be
	 * stored as sorted lists and merged in linear time. */

	result = NULL;
	chain = &result;

	for (ldenA=lista; ldenA; ldenA=ldenA->next) {
		
		for (ldenB=listb; ldenB; ldenB=ldenB->next) {
			if (ldenB->loader == ldenA->loader)
				goto common_element;
		}

		/* this loader is only in lista */
		*chain = ldenA;
		chain = &(ldenA->next);

common_element:
		/* XXX free the duplicated element */
		;
	}

	/* concat listb to the result */
	*chain = listb;

	return result;
}

/* classcache_lookup_name ******************************************************
 
   Lookup a name in the first level of the cache
   (internally used helper function)
   
   IN:
       name.............the name to look up
  
   RETURN VALUE:
       a pointer to the classcache_name_entry for this name, or
       null if no entry was found.
	   
*******************************************************************************/
 
static classcache_name_entry *
classcache_lookup_name(utf *name)
{
	classcache_name_entry * c;     /* hash table element */
	u4 key;           /* hashkey computed from classname */
	u4 slot;          /* slot in hashtable               */
	u2 i;

	key  = utf_hashkey(name->text, name->blength);
	slot = key & (classcache_hash.size - 1);
	c    = classcache_hash.ptr[slot];

	/* search external hash chain for the entry */
	while (c) {
		if (c->name->blength == name->blength) {
			for (i = 0; i < name->blength; i++)
				if (name->text[i] != c->name->text[i]) goto nomatch;
						
			/* entry found in hashtable */
			return c;
		}
			
	nomatch:
		c = c->hashlink; /* next element in external chain */
	}

	/* not found */
	return NULL;
}

/* classcache_new_name *********************************************************
 
   Return a classcache_name_entry for the given name. The entry is created
   if it is not already in the cache.
   (internally used helper function)
   
   IN:
       name.............the name to look up / create an entry for
  
   RETURN VALUE:
       a pointer to the classcache_name_entry for this name
	   
*******************************************************************************/
 
static classcache_name_entry *
classcache_new_name(utf *name)
{
	classcache_name_entry * c;     /* hash table element */
	u4 key;           /* hashkey computed from classname */
	u4 slot;          /* slot in hashtable               */
	u2 i;

	key  = utf_hashkey(name->text, name->blength);
	slot = key & (classcache_hash.size - 1);
	c    = classcache_hash.ptr[slot];

	/* search external hash chain for the entry */
	while (c) {
		if (c->name->blength == name->blength) {
			for (i = 0; i < name->blength; i++)
				if (name->text[i] != c->name->text[i]) goto nomatch;
						
			/* entry found in hashtable */
			return c;
		}
			
	nomatch:
		c = c->hashlink; /* next element in external chain */
	}

	/* location in hashtable found, create new entry */

	c = NEW(classcache_name_entry);

	c->name = name;
	c->classes = NULL;

	/* insert entry into hashtable */
	c->hashlink = classcache_hash.ptr[slot];
	classcache_hash.ptr[slot] = c;

	/* update number of hashtable-entries */
	classcache_hash.entries++;

	if (classcache_hash.entries > (classcache_hash.size * 2)) {

		/* reorganization of hashtable, average length of 
		   the external chains is approx. 2                */  

		u4 i;
		classcache_name_entry *c;
		hashtable newhash;  /* the new hashtable */

		/* create new hashtable, double the size */
		init_hashtable(&newhash, classcache_hash.size * 2);
		newhash.entries = classcache_hash.entries;

		/* transfer elements to new hashtable */
		for (i = 0; i < classcache_hash.size; i++) {
			c = (classcache_name_entry *) classcache_hash.ptr[i];
			while (c) {
				classcache_name_entry *nextc = c->hashlink;
				u4 slot = (utf_hashkey(c->name->text, c->name->blength)) & (newhash.size - 1);
						
				c->hashlink = newhash.ptr[slot];
				newhash.ptr[slot] = c;

				c = nextc;
			}
		}
	
		/* dispose old table */	
		MFREE(classcache_hash.ptr, void*, classcache_hash.size);
		classcache_hash = newhash;
	}

	return c;
}

/* classcache_lookup ***********************************************************
 
   Lookup a possibly loaded class
  
   IN:
       initloader.......initiating loader for resolving the class name
       classname........class name to look up
  
   RETURN VALUE:
       The return value is a pointer to the cached class object,
       or NULL, if the class is not in the cache.
   
*******************************************************************************/

classinfo *
classcache_lookup(classloader *initloader,utf *classname)
{
	classcache_name_entry *en;
	classcache_class_entry *clsen;
	classcache_loader_entry *lden;

	en = classcache_lookup_name(classname);
	
	if (en) {
		/* iterate over all class entries */
		for (clsen=en->classes; clsen; clsen=clsen->next) {
			/* check if this entry has been loaded by initloader */
			for (lden=clsen->loaders; lden; lden=lden->next) {
				if (lden->loader == initloader) {
					/* found the loaded class entry */
					CLASSCACHE_ASSERT(clsen->classobj);
					return clsen->classobj;
				}
			}
		}
	}
	
	/* not found */
	return NULL;
}

/* classcache_store ************************************************************
   
   Store a loaded class
  
   IN:
       initloader.......initiating loader used to load the class
       cls..............class object to cache
  
   RETURN VALUE:
       true.............everything ok, the class was stored in
                        the cache if necessary,
       false............an exception has been thrown.
   
*******************************************************************************/

bool
classcache_store(classloader *initloader,classinfo *cls)
{
	classcache_name_entry *en;
	classcache_class_entry *clsen;
	classcache_loader_entry *lden;

	CLASSCACHE_ASSERT(cls);

	/* XXX lock table */

	en = classcache_new_name(cls->name);

	CLASSCACHE_ASSERT(en);
	
	/* iterate over all class entries */
	for (clsen=en->classes; clsen; clsen=clsen->next) {
		
#ifdef CLASSCACHE_DEBUG
		/* check if this entry has already been loaded by initloader */
		/* It never should have been loaded before! */
		for (lden=clsen->loaders; lden; lden=lden->next) {
			if (lden->loader == initloader)
				CLASSCACHE_ASSERT(false);
		}
#endif

		/* check if initloader is constrained to this entry */
		for (lden=clsen->constraints; lden; lden=lden->next) {
			if (lden->loader == initloader) {
				/* we have to use this entry */
				/* check if is has already been resolved to another class */
				if (clsen->classobj && clsen->classobj != cls) {
					/* a loading constraint is violated */
					*exceptionptr = new_exception_message(string_java_lang_LinkageError,
							"loading constraint violated XXX add message");
					return false; /* exception */
				}

				/* record initloader as initiating loader */
				clsen->loaders = classcache_new_loader_entry(initloader,clsen->loaders);

				/* record the loaded class object */
				clsen->classobj = cls;

				/* done */
				return true;
			}
		}
		
	}

	/* create a new class entry for this class object with */
	/* initiating loader initloader                        */

	clsen = NEW(classcache_class_entry);
	clsen->classobj = cls;
	clsen->loaders = classcache_new_loader_entry(initloader,NULL);
	clsen->constraints = NULL;

	clsen->next = en->classes;
	en->classes = clsen;

	/* done */
	return true;
}

/* classcache_find_loader ******************************************************
 
   Find the class entry loaded by or constrained to a given loader
   (internally used helper function)
  
   IN:
       entry............the classcache_name_entry
       loader...........the loader to look for
  
   RETURN VALUE:
       the classcache_class_entry for the given loader, or
	   NULL if no entry was found
   
*******************************************************************************/

static classcache_class_entry *
classcache_find_loader(classcache_name_entry *entry,classloader *loader)
{
	classcache_class_entry *clsen;
	classcache_loader_entry *lden;
	
	CLASSCACHE_ASSERT(entry);

	/* iterate over all class entries */
	for (clsen=entry->classes; clsen; clsen=clsen->next) {
		
		/* check if this entry has already been loaded by initloader */
		for (lden=clsen->loaders; lden; lden=lden->next) {
			if (lden->loader == loader)
				return clsen; /* found */
		}
		
		/* check if loader is constrained to this entry */
		for (lden=clsen->constraints; lden; lden=lden->next) {
			if (lden->loader == loader)
				return clsen; /* found */
		}
	}

	/* not found */
	return NULL;
}

/* classcache_free_class_entry *************************************************
 
   Free the memory used by a class entry
  
   IN:
       clsen............the classcache_class_entry to free  
	   
*******************************************************************************/

static void
classcache_free_class_entry(classcache_class_entry *clsen)
{
	classcache_loader_entry *lden;
	classcache_loader_entry *next;
	
	CLASSCACHE_ASSERT(clsen);

	for (lden=clsen->loaders; lden; lden=next) {
		next = lden->next;
		FREE(lden,classcache_loader_entry);
	}
	for (lden=clsen->constraints; lden; lden=next) {
		next = lden->next;
		FREE(lden,classcache_loader_entry);
	}

	FREE(clsen,classcache_class_entry);
}

/* classcache_remove_class_entry ***********************************************
 
   Remove a classcache_class_entry from the list of possible resolution of
   a name entry
   (internally used helper function)
  
   IN:
       entry............the classcache_name_entry
       clsen............the classcache_class_entry to remove
  
*******************************************************************************/

static void
classcache_remove_class_entry(classcache_name_entry *entry,
							  classcache_class_entry *clsen)
{
	classcache_class_entry **chain;

	CLASSCACHE_ASSERT(entry);
	CLASSCACHE_ASSERT(clsen);

	chain = &(entry->classes);
	while (*chain) {
		if (*chain == clsen) {
			*chain = clsen->next;
			classcache_free_class_entry(clsen);
			return;
		}
		chain = &((*chain)->next);
	}
}

/* classcache_free_name_entry **************************************************
 
   Free the memory used by a name entry
  
   IN:
       entry............the classcache_name_entry to free  
	   
*******************************************************************************/

static void
classcache_free_name_entry(classcache_name_entry *entry)
{
	classcache_class_entry *clsen;
	classcache_class_entry *next;
	
	CLASSCACHE_ASSERT(entry);

	for (clsen=entry->classes; clsen; clsen=next) {
		next = clsen->next;
		classcache_free_class_entry(clsen);
	}

	FREE(entry,classcache_name_entry);
}

/* classcache_free *************************************************************
 
   Free the memory used by the class cache

   NOTE:
       The class cache may not be used any more after this call, except
	   when it is reinitialized with classcache_init.
  
*******************************************************************************/

void 
classcache_free()
{
	u4 slot;
	classcache_name_entry *entry;
	classcache_name_entry *next;

	for (slot=0; slot<classcache_hash.size; ++slot) {
		for (entry=(classcache_name_entry *)classcache_hash.ptr[slot];
			 entry;
			 entry = next)
		{
			next = entry->hashlink;
			classcache_free_name_entry(entry);
		}
	}

	MFREE(classcache_hash.ptr,voidptr,classcache_hash.size);
	classcache_hash.size = 0;
	classcache_hash.entries = 0;
	classcache_hash.ptr = NULL;
}

/* classcache_add_constraint ***************************************************
 
   Add a loading constraint
  
   IN:
       a................first initiating loader
       b................second initiating loader
       classname........class name
  
   RETURN VALUE:
       true.............everything ok, the constraint has been added,
       false............an exception has been thrown.
   
*******************************************************************************/

bool
classcache_add_constraint(classloader *a,classloader *b,utf *classname)
{
	classcache_name_entry *en;
	classcache_class_entry *clsenA;
	classcache_class_entry *clsenB;

	CLASSCACHE_ASSERT(classname);

#ifdef CLASSCACHE_DEBUG
	fprintf(stderr,"classcache_add_constraint(%p,%p,",a,b);
	utf_fprint_classname(stderr,classname);
	fprintf(stderr,")\n");
#endif

	/* a constraint with a == b is trivially satisfied */
	if (a == b)
		return true;

	/* XXX lock table */

	en = classcache_new_name(classname);

	CLASSCACHE_ASSERT(en);
	
	/* find the entry loaded by / constrained to each loader */
	clsenA = classcache_find_loader(en,a);
	clsenB = classcache_find_loader(en,b);

	if (clsenA && clsenB) {
		/* { both loaders have corresponding entries } */

		/* if the entries are the same, the constraint is already recorded */
		if (clsenA == clsenB)
			return true;

		/* check if the entries can be merged */
		if (clsenA->classobj != clsenB->classobj) {
			/* no, the constraint is violated */
			*exceptionptr = new_exception_message(string_java_lang_LinkageError,
					"loading constraint violated XXX add message");
			return false; /* exception */
		}

		/* yes, merge the entries */
		/* clsenB will be merged into clsenA */
		clsenA->loaders = classcache_merge_loaders(clsenA->loaders,
												   clsenB->loaders);
		
		clsenA->constraints = classcache_merge_loaders(clsenA->constraints,
													   clsenB->constraints);

		/* remove clsenB from the list of class entries */
		classcache_remove_class_entry(en,clsenB);
	}
	else {
		/* { at most one of the loaders has a corresponding entry } */

		/* set clsenA to the single class entry we have */
		if (!clsenA)
			clsenA = clsenB;

		if (!clsenA) {
			/* { no loader has a corresponding entry } */

			/* create a new class entry with the constraint (a,b,en->name) */
			clsenA = NEW(classcache_class_entry);
			clsenA->classobj = NULL;
			clsenA->loaders = NULL;
			clsenA->constraints = classcache_new_loader_entry(b,NULL);
			clsenA->constraints = classcache_new_loader_entry(a,clsenA->constraints);

			clsenA->next = en->classes;
			en->classes = clsenA;
		}
		else {
			/* make b the loader that has no corresponding entry */
			if (clsenB)
				b = a;
			
			/* loader b must be added to entry clsenA */
			clsenA->constraints = classcache_new_loader_entry(b,clsenA->constraints);
		}
	}

	/* done */
	return true;
}

/*============================================================================*/
/* DEBUG DUMPS                                                                */
/*============================================================================*/

/* classcache_debug_dump *******************************************************
 
   Print the contents of the loaded class cache to a stream
  
   IN:
       file.............output stream
  
*******************************************************************************/

void
classcache_debug_dump(FILE *file)
{
	classcache_name_entry *c;
	classcache_class_entry *clsen;
	classcache_loader_entry *lden;
	u4 slot;

	fprintf(file,"\n=== [loaded class cache] =====================================\n\n");
	fprintf(file,"hash size   : %d\n",classcache_hash.size);
	fprintf(file,"hash entries: %d\n",classcache_hash.entries);
	fprintf(file,"\n");

	for (slot=0; slot<classcache_hash.size; ++slot) {
		c = (classcache_name_entry *) classcache_hash.ptr[slot];

		for (; c; c=c->hashlink) {
			utf_fprint_classname(file,c->name);
			fprintf(file,"\n");

			/* iterate over all class entries */
			for (clsen=c->classes; clsen; clsen=clsen->next) {
				fprintf(file,"    %s\n",(clsen->classobj) ? "loaded" : "unresolved");
				fprintf(file,"    loaders:");
				for (lden=clsen->loaders; lden; lden=lden->next) {
					fprintf(file," %p",(void *)lden->loader);
				}
				fprintf(file,"\n    constraints:");
				for (lden=clsen->constraints; lden; lden=lden->next) {
					fprintf(file," %p",(void *)lden->loader);
				}
				fprintf(file,"\n");
			}
		}
	}
	fprintf(file,"\n==============================================================\n\n");
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

