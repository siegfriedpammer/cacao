/* src/toolbox/list.h - synchronized linked list

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

   Changes: Christian Thalinger

   $Id: list.h 5049 2006-06-23 12:07:26Z twisti $

*/


#ifndef _LIST_H
#define _LIST_H

#include "config.h"
#include "vm/types.h"

#include "vm/global.h"


/* ---------------------- interface description -----------------------------

The list management with this module works like this:
	
	- to be used in a list, a structure must have an element of type
	  'listnode'.
	  
	- there needs to be a structure of type 'list'.
	
	- the function list_init(l, nodeoffset) initializes the structure.
	  nodeoffset is the offset of the 'listnode' from the start of the
	  structure in bytes.
	  
	- The remaining functions provide inserting, removing and searching.
	  
This small example aims to demonstrate correct usage:



	void bsp() {
		struct node {
			listnode linkage;
			int value;
			} a,b,c, *el;
			
		list l;
		
		a.value = 7;
		b.value = 9;
		c.value = 11;
		
		list_init (&l, OFFSET(struct node,linkage) );
		list_addlast (&l, a);
		list_addlast (&l, b);
		list_addlast (&l, c);
		
		e = list_first (&l);
		while (e) {
			printf ("Element: %d\n", e->value);
			e = list_next (&l,e);
			}
	}
	
	
	The output from this program should be:
		7
		9
		11



The reason for the usage of 'nodeoffset' is that this way, the same node can
part of different lists (there must be one 'listnode' element for every
distinct list).

*/

/* listnode *******************************************************************/

typedef struct listnode listnode;

struct listnode {
	listnode *next;
	listnode *prev;
};


/* list ***********************************************************************/

typedef struct list list;

struct list {
#if defined(ENABLE_THREADS)
	java_objectheader  lock;            /* threads lock object                */
#endif
	listnode          *first;
	listnode          *last;
	s4                 nodeoffset;
};


/* function prototypes ********************************************************/

list *list_create(s4 nodeoffset);

void list_add_first(list *l, void *element);
void list_add_last(list *l, void *element);
void list_add_last_unsynced(list *l, void *element);

void list_add_before(list *l, void *element, void *newelement);

void list_remove(list *l, void *element);
 
void *list_first(list *l);
void *list_last(list *l);

void *list_next(list *l, void *element);
void *list_prev(list *l, void *element);

#endif /* _LIST_H */


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
