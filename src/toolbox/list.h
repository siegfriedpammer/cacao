/* src/toolbox/list.h - 

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

   Changes: Christian Thalinger

   $Id: list.h 3449 2005-10-19 19:56:46Z twisti $

*/


#ifndef _LIST_H
#define _LIST_H

typedef struct listnode {           /* structure for list element */
	struct listnode *next;
	struct listnode *prev;
} listnode;


typedef struct list {               /* structure for list head */
	listnode *first;
	listnode *last;
	int nodeoffset;
} list;


/* function prototypes */

void list_init(list *l, int nodeoffset);

void list_addlast(list *l, void *element);
void list_addfirst(list *l, void *element);

void list_remove(list *l, void *element);
 
void *list_first(list *l);
void *list_last(list *l);

void *list_next(list *l, void *element);
void *list_prev(list *l, void *element);


/*
---------------------- interface description -----------------------------

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
