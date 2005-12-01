/* src/vm/classcache.c - loaded class cache and loading constraints

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

   Changes: Christian Thalinger

   $Id: classcache.c 3837 2005-12-01 23:50:28Z twisti $

*/


#include <assert.h>

#include "config.h"
#include "vm/types.h"

#include "mm/memory.h"
#include "vm/classcache.h"
#include "vm/exceptions.h"
#include "vm/hashtable.h"
#include "vm/stringlocal.h"
#include "vm/utf8.h"


/* initial number of slots in the classcache hash table */
#define CLASSCACHE_INIT_SIZE  2048

/*============================================================================*/
/* DEBUG HELPERS                                                              */
/*============================================================================*/

/*#define CLASSCACHE_VERBOSE*/

/*============================================================================*/
/* THREAD-SAFE LOCKING                                                        */
/*============================================================================*/

	/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
	/* CAUTION: The static functions below are */
	/*          NOT synchronized!              */
	/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

#if defined(USE_THREADS)
# define CLASSCACHE_LOCK()      builtin_monitorenter(lock_hashtable_classcache)
# define CLASSCACHE_UNLOCK()    builtin_monitorexit(lock_hashtable_classcache)
#else
# define CLASSCACHE_LOCK()
# define CLASSCACHE_UNLOCK()
#endif

/*============================================================================*/
/* GLOBAL VARIABLES                                                           */
/*============================================================================*/

hashtable hashtable_classcache;

#if defined(USE_THREADS)
static java_objectheader *lock_hashtable_classcache;
#endif


/*============================================================================*/
/*                                                                            */
/*============================================================================*/

/* classcache_init *************************************************************
 
   Initialize the loaded class cache

   Note: NOT synchronized!
  
*******************************************************************************/

bool classcache_init(void)
{
	/* create the hashtable */

	hashtable_create(&hashtable_classcache, CLASSCACHE_INIT_SIZE);

#if defined(USE_THREADS)
	/* create utf hashtable lock object */

	lock_hashtable_classcache = NEW(java_objectheader);

# if defined(NATIVE_THREADS)
	initObjectLock(lock_hashtable_classcache);
# endif
#endif

	/* everything's ok */

	return true;
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

static classcache_loader_entry * classcache_new_loader_entry(
									classloader * loader,
									classcache_loader_entry * next)
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

static classcache_loader_entry * classcache_merge_loaders(
									classcache_loader_entry * lista,
									classcache_loader_entry * listb)
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

	for (ldenA = lista; ldenA; ldenA = ldenA->next) {

		for (ldenB = listb; ldenB; ldenB = ldenB->next) {
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

static classcache_name_entry *classcache_lookup_name(utf *name)
{
	classcache_name_entry *c;           /* hash table element                 */
	u4 key;                             /* hashkey computed from classname    */
	u4 slot;                            /* slot in hashtable                  */
/*  	u4 i; */

	key  = utf_hashkey(name->text, (u4) name->blength);
	slot = key & (hashtable_classcache.size - 1);
	c    = hashtable_classcache.ptr[slot];

	/* search external hash chain for the entry */

	while (c) {
		/* entry found in hashtable */

		if (c->name == name)
			return c;

		c = c->hashlink;                    /* next element in external chain */
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

static classcache_name_entry *classcache_new_name(utf *name)
{
	classcache_name_entry *c;	/* hash table element */
	u4 key;						/* hashkey computed from classname */
	u4 slot;					/* slot in hashtable               */
	u4 i;

	key  = utf_hashkey(name->text, (u4) name->blength);
	slot = key & (hashtable_classcache.size - 1);
	c    = hashtable_classcache.ptr[slot];

	/* search external hash chain for the entry */

	while (c) {
		/* entry found in hashtable */

		if (c->name == name)
			return c;

		c = c->hashlink;                    /* next element in external chain */
	}

	/* location in hashtable found, create new entry */

	c = NEW(classcache_name_entry);

	c->name = name;
	c->classes = NULL;

	/* insert entry into hashtable */
	c->hashlink = (classcache_name_entry *) hashtable_classcache.ptr[slot];
	hashtable_classcache.ptr[slot] = c;

	/* update number of hashtable-entries */
	hashtable_classcache.entries++;

	if (hashtable_classcache.entries > (hashtable_classcache.size * 2)) {

		/* reorganization of hashtable, average length of 
		   the external chains is approx. 2                */

		classcache_name_entry *c2;
		hashtable newhash;		/* the new hashtable */

		/* create new hashtable, double the size */

		hashtable_create(&newhash, hashtable_classcache.size * 2);
		newhash.entries = hashtable_classcache.entries;

		/* transfer elements to new hashtable */

		for (i = 0; i < hashtable_classcache.size; i++) {
			c2 = (classcache_name_entry *) hashtable_classcache.ptr[i];
			while (c2) {
				classcache_name_entry *nextc = c2->hashlink;
				u4 newslot =
					(utf_hashkey(c2->name->text, (u4) c2->name->blength)) & (newhash.size - 1);

				c2->hashlink = (classcache_name_entry *) newhash.ptr[newslot];
				newhash.ptr[newslot] = c2;

				c2 = nextc;
			}
		}

		/* dispose old table */

		MFREE(hashtable_classcache.ptr, void *, hashtable_classcache.size);
		hashtable_classcache = newhash;
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

   Note: synchronized with global tablelock
   
*******************************************************************************/

classinfo *classcache_lookup(classloader *initloader, utf *classname)
{
	classcache_name_entry *en;
	classcache_class_entry *clsen;
	classcache_loader_entry *lden;
	classinfo *cls = NULL;

	CLASSCACHE_LOCK();

	en = classcache_lookup_name(classname);

	if (en) {
		/* iterate over all class entries */

		for (clsen = en->classes; clsen; clsen = clsen->next) {
			/* check if this entry has been loaded by initloader */

			for (lden = clsen->loaders; lden; lden = lden->next) {
				if (lden->loader == initloader) {
					/* found the loaded class entry */

					assert(clsen->classobj);
					cls = clsen->classobj;
					goto found;
				}
			}
		}
	}

  found:
	CLASSCACHE_UNLOCK();
	return cls;
}


/* classcache_lookup_defined ***************************************************
 
   Lookup a class with the given name and defining loader
  
   IN:
       defloader........defining loader
       classname........class name
  
   RETURN VALUE:
       The return value is a pointer to the cached class object,
       or NULL, if the class is not in the cache.
   
*******************************************************************************/

classinfo *classcache_lookup_defined(classloader *defloader, utf *classname)
{
	classcache_name_entry *en;
	classcache_class_entry *clsen;
	classinfo *cls = NULL;

	CLASSCACHE_LOCK();

	en = classcache_lookup_name(classname);

	if (en) {
		/* iterate over all class entries */
		for (clsen = en->classes; clsen; clsen = clsen->next) {
			if (!clsen->classobj)
				continue;

			/* check if this entry has been defined by defloader */
			if (clsen->classobj->classloader == defloader) {
				cls = clsen->classobj;
				goto found;
			}
		}
	}

  found:
	CLASSCACHE_UNLOCK();
	return cls;
}


/* classcache_lookup_defined_or_initiated **************************************
 
   Lookup a class that has been defined or initiated by the given loader
  
   IN:
       loader...........defining or initiating loader
       classname........class name to look up
  
   RETURN VALUE:
       The return value is a pointer to the cached class object,
       or NULL, if the class is not in the cache.

   Note: synchronized with global tablelock
   
*******************************************************************************/

classinfo *classcache_lookup_defined_or_initiated(classloader *loader, 
												  utf *classname)
{
	classcache_name_entry *en;
	classcache_class_entry *clsen;
	classcache_loader_entry *lden;
	classinfo *cls = NULL;

	CLASSCACHE_LOCK();

	en = classcache_lookup_name(classname);

	if (en) {
		/* iterate over all class entries */

		for (clsen = en->classes; clsen; clsen = clsen->next) {

			/* check if this entry has been defined by loader */
			if (clsen->classobj && clsen->classobj->classloader == loader) {
				cls = clsen->classobj;
				goto found;
			}
			
			/* check if this entry has been initiated by loader */
			for (lden = clsen->loaders; lden; lden = lden->next) {
				if (lden->loader == loader) {
					/* found the loaded class entry */

					assert(clsen->classobj);
					cls = clsen->classobj;
					goto found;
				}
			}
		}
	}

  found:
	CLASSCACHE_UNLOCK();
	return cls;
}


/* classcache_store ************************************************************
   
   Store a loaded class. If a class of the same name has already been stored
   with the same initiating loader, then the given class CLS is freed (if
   possible) and the previously stored class is returned.
  
   IN:
       initloader.......initiating loader used to load the class
	                    (may be NULL indicating the bootstrap loader)
       cls..............class object to cache
	   mayfree..........true if CLS may be freed in case another class is
	                    returned
  
   RETURN VALUE:
       cls..............everything ok, the class was stored in the cache,
	   other classinfo..another class with the same (initloader,name) has been
	                    stored earlier. CLS has been freed and the earlier
						stored class is returned.
       NULL.............an exception has been thrown.
   
   Note: synchronized with global tablelock
   
*******************************************************************************/

classinfo * classcache_store(classloader * initloader,
							 classinfo * cls,
							 bool mayfree)
{
	classcache_name_entry *en;
	classcache_class_entry *clsen;
	classcache_loader_entry *lden;
#ifdef CLASSCACHE_VERBOSE
	char logbuffer[1024];
#endif
	
	assert(cls);
	assert(cls->loaded != 0);

	CLASSCACHE_LOCK();

#ifdef CLASSCACHE_VERBOSE
	sprintf(logbuffer,"classcache_store (%p,%d,", (void*)initloader,mayfree);
	utf_strcat(logbuffer, cls->name);
	strcat(logbuffer,")");
	log_text(logbuffer);
#endif

	en = classcache_new_name(cls->name);

	assert(en);

	/* iterate over all class entries */
	for (clsen = en->classes; clsen; clsen = clsen->next) {

		/* check if this entry has already been loaded by initloader */
		for (lden = clsen->loaders; lden; lden = lden->next) {
			if (lden->loader == initloader && clsen->classobj != cls) {
				/* A class with the same (initloader,name) pair has been stored already. */
				/* We free the given class and return the earlier one.                   */
#ifdef CLASSCACHE_VERBOSE
				dolog("replacing %p with earlier loaded class %p",cls,clsen->classobj);
#endif
				assert(clsen->classobj);
				if (mayfree)
					class_free(cls);
				cls = clsen->classobj;
				goto return_success;
			}
		}

		/* check if initloader is constrained to this entry */
		for (lden = clsen->constraints; lden; lden = lden->next) {
			if (lden->loader == initloader) {
				/* we have to use this entry */
				/* check if is has already been resolved to another class */
				if (clsen->classobj && clsen->classobj != cls) {
					/* a loading constraint is violated */
					*exceptionptr = exceptions_new_linkageerror(
										"loading constraint violated: ",cls);
					goto return_exception;
				}

				/* record initloader as initiating loader */
				clsen->loaders = classcache_new_loader_entry(initloader, clsen->loaders);

				/* record the loaded class object */
				clsen->classobj = cls;

				/* done */
				goto return_success;
			}
		}

	}

	/* create a new class entry for this class object with */
	/* initiating loader initloader                        */

	clsen = NEW(classcache_class_entry);
	clsen->classobj = cls;
	clsen->loaders = classcache_new_loader_entry(initloader, NULL);
	clsen->constraints = NULL;

	clsen->next = en->classes;
	en->classes = clsen;

  return_success:
	CLASSCACHE_UNLOCK();
	return cls;

  return_exception:
	CLASSCACHE_UNLOCK();
	return NULL;				/* exception */
}

/* classcache_store_unique *****************************************************
   
   Store a loaded class as loaded by the bootstrap loader. This is a wrapper 
   aroung classcache_store that throws an exception if a class with the same 
   name has already been loaded by the bootstrap loader.

   This function is used to register a few special classes during startup.
   It should not be used otherwise.
  
   IN:
       cls..............class object to cache
  
   RETURN VALUE:
       true.............everything ok, the class was stored.
       false............an exception has been thrown.
   
   Note: synchronized with global tablelock
   
*******************************************************************************/

bool classcache_store_unique(classinfo *cls)
{
	classinfo *result;

	result = classcache_store(NULL,cls,false);
	if (result == NULL)
		return false;

	if (result != cls) {
		*exceptionptr = new_internalerror("class already stored in the class cache");
		return false;
	}

	return true;
}

/* classcache_store_defined ****************************************************
   
   Store a loaded class after it has been defined. If the class has already
   been defined by the same defining loader in another thread, free the given
   class and returned the one which has been defined earlier.
  
   IN:
       cls..............class object to store. classloader must be set
	                    (classloader may be NULL, for bootloader)
  
   RETURN VALUE:
       cls..............everything ok, the class was stored the cache,
	   other classinfo..the class had already been defined, CLS was freed, the
	                    class which was defined earlier is returned,
       NULL.............an exception has been thrown.
   
*******************************************************************************/

classinfo * classcache_store_defined(classinfo *cls)
{
	classcache_name_entry *en;
	classcache_class_entry *clsen;
#ifdef CLASSCACHE_VERBOSE
	char logbuffer[1024];
#endif

	assert(cls);
	assert(cls->loaded != 0);

	CLASSCACHE_LOCK();

#ifdef CLASSCACHE_VERBOSE
	sprintf(logbuffer,"classcache_store_defined (%p,", (void*)cls->classloader);
	utf_strcat(logbuffer, cls->name);
	strcat(logbuffer,")");
	log_text(logbuffer);
#endif

	en = classcache_new_name(cls->name);

	assert(en);

	/* iterate over all class entries */
	for (clsen = en->classes; clsen; clsen = clsen->next) {
		
		/* check if this class has been defined by the same classloader */
		if (clsen->classobj && clsen->classobj->classloader == cls->classloader) {
			/* we found an earlier definition, delete the newer one */
			/* (if it is a different classinfo)                     */
			if (clsen->classobj != cls) {
#ifdef CLASSCACHE_VERBOSE
				dolog("replacing %p with earlier defined class %p",cls,clsen->classobj);
#endif
				class_free(cls);
				cls = clsen->classobj;
			}
			goto return_success;
		}
	}

	/* create a new class entry for this class object */
	/* the list of initiating loaders is empty at this point */

	clsen = NEW(classcache_class_entry);
	clsen->classobj = cls;
	clsen->loaders = NULL;
	clsen->constraints = NULL;

	clsen->next = en->classes;
	en->classes = clsen;

return_success:
	CLASSCACHE_UNLOCK();
	return cls;
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

static classcache_class_entry * classcache_find_loader(
									classcache_name_entry * entry,
									classloader * loader)
{
	classcache_class_entry *clsen;
	classcache_loader_entry *lden;

	assert(entry);

	/* iterate over all class entries */
	for (clsen = entry->classes; clsen; clsen = clsen->next) {

		/* check if this entry has already been loaded by initloader */
		for (lden = clsen->loaders; lden; lden = lden->next) {
			if (lden->loader == loader)
				return clsen;	/* found */
		}

		/* check if loader is constrained to this entry */
		for (lden = clsen->constraints; lden; lden = lden->next) {
			if (lden->loader == loader)
				return clsen;	/* found */
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

static void classcache_free_class_entry(classcache_class_entry * clsen)
{
	classcache_loader_entry *lden;
	classcache_loader_entry *next;

	assert(clsen);

	for (lden = clsen->loaders; lden; lden = next) {
		next = lden->next;
		FREE(lden, classcache_loader_entry);
	}
	for (lden = clsen->constraints; lden; lden = next) {
		next = lden->next;
		FREE(lden, classcache_loader_entry);
	}

	FREE(clsen, classcache_class_entry);
}

/* classcache_remove_class_entry ***********************************************
 
   Remove a classcache_class_entry from the list of possible resolution of
   a name entry
   (internally used helper function)
  
   IN:
       entry............the classcache_name_entry
       clsen............the classcache_class_entry to remove
  
*******************************************************************************/

static void classcache_remove_class_entry(classcache_name_entry * entry,
										  classcache_class_entry * clsen)
{
	classcache_class_entry **chain;

	assert(entry);
	assert(clsen);

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

static void classcache_free_name_entry(classcache_name_entry * entry)
{
	classcache_class_entry *clsen;
	classcache_class_entry *next;

	assert(entry);

	for (clsen = entry->classes; clsen; clsen = next) {
		next = clsen->next;
		classcache_free_class_entry(clsen);
	}

	FREE(entry, classcache_name_entry);
}

/* classcache_free *************************************************************
 
   Free the memory used by the class cache

   NOTE:
       The class cache may not be used any more after this call, except
	   when it is reinitialized with classcache_init.
  
   Note: NOT synchronized!
  
*******************************************************************************/

void classcache_free(void)
{
	u4 slot;
	classcache_name_entry *entry;
	classcache_name_entry *next;

	for (slot = 0; slot < hashtable_classcache.size; ++slot) {
		for (entry = (classcache_name_entry *) hashtable_classcache.ptr[slot]; entry; entry = next) {
			next = entry->hashlink;
			classcache_free_name_entry(entry);
		}
	}

	MFREE(hashtable_classcache.ptr, voidptr, hashtable_classcache.size);
	hashtable_classcache.size = 0;
	hashtable_classcache.entries = 0;
	hashtable_classcache.ptr = NULL;
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
   
   Note: synchronized with global tablelock
   
*******************************************************************************/

bool classcache_add_constraint(classloader * a,
							   classloader * b,
							   utf * classname)
{
	classcache_name_entry *en;
	classcache_class_entry *clsenA;
	classcache_class_entry *clsenB;

	assert(classname);

#ifdef CLASSCACHE_VERBOSE
	fprintf(stderr, "classcache_add_constraint(%p,%p,", (void *) a, (void *) b);
	utf_fprint_classname(stderr, classname);
	fprintf(stderr, ")\n");
#endif

	/* a constraint with a == b is trivially satisfied */
	if (a == b)
		return true;

	CLASSCACHE_LOCK();

	en = classcache_new_name(classname);

	assert(en);

	/* find the entry loaded by / constrained to each loader */
	clsenA = classcache_find_loader(en, a);
	clsenB = classcache_find_loader(en, b);

	if (clsenA && clsenB) {
		/* { both loaders have corresponding entries } */

		/* if the entries are the same, the constraint is already recorded */
		if (clsenA == clsenB)
			goto return_success;

		/* check if the entries can be merged */
		if (clsenA->classobj && clsenB->classobj
			&& clsenA->classobj != clsenB->classobj) {
			/* no, the constraint is violated */
			*exceptionptr = exceptions_new_linkageerror(
							  "loading constraint violated: ",clsenA->classobj);
			goto return_exception;
		}

		/* yes, merge the entries */
		/* clsenB will be merged into clsenA */
		clsenA->loaders = classcache_merge_loaders(clsenA->loaders, clsenB->loaders);
		clsenB->loaders = NULL;

		clsenA->constraints = classcache_merge_loaders(clsenA->constraints,
													   clsenB->constraints);
		clsenB->constraints = NULL;

		if (!clsenA->classobj)
			clsenA->classobj = clsenB->classobj;

		/* remove clsenB from the list of class entries */
		classcache_remove_class_entry(en, clsenB);
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
			clsenA->constraints = classcache_new_loader_entry(b, NULL);
			clsenA->constraints = classcache_new_loader_entry(a, clsenA->constraints);

			clsenA->next = en->classes;
			en->classes = clsenA;
		}
		else {
			/* make b the loader that has no corresponding entry */
			if (clsenB)
				b = a;

			/* loader b must be added to entry clsenA */
			clsenA->constraints = classcache_new_loader_entry(b, clsenA->constraints);
		}
	}

  return_success:
	CLASSCACHE_UNLOCK();
	return true;

  return_exception:
	CLASSCACHE_UNLOCK();
	return false;				/* exception */
}

/*============================================================================*/
/* DEBUG DUMPS                                                                */
/*============================================================================*/

/* classcache_debug_dump *******************************************************
 
   Print the contents of the loaded class cache to a stream
  
   IN:
       file.............output stream
  
   Note: synchronized with global tablelock
   
*******************************************************************************/

#ifndef NDEBUG
void classcache_debug_dump(FILE * file)
{
	classcache_name_entry *c;
	classcache_class_entry *clsen;
	classcache_loader_entry *lden;
	u4 slot;

	CLASSCACHE_LOCK();

	fprintf(file, "\n=== [loaded class cache] =====================================\n\n");
	fprintf(file, "hash size   : %d\n", (int) hashtable_classcache.size);
	fprintf(file, "hash entries: %d\n", (int) hashtable_classcache.entries);
	fprintf(file, "\n");

	for (slot = 0; slot < hashtable_classcache.size; ++slot) {
		c = (classcache_name_entry *) hashtable_classcache.ptr[slot];

		for (; c; c = c->hashlink) {
			utf_fprint_classname(file, c->name);
			fprintf(file, "\n");

			/* iterate over all class entries */
			for (clsen = c->classes; clsen; clsen = clsen->next) {
				if (clsen->classobj) {
					fprintf(file, "    loaded %p\n", (void *) clsen->classobj);
				}
				else {
					fprintf(file, "    unresolved\n");
				}
				fprintf(file, "        loaders:");
				for (lden = clsen->loaders; lden; lden = lden->next) {
					fprintf(file, "<%p> %p", (void *) lden, (void *) lden->loader);
				}
				fprintf(file, "\n        constraints:");
				for (lden = clsen->constraints; lden; lden = lden->next) {
					fprintf(file, "<%p> %p", (void *) lden, (void *) lden->loader);
				}
				fprintf(file, "\n");
			}
		}
	}
	fprintf(file, "\n==============================================================\n\n");

	CLASSCACHE_UNLOCK();
}
#endif /* NDEBUG */

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
