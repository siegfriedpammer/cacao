/************************* toolbox/chain.h *************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Management of doubly linked lists with external linking

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1996/10/03

*******************************************************************************/

#ifndef CHAIN_H
#define CHAIN_H

typedef struct chainlink {          /* structure for list element */
	struct chainlink *next,*prev;
	void *element;
	} chainlink;

typedef struct chain {	            /* structure for list */
	int  usedump;   

	chainlink *first,*last;
	chainlink *active;
	} chain;


chain *chain_new ();
chain *chain_dnew ();
void chain_free(chain *c);

void chain_addafter(chain *c, void *element);
void chain_addbefore(chain *c, void *element);
void chain_addlast (chain *c, void *element);
void chain_addfirst (chain *c, void *element);

void chain_remove(chain *c);
void *chain_remove_go_prev(chain *c);
void chain_removespecific(chain *c, void *element);

void *chain_next(chain *c);
void *chain_prev(chain *c);
void *chain_this(chain *c);

void *chain_first(chain *c);
void *chain_last(chain *c);


/*
--------------------------- interface description ------------------------

Usage of these functions for list management is possible without additional
preparation in the element structures, as opposed to the module 'list'.

Consequently, the functions are a little slower and need more memory.

A new list is created with
	chain_new
or  chain_dnew.
The latter allocates all additional data structures on the dump memory (faster)
for which no explicit freeing is necessary after the processing. Care needs to
be taken to not accidentally free parts of these structures by calling
'dump_release' too early.

After usage, a list can be freed with
	chain_free.
(use only if the list was created with 'chain_new')


Adding elements is easy with:
	chain_addafter, chain_addlast, chain_addbefore, chain_addfirst		
	
Search the list with:
	chain_first, chain_last, chain_prev, chain_next, chain_this
	
Delete elements from the list:
	chain_remove, chain_remove_go_prev, chain_removespecific
	
	
ATTENTION: As mentioned earlier, there are no pointers to the list or to other
nodes inside the list elements, so list elements cannot be used as pointers
into the list. Therefore a 'cursor' is used to make one element current. Every
insertion/deletion occurs at a position relative to this cursor.

*/

#endif
