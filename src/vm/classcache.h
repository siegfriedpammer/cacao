/* src/vm/classcache.h - loaded class cache and loading constraints

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

   $Id: classcache.h 2181 2005-04-01 16:53:33Z edwin $

*/


#ifndef _CLASSCACHE_H
#define _CLASSCACHE_H

#include <stdio.h>  /* for FILE */

#include "vm/references.h"


/* forward declarations *******************************************************/

typedef struct classcache_name_entry classcache_name_entry;
typedef struct classcache_class_entry classcache_class_entry;
typedef struct classcache_loader_entry classcache_loader_entry;

typedef java_objectheader classloader;

/* structs ********************************************************************/


/*----------------------------------------------------------------------------*/
/* The Loaded Class Cache                                                     */
/*                                                                            */
/* The loaded class cache is implemented as a two-level data structure.       */
/*                                                                            */
/* The first level is a hash table indexed by class names. For each class     */
/* name in the cache there is a classcache_name_entry, which collects all     */
/* information about classes with this class name.                            */
/*                                                                            */
/* Second level: For each classcache_name_entry there is a list of            */
/* classcache_class_entry:s representing the possible different resolutions   */
/* of the class name.                                                         */
/*                                                                            */
/* A classcache_class_entry records the following:                            */
/*                                                                            */
/* - the loaded class object, if this entry has been resolved, otherwise NULL */
/* - the list of initiating loaders which have resolved the class name to     */
/*   this class object                                                        */
/* - the list of initiating loaders which are constrained to resolve this     */
/*   class name to this class object in the future                            */
/*                                                                            */
/* The classcache_class_entry:s approximate the equivalence classes created   */
/* by the loading constraints and (XXX?) the equivalence of loaded classes.   */
/*                                                                            */
/* When a loading constraint (loaderA,loaderB,NAME) is added, then the        */
/* classcache_class_entry:s for NAME containing loaderA and loaderB resp.     */
/* must be merged into one entry. If this is impossible, because the entries  */
/* have already been resolved to different class objects, then the constraint */
/* is violated and an expception must be thrown.                              */
/*----------------------------------------------------------------------------*/


/* classcache_name_entry
 *
 * For each classname a classcache_name_entry struct is created.
 */

struct classcache_name_entry
{
	utf                     *name;        /* class name                       */
	classcache_name_entry   *hashlink;	  /* link for external chaining       */
	classcache_class_entry  *classes;     /* equivalence classes for this name*/
};

struct classcache_class_entry
{
	classinfo               *classobj;    /* the loaded class object, or NULL */
	classcache_loader_entry *loaders;
	classcache_loader_entry *constraints;
	classcache_class_entry  *next;        /* next class entry for same name   */
};

struct classcache_loader_entry
{
	classloader              *loader;     /* class loader object              */
	classcache_loader_entry  *next;       /* next loader entry in the list    */
};

/* function prototypes ********************************************************/

/* classcache_init *************************************************************
 
   Initialize the loaded class cache
  
*******************************************************************************/

void classcache_init();

/* classcache_free *************************************************************
 
   Free the memory used by the class cache

   NOTE:
       The class cache may not be used any more after this call, except
	   when it is reinitialized with classcache_init.
  
*******************************************************************************/

void classcache_free();

/* classcache_lookup ***********************************************************
 
   Lookup a possibly loaded class
  
   IN:
       initloader.......initiating loader for resolving the class name
       classname........class name to look up
  
   RETURN VALUE:
       The return value is a pointer to the cached class object,
       or NULL, if the class is not in the cache.
   
*******************************************************************************/

classinfo * classcache_lookup(classloader *initloader,utf *classname);

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

bool classcache_store(classloader *initloader,classinfo *cls);

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

bool classcache_add_constraint(classloader *a,classloader *b,utf *classname);

/* classcache_debug_dump *******************************************************
 
   Print the contents of the loaded class cache to a stream
  
   IN:
       file.............output stream
  
*******************************************************************************/

void classcache_debug_dump(FILE *file);
	
#endif /* _CLASSCACHE_H */

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

