/************************** toolbox/list.h *************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Management of doubly linked lists

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1996/10/03

*******************************************************************************/

#ifndef LIST_H
#define LIST_H

typedef struct listnode {           /* structure for list element */
	struct listnode *next,*prev;
	} listnode;

typedef struct list {               /* structure for list head */
	listnode *first,*last;
	int nodeoffset;
	} list;


void list_init (list *l, int nodeoffset);

void list_addlast (list *l, void *element);
void list_addfirst (list *l, void *element);

void list_remove (list *l, void *element);
 
void *list_first (list *l);
void *list_last (list *l);

void *list_next (list *l, void *element);
void *list_prev (list *l, void *element);


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

#endif
